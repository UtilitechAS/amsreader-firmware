<script>
    import { metertype, boardtype } from './Helpers.js';
    import { getSysinfo } from './DataStores.js';
  
    export let sysinfo = {}
    export let data = {}

    getSysinfo();
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">Device information</strong>
        <div class="my-3">
            Chip: {sysinfo.chip}
        </div>
        <div class="my-3">
            Device: {boardtype(sysinfo.chip, sysinfo.board)}
        </div>
        <div class="my-3">
            Firmware version: {sysinfo.version}
        </div>
    </div>
    {#if sysinfo.meter}
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">Meter</strong>
        <div class="my-3">
            Manufacturer: {metertype(sysinfo.meter.mfg)}
        </div>
        <div class="my-3">
            Model: {sysinfo.meter.model}
        </div>
        <div class="my-3">
            ID: {sysinfo.meter.id}
        </div>
    </div>
    {/if}
    {#if sysinfo.net}
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">Network</strong>
        <div class="my-3">
            IP: {sysinfo.net.ip}
        </div>
        <div class="my-3">
            Mask: {sysinfo.net.mask}
        </div>
        <div class="my-3">
            Gateway: {sysinfo.net.gw}
        </div>
        <div class="my-3">
            DNS: {sysinfo.net.dns1} {#if sysinfo.net.dns2 != '0.0.0.0'}/ {sysinfo.net.dns2}{/if}
        </div>
    </div>
    {/if}
</div>
