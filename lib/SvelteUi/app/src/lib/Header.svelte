<script>
    import { Link } from "svelte-navigator";
    import GitHubLogo from './../assets/github.svg';
    import Uptime from "./Uptime.svelte";
    import Badge from './Badge.svelte';
    import Clock from './Clock.svelte';
    import GearIcon from './GearIcon.svelte';

    export let data = {}
    let timestamp = new Date(0);
</script>

<nav class="bg-violet-600 p-1 rounded-md mx-2">
      <div class="flex flex-wrap space-x-4 text-sm text-gray-300">
        <div class="flex-none text-lg text-gray-100 p-2">
          <Link to="/">AMS reader <span>v0.0.0</span></Link>
        </div>
        <div class="flex-none my-auto p-2 flex space-x-4">
          <div class="flex-none my-auto"><Uptime epoch={data.u}/></div>
          <div class="flex-none my-auto">{ data.t ? data.t.toFixed(1) : '-' }&deg;C</div>
          <div class="flex-none my-auto">Free mem: {data.m ? (data.m/1000).toFixed(1) : '-'}kb</div>
        </div>
        <div class="flex-auto my-auto justify-center p-2">
          <Badge title="ESP" text={data.v > 0 ? data.v.toFixed(2)+"V" : "ESP"} color={data.em === 1 ? 'green' : data.em === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
          <Badge title="HAN" text="HAN" color={data.hm === 1 ? 'green' : data.hm === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
          <Badge title="WiFi" text={data.r ? data.r.toFixed(0)+"dBm" : "WiFi"} color={data.wm === 1 ? 'green' : data.wm === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
          <Badge title="MQTT" text="MQTT" color={data.mm === 1 ? 'green' : data.mm === 2 ? 'yellow' : data.em === 3 ? 'red' : 'gray'}/>
        </div>
        <div class="flex-auto p-2 flex flex-row-reverse">
          <div class="flex-none">
            <a class="float-right" href='https://github.com/gskjold/AmsToMqttBridge' target='_blank' rel="noreferrer" aria-label="GitHub"><img class="gh-logo" src={GitHubLogo} alt="GitHub repo"/></a>
          </div>
          <div class="flex-none my-auto px-2">
            <Clock timestamp={ data.c ? new Date(data.c * 1000) : new Date(0) } />
          </div>
          <div class="flex-none px-2 mt-1">
            <Link to="/configuration"><GearIcon/></Link>
          </div>
        </div> 
      </div>
</nav>