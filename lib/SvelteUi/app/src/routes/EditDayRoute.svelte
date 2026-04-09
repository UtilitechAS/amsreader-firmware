<script>
  import DataEdit from '../lib/DataEdit.svelte';
  import { dayPlotStore } from '../lib/DataStores.js';

  let basepath = "/";
  let dayPlot;
  
  dayPlotStore.subscribe(v => dayPlot = v);
</script>

<DataEdit prefix="UTC Hour" data={dayPlot} url="/dayplot" {basepath} />
