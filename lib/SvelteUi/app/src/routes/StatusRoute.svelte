<script>
  import StatusPage from '../lib/StatusPage.svelte';
  import { dataStore, sysinfoStore } from '../lib/DataStores.js';

  let data, sysinfo;
  
  dataStore.subscribe(v => data = v);
  sysinfoStore.subscribe(v => sysinfo = v);
</script>

<StatusPage {data} {sysinfo} />
