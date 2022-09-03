<script>
    import { pricesStore, dayPlotStore, monthPlotStore, temperaturesStore } from './DataStores.js';
    import { metertype } from './Helpers.js';
    import PowerGauge from './PowerGauge.svelte';
    import VoltPlot from './VoltPlot.svelte';
    import AmpPlot from './AmpPlot.svelte';
    import ReactiveData from './ReactiveData.svelte';
    import AccountingData from './AccountingData.svelte';
    import PricePlot from './PricePlot.svelte';
    import DayPlot from './DayPlot.svelte';
    import MonthPlot from './MonthPlot.svelte';
    import TemperaturePlot from './TemperaturePlot.svelte';

    export let data = {}
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
    <div class="bg-white m-2 p-2 rounded shadow-lg">
        <PowerGauge val={data.i ? data.i : 0} max={data.im} unit="W" label="Import"/>
        <div class="grid grid-cols-2">
            <div>{data.mt ? metertype(data.mt) : '-'}</div>
            <div class="text-right">{data.ic ? data.ic.toFixed(1) : '-'} kWh</div>
        </div>
    </div>
    {#if data.om}
        <div class="bg-white m-2 p-2 rounded shadow-lg">
            <PowerGauge val={data.e ? data.e : 0} max={data.om} unit="W" label="Export"/>
            <div class="grid grid-cols-2">
                <div></div>
                <div class="text-right">{data.ec ? data.ec.toFixed(1) : '-'} kWh</div>
            </div>
        </div>
    {/if}
    <div class="bg-white m-2 p-2 rounded shadow-lg">
        <VoltPlot u1={data.u1} u2={data.u2} u3={data.u3} ds={data.ds}/>
    </div>
    <div class="bg-white m-2 p-2 rounded shadow-lg">
        <AmpPlot u1={data.u1} u2={data.u2} u3={data.u3} i1={data.i1} i2={data.i2} i3={data.i3} max={data.mf ? data.mf : 32}/>
    </div>
    <div class="bg-white m-2 p-2 rounded shadow-lg">
        <ReactiveData importInstant={data.ri} exportInstant={data.re} importTotal={data.ric} exportTotal={data.rec}/>
    </div>
    <div class="bg-white m-2 p-2 rounded shadow-lg">
        <AccountingData data={data.ea} currency={prices.currency}/>
    </div>
    {#if prices.currency}
        <div class="bg-white m-2 p-2 rounded shadow-lg xl:col-span-6 lg:col-span-3 md:col-span-3 sm:col-span-2">
            <PricePlot json={prices}/>
        </div>
    {/if}
    <div class="bg-white m-2 p-2 rounded shadow-lg xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2">
        <DayPlot json={dayPlot} />
    </div>
    <div class="bg-white m-2 p-2 rounded shadow-lg xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2">
        <MonthPlot json={monthPlot} />
    </div>
    {#if data.t && data.t != -127 && temperatures.c > 1}
    <div class="bg-white m-2 p-2 rounded shadow-lg xl:col-span-6 lg:col-span-4 md:col-span-3 sm:col-span-2">
        <TemperaturePlot json={temperatures} />
    </div>
    {/if}
</div>