<script>
    import { Link } from "svelte-navigator";
    import { sysinfoStore } from './DataStores.js';
    import { upgrade, upgradeWarningText } from './UpgradeHelper';
    import { boardtype, isBusPowered, wiki, bcol } from './Helpers.js';
    import { translationsStore } from "./TranslationService.js";
    import FavIco from './../assets/favicon.svg';
    import Uptime from "./Uptime.svelte";
    import Badge from './Badge.svelte';
    import Clock from './Clock.svelte';
    import GearIcon from './GearIcon.svelte';
    import InfoIcon from "./InfoIcon.svelte";
    import HelpIcon from "./HelpIcon.svelte";

    export let basepath = "/";
    export let data = {};
    let sysinfo = {};

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

    let progress;
    sysinfoStore.subscribe(update => {
      sysinfo = update;
    });

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    $: {
      progress = Math.max(0, sysinfo.upgrade.p);
    }
</script>

<nav class="hdr">
      <div class="flex flex-wrap space-x-4 text-sm text-gray-300">
        <div class="flex text-lg text-gray-100 p-2">
          <Link to="/">AMS reader <span>{sysinfo.version}</span></Link>
        </div>
        <div class="flex-none my-auto p-2 flex space-x-4">
          <div class="flex-none my-auto"><Uptime epoch={data.u}/></div>
          {#if data.t > -50}
          <div class="flex-none my-auto">{ data.t > -50 ? data.t.toFixed(1) : '-' }&deg;C</div>
          {/if}
          <div class="flex-none my-auto">{translations.header?.mem ?? "Free"}: {data.m ? (data.m/1000).toFixed(1) : '-'}kb</div>
        </div>
        <div class="flex-auto flex-wrap my-auto justify-center p-2">
          <Badge title="ESP" text={sysinfo.booting ? (translations.header?.booting ?? "Booting") : data.v > 2.0 ? data.v.toFixed(2)+"V" : "ESP"} color={bcol(sysinfo.booting ? 2 : data.em)}/>
          <Badge title="HAN"  text="HAN"  color={bcol(sysinfo.booting ? 9 : data.hm)}/>
          <Badge title="WiFi" text={data.r ? data.r.toFixed(0)+"dBm" : "WiFi"} color={bcol(sysinfo.booting ? 9 : data.wm)}/>
          <Badge title="MQTT" text="MQTT" color={bcol(sysinfo.booting ? 9 : data.mm)}/>
        </div>
        {#if data.he < 0 || data.he > 0}
        <div class="bd-red">{ (translations.header?.han ?? "HAN") + ': ' + (translations.errors?.han?.[data.he] ?? data.he) }</div>
        {/if}
        {#if data.me < 0}
        <div class="bd-red">{ (translations.header?.mqtt ?? "MQTT") + ': ' + (translations.errors?.mqtt?.[data.me] ?? data.me) }</div>
        {/if}
        {#if data.ee > 0 || data.ee < 0}
        <div class="bd-red">{ (translations.header?.price ?? "PS") + ': ' + (translations.errors?.price?.[data.ee] ?? data.ee) }</div>
        {/if}
      <div class="flex-auto p-2 flex flex-row-reverse flex-wrap">
          <div class="flex-none">
            <a class="float-right" href='https://github.com/UtilitechAS/amsreader-firmware' target='_blank' rel="noreferrer" aria-label="GitHub"><img class="logo" src={(basepath + "/logo.svg").replace('//','/')} alt="GitHub repo"/></a>
          </div>
          <div class="flex-none my-auto px-2">
            <Clock timestamp={ data.c ? new Date(data.c * 1000) : new Date(0) } offset={sysinfo.clock_offset} fullTimeColor="text-red-500" />
          </div>
          {#if sysinfo.vndcfg && sysinfo.usrcfg}
          <div class="flex-none px-1 mt-1" title={translations.header?.config ?? ""}>
            <Link to="/configuration"><GearIcon/></Link>
          </div>
          <div class="flex-none px-1 mt-1" title={translations.header?.status ?? ""}>
            <Link to="/status"><InfoIcon/></Link>
          </div>
          {/if}
          <div class="flex-none px-1 mt-1" title={translations.header?.doc ?? ""}>
            <a href="{wiki('')}" target='_blank' rel="noreferrer"><HelpIcon/></a>
          </div>
          {#if sysinfo.upgrading}
            <div class="flex-none mr-3 mt-1 text-yellow-300">Upgrading to {sysinfo.upgrade.t}, {progress.toFixed(1)}%</div>
          {:else if sysinfo.fwconsent === 1 && sysinfo.upgrade.n}
          <div class="flex-none mr-3 text-yellow-500" title={(translations.header?.new_version ?? "New version") + ': ' + sysinfo.upgrade.n}>
            {#if sysinfo.security == 0 || data.a}
            <button on:click={askUpgrade} class="flex"><span class="mt-1">{translations.header?.new_version ?? "New version"}: {sysinfo.upgrade.n}</span></button>
            {:else}
            <span>{translations.header?.new_version ?? "New version"}: {sysinfo.upgrade.n}</span>
            {/if}
          </div>
          {/if}
        </div> 
      </div>
</nav>