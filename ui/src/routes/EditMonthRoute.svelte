<script>
  import DataEdit from '../lib/DataEdit.svelte';
  import { monthPlotStore } from '../lib/DataStores.js';

  let basepath = "/";
  let monthPlot;
  
  monthPlotStore.subscribe(v => monthPlot = v);
</script>

<DataEdit prefix="Day" data={monthPlot} url="/monthplot" {basepath} />
