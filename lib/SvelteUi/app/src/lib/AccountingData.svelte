<script>
    import { fmtnum } from "./Helpers";


    export let data;
    export let currency;
    export let hasExport;

    let hasCost = false;
    let cols = 3
    $: {
        cols = currency ? 3 : 2;
        hasCost = data && data.h && (data.h.c || data.d.c || data.m.c || data.h.i || data.d.i || data.m.i);
    }
</script>

<div class="mx-2 text-sm">
    <strong>Real time calculation</strong>
    <br/><br/>

    {#if data}
        {#if hasExport}
            <strong>Import</strong>
            <div class="grid grid-cols-{cols} mb-3">
                <div>Hour</div>
                <div class="text-right">{fmtnum(data.h.u,2)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.h.c,2)} {currency}</div>{/if}
                <div>Day</div>
                <div class="text-right">{fmtnum(data.d.u,1)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.d.c,1)} {currency}</div>{/if}
                <div>Month</div>
                <div class="text-right">{fmtnum(data.m.u)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.m.c)} {currency}</div>{/if}
            </div>
            <strong>Export</strong>
            <div class="grid grid-cols-{cols}">
                <div>Hour</div>
                <div class="text-right">{fmtnum(data.h.p,2)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.h.i,2)} {currency}</div>{/if}
                <div>Day</div>
                <div class="text-right">{fmtnum(data.d.p,1)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.d.i,1)} {currency}</div>{/if}
                <div>Month</div>
                <div class="text-right">{fmtnum(data.m.p)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.m.i)} {currency}</div>{/if}
            </div>
        {:else}
            <strong>Consumption</strong>
            <div class="grid grid-cols-2 mb-3">
                <div>Hour</div>
                <div class="text-right">{fmtnum(data.h.u,2)} kWh</div>
                <div>Day</div>
                <div class="text-right">{fmtnum(data.d.u,1)} kWh</div>
                <div>Month</div>
                <div class="text-right">{fmtnum(data.m.u)} kWh</div>
            </div>
            {#if hasCost}
                <strong>Cost</strong>
                <div class="grid grid-cols-2">
                    <div>Hour</div>
                    <div class="text-right">{fmtnum(data.h.c,2)} {currency}</div>
                    <div>Day</div>
                    <div class="text-right">{fmtnum(data.d.c,1)} {currency}</div>
                    <div>Month</div>
                    <div class="text-right">{fmtnum(data.m.c)} {currency}</div>
                </div>
            {/if}
        {/if}
    {/if}
</div>