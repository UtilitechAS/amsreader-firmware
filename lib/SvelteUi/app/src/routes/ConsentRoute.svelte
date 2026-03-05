<script>
  import ConsentComponent from '../lib/ConsentComponent.svelte';
  import { sysinfoStore } from '../lib/DataStores.js';

  let basepath = "/";
  let sysinfo;
  
  sysinfoStore.subscribe(v => sysinfo = v);
</script>

<ConsentComponent {basepath} {sysinfo} />
