<script>
  import { Router, Route, navigate } from "svelte-navigator";
  import { getSysinfo, sysinfoStore, dataStore, pricesStore, dayPlotStore, monthPlotStore, temperaturesStore } from './lib/DataStores.js';
  import { translationsStore, getTranslations } from "./lib/TranslationService.js";
  import Favicon from './assets/favicon.svg'; // Need this for the build
  import Header from './lib/Header.svelte';
  import Dashboard from './lib/Dashboard.svelte';
  import ConfigurationPanel from './lib/ConfigurationPanel.svelte';
  import StatusPage from './lib/StatusPage.svelte';
  import VendorPanel from './lib/VendorPanel.svelte';
  import SetupPanel from './lib/SetupPanel.svelte';
  import Mask from './lib/Mask.svelte';
  import FileUploadComponent from "./lib/FileUploadComponent.svelte";
  import ConsentComponent from "./lib/ConsentComponent.svelte";
  import PriceConfig from "./lib/PriceConfig.svelte";
  import DataEdit from "./lib/DataEdit.svelte";
  
  let basepath = document.getElementsByTagName('base')[0].getAttribute("href");
  if(!basepath) basepath = "/";

  let prices;
  pricesStore.subscribe(update => {
      prices = update;
  });

  let dayPlot;
  dayPlotStore.subscribe(update => {
      dayPlot = update;
  });

  let monthPlot;
  monthPlotStore.subscribe(update => {
      monthPlot = update;
  });

  let temperatures;
  temperaturesStore.subscribe(update => {
      temperatures = update;
  });

  let translations = {};
  translationsStore.subscribe(update => {
    translations = update;
  });

  let sysinfo = {};
  sysinfoStore.subscribe(update => {
    sysinfo = update;
    if(sysinfo.vndcfg === false) {
      navigate(basepath + "vendor");
    } else if(sysinfo.usrcfg === false) {
      navigate(basepath + "setup");
    } else if(sysinfo.fwconsent === 0) {
      navigate(basepath + "consent");
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
  });

  let data = {};
  dataStore.subscribe(update => {
    data = update;
  });

</script>

<div class="container mx-auto m-3">
  <Router basepath={basepath}>
    <Header data={data} basepath={basepath}/>
    <Route path="/">
      <Dashboard data={data} sysinfo={sysinfo} prices={prices} dayPlot={dayPlot} monthPlot={monthPlot} temperatures={temperatures} translations={translations}/>
    </Route>
    <Route path="/configuration">
      <ConfigurationPanel sysinfo={sysinfo} basepath={basepath} data={data}/>
    </Route>
    <Route path="/priceconfig">
      <PriceConfig basepath={basepath}/>
    </Route>
    <Route path="/status">
      <StatusPage sysinfo={sysinfo} data={data}/>
    </Route>
    <Route path="/mqtt-ca">
      <FileUploadComponent title="CA" action="/mqtt-ca"/>
    </Route>
    <Route path="/mqtt-cert">
      <FileUploadComponent title="certificate" action="/mqtt-cert"/>
    </Route>
    <Route path="/mqtt-key">
      <FileUploadComponent title="private key" action="/mqtt-key"/>
    </Route>
    <Route path="/consent">
      <ConsentComponent sysinfo={sysinfo} basepath={basepath}/>
    </Route>
    <Route path="/setup">
      <SetupPanel sysinfo={sysinfo}/>
    </Route>
    <Route path="/vendor">
      <VendorPanel sysinfo={sysinfo} basepath={basepath}/>
    </Route>
    <Route path="/edit-day">
      <DataEdit prefix="UTC Hour" data={dayPlot} url="/dayplot" basepath={basepath}/>
    </Route>
    <Route path="/edit-month">
      <DataEdit prefix="Day" data={monthPlot} url="/monthplot" basepath={basepath}/>
    </Route>
  </Router>

  {#if sysinfo.upgrading}
  <Mask active=true message="Device is upgrading, please wait"/>
  {:else if sysinfo.booting}
    {#if sysinfo.trying}
      <Mask active=true message="Device is booting, please wait. Trying to reach it on {sysinfo.trying}"/>
    {:else}
      <Mask active=true message="Device is booting, please wait"/>
    {/if}
  {/if}
</div>
