<script>
    import { sysinfoStore } from "./DataStores.js";
    import {
        translationsStore,
        getTranslations,
    } from "./TranslationService.js";
    import fetchWithTimeout from "./fetchWithTimeout";
    import Mask from "./Mask.svelte";
    import { navigate } from "svelte-navigator";
    import { wiki } from "./Helpers";
    import {
        meterPresets,
        getMeterPresetById,
        buildMeterStateFromPreset,
        createMeterStateFromConfiguration,
        describePresetSummary,
        applyMeterStateToConfiguration,
    } from "./meterPresets.js";
    import NeasLogo from "../assets/neas_logotype_white.svg";

    export let basepath = "/";
    export let sysinfo = {};

    let translations = {};
    translationsStore.subscribe((update) => {
        translations = update;
    });

    let loadingOrSaving = false;
    let consentChoice = "";
    let autoUpdateChoice = "";
    let languageChoice = "no"; // Default to Norwegian
    let languages = [
        { code: "en", name: "English" },
        { code: "no", name: "Norsk" },
    ];
    let canSave = false;
    let meterState = createMeterStateFromConfiguration();
    let selectedMeterPresetId = "";
    let selectedMeterPreset = null;

    // Load Norwegian translations on component initialization
    if (languageChoice === "no") {
        getTranslations("no");
    }

    $: if (
        translations?.language?.code &&
        !languages.find((lang) => lang.code === translations.language.code)
    ) {
        languages = [
            ...languages,
            {
                code: translations.language.code,
                name: translations.language.name ?? translations.language.code,
            },
        ];
    }

    $: if (sysinfo) {
        if (sysinfo.fwconsent === 1 || sysinfo.fwconsent === 2) {
            consentChoice = String(sysinfo.fwconsent);
        }
        const autoFlag = sysinfo?.upgrade?.auto;
        if (autoFlag === true) {
            autoUpdateChoice = "true";
        } else if (autoFlag === false) {
            autoUpdateChoice = "false";
        }
        const sysLang = sysinfo?.ui?.lang;
        if (sysLang && !languages.find((lang) => lang.code === sysLang)) {
            languages = [
                ...languages,
                {
                    code: sysLang,
                    name: translations.language?.name ?? sysLang.toUpperCase(),
                },
            ];
        }
        if (!languageChoice && sysLang) {
            languageChoice = sysLang;
        }
    }

    $: {
        if (!languageChoice) {
            languageChoice = "no"; // Ensure Norwegian is always the default
        }
    }

    $: canSave =
        consentChoice !== "" && autoUpdateChoice !== "" && !loadingOrSaving;

    async function handleLanguageChange(e) {
        const selected = e.target.value;
        if (selected === "hub") {
            try {
                const response = await fetchWithTimeout(
                    "http://hub.amsleser.no/hub/language/list.json",
                );
                languages = await response.json();
                languageChoice = translations.language.code;
            } catch (err) {
                console.error("Failed to load languages from hub", err);
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
            meterState = buildMeterStateFromPreset(
                createMeterStateFromConfiguration(),
                preset,
            );
        } else {
            meterState = createMeterStateFromConfiguration();
        }
    }

    function clearMeterPresetSelection() {
        selectedMeterPresetId = "";
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
            encryptionKey: state.encryptionKey ?? "",
            authenticationKey: state.authenticationKey ?? "",
            multipliers: {
                watt: state.multipliers?.watt ?? 1,
                volt: state.multipliers?.volt ?? 1,
                amp: state.multipliers?.amp ?? 1,
                kwh: state.multipliers?.kwh ?? 1,
            },
        };
    }

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target);
        const data = new URLSearchParams();
        for (let field of formData) {
            const [key, value] = field;
            data.append(key, typeof value === "string" ? value : String(value));
        }

        const consentValue = formData.get("sf");
        const autoValue = formData.get("fwa");
        const autoDecision =
            autoValue === "true" ? true : autoValue === "false" ? false : null;

        const meterPayload = buildMeterPayload();
        if (meterPayload) {
            data.set("m", "true");
            data.set("mo", String(meterPayload.source ?? 1));
            data.set("ma", String(meterPayload.parser ?? 0));
            data.set("mb", String(meterPayload.baud ?? 0));
            data.set("mp", String(meterPayload.parity ?? 3));
            data.set("mi", meterPayload.invert ? "true" : "false");
            data.set("md", String(meterPayload.distributionSystem ?? 2));
            data.set("mf", String(meterPayload.mainFuse ?? 0));
            data.set("mr", String(meterPayload.production ?? 0));
            data.set("ms", String(meterPayload.buffer ?? 256));
            if (meterPayload.encrypted) {
                data.set("me", "true");
                data.set("mek", meterPayload.encryptionKey ?? "");
                data.set("mea", meterPayload.authenticationKey ?? "");
            }
            data.set("mmw", String(meterPayload.multipliers?.watt ?? 1));
            data.set("mmv", String(meterPayload.multipliers?.volt ?? 1));
            data.set("mma", String(meterPayload.multipliers?.amp ?? 1));
            data.set("mmc", String(meterPayload.multipliers?.kwh ?? 1));
        }

        const response = await fetch("save", {
            method: "POST",
            body: data,
        });
        let res = await response.json();
        loadingOrSaving = false;

        sysinfoStore.update((s) => {
            s.fwconsent =
                consentValue === "1" ? 1 : consentValue === "2" ? 2 : 0;
            if (!s.ui || typeof s.ui !== "object") {
                s.ui = {};
            }
            if (languageChoice) {
                s.ui.lang = languageChoice;
            }
            if (!s.upgrade || typeof s.upgrade !== "object") {
                s.upgrade = { x: -1, e: 0, f: null, t: null, m: false };
            }
            if (autoDecision !== null) {
                s.upgrade.auto = autoDecision;
            }
            if (meterPayload) {
                const config = applyMeterStateToConfiguration(
                    s.m ?? {},
                    meterState,
                );
                config.appliedPresetId = meterState.appliedPresetId;
                s.m = config;
            }
            s.booting = res.reboot;
            return s;
        });
        navigate(basepath);
    }
</script>

<div
    class="min-h-screen bg-neas-green flex flex-col items-center p-4"
>
    <!-- Neas Logo -->
    <div class="mb-8">
        <svg
            class="w-24 h-24 text-blue-600 dark:text-blue-400"
            viewBox="0 0 100 100"
            fill="currentColor"
        >
            <!-- Simple modern logo placeholder - replace with actual Neas logo -->
            <img alt="Neas logo" src={NeasLogo} class="w-full h-full" />
        </svg>
    </div>

    <!-- Main Card -->
    <div
        class="bg-neas-green dark:neas-green rounded-2xl max-w-md w-full p-8"
    >
        <form on:submit|preventDefault={handleSubmit} autocomplete="off">
            <!-- Title -->
            <div class="text-center mb-8">
                <h1
                    class="text-2xl font-semibold text-slate-900 dark:text-slate-100 mb-2"
                >
                    {translations.consent?.title ?? "Initial Setup"}
                </h1>
                <p class="text-sm text-slate-600 dark:text-slate-400">
                    Complete the setup to get started
                </p>
            </div>
            <!-- Data Collection Consent -->
            <div class="mb-6">
                <div class="mb-3">
                    <h3
                        class="text-sm font-medium text-slate-900 dark:text-slate-100 mb-1"
                    >
                        {translations.consent?.one_click ?? "Data Collection"}
                    </h3>
                    <p class="text-xs text-slate-600 dark:text-slate-400 mb-2">
                        <a
                            href={wiki(
                                "Data-collection-on-one-click-firmware-upgrade",
                            )}
                            target="_blank"
                            class="text-blue-600 hover:text-blue-700 dark:text-blue-400 dark:hover:text-blue-300 underline"
                        >
                            {translations.consent?.read_more ?? "Read more"}
                        </a>
                    </p>
                </div>
                <div class="space-y-2">
                    <label
                        class="flex items-center p-3 border border-slate-200 dark:border-slate-600 rounded-lg bg-neas-green-100 dark:bg-neas-green-90 hover:bg-neas-green-90 dark:hover:bg-neas-green cursor-pointer transition-colors"
                    >
                        <input
                            type="radio"
                            name="sf"
                            value="1"
                            bind:group={consentChoice}
                            class="mr-3 text-blue-600"
                            required
                        />
                        <span class="text-sm text-slate-700 dark:text-slate-300"
                            >{translations.consent?.yes ??
                                "Yes, allow data collection"}</span
                        >
                    </label>
                    <label
                        class="flex items-center p-3 border border-slate-200 dark:border-slate-600 rounded-lg bg-neas-green-100 dark:bg-neas-green-90 hover:bg-neas-green-90 dark:hover:bg-neas-green cursor-pointer transition-colors"
                    >
                        <input
                            type="radio"
                            name="sf"
                            value="2"
                            bind:group={consentChoice}
                            class="mr-3 text-blue-600"
                            required
                        />
                        <span class="text-sm text-slate-700 dark:text-slate-300"
                            >{translations.consent?.no ??
                                "No, disable data collection"}</span
                        >
                    </label>
                </div>
            </div>

            <!-- Automatic Updates -->
            <input type="hidden" name="fw" value="true" />
            <div class="mb-6">
                <div class="mb-3">
                    <h3
                        class="text-sm font-medium text-slate-900 dark:text-slate-100 mb-1"
                    >
                        {translations.consent?.auto_update ??
                            "Automatic Updates"}
                    </h3>
                </div>
                <div class="space-y-2">
                    <label
                        class="flex items-center p-3 border border-slate-200 dark:border-slate-600 rounded-lg bg-neas-green-100 dark:bg-neas-green-90 hover:bg-neas-green-90 dark:hover:bg-neas-green cursor-pointer transition-colors"
                    >
                        <input
                            type="radio"
                            name="fwa"
                            value="true"
                            bind:group={autoUpdateChoice}
                            class="mr-3 text-blue-600"
                            required
                        />
                        <span class="text-sm text-slate-700 dark:text-slate-300"
                            >{translations.consent?.yes ??
                                "Yes, enable automatic updates"}</span
                        >
                    </label>
                    <label
                        class="flex items-center p-3 border border-slate-200 dark:border-slate-600 rounded-lg bg-neas-green-100 dark:bg-neas-green-90 hover:bg-neas-green-90 dark:hover:bg-neas-green cursor-pointer transition-colors"
                    >
                        <input
                            type="radio"
                            name="fwa"
                            value="false"
                            bind:group={autoUpdateChoice}
                            class="mr-3 text-blue-600"
                            required
                        />
                        <span class="text-sm text-slate-700 dark:text-slate-300"
                            >{translations.consent?.no ??
                                "No, manual updates only"}</span
                        >
                    </label>
                </div>
            </div>
            <!-- Meter Configuration -->
            <div class="mb-6">
                <label
                    class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-3"
                    for="consent-meter-preset"
                >
                    {translations.conf?.meter?.preset?.title ??
                        "Meter Configuration"}
                </label>
                <div class="space-y-2">
                    <select
                        id="consent-meter-preset"
                        class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors"
                        bind:value={selectedMeterPresetId}
                        on:change={() =>
                            handlePresetSelection(selectedMeterPresetId)}
                    >
                        <option value=""
                            >{translations.conf?.meter?.preset?.manual ??
                                "Select your meter type..."}</option
                        >
                        {#each meterPresets as preset}
                            <option value={preset.id}>{preset.label}</option>
                        {/each}
                    </select>
                    {#if selectedMeterPresetId}
                        <button
                            type="button"
                            class="text-xs text-blue-600 hover:text-blue-700 dark:text-blue-400 dark:hover:text-blue-300 font-medium"
                            on:click={clearMeterPresetSelection}
                        >
                            {translations.conf?.meter?.preset?.clear ??
                                "Clear selection"}
                        </button>
                    {/if}
                </div>
                {#if selectedMeterPreset}
                    <div
                        class="mt-3 p-3 bg-blue-50 dark:bg-blue-900/20 border border-blue-200 dark:border-blue-800 rounded-lg"
                    >
                        <p
                            class="text-xs text-blue-800 dark:text-blue-200 leading-relaxed"
                        >
                            {describePresetSummary(selectedMeterPreset)}
                            {#if selectedMeterPreset.notes}
                                <span class="block mt-1 font-medium"
                                    >{selectedMeterPreset.notes}</span
                                >
                            {/if}
                        </p>
                    </div>
                {/if}
            </div>
            {#if selectedMeterPreset}
                <!-- Voltage Configuration -->
                <div class="mb-6">
                    <label
                        for="voltage-select"
                        class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2"
                    >
                        {translations.common?.voltage ?? "Voltage System"}
                    </label>
                    <select
                        id="voltage-select"
                        bind:value={meterState.distributionSystem}
                        class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors"
                    >
                        <option value={2}>400V (TN - Three-phase)</option>
                        <option value={1}>230V (IT/TT - Single-phase)</option>
                    </select>
                </div>

                <!-- Main Fuse Configuration -->
                <div class="mb-6">
                    <label
                        for="main-fuse-input"
                        class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2"
                    >
                        {translations.conf?.meter?.fuse ?? "Main Fuse"}
                    </label>
                    <div class="relative">
                        <input
                            id="main-fuse-input"
                            type="number"
                            bind:value={meterState.mainFuse}
                            min="5"
                            max="65535"
                            class="w-full px-3 py-2 pr-8 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors"
                        />
                        <span
                            class="absolute right-3 top-2 text-slate-500 dark:text-slate-400 text-sm"
                            >A</span
                        >
                    </div>
                </div>
            {/if}
            <!-- Language Selection -->
            <div class="mb-8">
                <label
                    for="language-select"
                    class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2"
                >
                    {translations.consent?.language ?? "Language"}
                </label>
                <select
                    id="language-select"
                    name="ulang"
                    class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors"
                    bind:value={languageChoice}
                    on:change={handleLanguageChange}
                >
                    {#each languages as lang}
                        <option value={lang.code}>{lang.name}</option>
                    {/each}
                    <option value="hub"
                        >{translations.consent?.load_from_server ??
                            "Load from server"}</option
                    >
                </select>
            </div>

            <!-- Submit Button -->
            <div class="text-center">
                <button
                    type="submit"
                    class="w-full bg-neas-lightgreen hover:bg-neas-lightgreen-30 disabled:bg-slate-400 disabled:cursor-not-allowed text-white font-medium py-3 px-6 rounded-lg transition-colors focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 dark:focus:ring-offset-slate-800"
                    disabled={!canSave}
                >
                    {translations.btn?.save ?? "Complete Setup"}
                </button>
            </div>
        </form>
    </div>
</div>

<Mask
    active={loadingOrSaving}
    message={translations.consent?.mask_saving ?? "Saving setup..."}
/>
