<script>
  import { Link } from "svelte-navigator";
  import { sysinfoStore } from "./DataStores.js";
  import { upgrade, upgradeWarningText } from "./UpgradeHelper";
  import { boardtype, isBusPowered, wiki, bcol } from "./Helpers.js";
  import { translationsStore } from "./TranslationService.js";
  import NeasLogo from "./../assets/neas_logotype_white.svg";
  import FavIco from "./../assets/favicon.svg";
  import Uptime from "./Uptime.svelte";
  import Badge from "./Badge.svelte";
  import Clock from "./Clock.svelte";
  import GearIcon from "./GearIcon.svelte";
  import InfoIcon from "./InfoIcon.svelte";
  import HelpIcon from "./HelpIcon.svelte";
  import WifiLowIcon from "./../assets/wifi-low-light.svg";
  import WifiMediumIcon from "./../assets/wifi-medium-light.svg";
  import WifiHighIcon from "./../assets/wifi-high-light.svg";
  import WifiOffIcon from "./../assets/wifi-off-light.svg";

  let wifiIcon = WifiOffIcon;
  let wifiTitle = "Wi-Fi offline";

  export let basepath = "/";
  export let data = {};
  let sysinfo = {};

  function askUpgrade() {
    if (
      confirm(
        (translations.header?.upgrade ?? "Upgrade to {0}?").replace(
          "{0}",
          sysinfo.upgrade.n,
        ),
      )
    ) {
      upgrade(sysinfo.upgrade.n);
      sysinfoStore.update((s) => {
        s.upgrade.t = sysinfo.upgrade.n;
        s.upgrade.p = 0;
        s.upgrading = true;
        return s;
      });
    }
  }

  let progress;
  sysinfoStore.subscribe((update) => {
    sysinfo = update;
  });

  let translations = {};
  translationsStore.subscribe((update) => {
    translations = update;
  });

  $: {
    progress = Math.max(0, sysinfo.upgrade.p);
  }

  $: {
    const rssi = data?.r;

    if (typeof rssi === "number") {
      if (rssi >= -50) {
        wifiIcon = WifiHighIcon;
        wifiTitle = `Wi-Fi strong (${rssi} dBm)`;
      } else if (rssi >= -60) {
        wifiIcon = WifiMediumIcon;
        wifiTitle = `Wi-Fi medium (${rssi} dBm)`;
      } else if (rssi >= -75) {
        wifiIcon = WifiLowIcon;
        wifiTitle = `Wi-Fi weak (${rssi} dBm)`;
      } else {
        wifiIcon = WifiOffIcon;
        wifiTitle = `Wi-Fi very weak/offline (${rssi} dBm)`;
      }
    }
  }
</script>

<nav class="bg-neas-green-90 rounded-md">
  <div class="flex flex-wrap space-x-4 text-l text-neas-gray">
    <div class="flex text-xl text-neas-gray p-2 flex-auto">
      <Link to="/" class="flex space-x-2">
        <img class="p-1" alt="Neas logo" src={NeasLogo} />
      </Link>
    </div>
    <div class="flex-none my-auto p-2 flex space-x-4">
      <div class="flex-none my-auto"><Uptime epoch={data.u} /></div>
      {#if data.t > -50}
        <div class="flex-none my-auto">
          {data.t > -50 ? data.t.toFixed(1) : "-"}&deg;C
        </div>
      {/if}
    </div>
    <div class="flex-auto flex-wrap my-auto justify-center p-2">
      <Badge
        title="Strømmåler"
        text="Strømmåler"
        color={bcol(sysinfo.booting ? 9 : data.hm)}
      />
      <Badge
        title="Minside"
        text="Minside"
        color={bcol(sysinfo.booting ? 9 : data.mm)}
      />
    </div>
    {#if data.he < 0 || data.he > 0}
      <div class="bd-red">
        {(translations.header?.han ?? "Strømmåler") +
          ": " +
          (translations.errors?.han?.[data.he] ?? data.he)}
      </div>
    {/if}
    {#if data.me < 0}
      <div class="bd-red">
        {(translations.header?.mqtt ?? "Minside") +
          ": " +
          (translations.errors?.mqtt?.[data.me] ?? data.me)}
      </div>
    {/if}
    {#if data.ee > 0 || data.ee < 0}
      <div class="bd-red">
        {(translations.header?.price ?? "PS") +
          ": " +
          (translations.errors?.price?.[data.ee] ?? data.ee)}
      </div>
    {/if}
    <div class="flex-auto p-2 flex flex-row-reverse flex-wrap">
      <div class="flex-none flex text-xl text-neas-gray p-2 flex-auto">
        <img class="h-10 w-10" src={wifiIcon} alt={wifiTitle} />
      </div>
      <div class="flex-none my-auto px-2">
        <Clock
          timestamp={data.c ? new Date(data.c * 1000) : new Date(0)}
          offset={sysinfo.clock_offset}
          fullTimeColor="text-red-500"
        />
      </div>
      {#if sysinfo.vndcfg && sysinfo.usrcfg}
        <div
          class="flex-none px-1 mt-1 pt-[0.5rem]"
          title={translations.header?.config ?? ""}
        >
          <Link to="/configuration"><GearIcon /></Link>
        </div>
        <div
          class="flex-none px-1 mt-1 pt-[0.5rem]"
          title={translations.header?.status ?? ""}
        >
          <Link to="/status"><InfoIcon /></Link>
        </div>
      {/if}

      {#if sysinfo.upgrading}
        <div class="flex-none mr-3 mt-1 text-yellow-300">
          Upgrading to {sysinfo.upgrade.t}, {progress.toFixed(1)}%
        </div>
      {:else if sysinfo.fwconsent === 1 && sysinfo.upgrade.n}
        <div
          class="flex-none mr-3 text-yellow-500"
          title={(translations.header?.new_version ?? "New version") +
            ": " +
            sysinfo.upgrade.n}
        >
          {#if sysinfo.security == 0 || data.a}
            <button on:click={askUpgrade} class="flex"
              ><span class="mt-1"
                >{translations.header?.new_version ?? "New version"}: {sysinfo
                  .upgrade.n}</span
              ></button
            >
          {:else}
            <span
              >{translations.header?.new_version ?? "New version"}: {sysinfo
                .upgrade.n}</span
            >
          {/if}
        </div>
      {/if}
    </div>
  </div>
</nav>
