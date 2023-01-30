<script>
    export let data;
    export let currency;
    export let hasExport;
</script>

<div class="mx-2 text-sm">
    <strong>Real time calculation</strong>
    <br/><br/>

    {#if data}
        {#if hasExport && currency}
            <strong>Import</strong>
            <div class="grid grid-cols-3 mb-3">
                <div>Hour</div>
                <div class="text-right">{data.h.u ? data.h.u.toFixed(2) : '-'} kWh</div>
                <div class="text-right">{data.h.c ? data.h.c.toFixed(2) : '-'} {currency}</div>
                <div>Day</div>
                <div class="text-right">{data.d.u ? data.d.u.toFixed(1) : '-'} kWh</div>
                <div class="text-right">{data.d.c ? data.d.c.toFixed(1) : '-'} {currency}</div>
                <div>Month</div>
                <div class="text-right">{data.m.u ? data.m.u.toFixed(0) : '-'} kWh</div>
                <div class="text-right">{data.m.c ? data.m.c.toFixed(0) : '-'} {currency}</div>
            </div>
            <strong>Export</strong>
            <div class="grid grid-cols-3">
                <div>Hour</div>
                <div class="text-right">{data.h.p ? data.h.p.toFixed(2) : '-'} kWh</div>
                <div class="text-right">{data.h.i ? data.h.i.toFixed(2) : '-'} {currency}</div>
                <div>Day</div>
                <div class="text-right">{data.d.p ? data.d.p.toFixed(1) : '-'} kWh</div>
                <div class="text-right">{data.d.i ? data.d.i.toFixed(1) : '-'} {currency}</div>
                <div>Month</div>
                <div class="text-right">{data.m.p ? data.m.p.toFixed(0) : '-'} kWh</div>
                <div class="text-right">{data.m.i ? data.m.i.toFixed(0) : '-'} {currency}</div>
            </div>
        {:else if hasExport}
            <strong>Import</strong>
            <div class="grid grid-cols-2 mb-3">
                <div>Hour</div>
                <div class="text-right">{data.h.u ? data.h.u.toFixed(2) : '-'} kWh</div>
                <div>Day</div>
                <div class="text-right">{data.d.u ? data.d.u.toFixed(1) : '-'} kWh</div>
                <div>Month</div>
                <div class="text-right">{data.m.u ? data.m.u.toFixed(0) : '-'} kWh</div>
            </div>
            <strong>Export</strong>
            <div class="grid grid-cols-2">
                <div>Hour</div>
                <div class="text-right">{data.h.p ? data.h.p.toFixed(2) : '-'} kWh</div>
                <div>Day</div>
                <div class="text-right">{data.d.p ? data.d.p.toFixed(1) : '-'} kWh</div>
                <div>Month</div>
                <div class="text-right">{data.m.p ? data.m.p.toFixed(0) : '-'} kWh</div>
            </div>
        {:else}
            <strong>Consumption</strong>
            <div class="grid grid-cols-2 mb-3">
                <div>Hour</div>
                <div class="text-right">{data.h.u ? data.h.u.toFixed(2) : '-'} kWh</div>
                <div>Day</div>
                <div class="text-right">{data.d.u ? data.d.u.toFixed(1) : '-'} kWh</div>
                <div>Month</div>
                <div class="text-right">{data.m.u ? data.m.u.toFixed(0) : '-'} kWh</div>
            </div>
            {#if currency}
                <strong>Cost</strong>
                <div class="grid grid-cols-2">
                    <div>Hour</div>
                    <div class="text-right">{data.h.c ? data.h.c.toFixed(2) : '-'} {currency}</div>
                    <div>Day</div>
                    <div class="text-right">{data.d.c ? data.d.c.toFixed(1) : '-'} {currency}</div>
                    <div>Month</div>
                    <div class="text-right">{data.m.c ? data.m.c.toFixed(0) : '-'} {currency}</div>
                </div>
            {/if}
        {/if}
    {/if}
</div>