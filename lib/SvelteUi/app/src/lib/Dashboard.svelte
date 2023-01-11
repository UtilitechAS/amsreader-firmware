<script>
    import { pricesStore, dayPlotStore, monthPlotStore, temperaturesStore } from './DataStores.js';
    import { metertype, uiVisibility } from './Helpers.js';
    import PowerGauge from './PowerGauge.svelte';
    import VoltPlot from './VoltPlot.svelte';
    import AmpPlot from './AmpPlot.svelte';
    import ReactiveData from './ReactiveData.svelte';
    import AccountingData from './AccountingData.svelte';
    import PricePlot from './PricePlot.svelte';
    import DayPlot from './DayPlot.svelte';
    import MonthPlot from './MonthPlot.svelte';
    import TemperaturePlot from './TemperaturePlot.svelte';
    import TariffPeakChart from './TariffPeakChart.svelte';

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

<div class="grid xl:grid-cols-6 lg:grid-cols-4  md:grid-cols-3 sm:grid-cols-2">
    {#if uiVisibility(sysinfo.ui.i, data.i)}
    <div class="cnt">
        <div class="grid grid-cols-2">
            <div class="col-span-2">
                <PowerGauge val={data.i ? data.i : 0} max={data.im} unit="W" label="Import" sub={data.p} subunit={prices.currency}/>
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
                    <PowerGauge val={data.e ? data.e : 0} max={data.om ? data.om : 10000} unit="W" label="Export"/>
                </div>
                <div></div>
                <div class="text-right">{data.ec ? data.ec.toFixed(1) : '-'} kWh</div>
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.v, data.u1 > 100 || data.u2 > 100 || data.u3 > 100)}
    <div class="cnt">
        <VoltPlot u1={data.u1} u2={data.u2} u3={data.u3} ds={data.ds}/>
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.a, data.i1 > 0.01 || data.i2 > 0.01 || data.i3 > 0.01)}
    <div class="cnt">
        <AmpPlot u1={data.u1} u2={data.u2} u3={data.u3} i1={data.i1} i2={data.i2} i3={data.i3} max={data.mf ? data.mf : 32}/>
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.r, data.ri > 0 || data.re > 0 || data.ric > 0 || data.rec > 0)}
    <div class="cnt">
        <ReactiveData importInstant={data.ri} exportInstant={data.re} importTotal={data.ric} exportTotal={data.rec}/>
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.c, data.ea)}
    <div class="cnt">
        <AccountingData data={data.ea} currency={prices.currency}/>
    </div>
    {/if}
    {#if data && data.pr && (data.pr.startsWith("10YNO") || data.pr == '10Y1001A1001A48H')}
    <div class="cnt h-64">
        <TariffPeakChart />
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.p, (typeof data.p == "number") && !Number.isNaN(data.p))}
        <div class="cnt xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2 h-64">
            <PricePlot json={prices}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.d, dayPlot)}
    <div class="cnt xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2 h-64">
        <DayPlot json={dayPlot} />
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.m, monthPlot)}
    <div class="cnt xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2 h-64">
        <MonthPlot json={monthPlot} />
    </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.s, data.t && data.t != -127 && temperatures.c > 1)}
    <div class="cnt xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2 h-64">
        <TemperaturePlot json={temperatures} />
    </div>
    {/if}
</div>