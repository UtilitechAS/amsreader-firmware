<script>
    import { Link } from "svelte-navigator";
    import { sysinfoStore } from './DataStores.js';
    import GitHubLogo from './../assets/github.svg';
    import Uptime from "./Uptime.svelte";
    import Badge from './Badge.svelte';
    import Clock from './Clock.svelte';
    import GearIcon from './GearIcon.svelte';
    import InfoIcon from "./InfoIcon.svelte";
    import HelpIcon from "./HelpIcon.svelte";
    import ReloadIcon from "./ReloadIcon.svelte";

    export let data = {}
    export let sysinfo = {}

    async function reboot() {
      const response = await fetch('/reboot', {
            method: 'POST'
        });
        let res = (await response.json())
    }

    const askReload = function() {
      if(confirm('Are you sure you want to reboot the device?')) {
        sysinfoStore.update(s => {
            s.booting = true;
            return s;
        });
        reboot();
      }
    }
</script>

<nav class="bg-violet-600 p-1 rounded-md mx-2">
      <div class="flex flex-wrap space-x-4 text-sm text-gray-300">
        <div class="flex-none text-lg text-gray-100 p-2">
          <Link to="/">AMS reader <span>{sysinfo.version}</span></Link>
        </div>
        <div class="flex-none my-auto p-2 flex space-x-4">
          <div class="flex-none my-auto"><Uptime epoch={data.u}/></div>
          <div class="flex-none my-auto">{ data.t ? data.t.toFixed(1) : '-' }&deg;C</div>
          <div class="flex-none my-auto">Free mem: {data.m ? (data.m/1000).toFixed(1) : '-'}kb</div>
        </div>
        <div class="flex-auto my-auto justify-center p-2">
          <Badge title="ESP" text={sysinfo.booting ? 'Booting' : data.v > 0 ? data.v.toFixed(2)+"V" : "ESP"} color={sysinfo.booting ? 'yellow' : data.em === 1 ? 'green' : data.em === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
          <Badge title="HAN" text="HAN" color={sysinfo.booting ? 'gray' : data.hm === 1 ? 'green' : data.hm === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
          <Badge title="WiFi" text={data.r ? data.r.toFixed(0)+"dBm" : "WiFi"} color={sysinfo.booting ? 'gray' : data.wm === 1 ? 'green' : data.wm === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
          <Badge title="MQTT" text="MQTT" color={sysinfo.booting ? 'gray' : data.mm === 1 ? 'green' : data.mm === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
        </div>
        <div class="flex-auto p-2 flex flex-row-reverse">
          <div class="flex-none">
            <a class="float-right" href='https://github.com/gskjold/AmsToMqttBridge' target='_blank' rel="noreferrer" aria-label="GitHub"><img class="gh-logo" src={GitHubLogo} alt="GitHub repo"/></a>
          </div>
          <div class="flex-none my-auto px-2">
            <Clock timestamp={ data.c ? new Date(data.c * 1000) : new Date(0) } />
          </div>
          <div class="flex-none px-1 mt-1" title="Configuration">
            <Link to="/configuration"><GearIcon/></Link>
          </div>
          <div class="flex-none px-1 mt-1" title="Configuration">
            <button on:click={askReload} class={sysinfo.booting ? 'text-yellow-300' : ''}><ReloadIcon/></button>
          </div>
          <div class="flex-none px-1 mt-1" title="Device information">
            <Link to="/status"><InfoIcon/></Link>
          </div>
          <div class="flex-none px-1 mt-1" title="Device information">
            <a href="https://github.com/gskjold/AmsToMqttBridge/wiki" target='_blank' rel="noreferrer"><HelpIcon/></a>
          </div>
        </div> 
      </div>
</nav>