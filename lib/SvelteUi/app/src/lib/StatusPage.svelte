<script>
    import { metertype, boardtype, isBusPowered, getBaseChip, wiki } from './Helpers.js';
    import { getSysinfo, sysinfoStore } from './DataStores.js';
    import { upgrade, upgradeWarningText } from './UpgradeHelper';
    import { translationsStore } from './TranslationService.js';
    import { Link } from 'svelte-navigator';
    import Clock from './Clock.svelte';
    import Mask from './Mask.svelte';
    import { scanForDevice } from './Helpers.js';
  
    export let data;
    export let sysinfo;

    let cfgItems = [{
        name: 'WiFi',
        key: 'iw'
    },{
        name: 'MQTT',
        key: 'im'
    },{
        name: 'Web',
        key: 'ie'
    },{
        name: 'Meter',
        key: 'it'
    },{
        name: 'Thresholds',
        key: 'ih'
    },{
        name: 'GPIO',
        key: 'ig'
    },{
        name: 'NTP',
        key: 'in'
    },{
        name: 'Price',
        key: 'is'
    }];
  
    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });
 
    function askUpgrade() {
        if(confirm((translations.header?.upgrade ?? "Upgrade to {0}?").replace('{0}',sysinfo.upgrade.n))) {
            upgrade(sysinfo.upgrade.n);
            sysinfoStore.update(s => {
                s.upgrade.t = sysinfo.upgrade.n;
                s.upgrade.p = 0;
                s.upgrading = true;
                return s;
            });
        }
    }

    async function reboot() {
      const response = await fetch('reboot', {
            method: 'POST'
        });
        let res = (await response.json())
    }

    const askReboot = function() {
      if(confirm((translations.device?.reboot_confirm ?? "Reboot?"))) {
        sysinfoStore.update(s => {
            s.booting = true;
            return s;
        });
        reboot();
      }
    }

    let firmwareFileInput;
    let firmwareFiles = [];
    let firmwareUploading = false;

    let configFileInput;
    let configFiles = [];
    let configUploading = false;

    getSysinfo();

    let newconfig = {
        hostname: '',
        ip: ''
    };

    function uploadConfigFile(e) {
        configUploading=true;
        const formData = new FormData();
        formData.append('file', configFiles[0]);

        const upload = fetch('configfile', {
            method: 'POST',
            body: formData
        }).then((response) => response.json()).then((res) => {
            sysinfoStore.update(s => {
                if(newconfig && newconfig.hostname) s.hostname = newconfig.hostname;
                s.booting = res.reboot;
                if(newconfig && newconfig.ip) s.net.ip = newconfig.ip;
                setTimeout(scanForDevice, 5000, sysinfo);
                return s;
            });
        }).catch((error) => {
            console.error('Error:', error);
            setTimeout(scanForDevice, 5000, sysinfo);
        });
    };

    function changeFirmwareChannel() {
        const formData = new FormData();
        formData.append('channel', sysinfo.upgrade.c);
        fetch('fwchannel', {
            method: 'POST',
            body: formData
        });
    };

    $: {
        if(configFiles.length == 1) {
            let file = configFiles[0];
            let reader = new FileReader();
            let parseConfigFile = ( e ) => {
                let lines = e.target.result.split('\n');
                for(let i in lines) {
                    let line = lines[i];
                    if(line.startsWith('hostname ')) {
                        newconfig.hostname = line.split(' ')[1];
                    } else if(line.startsWith('ip ')) {
                        newconfig.ip = line.split(' ')[1];
                    }
                }
            };
            reader.onload = parseConfigFile;
            reader.readAsText(file);
        }
    }
</script>

<div class="grid xl:grid-cols-5 lg:grid-cols-3 md:grid-cols-2">
    <div class="cnt">
        <strong class="text-sm">{translations.status?.device.title ?? "Device"}</strong>
        <div class="my-2">
            {translations.status?.device?.chip ?? "Chip"}: {sysinfo.chip} {#if sysinfo.cpu}({sysinfo.cpu}MHz){/if}
        </div>
        <div class="my-2">
            {translations.status?.device?.device ?? "Device"}: <Link to="/vendor">{boardtype(sysinfo.chip, sysinfo.board)}</Link>
        </div>
        <div class="my-2">
            {translations.status?.device?.mac ?? "MAC"}: {sysinfo.mac}
        </div>
        {#if sysinfo.apmac && sysinfo.apmac != sysinfo.mac}
        <div class="my-2">
            {translations.status?.device?.apmac ?? "AP MAC"}: {sysinfo.apmac}
        </div>
        <div class="my-2">
            {translations.status?.device?.last_boot ?? "Last boot"}:
            {#if data.u > 0}
            <Clock timestamp={new Date(new Date().getTime() - (data.u * 1000))} fullTimeColor="" offset={sysinfo.clock_offset}/>
            {:else}
            -
            {/if}
        </div>
        <div class="my-2">
            {translations.status?.device?.reason ?? "Reason"}: {(translations[getBaseChip(sysinfo.chip)]?.reason?.[sysinfo.boot_reason] ?? sysinfo.boot_reason)} ({sysinfo.boot_reason}/{sysinfo.ex_cause})
        </div>
        {/if}
        {#if data?.a}
        <div class="my-2">
            <Link to="/consent">
                <span class="btn-pri-sm">{translations.status?.device?.btn_consents ?? "Consents"}</span>
            </Link>
            <button on:click={askReboot} class="btn-yellow-sm float-right">{translations.btn?.reboot ?? "Reboot"}</button>
        </div>
        {/if}
     </div>
    {#if sysinfo.meter}
    <div class="cnt">
        <strong class="text-sm">{translations.status?.meter?.title ?? "Meter"}</strong>
        <div class="my-2">
            {translations.status?.meter?.manufacturer ?? "Manufacturer"}: {metertype(sysinfo.meter.mfg)}
        </div>
        <div class="my-2">
            {translations.status?.meter?.model ?? "Model"}: {sysinfo.meter.model ? sysinfo.meter.model : "unknown"}
        </div>
        <div class="my-2">
            {translations.status?.meter?.id ?? "ID"}: {sysinfo.meter.id ? sysinfo.meter.id : "unknown"}
        </div>
    </div>
    {/if}
    {#if sysinfo.net}
    <div class="cnt">
        <strong class="text-sm">{translations.status?.network?.title ?? "Network"}</strong>
        <div class="my-2">
            {translations.conf?.network?.ip ?? "IP"}: {sysinfo.net.ip}
        </div>
        <div class="my-2">
            {translations.conf?.network?.mask ?? "Mask"}: {sysinfo.net.mask}
        </div>
        <div class="my-2">
            {translations.conf?.network?.gw ?? "Gateway"}: {sysinfo.net.gw}
        </div>
        <div class="my-2">
            {#if sysinfo.net.dns1}{translations.conf?.network?.dns ?? "DNS"}: {sysinfo.net.dns1}{/if}
            {#if sysinfo.net.dns2}{translations.conf?.network?.dns ?? "DNS"}: {sysinfo.net.dns2}{/if}
        </div>
        {#if sysinfo.net.ipv6}
            <div class="my-2">
                IPv6: <span style="font-size: 14px;">{sysinfo.net.ipv6.replace(/\b:?(?:0+:?){2,}/, '::')}</span>
            </div>
            <div class="my-2">
                {#if sysinfo.net.dns1v6}DNSv6: <span style="font-size: 14px;">{sysinfo.net.dns1v6.replace(/\b:?(?:0+:?){2,}/, '::')}</span>{/if}
                {#if sysinfo.net.dns2v6}DNSv6: <span style="font-size: 14px;">{sysinfo.net.dns2v6.replace(/\b:?(?:0+:?){2,}/, '::')}</span>{/if}
            </div>
        {/if}
    </div>
    {/if}
    <div class="cnt">
        <strong class="text-sm">{translations.status?.firmware?.title ?? "Firmware"}</strong>
        <a href="{wiki('statusinformation-screen')}" target="_blank" class="float-right">&#9432;</a>
        {#if sysinfo.fwconsent === 1}
            <div class="my-2">
                Channel: 
                <select class="in-s w-full" bind:value={sysinfo.upgrade.c} on:change={changeFirmwareChannel}>
                    <option value={0}>Stable</option>
                    <option value={1}>Early</option>
                    <option value={2}>Release Candidate</option>
                    <option value={3} disabled>Snapshot</option>
                </select>
            </div>
        {/if}
        <div class="my-2">
            {translations.status?.firmware?.installed ?? "Installed"}: {sysinfo.version}
        </div>
        {#if sysinfo.upgrade.t && sysinfo.upgrade.t != sysinfo.version && sysinfo.upgrade.e != 0 && sysinfo.upgrade.e != 123}
            <div class="my-2">
                <div class="bd-yellow">
                    {(translations.status?.firmware?.failed ?? "Upgrade from {0} to {1} failed").replace('{0}', sysinfo.upgrade.f).replace('{1}', sysinfo.upgrade.t)}
                    {(translations.errors?.upgrade?.[sysinfo.upgrade.e] ?? sysinfo.upgrade.e)}
                </div>
            </div>
        {/if}
        {#if sysinfo.upgrade.n}
            <div class="my-2 flex">
                {translations.status?.firmware?.latest ?? "Latest"}: 
                <a href={"https://github.com/UtilitechAS/amsreader-firmware/releases/tag/" + sysinfo.upgrade.n} class="ml-2 text-blue-600 hover:text-blue-800" target='_blank' rel="noreferrer">{sysinfo.upgrade.n}</a>
                {#if (sysinfo.security == 0 || data.a) && sysinfo.fwconsent === 1 && sysinfo.upgrade.n && sysinfo.upgrade.n != sysinfo.version}
                    <div class="flex-none ml-2 text-green-500" title={translations.status?.firmware?.install ?? "Install"}>
                        <button on:click={askUpgrade}>&#8659;</button>
                    </div>
                {/if}
            </div>
            {#if sysinfo.fwconsent === 2}
            <div class="my-2">
                <div class="bd-yellow">{translations.status?.firmware?.no_one_click ?? "One-click upgrade disabled"}</div>
            </div>
            {/if}
        {/if}
        {#if (sysinfo.security == 0 || data.a) && isBusPowered(sysinfo.board) }
            <div class="bd-red">
                {upgradeWarningText(boardtype(sysinfo.chip, sysinfo.board))}
            </div>
        {/if}
        {#if sysinfo.security == 0 || data.a}
            <div class="my-2 flex">
                <form action="firmware" enctype="multipart/form-data" method="post" on:submit={() => firmwareUploading=true} autocomplete="off">
                    <input style="display:none" name="file" type="file" accept=".bin" bind:this={firmwareFileInput} bind:files={firmwareFiles}>
                    {#if firmwareFiles.length == 0}
                    <button type="button" on:click={()=>{firmwareFileInput.click();}} class="btn-pri-sm float-right">{translations.status?.firmware?.btn_select_file ?? "Select file"}</button>
                    {:else}
                    {firmwareFiles[0].name}
                    <button type="submit" class="btn-pri-sm float-right ml-2">{translations.btn?.upload ?? "Upload"}</button>
                    {/if}
                </form>
            </div>
        {/if}
    </div>
    {#if sysinfo.security == 0 || data.a}
    <div class="cnt">
        <strong class="text-sm">{translations.status?.backup?.title ?? "Backup"}</strong>
        <form method="get" action="configfile.cfg" autocomplete="off">
            <div class="grid grid-cols-2">
                {#each cfgItems as el}
                    <label class="my-1 mx-3"><input type="checkbox" class="rounded" name="{el.key}" value="true" checked/> {translations.status?.backup?.[el.key] ?? el.name}</label>
                {/each}
                <label class="my-1 mx-3 col-span-2"><input type="checkbox" class="rounded" name="ic" value="true"/> {translations.status?.backup?.secrets ?? "Include secrets"}<br/><small>{translations.status?.backup?.secrets_desc ?? ""}</small></label>
            </div>
            {#if configFiles.length == 0}
            <button type="submit" class="btn-pri-sm float-right">{translations.status?.backup?.btn_download ?? "Download"}</button>
            {/if}
        </form>
        <form on:submit|preventDefault={uploadConfigFile} autocomplete="off">
            <input style="display:none" name="file" type="file" accept=".cfg" bind:this={configFileInput} bind:files={configFiles}>
            {#if configFiles.length == 0}
            <button type="button" on:click={()=>{configFileInput.click();}} class="btn-pri-sm">{translations.status?.backup?.btn_select_file ?? "Select file"}</button>
            {:else}
            {configFiles[0].name}
            <button type="submit" class="btn-pri-sm">{translations.btn?.upload ?? "Upload"}</button>
            {/if}
        </form>
    </div>
    {/if}
</div>
<Mask active={firmwareUploading} message={translations.status?.mask?.firmware ?? "Uploading"}/>
<Mask active={configUploading} message={translations.status?.mask?.config ?? "Uploading"}/>
