<script>
    import { metertype, boardtype, isBusPowered, getResetReason, httpError } from './Helpers.js';
    import { getSysinfo, gitHubReleaseStore, sysinfoStore } from './DataStores.js';
    import { upgrade, getNextVersion, upgradeWarningText } from './UpgradeHelper';
    import DownloadIcon from './DownloadIcon.svelte';
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
        name: 'Price API',
        key: 'is'
    }];
  
    let nextVersion = {};
    gitHubReleaseStore.subscribe(releases => {
      nextVersion = getNextVersion(sysinfo.version, releases);
      if(!nextVersion) {
        nextVersion = releases[0];
      }
    });
 
    function askUpgrade() {
        if(confirm('Do you want to upgrade this device to ' + nextVersion.tag_name + '?')) {
            if((sysinfo.board != 2 && sysinfo.board != 4 && sysinfo.board != 7) || confirm(upgradeWarningText(boardtype(sysinfo.chip, sysinfo.board)))) {
                sysinfoStore.update(s => {
                    s.upgrading = true;
                    return s;
                });
                upgrade(nextVersion.tag_name);
            }
        }
    }

    async function reboot() {
      const response = await fetch('reboot', {
            method: 'POST'
        });
        let res = (await response.json())
    }

    const askReboot = function() {
      if(confirm('Are you sure you want to reboot the device?')) {
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

        const upload = fetch('/configfile', {
            method: 'POST',
            body: formData
        }).then((response) => response.json()).then((res) => {
            sysinfoStore.update(s => {
                console.log('updating sysinfo with: ', newconfig);
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
        <strong class="text-sm">Device information</strong>
        <div class="my-2">
            Chip: {sysinfo.chip} ({sysinfo.cpu}MHz)
        </div>
        <div class="my-2">
            Device: <Link to="/vendor">{boardtype(sysinfo.chip, sysinfo.board)}</Link>
        </div>
        <div class="my-2">
            MAC: {sysinfo.mac}
        </div>
        {#if sysinfo.apmac && sysinfo.apmac != sysinfo.mac}
        <div class="my-2">
            AP MAC: {sysinfo.apmac}
        </div>
        <div class="my-2">
            Last boot:
            {#if data.u > 0}
            <Clock timestamp={new Date(new Date().getTime() - (data.u * 1000))} fullTimeColor="" />
            {:else}
            -
            {/if}
        </div>
        <div class="my-2">
            Reason: {getResetReason(sysinfo)} ({sysinfo.boot_reason}/{sysinfo.ex_cause})
        </div>
        {/if}
        <div class="my-2">
            <Link to="/consent">
                <span class="btn-pri-sm">Update consents</span>
            </Link>
            <button on:click={askReboot} class="btn-yellow-sm float-right">Reboot</button>
        </div>
     </div>
    {#if sysinfo.meter}
    <div class="cnt">
        <strong class="text-sm">Meter</strong>
        <div class="my-2">
            Manufacturer: {metertype(sysinfo.meter.mfg)}
        </div>
        <div class="my-2">
            Model: {sysinfo.meter.model ? sysinfo.meter.model : "unknown"}
        </div>
        <div class="my-2">
            ID: {sysinfo.meter.id ? sysinfo.meter.id : "unknown"}
        </div>
    </div>
    {/if}
    {#if sysinfo.net}
    <div class="cnt">
        <strong class="text-sm">Network</strong>
        <div class="my-2">
            IP: {sysinfo.net.ip}
        </div>
        <div class="my-2">
            Mask: {sysinfo.net.mask}
        </div>
        <div class="my-2">
            Gateway: {sysinfo.net.gw}
        </div>
        <div class="my-2">
            DNS: {sysinfo.net.dns1} {#if sysinfo.net.dns2}/ {sysinfo.net.dns2}{/if}
        </div>
    </div>
    {/if}
    <div class="cnt">
        <strong class="text-sm">Firmware</strong>
        <div class="my-2">
            Installed version: {sysinfo.version}
        </div>
        {#if sysinfo.upgrade.t && sysinfo.upgrade.t != sysinfo.version}
        <div class="my-2">
            <div class="bd-yellow">Previous upgrade attempt from {sysinfo.upgrade.f} to {sysinfo.upgrade.t} failed. {httpError(sysinfo.upgrade.e)}</div>
        </div>
        {/if}
        {#if nextVersion}
            <div class="my-2 flex">
                Latest version: 
                <a href={nextVersion.html_url} class="ml-2 text-blue-600 hover:text-blue-800" target='_blank' rel="noreferrer">{nextVersion.tag_name}</a>
                {#if (sysinfo.security == 0 || data.a) && sysinfo.fwconsent === 1 && nextVersion && nextVersion.tag_name != sysinfo.version}
                <div class="flex-none ml-2 text-green-500" title="Install this version">
                    <button on:click={askUpgrade}><DownloadIcon/></button>
                </div>
                {/if}
            </div>
            {#if sysinfo.fwconsent === 2}
            <div class="my-2">
                <div class="bd-yellow">You have disabled one-click firmware upgrade, link to self-upgrade is disabled</div>
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
            <form action="/firmware" enctype="multipart/form-data" method="post" on:submit={() => firmwareUploading=true} autocomplete="off">
                <input style="display:none" name="file" type="file" accept=".bin" bind:this={firmwareFileInput} bind:files={firmwareFiles}>
                {#if firmwareFiles.length == 0}
                <button type="button" on:click={()=>{firmwareFileInput.click();}} class="btn-pri-sm float-right">Select firmware file for upgrade</button>
                {:else}
                {firmwareFiles[0].name}
                <button type="submit" class="btn-pri-sm float-right">Upload</button>
                {/if}
            </form>
        </div>
        {/if}
    </div>
    {#if sysinfo.security == 0 || data.a}
    <div class="cnt">
        <strong class="text-sm">Backup & restore</strong>
        <form method="get" action="/configfile.cfg" autocomplete="off">
            <div class="grid grid-cols-2">
                {#each cfgItems as el}
                    <label class="my-1 mx-3"><input type="checkbox" class="rounded" name="{el.key}" value="true" checked/> {el.name}</label>
                {/each}
                <label class="my-1 mx-3 col-span-2"><input type="checkbox" class="rounded" name="ic" value="true"/> Include Secrets<br/><small>(SSID, PSK, passwords and tokens)</small></label>
            </div>
            {#if configFiles.length == 0}
            <button type="submit" class="btn-pri-sm float-right">Download</button>
            {/if}
        </form>
        <form on:submit|preventDefault={uploadConfigFile} autocomplete="off">
            <input style="display:none" name="file" type="file" accept=".cfg" bind:this={configFileInput} bind:files={configFiles}>
            {#if configFiles.length == 0}
            <button type="button" on:click={()=>{configFileInput.click();}} class="btn-pri-sm">Select file...</button>
            {:else}
            {configFiles[0].name}
            <button type="submit" class="btn-pri-sm">Upload</button>
            {/if}
        </form>
    </div>
    {/if}
</div>
<Mask active={firmwareUploading} message="Uploading firmware, please wait"/>
<Mask active={configUploading} message="Uploading configuration, please wait"/>
