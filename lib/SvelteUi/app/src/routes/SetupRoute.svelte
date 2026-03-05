<script>
  import SetupPanel from '../lib/SetupPanel.svelte';
  import { sysinfoStore } from '../lib/DataStores.js';

  let sysinfo;
  
  sysinfoStore.subscribe(v => sysinfo = v);
</script>

<SetupPanel {sysinfo} />
