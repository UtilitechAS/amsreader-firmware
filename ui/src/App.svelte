<script>
  import Router from "svelte-spa-router";
  import { push } from "svelte-spa-router";
  import { getTariff, sysinfoStore, dataStore, getSysinfo } from './lib/DataStores.js';
  import { translationsStore, getTranslations } from "./lib/TranslationService.js";
  import Header from './lib/Header.svelte';
  import DashboardRoute from './routes/DashboardRoute.svelte';
  import ConfigurationRoute from './routes/ConfigurationRoute.svelte';
  import StatusRoute from './routes/StatusRoute.svelte';
  import PriceConfigRoute from './routes/PriceConfigRoute.svelte';
  import MqttCaRoute from './routes/MqttCaRoute.svelte';
  import MqttCertRoute from './routes/MqttCertRoute.svelte';
  import MqttKeyRoute from './routes/MqttKeyRoute.svelte';
  import ConsentRoute from './routes/ConsentRoute.svelte';
  import SetupRoute from './routes/SetupRoute.svelte';
  import VendorRoute from './routes/VendorRoute.svelte';
  import EditDayRoute from './routes/EditDayRoute.svelte';
  import EditMonthRoute from './routes/EditMonthRoute.svelte';
  import Mask from './lib/Mask.svelte';
  import { updateRealtime } from "./lib/RealtimeStore.js";
  
  let basepath = document.getElementsByTagName('base')[0].getAttribute("href");
  if(!basepath) basepath = "/";

  let translations = {};
  translationsStore.subscribe(update => {
    translations = update;
  });

  let sito;
  let data = {};
  let sysinfo = {};
  let currentVersion;
  sysinfoStore.subscribe(update => {
    sysinfo = update;
    if(sysinfo.vndcfg === false) {
      push("/vendor");
    } else if(sysinfo.usrcfg === false) {
      push("/setup");
    } else if(sysinfo.fwconsent === 0) {
      push("/consent");
    }

    if(sysinfo.ui.k === 1) {
        document.documentElement.classList.add('dark')
    } else if (sysinfo.ui.k === 0) {
        document.documentElement.classList.remove('dark')
    } else {
        if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
            document.documentElement.classList.add('dark')
        } else {
            document.documentElement.classList.remove('dark')
        }
    }

    if(sysinfo.ui.lang && sysinfo.ui.lang != translations?.language?.code) {
      getTranslations(sysinfo.ui.lang);
    }

    if(sysinfo.version && currentVersion && sysinfo.version != currentVersion) {
      window.location.reload();
    }

    currentVersion = sysinfo.version;

    if(sito) clearTimeout(sito);
    sito = setTimeout(getSysinfo, !data || !data.u || data.u < 30 || sysinfo?.upgrading ? 10000 : 300000);
  });

  dataStore.subscribe(update => {
    data = update;
    updateRealtime(update);
  });

  getTariff();
</script>

<div class="container mx-auto m-3">
  <Header data={data} basepath={basepath}/>
  
  <Router routes={{
    '/': DashboardRoute,
    '/configuration': ConfigurationRoute,
    '/priceconfig': PriceConfigRoute,
    '/status': StatusRoute,
    '/mqtt-ca': MqttCaRoute,
    '/mqtt-cert': MqttCertRoute,
    '/mqtt-key': MqttKeyRoute,
    '/consent': ConsentRoute,
    '/setup': SetupRoute,
    '/vendor': VendorRoute,
    '/edit-day': EditDayRoute,
    '/edit-month': EditMonthRoute,
  }} />

  {#if sysinfo.booting}
    {#if sysinfo.trying}
      <Mask active=true message="Device is booting, please wait. Trying to reach it on {sysinfo.trying}"/>
    {:else}
      <Mask active=true message="Device is booting, please wait"/>
    {/if}
  {/if}
</div>
