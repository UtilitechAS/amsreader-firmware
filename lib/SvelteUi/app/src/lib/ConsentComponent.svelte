<script>
    import { sysinfoStore } from './DataStores.js';
    import { translationsStore } from './TranslationService.js';
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
    let canSave = false;

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
    }

    $: canSave = consentChoice !== '' && autoUpdateChoice !== '' && !loadingOrSaving;

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
                <button type="submit" class="btn-pri" disabled={!canSave}>{translations.btn?.save ?? "Save"}</button>
            </div>
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message={translations.consent?.mask_saving ?? "Saving"}/>
