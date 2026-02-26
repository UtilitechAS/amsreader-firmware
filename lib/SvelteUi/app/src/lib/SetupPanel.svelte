<script>
    import { sysinfoStore, networksStore } from './DataStores.js';
    import { translationsStore } from './TranslationService.js';
    import Mask from './Mask.svelte'
    import SubnetOptions from './SubnetOptions.svelte';
    import { scanForDevice, charAndNumPattern, asciiPatternExt, ipPattern } from './Helpers.js';

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let form;
    let ssid = '';
    let psk = '';
    let manual = false;
    let networks = {};
    networksStore.subscribe(update => {
        networks = update;
        manual = update?.c == 0;
        ssid = update?.n[0]?.s ?? '';
    });

    export let sysinfo = {}

    let staticIp = false;
    let connectionMode = 1;
    let loadingOrSaving = false;
    let wifiTestInProgress = false;
    let wifiTestOk = false;
    let wifiTestError = 0;

    function updateSysinfo(url) {
        sysinfoStore.update(s => {
            s.trying = url;
            return s;
        });
    }

    async function handleSubmit() {
        loadingOrSaving = true;
        const formData = new FormData(form);
        const data = new URLSearchParams();
        for (let field of formData) {
            const [key, value] = field;
			data.append(key, value)
        }

        const response = await fetch('save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())
        loadingOrSaving = false;

        sysinfoStore.update(s => {
            s.hostname = formData.get('sh');
            s.usrcfg = res.success;
            s.booting = res.reboot;
            if(staticIp) {
                s.net.ip = formData.get('si');
                s.net.mask = formData.get('su');
                s.net.gw = formData.get('sg');
                s.net.dns1 = formData.get('sd');
            }
            if(res.reboot) setTimeout(scanForDevice, 5000, sysinfo, updateSysinfo);
            return s;
        });
    }

    async function wifiTest() {
        let response;
        if(wifiTestInProgress) {
            response = await fetch('wifitest.json');
        } else {
            wifiTestInProgress = true;
            wifiTestOk = false;
            const data = new URLSearchParams();
            data.append('ssid', ssid);
            data.append('psk', psk);
            response = await fetch('wifitest.json', {
                method: 'POST',
                body: data
            });
        }
        const res = await response.json();
        if(res?.time == 0) {
            wifiTestInProgress = false;
            wifiTestOk = res.status == 3;
            wifiTestError = res.status;
            if(wifiTestOk) {
                sysinfoStore.update(s => {
                    s.net.ip = res.ip;
                    return s;
                });
                handleSubmit();
            }
        } else if(wifiTestInProgress) {
            if(res.time > 30000) {
                wifiTestError = 4;
                wifiTestInProgress = false;
            } else {
                setTimeout(wifiTest, 2000);
            }
        }
    }
</script>


<div class="grid xl:grid-cols-4 lg:grid-cols-3 md:grid-cols-2">
    <div class="cnt">
        <form bind:this={form} on:submit|preventDefault={handleSubmit}>
            <input type="hidden" name="s" value="true"/>
            <strong class="text-sm">{translations.setup?.title ?? "Setup"}</strong>
            <div class="my-3">
                {translations.conf?.connection?.title ?? "Connection"}<br/>
                <select name="sc" class="in-s" bind:value={connectionMode}>
                    <option value={1}>{translations.conf?.connection?.wifi ?? "Connect to WiFi"}</option>
                    <option value={2}>{translations.conf?.connection?.ap ?? "Standalone access point"}</option>
                    {#if sysinfo.if && sysinfo.if.eth}
                    <option value={3}>{translations.conf?.connection?.eth ?? "Ethernet"}</option>
                    {/if}
                </select>
            </div>
            {#if connectionMode == 1 || connectionMode == 2}
                <div class="my-3">
                    {translations.conf?.connection?.ssid ?? "SSID"}
                    <label class="float-right mr-3"><input type="checkbox" value="true" bind:checked={manual} class="rounded mb-1"/> manual</label>
                    <br/>
                    {#if manual}
                        <input name="ss" bind:value={ssid} type="text" pattern={asciiPatternExt} class="in-s" required={connectionMode == 1 || connectionMode == 2}/>
                    {:else}
                        <select name="ss" bind:value={ssid} class="in-s" required={connectionMode == 1 || connectionMode == 2}>
                            {#if networks?.c == -1}
                                <option value="" selected disabled>Scanning...</option>
                            {/if}
                            {#if networks?.n}
                                {#each networks?.n as network}
                                    <option value={network.s}>{network.s} ({network.e}, RSSI: {network.r})</option>
                                {/each}
                            {/if}
                        </select>
                    {/if}
                </div>
                <div class="my-3">
                    {translations.conf?.connection?.psk ?? "Password"}<br/>
                    <input name="sp" bind:value={psk} type="password" pattern={asciiPatternExt} class="in-s" autocomplete="off" required={connectionMode == 2}/>
                </div>
            {/if}
            <div>
                {translations.conf?.general?.hostname ?? "Hostname"}
                <input name="sh" bind:value={sysinfo.hostname} type="text" class="in-s" maxlength="32" pattern={charAndNumPattern} placeholder="Optional, ex.: ams-reader" autocomplete="off"/>
            </div>
            <div class="my-3">
                <label><input type="checkbox" name="sm" value="static" class="rounded mb-1" bind:checked={staticIp} /> {translations.setup?.static ?? "Static IP"}</label>
                {#if staticIp}
                <br/>
                <div class="flex">
                    <input name="si" type="text" class="in-f w-full" required={staticIp} pattern={ipPattern}/>
                    <select name="su" class="in-l" required={staticIp}>
                        <SubnetOptions/>
                    </select>
                </div>
                {/if}
            </div>
            {#if staticIp}
            <div class="my-3 flex">
                <div>
                    {translations.conf?.network?.gw ?? "Gateway"}<br/>
                    <input name="sg" type="text" class="in-f w-full" pattern={ipPattern}/>
                </div>
                <div>
                    {translations.conf?.network?.dns ?? "DNS"}<br/>
                    <input name="sd" type="text" class="in-l w-full" pattern={ipPattern}/>
                </div>
            </div>
            {/if}
            <div class="my-3">
                {#if connectionMode != 1}
                    <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
                {:else if wifiTestOk}
                    <div class="bd-green">{translations.setup?.testok ?? "Connection successful (" + sysinfo.net.ip + ")"}</div>
                    <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
                {:else if wifiTestInProgress}
                    <div class="bd-yellow">{translations.setup?.testconn ?? "Testing connection"}</div>
                {:else}
                    {#if wifiTestError}
                        <div class="bd-red">{ (translations.setup?.testfail ?? "Connection failed") + ': ' + (translations.errors?.wifi?.[wifiTestError] ?? wifiTestError) }</div>
                        <button type="submit" class="btn-pri">{translations.btn?.forcesave ?? "Force save"}</button>
                    {:else}
                        <button type="button" class="btn-pri" on:click={wifiTest}>{translations.btn?.save ?? "Save"}</button>
                    {/if}
                {/if}
            </div>
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message={translations.setup?.mask ?? "Saving"}/>
