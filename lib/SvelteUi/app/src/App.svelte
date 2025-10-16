<script>
  import { Router, Route, navigate } from "svelte-navigator";
  import {
    getTariff,
    tariffStore,
    sysinfoStore,
    dataStore,
    pricesStore,
    dayPlotStore,
    monthPlotStore,
    temperaturesStore,
    getSysinfo,
  } from "./lib/DataStores.js";
  import {
    translationsStore,
    getTranslations,
  } from "./lib/TranslationService.js";
  import Favicon from "./assets/favicon.svg"; // Need this for the build
  import NeasLogo from "./assets/neas_logotype_white.svg";
  import WifiLowIcon from "./assets/wifi-low-light.svg";
  import WifiMediumIcon from "./assets/wifi-medium-light.svg";
  import WifiHighIcon from "./assets/wifi-high-light.svg";
  import WifiOffIcon from "./assets/wifi-off-light.svg";
  import Header from "./lib/Header.svelte";
  import Dashboard from "./lib/Dashboard.svelte";
  import ConfigurationPanel from "./lib/ConfigurationPanel.svelte";
  import StatusPage from "./lib/StatusPage.svelte";
  import VendorPanel from "./lib/VendorPanel.svelte";
  import SetupPanel from "./lib/SetupPanel.svelte";
  import Welcome from "./lib/Welcome.svelte";
  import Mask from "./lib/Mask.svelte";
  import FileUploadComponent from "./lib/FileUploadComponent.svelte";
  import ConsentComponent from "./lib/ConsentComponent.svelte";
  import PriceConfig from "./lib/PriceConfig.svelte";
  import DataEdit from "./lib/DataEdit.svelte";
  import { updateRealtime } from "./lib/RealtimeStore.js";
  import { onMount, onDestroy } from "svelte";

  let basepath = document.getElementsByTagName("base")[0].getAttribute("href");
  if (!basepath) basepath = "/";

  let prices;
  pricesStore.subscribe((update) => {
    prices = update;
  });

  let dayPlot;
  dayPlotStore.subscribe((update) => {
    dayPlot = update;
  });

  let monthPlot;
  monthPlotStore.subscribe((update) => {
    monthPlot = update;
  });

  let temperatures;
  temperaturesStore.subscribe((update) => {
    temperatures = update;
  });

  let translations = {};
  translationsStore.subscribe((update) => {
    translations = update;
  });

  let sito;
  let data = {};
  let sysinfo = {};
  let currentVersion;
  const normalizePathname = (pathname = "") => {
    if (typeof pathname !== "string") {
      return "/";
    }

    let normalized = pathname || "/";

    if (basepath && basepath !== "/" && normalized.startsWith(basepath)) {
      normalized = normalized.slice(basepath.length);
      if (!normalized.startsWith("/")) {
        normalized = `/${normalized}`;
      }
    }

    if (!normalized.startsWith("/")) {
      normalized = `/${normalized}`;
    }

    if (normalized.length > 1) {
      normalized = normalized.replace(/\/+$/, "");
    }

    return normalized || "/";
  };

  sysinfoStore.subscribe((update) => {
    sysinfo = update;
    const currentPath = (() => {
      if (typeof window === "undefined") return "";
      const normalized = normalizePathname(window.location.pathname ?? "/");
      return normalized.replace(/^\/+/, "");
    })();

    if (sysinfo.vndcfg === false) {
      if (currentPath === "setup") {
        navigate(basepath + "vendor");
      } else if (currentPath !== "welcome" && currentPath !== "vendor") {
        navigate(basepath + "welcome");
      }
    } else if (sysinfo.usrcfg === false) {
      if (currentPath !== "welcome" && currentPath !== "setup") {
        navigate(basepath + "welcome");
      }
    } else if (sysinfo.fwconsent === 0) {
      if (currentPath !== "consent") navigate(basepath + "consent");
    }

    if (sysinfo.ui.k === 1) {
      document.documentElement.classList.add("dark");
    } else if (sysinfo.ui.k === 0) {
      document.documentElement.classList.remove("dark");
    } else {
      if (window.matchMedia("(prefers-color-scheme: dark)").matches) {
        document.documentElement.classList.add("dark");
      } else {
        document.documentElement.classList.remove("dark");
      }
    }

    if (sysinfo.ui.lang && sysinfo.ui.lang != translations?.language?.code) {
      getTranslations(sysinfo.ui.lang);
    }

    if (
      sysinfo.version &&
      currentVersion &&
      sysinfo.version != currentVersion
    ) {
      window.location.reload();
    }

    currentVersion = sysinfo.version;

    if (sito) clearTimeout(sito);
    sito = setTimeout(
      getSysinfo,
      !data || !data.u || data.u < 30 || sysinfo?.upgrading ? 10000 : 300000,
    );
  });

  dataStore.subscribe((update) => {
    data = update;
    updateRealtime(update);
  });

  let tariffData = {};
  tariffStore.subscribe((update) => {
    tariffData = update;
  });
  getTariff();

  let currentPathname =
    typeof window !== "undefined"
      ? normalizePathname(window.location.pathname ?? "/")
      : "/";

  const shouldHideHeaderPath = (pathname = "/") =>
    pathname === "/setup" || pathname === "/consent" || pathname === "/welcome";

  const updateCurrentPathname = () => {
    if (typeof window !== "undefined") {
      currentPathname = normalizePathname(window.location.pathname ?? "/");
    }
  };

  $: hideHeader = shouldHideHeaderPath(currentPathname);

  onMount(() => {
    updateCurrentPathname();

    const handlePopState = () => updateCurrentPathname();
    window.addEventListener("popstate", handlePopState);

    const originalPushState = history.pushState;
    const originalReplaceState = history.replaceState;

    history.pushState = function (...args) {
      originalPushState.apply(this, args);
      updateCurrentPathname();
    };

    history.replaceState = function (...args) {
      originalReplaceState.apply(this, args);
      updateCurrentPathname();
    };

    return () => {
      window.removeEventListener("popstate", handlePopState);
      history.pushState = originalPushState;
      history.replaceState = originalReplaceState;
    };
  });

  onDestroy(() => {
    updateCurrentPathname();
  });
</script>

<div class="container mx-auto m-3">
  <Router {basepath}>
    {#if !hideHeader}
      <Header {data} {basepath} />
    {/if}
    <Route path="/welcome">
      <Welcome {basepath} />
    </Route>
    <Route path="/">
      <Dashboard
        {data}
        {sysinfo}
        {prices}
        {dayPlot}
        {monthPlot}
        {temperatures}
        {translations}
        {tariffData}
      />
    </Route>
    <Route path="/configuration">
      <ConfigurationPanel {sysinfo} {basepath} {data} />
    </Route>
    <Route path="/priceconfig">
      <PriceConfig {basepath} />
    </Route>
    <Route path="/status">
      <StatusPage {sysinfo} {data} />
    </Route>
    <Route path="/mqtt-ca">
      <FileUploadComponent title="CA" action="/mqtt-ca" />
    </Route>
    <Route path="/mqtt-cert">
      <FileUploadComponent title="certificate" action="/mqtt-cert" />
    </Route>
    <Route path="/mqtt-key">
      <FileUploadComponent title="private key" action="/mqtt-key" />
    </Route>
    <Route path="/consent">
      <ConsentComponent {sysinfo} {basepath} />
    </Route>
    <Route path="/setup">
      <SetupPanel {sysinfo} {data} />
    </Route>
    <Route path="/vendor">
      <VendorPanel {sysinfo} {basepath} />
    </Route>
    <Route path="/edit-day">
      <DataEdit prefix="UTC Hour" data={dayPlot} url="/dayplot" {basepath} />
    </Route>
    <Route path="/edit-month">
      <DataEdit prefix="Day" data={monthPlot} url="/monthplot" {basepath} />
    </Route>
  </Router>

  {#if sysinfo.booting}
    {#if sysinfo.trying}
      <Mask
        active="true"
        message="Device is booting, please wait. Trying to reach it on {sysinfo.trying}"
      />
    {:else}
      <Mask active="true" message="Device is booting, please wait" />
    {/if}
  {/if}
</div>
