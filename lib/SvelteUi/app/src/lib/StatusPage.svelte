<script>
    import { metertype, boardtype } from './Helpers.js';
    import { getSysinfo, gitHubReleaseStore, sysinfoStore } from './DataStores.js';
    import { upgrade, getNextVersion } from './UpgradeHelper';
    import DownloadIcon from './DownloadIcon.svelte';
    import UploadIcon from './UploadIcon.svelte';
  
    export let sysinfo = {}
    export let data = {}

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

    let fileinput;

    getSysinfo();
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">Device information</strong>
        <div class="my-2">
            Chip: {sysinfo.chip}
        </div>
        <div class="my-2">
            Device: {boardtype(sysinfo.chip, sysinfo.board)}
        </div>
    </div>
    {#if sysinfo.meter}
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
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
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
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
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">Firmware</strong>
        <div class="my-2">
            Installed version: {sysinfo.version}
        </div>
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
            <div class="my-auto bg-yellow-500 text-yellow-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">You have not consented to OTA firmware upgrade, link to self-upgrade is disabled</div>
        </div>
        {/if}
        {#if sysinfo.board == 2 || sysinfo.board == 4 || sysinfo.board == 7 }
        <div class="my-auto bg-red-500 text-red-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">
            {boardtype(sysinfo.chip, sysinfo.board)} must be connected to an external power supply during firmware upgrade. Failure to do so may cause power-down during upload resulting in non-functioning unit. 
        </div>
        {/if}
        <div class="my-2 flex">
            <form action="/firmware" enctype="multipart/form-data" method="post">
                <input style="display:none" name="file" type="file" accept=".bin" bind:this={fileinput}>
                {#if fileinput && fileinput.files.length == 0}
                <button type="button" on:click={()=>{fileinput.click();}} class="text-xs py-1 px-2 rounded bg-blue-500 text-white float-right mr-3">Choose firmware file</button>
                {:else if fileinput}
                {fileinput.files[0].name}
                <button type="submit" class="ml-2 text-xs py-1 px-2 rounded bg-blue-500 text-white float-right mr-3">Upload</button>
                {/if}
            </form>
        </div>
    </div>
</div>
