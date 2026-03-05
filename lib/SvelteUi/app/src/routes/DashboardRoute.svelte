<script>
  import Dashboard from '../lib/Dashboard.svelte';
  import { dataStore, sysinfoStore, importPricesStore, exportPricesStore, dayPlotStore, monthPlotStore, temperaturesStore, tariffStore } from '../lib/DataStores.js';
  import { translationsStore } from '../lib/TranslationService.js';

  let data, sysinfo, importPrices, exportPrices, dayPlot, monthPlot, temperatures, translations, tariffData;
  
  dataStore.subscribe(v => data = v);
  sysinfoStore.subscribe(v => sysinfo = v);
  importPricesStore.subscribe(v => importPrices = v);
  exportPricesStore.subscribe(v => exportPrices = v);
  dayPlotStore.subscribe(v => dayPlot = v);
  monthPlotStore.subscribe(v => monthPlot = v);
  temperaturesStore.subscribe(v => temperatures = v);
  translationsStore.subscribe(v => translations = v);
  tariffStore.subscribe(v => tariffData = v);
</script>

<Dashboard {data} {sysinfo} {importPrices} {exportPrices} {dayPlot} {monthPlot} {temperatures} {translations} {tariffData} />
