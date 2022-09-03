<script>
  import { dataStore } from './lib/DataStores.js';
  import Header from './lib/Header.svelte';
  import Dashboard from './lib/Dashboard.svelte';

  let data = {};
  dataStore.subscribe(update => {
    data = update;
  });
  </script>

<div class="container mx-auto m-3">
  <Header data={data}/>
  <Dashboard data={data}/>
</div>
