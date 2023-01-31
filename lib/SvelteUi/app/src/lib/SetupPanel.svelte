<script>
    import { sysinfoStore } from './DataStores.js';
    import Mask from './Mask.svelte'
    import SubnetOptions from './SubnetOptions.svelte';

    export let sysinfo = {}

    let staticIp = false;
    let loadingOrSaving = false;

    let tries = 0; 
    function scanForDevice() {
        var url = "";
        tries++;

        var retry = function() {
            setTimeout(scanForDevice, 1000);
        };

        if(sysinfo.net.ip && tries%3 == 0) {
            if(sysinfo.net.ip == '0.0.0.0') {
                retry();
                return;
            };
            url = "http://" + sysinfo.net.ip;
        } else if(sysinfo.hostname && tries%3 == 1) {
            url = "http://" + sysinfo.hostname;
        } else if(sysinfo.hostname && tries%3 == 2) {
            url = "http://" + sysinfo.hostname + ".local";
        } else {
            url = "";
        }
        if(console) console.log("Trying url " + url);
        sysinfoStore.update(s => {
            s.trying = url;
            return s;
        });
        
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
        const formData = new FormData(e.target);
        const data = new URLSearchParams();
        for (let field of formData) {
            const [key, value] = field;
			data.append(key, value)
        }

        const response = await fetch('/save', {
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
            setTimeout(scanForDevice, 5000);
            return s;
        });
    }
</script>


<div class="grid xl:grid-cols-4 lg:grid-cols-3 md:grid-cols-2">
    <div class="cnt">
        <form on:submit|preventDefault={handleSubmit}>
            <input type="hidden" name="s" value="true"/>
            <strong class="text-sm">Setup</strong>
            <div class="my-3">
                SSID<br/>
                <input name="ss" type="text" class="in-s" required/>
            </div>
            <div class="my-3">
                PSK<br/>
                <input name="sp" type="password" class="in-s" autocomplete="off"/>
            </div>
            <div>
                Hostname
                <input name="sh" bind:value={sysinfo.hostname} type="text" class="in-s" maxlength="32" pattern="[a-z0-9_-]+" placeholder="Optional, ex.: ams-reader" autocomplete="off"/>
            </div>
            <div class="my-3">
                <label><input type="checkbox" name="sm" value="static" class="rounded mb-1" bind:checked={staticIp} /> Static IP</label>
                {#if staticIp}
                <br/>
                <div class="flex">
                    <input name="si" type="text" class="in-f w-full" required={staticIp}/>
                    <select name="su" class="in-l" required={staticIp}>
                        <SubnetOptions/>
                    </select>
                </div>
                {/if}
            </div>
            {#if staticIp}
            <div class="my-3 flex">
                <div>
                    Gateway<br/>
                    <input name="sg" type="text" class="in-f w-full"/>
                </div>
                <div>
                    DNS<br/>
                    <input name="sd" type="text" class="in-l w-full"/>
                </div>
            </div>
            {/if}
            <div class="my-3">
                <button type="submit" class="btn-pri">Save</button>
            </div>
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message="Saving your configuration to the device"/>
