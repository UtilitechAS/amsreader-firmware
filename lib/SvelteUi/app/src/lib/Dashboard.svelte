<script>
    import { ampcol, exportcol, metertype, uiVisibility, formatUnit } from './Helpers.js';
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
    export let prices = {}
    export let dayPlot = {}
    export let monthPlot = {}
    export let temperatures = {};
    export let translations = {};
    export let tariffData = {};

    let it,et,threePhase, l1e, l2e, l3e;
    $: {
        it = formatUnit(data?.ic * 1000, "Wh");
        et = formatUnit(data?.ec * 1000, "Wh");
        l1e = data?.l1?.u > 0.0 || data?.l1?.i > 0.0 || data?.l1?.p > 0.0 || data?.l1?.q > 0.0;
        l2e = data?.l2?.u > 0.0 || data?.l2?.i > 0.0 || data?.l2?.p > 0.0 || data?.l2?.q > 0.0;
        l3e = data?.l3?.u > 0.0 || data?.l3?.i > 0.0 || data?.l3?.p > 0.0 || data?.l3?.q > 0.0;
        threePhase = l1e && l2e && l3e;
    }
</script>

<div class="grid 2xl:grid-cols-6 xl:grid-cols-5 lg:grid-cols-4 md:grid-cols-3 sm:grid-cols-2">
    {#if uiVisibility(sysinfo.ui.i, data.i)}
        <div class="cnt">
            <div class="grid grid-cols-2">
                <div class="col-span-2">
                    <PowerGauge val={data.i ? data.i : 0} max={data.im ? data.im : 15000} unit="W" label={translations.common?.import ?? "Import"} sub={data.p} subunit={data.pc} colorFn={ampcol}/>
                </div>
                <div>{data.mt ? metertype(data.mt) : '-'}</div>
                <div class="text-right">{it[0]} {it[1]}</div>
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.e, data.om || data.e > 0)}
        <div class="cnt">
            <div class="grid grid-cols-2">
                <div class="col-span-2">
                    <PowerGauge val={data.e ? data.e : 0} max={data.om ? data.om * 1000 : 10000} unit="W" label={translations.common?.export ?? "Export"} sub={data.px} subunit={data.pc} colorFn={exportcol}/>
                </div>
                <div></div>
                <div class="text-right">{et[0]} {et[1]}</div>
            </div>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.v, data.l1 && (data.l1.u > 100 || data.l2.u > 100 || data.l3.u > 100))}
        <div class="cnt">
            {#if data.l1}
            <VoltPlot title={translations.common?.voltage ?? "Volt"} u1={data.l1.u} u2={data.l2.u} u3={data.l3.u} ds={data.ds}/>
            {/if}
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.a, data.l1 && (data.l1.i > 0.01 || data.l2.i > 0.01 || data.l3.i > 0.01))}
        <div class="cnt">
            {#if data.l1}
                <PerPhasePlot title={translations.common?.amperage ?? "Amp"} unit="A" importColorFn={ampcol} exportColorFn={exportcol}
                    maxImport={data.mf}
                    maxExport={data.om ? threePhase ? data.om / 0.4 / Math.sqrt(3) : data.om / 0.23 : 0}
                    l1={data.l1 && data.l1.u > 100} 
                    l2={data.l2 && data.l2.u > 100} 
                    l3={data.l3 && data.l3.u > 100}
                    l2x={data.l2.e}
                    l1i={Math.max(data.l1.i,0)}
                    l2i={Math.max(data.l2.i,0)}
                    l3i={Math.max(data.l3.i,0)}
                    l1e={Math.max(data.l1.i*-1,0)}
                    l2e={Math.max(data.l2.i*-1,0)}
                    l3e={Math.max(data.l3.i*-1,0)}
                />
            {/if}
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.h, data.l1 && (data.l1.p > 0.01 || data.l2.p > 0.01 || data.l3.p > 0.01 || data.l1.q > 0.01 || data.l2.q > 0.01 || data.l3.q > 0.01))}
        <div class="cnt">
            {#if data.l1}
                <PerPhasePlot title={translations.dashboard?.phase ?? "Phase"} unit="W" importColorFn={ampcol} exportColorFn={exportcol}
                    maxImport={(data.mf ? data.mf : 32) * 230}
                    maxExport={data.om ? threePhase ? (data.om * 1000) / Math.sqrt(3) : data.om * 1000 : 0}
                    l1={data.l1 && data.l1.u > 100} 
                    l2={data.l2 && data.l2.u > 100} 
                    l3={data.l3 && data.l3.u > 100}
                    l1i={data.l1.p}
                    l2i={data.l2.p}
                    l3i={data.l3.p}
                    l1e={data.l1.q}
                    l2e={data.l2.q}
                    l3e={data.l3.q}
                />
            {/if}
        </div> 
    {/if}
    {#if uiVisibility(sysinfo.ui.f, data.l1 && (data.l1.f > 0.01 || data.l2.f > 0.01 || data.l3.f > 0.01))}
        <div class="cnt">
            {#if data.l1}
                <PerPhasePlot title={translations.dashboard?.pf ?? "Pf"} importColorFn={exportcol} exportColorFn={exportcol}
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
            <ReactiveData importInstant={data.ri} exportInstant={data.re} importTotal={data.ric} exportTotal={data.rec} translations={translations}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.c, data.ea)}
        <div class="cnt">
            <AccountingData sysinfo={sysinfo} data={data.ea} currency={data.pc} hasExport={data.om > 0 || data.e > 0} translations={translations}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.t, data.pr && (data.pr.startsWith("NO") || data.pr.startsWith("10YNO") || data.pr.startsWith('10Y1001A1001A4')))}
        <div class="cnt h-64">
            <TariffPeakChart title={translations.dashboard?.tariffpeak ?? "Tariff peaks"} tariffData={tariffData} translations={translations}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.l, data.hm == 1)}
        <div class="cnt gwf">
            <RealtimePlot title={translations.dashboard?.realtime ?? "Real time"}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.p, data.pe && !Number.isNaN(data.p))}
        <div class="cnt gwf">
            <PricePlot title={translations.dashboard?.price ?? "Price"} json={prices} sysinfo={sysinfo}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.d, dayPlot)}
        <div class="cnt gwf">
            <DayPlot title={translations.dashboard?.day ?? "24 hours"} json={dayPlot} sysinfo={sysinfo}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.m, monthPlot)}
        <div class="cnt gwf">
            <MonthPlot title={translations.dashboard?.month ?? "{0} days"} json={monthPlot} sysinfo={sysinfo}/>
        </div>
    {/if}
    {#if uiVisibility(sysinfo.ui.s, data.t && data.t != -127 && temperatures.c > 1)}
        <div class="cnt gwf">
            <TemperaturePlot title={translations.dashboard?.temperature ?? "Temperature"} json={temperatures} />
        </div>
    {/if}
</div>