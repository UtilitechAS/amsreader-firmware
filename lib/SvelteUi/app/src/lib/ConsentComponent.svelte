<script>
    import { sysinfoStore } from './DataStores.js';
    import { translationsStore, getTranslations } from './TranslationService.js';
    import fetchWithTimeout from './fetchWithTimeout';
    import Mask from './Mask.svelte'
    import { navigate } from 'svelte-navigator';
    import { wiki } from './Helpers';
    import { meterPresets, getMeterPresetById, buildMeterStateFromPreset, createMeterStateFromConfiguration, describePresetSummary, applyMeterStateToConfiguration } from './meterPresets.js';

    export let basepath = "/";
    export let sysinfo = {};

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let loadingOrSaving = false;
    let consentChoice = '';
    let autoUpdateChoice = '';
    let languageChoice = '';
    let languages = [
        { code: 'en', name: 'English' },
        { code: 'no', name: 'Norsk' }
    ];
    let canSave = false;
    let meterState = createMeterStateFromConfiguration();
    let selectedMeterPresetId = '';
    let selectedMeterPreset = null;

    $: if (translations?.language?.code && !languages.find(lang => lang.code === translations.language.code)) {
        languages = [...languages, { code: translations.language.code, name: translations.language.name ?? translations.language.code }];
    }

    $: if (sysinfo) {
        if (sysinfo.fwconsent === 1 || sysinfo.fwconsent === 2) {
            consentChoice = String(sysinfo.fwconsent);
        }
        const autoFlag = sysinfo?.upgrade?.auto;
        if (autoFlag === true) {
            autoUpdateChoice = 'true';
        } else if (autoFlag === false) {
            autoUpdateChoice = 'false';
        }
        const sysLang = sysinfo?.ui?.lang;
        if (sysLang && !languages.find(lang => lang.code === sysLang)) {
            languages = [...languages, { code: sysLang, name: translations.language?.name ?? sysLang.toUpperCase() }];
        }
        if (!languageChoice && sysLang) {
            languageChoice = sysLang;
        }
    }

    $: {
        if (!languageChoice && translations?.language?.code) {
            languageChoice = translations.language.code;
        }
    }

    $: canSave = consentChoice !== '' && autoUpdateChoice !== '' && !loadingOrSaving;

    async function handleLanguageChange(e) {
        const selected = e.target.value;
        if (selected === 'hub') {
            try {
                const response = await fetchWithTimeout('http://hub.amsleser.no/hub/language/list.json');
                languages = await response.json();
                languageChoice = translations.language.code;
            } catch (err) {
                console.error('Failed to load languages from hub', err);
                languageChoice = translations.language.code;
            }
            return;
        }

        languageChoice = selected;
        if (languageChoice) {
            await getTranslations(languageChoice);
        }
    }

    function handlePresetSelection(presetId) {
        selectedMeterPresetId = presetId;
        const preset = getMeterPresetById(presetId);
        selectedMeterPreset = preset ?? null;
        if (preset) {
            meterState = buildMeterStateFromPreset(createMeterStateFromConfiguration(), preset);
        } else {
            meterState = createMeterStateFromConfiguration();
        }
    }

    function clearMeterPresetSelection() {
        selectedMeterPresetId = '';
        selectedMeterPreset = null;
        meterState = createMeterStateFromConfiguration();
    }

    function buildMeterPayload() {
        if (!selectedMeterPreset) {
            return null;
        }
        const state = meterState ?? createMeterStateFromConfiguration();
        return {
            source: state.source ?? 1,
            parser: state.parser ?? 0,
            baud: state.baud ?? 0,
            parity: state.parity ?? 3,
            invert: state.invert ?? false,
            distributionSystem: state.distributionSystem ?? 2,
            mainFuse: state.mainFuse ?? 0,
            production: state.production ?? 0,
            buffer: state.buffer ?? 256,
            encrypted: state.encrypted ?? false,
            encryptionKey: state.encryptionKey ?? '',
            authenticationKey: state.authenticationKey ?? '',
            multipliers: {
                watt: state.multipliers?.watt ?? 1,
                volt: state.multipliers?.volt ?? 1,
                amp: state.multipliers?.amp ?? 1,
                kwh: state.multipliers?.kwh ?? 1
            }
        };
    }

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target)
        const data = new URLSearchParams()
        for (let field of formData) {
            const [key, value] = field
            data.append(key, typeof value === 'string' ? value : String(value))
        }

        const consentValue = formData.get('sf');
        const autoValue = formData.get('fwa');
        const autoDecision = autoValue === 'true' ? true : autoValue === 'false' ? false : null;

        const meterPayload = buildMeterPayload();
        if (meterPayload) {
            data.set('m', 'true');
            data.set('mo', String(meterPayload.source ?? 1));
            data.set('ma', String(meterPayload.parser ?? 0));
            data.set('mb', String(meterPayload.baud ?? 0));
            data.set('mp', String(meterPayload.parity ?? 3));
            data.set('mi', meterPayload.invert ? 'true' : 'false');
            data.set('md', String(meterPayload.distributionSystem ?? 2));
            data.set('mf', String(meterPayload.mainFuse ?? 0));
            data.set('mr', String(meterPayload.production ?? 0));
            data.set('ms', String(meterPayload.buffer ?? 256));
            if (meterPayload.encrypted) {
                data.set('me', 'true');
                data.set('mek', meterPayload.encryptionKey ?? '');
                data.set('mea', meterPayload.authenticationKey ?? '');
            }
            data.set('mmw', String(meterPayload.multipliers?.watt ?? 1));
            data.set('mmv', String(meterPayload.multipliers?.volt ?? 1));
            data.set('mma', String(meterPayload.multipliers?.amp ?? 1));
            data.set('mmc', String(meterPayload.multipliers?.kwh ?? 1));
        }

        const response = await fetch('save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())
        loadingOrSaving = false;

        sysinfoStore.update(s => {
            s.fwconsent = consentValue === '1' ? 1 : consentValue === '2' ? 2 : 0;
            if (!s.ui || typeof s.ui !== 'object') {
                s.ui = {};
            }
            if (languageChoice) {
                s.ui.lang = languageChoice;
            }
            if (!s.upgrade || typeof s.upgrade !== 'object') {
                s.upgrade = { x: -1, e: 0, f: null, t: null, m: false };
            }
            if (autoDecision !== null) {
                s.upgrade.auto = autoDecision;
            }
            if (meterPayload) {
                const config = applyMeterStateToConfiguration(s.m ?? {}, meterState);
                config.appliedPresetId = meterState.appliedPresetId;
                s.m = config;
            }
            s.booting = res.reboot;
            return s;
        });
        navigate(basepath);
    }
</script>

<div class="grid xl:grid-cols-3 lg:grid-cols-2">
    <div class="cnt">
        <form on:submit|preventDefault={handleSubmit} autocomplete="off">
            <div>
                {translations.consent?.title ?? "Consents"}
            </div>
            <hr/>
            <div class="my-3">
                {translations.consent?.one_click ?? "One-click"}<br/>
                <a href="{wiki('Data-collection-on-one-click-firmware-upgrade')}" target="_blank" class="text-blue-600 hover:text-blue-800">{translations.consent?.read_more ?? "Read more"}</a><br/>
                <label><input type="radio" name="sf" value="1" bind:group={consentChoice} class="rounded m-2" required/> {translations.consent?.yes ?? "Yes"}</label>
                <label><input type="radio" name="sf" value="2" bind:group={consentChoice} class="rounded m-2" required/> {translations.consent?.no ?? "No"}</label><br/>
            </div>
            <input type="hidden" name="fw" value="true"/>
            <div class="my-3">
                {translations.consent?.auto_update ?? "Automatic firmware updates"}<br/>
                <label><input type="radio" name="fwa" value="true" bind:group={autoUpdateChoice} class="rounded m-2" required/> {translations.consent?.yes ?? "Yes"}</label>
                <label><input type="radio" name="fwa" value="false" bind:group={autoUpdateChoice} class="rounded m-2" required/> {translations.consent?.no ?? "No"}</label><br/>
            </div>
            <div class="my-3">
                <label class="block text-sm font-medium text-slate-700 dark:text-slate-200" for="consent-meter-preset">{translations.conf?.meter?.preset?.title ?? "Meter preset"}</label>
                <div class="mt-2 flex gap-2">
                    <select id="consent-meter-preset" class="in-s w-full" bind:value={selectedMeterPresetId} on:change={(event) => handlePresetSelection(event.target.value)}>
                        <option value="">{translations.conf?.meter?.preset?.manual ?? "Manual configuration"}</option>
                        {#each meterPresets as preset}
                            <option value={preset.id}>{preset.label}</option>
                        {/each}
                    </select>
                    {#if selectedMeterPresetId}
                        <button type="button" class="text-xs text-blue-600 hover:text-blue-800 dark:text-blue-300 dark:hover:text-blue-200" on:click={clearMeterPresetSelection}>
                            {translations.conf?.meter?.preset?.clear ?? "Clear"}
                        </button>
                    {/if}
                </div>
                {#if selectedMeterPreset}
                    <div class="mt-1 text-xs text-slate-500 dark:text-slate-400 leading-snug">
                        {describePresetSummary(selectedMeterPreset)}
                        {#if selectedMeterPreset.notes}
                            <span class="block">{selectedMeterPreset.notes}</span>
                        {/if}
                    </div>
                {/if}
            </div>
            <div class="my-3">
                {translations.consent?.language ?? "Language"}<br/>
                <select name="ulang" class="in-s" bind:value={languageChoice} on:change={handleLanguageChange}>
                    {#each languages as lang}
                        <option value={lang.code}>{lang.name}</option>
                    {/each}
                    <option value="hub">{translations.consent?.load_from_server ?? "Load from server"}</option>
                </select>
            </div>
            <div class="my-3">
                <button type="submit" class="btn-pri" disabled={!canSave}>{translations.btn?.save ?? "Save"}</button>
            </div>
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message={translations.consent?.mask_saving ?? "Saving"}/>
