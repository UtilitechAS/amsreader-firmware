<script>
  import { Router, Route } from "svelte-navigator";

  import { getSysinfo, sysinfoStore, dataStore } from './lib/DataStores.js';
  import Header from './lib/Header.svelte';
  import Dashboard from './lib/Dashboard.svelte';
  import ConfigurationPanel from './lib/ConfigurationPanel.svelte';
  import StatusPage from './lib/StatusPage.svelte';
  import VendorPanel from './lib/VendorPanel.svelte';
  import SetupPanel from './lib/SetupPanel.svelte';
  import Mask from './lib/Mask.svelte';

  let sysinfo = {};
  sysinfoStore.subscribe(update => {
    sysinfo = update;
  });
  getSysinfo();
  let data = {};
  dataStore.subscribe(update => {
    data = update;
  });
  </script>

<div class="container mx-auto m-3">
  <Router>
    <Header data={data} sysinfo={sysinfo}/>
    <Route path="/">
      {#if sysinfo.vndcfg === false}
      <VendorPanel sysinfo={sysinfo}/>
      {:else if sysinfo.usrcfg === false}
      <SetupPanel sysinfo={sysinfo}/>
      {:else}
      <Dashboard data={data}/>
      {/if}
    </Route>
    <Route path="/configuration">
      <ConfigurationPanel sysinfo={sysinfo}/>
    </Route>
    <Route path="/status">
      <StatusPage sysinfo={sysinfo} data={data}/>
    </Route>
  </Router>

  {#if sysinfo.upgrading}
  <Mask active=true message="Device is upgrading, please wait"/>
  {:else if sysinfo.booting}
  <Mask active=true message="Device is booting, please wait"/>
  {/if}
</div>
