<script>
    import { sysinfoStore } from './DataStores.js';
    import Mask from './Mask.svelte'

    export let sysinfo = {}

    let staticIp = false;
    let loadingOrSaving = false;

    let tries = 0; 
    function scanForDevice() {
        var url = "";
        tries++;

        if(sysinfo.net.ip && tries%3 == 0) {
            url = "http://" + sysinfo.net.ip;
        } else if(sysinfo.hostname && tries%3 == 1) {
            url = "http://" + sysinfo.hostname;
        } else if(sysinfo.hostname && tries%3 == 2) {
            url = "http://" + sysinfo.hostname + ".local";
        } else {
            url = "";
        }
        if(console) console.log("Trying url " + url);

        var retry = function() {
            setTimeout(scanForDevice, 1000);
        };
        
        var xhr = new XMLHttpRequest();
        xhr.timeout = 5000;
        xhr.addEventListener('abort', retry);
        xhr.addEventListener('error', retry);
        xhr.addEventListener('timeout', retry);
        xhr.addEventListener('load', function(e) {
            window.location.href = url ? url : "/";
        });
        xhr.open("GET", url + "/is-alive", true);
        xhr.send();
    };

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target)
        const data = new URLSearchParams()
        for (let field of formData) {
            const [key, value] = field
            data.append(key, value)
        }

        const response = await fetch('/save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())
        loadingOrSaving = false;

        sysinfoStore.update(s => {
            s.usrcfg = res.success;
            s.booting = res.reboot;
            setTimeout(scanForDevice, 5000);
            return s;
        });
    }
</script>


<div class="z-10" aria-modal="true">
    <div class="fixed inset-0 bg-gray-500 bg-opacity-50 flex items-center justify-center">
        <div class="bg-white m-2 p-3 rounded-md shadow-lg pb-4 text-gray-700 w-96">
            <form on:submit|preventDefault={handleSubmit}>
                <input type="hidden" name="s" value="true"/>
                <strong class="text-sm">Setup</strong>
                <div class="my-3">
                    SSID<br/>
                    <input name="ss" type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
                </div>
                <div class="my-3">
                    PSK<br/>
                    <input name="sp" type="password" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
                </div>
                <div>
                    Hostname:
                    <input name="sh" bind:value={sysinfo.hostname} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full" maxlength="32" pattern="[a-z0-9_-]+" placeholder="Optional, ex.: ams-reader"/>
                </div>
                <div class="my-3">
                    <label><input type="checkbox" name="sm" value="static" class="rounded mb-1" bind:checked={staticIp} /> Static IP</label>
                    {#if staticIp}
                    <br/>
                    <div class="flex">
                        <input name="si" type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full" required={staticIp}/>
                        <select name="su" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300" required={staticIp}>
                            <option value="255.255.255.0">/24</option>
                            <option value="255.255.0.0">/16</option>
                            <option value="255.0.0.0">/8</option>
                        </select>
                    </div>
                    {/if}
                </div>
                {#if staticIp}
                <div class="my-3 flex">
                    <div>
                        Gateway<br/>
                        <input name="sg" type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    </div>
                    <div>
                        DNS<br/>
                        <input name="sd" type="text" class="h-10 rounded-r-md border-l-0 shadow-sm border-gray-300 w-full"/>
                    </div>
                </div>
                {/if}
                <div class="my-3">
                    <label><input type="checkbox" name="sf" value="true" class="rounded mb-1"/> Enable OTA upgrade (implies data collection)</label><br/>
                    <a href="https://github.com/gskjold/AmsToMqttBridge/wiki/Data-collection-on-OTA-firmware-upgrade" target="_blank" class="text-blue-600 hover:text-blue-800">Read more</a>
                </div>
                <div class="my-3">
                    <button type="submit" class="font-bold py-1 px-4 rounded bg-blue-500 text-white float-right">Save</button>
                </div>
            </form>
        </div>
    </div>
</div>

<Mask active={loadingOrSaving} message="Saving your configuration to the device"/>
