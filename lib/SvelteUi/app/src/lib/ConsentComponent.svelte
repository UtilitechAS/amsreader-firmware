<script>
    import { sysinfoStore } from './DataStores.js';
    import { translationsStore, getTranslations } from './TranslationService.js';
    import fetchWithTimeout from './fetchWithTimeout';
    import Mask from './Mask.svelte'
    import { navigate } from 'svelte-navigator';
    import { wiki } from './Helpers';

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

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target)
        const data = new URLSearchParams()
        for (let field of formData) {
            const [key, value] = field
            data.append(key, value)
        }

        const consentValue = formData.get('sf');
        const autoValue = formData.get('fwa');
        const autoDecision = autoValue === 'true' ? true : autoValue === 'false' ? false : null;

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
                s.upgrade = {};
            }
            if (autoDecision !== null) {
                s.upgrade.auto = autoDecision;
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
