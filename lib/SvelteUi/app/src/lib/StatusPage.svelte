<script>
    import { metertype, boardtype } from './Helpers.js';
    import { getSysinfo, gitHubReleaseStore, sysinfoStore } from './DataStores.js';
    import { upgrade, getNextVersion } from './UpgradeHelper';
    import DownloadIcon from './DownloadIcon.svelte';
    import { Link } from 'svelte-navigator';
    import Mask from './Mask.svelte';
  
    export let sysinfo;
  
    let nextVersion = {};
    gitHubReleaseStore.subscribe(releases => {
      nextVersion = getNextVersion(sysinfo.version, releases);
      if(!nextVersion) {
        nextVersion = releases[0];
      }
    });
 
    function askUpgrade() {
        if(confirm('Do you want to upgrade this device to ' + nextVersion.tag_name + '?')) {
            if((sysinfo.board != 2 && sysinfo.board != 4 && sysinfo.board != 7) || confirm('WARNING: ' + boardtype(sysinfo.chip, sysinfo.board) + ' must be connected to an external power supply during firmware upgrade. Failure to do so may cause power-down during upload resulting in non-functioning unit.')) {
                sysinfoStore.update(s => {
                    s.upgrading = true;
                    return s;
                });
                upgrade(nextVersion);
            }
        }
    }

    async function reboot() {
      const response = await fetch('/reboot', {
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

    let fileinput;
    let files = [];
    let uploading = false;
    getSysinfo();
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
    <div class="cnt">
        <strong class="text-sm">Device information</strong>
        <div class="my-2">
            Chip: {sysinfo.chip}
        </div>
        <div class="my-2">
            Device: {boardtype(sysinfo.chip, sysinfo.board)}
        </div>
        <div class="my-2">
            MAC: {sysinfo.mac}
        </div>
        <div class="my-2">
            <Link to="/consent">
                <span class="text-xs py-1 px-2 rounded bg-blue-500 text-white mr-3 ">Change consents</span>
            </Link>
            <button on:click={askReboot} class="text-xs py-1 px-2 rounded bg-yellow-500 text-white mr-3 float-right">Reboot</button>
        </div>
     </div>
    {#if sysinfo.meter}
    <div class="cnt">
        <strong class="text-sm">Meter</strong>
        <div class="my-2">
            Manufacturer: {metertype(sysinfo.meter.mfg)}
        </div>
        <div class="my-2">
            Model: {sysinfo.meter.model}
        </div>
        <div class="my-2">
            ID: {sysinfo.meter.id}
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
            DNS: {sysinfo.net.dns1} {#if sysinfo.net.dns2 != '0.0.0.0'}/ {sysinfo.net.dns2}{/if}
        </div>
    </div>
    {/if}
    <div class="cnt">
        <strong class="text-sm">Firmware</strong>
        <div class="my-2">
            Installed version: {sysinfo.version}
        </div>
        {#if nextVersion}
        <div class="my-2 flex">
            Latest version: 
            <a href={nextVersion.html_url} class="ml-2 text-blue-600 hover:text-blue-800" target='_blank' rel="noreferrer">{nextVersion.tag_name}</a>
            {#if sysinfo.fwconsent === 1 && nextVersion && nextVersion.tag_name}
            <div class="flex-none ml-2 text-green-500" title="Install this version">
                <button on:click={askUpgrade}><DownloadIcon/></button>
            </div>
            {/if}
        </div>
        {#if sysinfo.fwconsent === 2}
        <div class="my-2">
            <div class="bd-ylo">You have disabled one-click firmware upgrade, link to self-upgrade is disabled</div>
        </div>
        {/if}
        {/if}
        {#if sysinfo.board == 2 || sysinfo.board == 4 || sysinfo.board == 7 }
        <div class="bd-red">
            {boardtype(sysinfo.chip, sysinfo.board)} must be connected to an external power supply during firmware upgrade. Failure to do so may cause power-down during upload resulting in non-functioning unit. 
        </div>
        {/if}
        <div class="my-2 flex">
            <form action="/firmware" enctype="multipart/form-data" method="post" on:submit={() => uploading=true}>
                <input style="display:none" name="file" type="file" accept=".bin" bind:this={fileinput} bind:files={files}>
                {#if files.length == 0}
                <button type="button" on:click={()=>{fileinput.click();}} class="text-xs py-1 px-2 rounded bg-blue-500 text-white float-right mr-3">Select firmware file for upgrade</button>
                {:else}
                {files[0].name}
                <button type="submit" class="ml-2 text-xs py-1 px-2 rounded bg-blue-500 text-white float-right mr-3">Upload</button>
                {/if}
            </form>
        </div>
    </div>
</div>
<Mask active={uploading} message="Uploading firmware, please wait"/>
