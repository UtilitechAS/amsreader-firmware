<script>
  import { Router, Route } from "svelte-navigator";

  import { sysinfoStore, dataStore } from './lib/DataStores.js';
  import Header from './lib/Header.svelte';
  import Dashboard from './lib/Dashboard.svelte';
  import ConfigurationPanel from './lib/ConfigurationPanel.svelte';

  let sysinfo = {};
  sysinfoStore.subscribe(update => {
    sysinfo = update;
  });
  let data = {};
  dataStore.subscribe(update => {
    data = update;
  });
  </script>

<div class="container mx-auto m-3">
  <Router>
    <Header data={data} sysinfo={sysinfo}/>
    <Route path="/">
      <Dashboard data={data}/>
    </Route>
    <Route path="/configuration">
      <ConfigurationPanel sysinfo={sysinfo}/>
    </Route>
  </Router>
  
</div>
