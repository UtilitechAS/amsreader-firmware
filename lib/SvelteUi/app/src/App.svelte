<script>
  import { Router, Route, navigate } from "svelte-navigator";
  import { getSysinfo, sysinfoStore, dataStore } from './lib/DataStores.js';
  import Header from './lib/Header.svelte';
  import Dashboard from './lib/Dashboard.svelte';
  import ConfigurationPanel from './lib/ConfigurationPanel.svelte';
  import StatusPage from './lib/StatusPage.svelte';
  import VendorPanel from './lib/VendorPanel.svelte';
  import SetupPanel from './lib/SetupPanel.svelte';
  import Mask from './lib/Mask.svelte';
  import FileUploadComponent from "./lib/FileUploadComponent.svelte";
  import ConsentComponent from "./lib/ConsentComponent.svelte";
  
  let sysinfo = {};
  sysinfoStore.subscribe(update => {
    sysinfo = update;
    if(sysinfo.vndcfg === false) {
      navigate("/vendor");
    } else if(sysinfo.usrcfg === false) {
      navigate("/setup");
    } else if(sysinfo.fwconsent === 0) {
      navigate("/consent");
    }
  });
  getSysinfo();
  let data = {};
  dataStore.subscribe(update => {
    data = update;
  });
  </script>

<div class="container mx-auto m-3">
  <Router>
    <Header data={data}/>
    <Route path="/">
      <Dashboard data={data} sysinfo={sysinfo}/>
    </Route>
    <Route path="/configuration">
      <ConfigurationPanel sysinfo={sysinfo}/>
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
      <ConsentComponent sysinfo={sysinfo}/>
    </Route>
    <Route path="/setup">
      <SetupPanel sysinfo={sysinfo}/>
    </Route>
    <Route path="/vendor">
      <VendorPanel sysinfo={sysinfo}/>
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
