<script>
  import { Router, Route, navigate } from "svelte-navigator";
  import { getSysinfo, sysinfoStore, dataStore } from './lib/DataStores.js';
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
  
  let basepath = document.getElementsByTagName('base')[0].getAttribute("href");
  if(!basepath) basepath = "/";

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
        console.log("dark");
        document.documentElement.classList.add('dark')
    } else if (sysinfo.ui.k === 0) {
        console.log("light");
        document.documentElement.classList.remove('dark')
    } else {
        if (window.matchMedia('(prefers-color-scheme: dark)').matches) {
            console.log("dark auto");
            document.documentElement.classList.add('dark')
        } else {
            console.log("light auto");
            document.documentElement.classList.remove('dark')
        }
    }
  });
  getSysinfo();
  let data = {};
  dataStore.subscribe(update => {
    data = update;
  });

</script>

<div class="container mx-auto m-3">
  <Router basepath={basepath}>
    <Header data={data} basepath={basepath}/>
    <Route path="/">
      <Dashboard data={data} sysinfo={sysinfo}/>
    </Route>
    <Route path="/configuration">
      <ConfigurationPanel sysinfo={sysinfo} basepath={basepath}/>
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
