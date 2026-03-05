<script>
  import VendorPanel from '../lib/VendorPanel.svelte';
  import { sysinfoStore } from '../lib/DataStores.js';

  let basepath = "/";
  let sysinfo;
  
  sysinfoStore.subscribe(v => sysinfo = v);
</script>

<VendorPanel {basepath} {sysinfo} />
