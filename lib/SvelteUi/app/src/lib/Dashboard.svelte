<script>
    import { pricesStore, dayPlotStore, monthPlotStore, temperaturesStore } from './DataStores.js';
    import { ampcol, exportcol, metertype, uiVisibility } from './Helpers.js';
    import PowerGauge from './PowerGauge.svelte';
    import VoltPlot from './VoltPlot.svelte';
    import ReactiveData from './ReactiveData.svelte';
    import AccountingData from './AccountingData.svelte';
    import PricePlot from './PricePlot.svelte';
    import DayPlot from './DayPlot.svelte';
    import MonthPlot from './MonthPlot.svelte';
    import TemperaturePlot from './TemperaturePlot.svelte';
    import TariffPeakChart from './TariffPeakChart.svelte';
    import RealtimePlot from './RealtimePlot.svelte';
    import PerPhasePlot from './PerPhasePlot.svelte';

    export let data = {}
    export let sysinfo = {}
    let prices = {}
    let dayPlot = {}
    let monthPlot = {}
    let temperatures = {};
    pricesStore.subscribe(update => {
        prices = update;
    });
    dayPlotStore.subscribe(update => {
        dayPlot = update;
    });
    monthPlotStore.subscribe(update => {
        monthPlot = update;
    });
    temperaturesStore.subscribe(update => {
        temperatures = update;
    });
</script>

<div class="grid 2xl:grid-cols-6 xl:grid-cols-5 lg:grid-cols-4 md:grid-cols-3 sm:grid-cols-2">
    {#if uiVisibility(sysinfo.ui.i, data.i)}
        <div class="cnt">
            <div class="grid grid-cols-2">
                <div class="col-span-2">
                    <PowerGauge val={data.i ? data.i : 0} max={data.im ? data.im : 15000} unit="W" label="Import" sub={data.p} subunit={data.pc} colorFn={ampcol}/>
                </div>
                <div>{data.mt ? metertype(data.mt) : '-'}</div>
                <div class="text-right">{data.ic ? data.ic.toFixed(1) : '-'} kWh</div>
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.e, data.om || data.e > 0)}
        <div class="cnt">
            <div class="grid grid-cols-2">
                <div class="col-span-2">
                    <PowerGauge val={data.e ? data.e : 0} max={data.om ? data.om * 1000 : 10000} unit="W" label="Export" colorFn={exportcol}/>
                </div>
                <div></div>
                <div class="text-right">{data.ec ? data.ec.toFixed(1) : '-'} kWh</div>
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.v, data.l1 && (data.l1.u > 100 || data.l2.u > 100 || data.l3.u > 100))}
        <div class="cnt">
            {#if data.l1}
            <VoltPlot u1={data.l1.u} u2={data.l2.u} u3={data.l3.u} ds={data.ds}/>
            {/if}
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.a, data.l1 && (data.l1.i > 0.01 || data.l2.i > 0.01 || data.l3.i > 0.01))}
        <div class="cnt">
            {#if data.l1}
                <PerPhasePlot title="Amperage" unit="A" importColorFn={ampcol} exportColorFn={exportcol}
                    maxImport={data.mf}
                    maxExport={data.om ? data.om / 230 : 0}
                    l1={data.l1 && data.l1.u > 100} 
                    l2={data.l2 && data.l2.u > 100} 
                    l3={data.l3 && data.l3.u > 100}
                    l2x={data.i2e}
                    l1i={Math.max(data.l1.i,0)}
                    l2i={Math.max(data.l2.i,0)}
                    l3i={Math.max(data.l3.i,0)}
                    l1e={Math.min(data.l1.i*-1,0)}
                    l2e={Math.min(data.l2.i*-1,0)}
                    l3e={Math.min(data.l3.i*-1,0)}
                />
            {/if}
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.h, data.l1 && (data.l1.p > 0.01 || data.l2.p > 0.01 || data.l3.p > 0.01 || data.l1.q > 0.01 || data.l2.q > 0.01 || data.l3.q > 0.01))}
        <div class="cnt">
            {#if data.l1}
                <PerPhasePlot title="Phase power" unit="W" importColorFn={ampcol} exportColorFn={exportcol}
                    maxImport={(data.mf ? data.mf : 32) * 230}
                    maxExport={data.om}
                    l1={data.l1 && data.l1.u > 100} 
                    l2={data.l2 && data.l2.u > 100} 
                    l3={data.l3 && data.l3.u > 100}
                    l1i={data.l1.p}
                    l1e={data.l1.q}
                    l2i={data.l2.p}
                    l2e={data.l2.q}
                    l3i={data.l3.p}
                    l3e={data.l3.q}
                />
            {/if}
        </div> 
    {/if}
    {#if uiVisibility(sysinfo.ui.f, data.l1 && (data.l1.f > 0.01 || data.l2.f > 0.01 || data.l3.f > 0.01))}
        <div class="cnt">
            {#if data.l1}
                <PerPhasePlot title="Power factor" importColorFn={exportcol} exportColorFn={exportcol}
                    maxImport={1.0}
                    l1={data.l1 && data.l1.u > 100} 
                    l2={data.l2 && data.l2.u > 100} 
                    l3={data.l3 && data.l3.u > 100}
                    l1i={data.l1.f}
                    l2i={data.l2.f}
                    l3i={data.l3.f}
                />
            {/if}
        </div> 
    {/if}
    {#if uiVisibility(sysinfo.ui.r, data.ri > 0 || data.re > 0 || data.ric > 0 || data.rec > 0)}
        <div class="cnt">
            <ReactiveData importInstant={data.ri} exportInstant={data.re} importTotal={data.ric} exportTotal={data.rec}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.c, data.ea)}
        <div class="cnt">
            <AccountingData sysinfo={sysinfo} data={data.ea} currency={data.pc} hasExport={data.om > 0 || data.e > 0}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.t, data.pr && (data.pr.startsWith("10YNO") || data.pr.startsWith('10Y1001A1001A4')))}
        <div class="cnt h-64">
            <TariffPeakChart />
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.l)}
        <div class="cnt gwf">
            <RealtimePlot/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.p, data.pe && !Number.isNaN(data.p))}
        <div class="cnt gwf">
            <PricePlot json={prices} sysinfo={sysinfo}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.d, dayPlot)}
        <div class="cnt gwf">
            <DayPlot json={dayPlot} sysinfo={sysinfo}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.m, monthPlot)}
        <div class="cnt gwf">
            <MonthPlot json={monthPlot} sysinfo={sysinfo}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.s, data.t && data.t != -127 && temperatures.c > 1)}
        <div class="cnt gwf">
            <TemperaturePlot json={temperatures} />
        </div>
    {/if}
</div>