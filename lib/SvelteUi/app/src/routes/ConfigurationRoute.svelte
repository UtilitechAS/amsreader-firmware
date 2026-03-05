<script>
  import ConfigurationPanel from '../lib/ConfigurationPanel.svelte';
  import { dataStore, sysinfoStore } from '../lib/DataStores.js';

  let basepath = "/";
  let data, sysinfo;
  
  dataStore.subscribe(v => data = v);
  sysinfoStore.subscribe(v => sysinfo = v);
</script>

<ConfigurationPanel {basepath} {sysinfo} {data} />
