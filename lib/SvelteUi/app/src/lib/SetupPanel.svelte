<script>
    import { sysinfoStore, networksStore } from './DataStores.js';
    import { translationsStore } from './TranslationService.js';
    import Mask from './Mask.svelte'
    import SubnetOptions from './SubnetOptions.svelte';
    import { scanForDevice, charAndNumPattern, asciiPatternExt, ipPattern } from './Helpers.js';
    import WifiLowIcon from "./../assets/wifi-low-light.svg";
    import WifiMediumIcon from "./../assets/wifi-medium-light.svg";
    import WifiHighIcon from "./../assets/wifi-high-light.svg";
    import WifiOffIcon from "./../assets/wifi-off-light.svg";

    let wifiIcon = WifiOffIcon;
    let wifiTitle = "Wi-Fi offline";

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let manual = false;
    let networks = {};
    networksStore.subscribe(update => {
        networks = update;
    });

    export let sysinfo = {}

    let staticIp = false;
    let connectionMode = 1;
    let loadingOrSaving = false;

    function updateSysinfo(url) {
        sysinfoStore.update(s => {
            s.trying = url;
            return s;
        });
    }

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target);
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


    $: {
    const rssi = data?.r;

    if (typeof rssi === "number") {
      if (rssi >= -50) {
        wifiIcon = WifiHighIcon;
        wifiTitle = `Wi-Fi strong (${rssi} dBm)`;
      } else if (rssi >= -60) {
        wifiIcon = WifiMediumIcon;
        wifiTitle = `Wi-Fi medium (${rssi} dBm)`;
      } else if (rssi >= -75) {
        wifiIcon = WifiLowIcon;
        wifiTitle = `Wi-Fi weak (${rssi} dBm)`;
      } else {
        wifiIcon = WifiOffIcon;
        wifiTitle = `Wi-Fi very weak/offline (${rssi} dBm)`;
      }
    }
  }
</script>


<div class="grid xl:grid-cols-4 lg:grid-cols-3 md:grid-cols-2">
    <div class="cnt">
        <form on:submit|preventDefault={handleSubmit}>
            <input type="hidden" name="s" value="true"/>
            <strong class="text-sm">{translations.setup?.title ?? "Setup"}</strong>
            <div class="my-3">
                {translations.conf?.connection?.title ?? "Connection"}<br/>
                <select name="sc" class="in-s" bind:value={connectionMode}>
                    <option value={1}>{translations.conf?.connection?.wifi ?? "Connect to WiFi"}</option>
                    {#if sysinfo.if && sysinfo.if.eth}
                    <option value={3}>{translations.conf?.connection?.eth ?? "Ethernet"}</option>
                    {/if}
                </select>
            </div>
            {#if connectionMode == 1 || connectionMode == 2}
                <div class="my-3">
                    {translations.conf?.connection?.ssid ?? "SSID"}
                    <br/>
                    {#if manual}
                        <input name="ss" type="text" pattern={asciiPatternExt} class="in-s" required={connectionMode == 1 || connectionMode == 2}/>
                    {:else}
                        {#if networks?.c == -1}
                            <p class="text-sm italic">{translations.conf?.connection?.scanning ?? "Scanning..."}</p>
                        {/if}
                        {#if networks?.n?.length}
                            <ul class="space-y-1">
                                {#each networks.n as network (network.s)}
                                    <li>
                                        <label class="flex items-center gap-2">
                                            <input type="radio" name="ss" value={network.s} required={connectionMode == 1 || connectionMode == 2}/>
                                            <span class="flex items-center justify-between w-full">
                                                <span>{network.s}</span>
                                                <img class="h-7 w-7" src={wifiIcon} alt={network.r}/>
                                            </span>
                                        </label>
                                    </li>
                                {/each}
                            </ul>
                        {:else if networks?.c != -1}
                            <p class="text-sm italic">{translations.conf?.connection?.noNetworks ?? "No networks found."}</p>
                        {/if}
                    {/if}
                </div>
                <div class="my-3">
                    {translations.conf?.connection?.psk ?? "Password"}<br/>
                    <input name="sp" type="password" pattern={asciiPatternExt} class="in-s" autocomplete="off" required={connectionMode == 2}/>
                </div>
            {/if}
            <div>
                {translations.conf?.general?.hostname ?? "Hostname"}
                <input name="sh" bind:value={sysinfo.hostname} type="text" class="in-s" maxlength="32" pattern={charAndNumPattern} placeholder="Optional, ex.: ams-reader" autocomplete="off"/>
            </div>
            <div class="my-3">
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
                <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
            </div>
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message={translations.setup?.mask ?? "Lagrer"}/>
