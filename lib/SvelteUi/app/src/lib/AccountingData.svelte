<script>
    export let data;
    export let currency;
</script>

<div class="mx-2 text-sm">
    <strong>Real time calculation</strong>
    
    <div class="grid grid-cols-2 mt-4">
        <div>Hour</div>
        <div class="text-right">{data && data.h && data.h.u ? data.h.u.toFixed(2) : '-'} kWh</div>
        <div>Day</div>
        <div class="text-right">{data && data.d && data.d.u ? data.d.u.toFixed(1) : '-'} kWh</div>
        <div>Month</div>
        <div class="text-right">{data && data.m && data.m.u ? data.m.u.toFixed(0) : '-'} kWh</div>
    </div>
    {#if currency}
        <div class="grid grid-cols-2 mt-4">
            <div>Hour</div>
            <div class="text-right">{data && data.h && data.h.c ? data.h.c.toFixed(2) : '-'} {currency}</div>
            <div>Day</div>
            <div class="text-right">{data && data.d && data.d.c ? data.d.c.toFixed(1) : '-'} {currency}</div>
            <div>Month</div>
            <div class="text-right">{data && data.m && data.m.c ? data.m.c.toFixed(0) : '-'} {currency}</div>
        </div>
    {/if}
    <div class="grid grid-cols-2 mt-4">
        <div>Max</div>
        <div class="text-right">
            {data && data.x ? data.x.toFixed(1) : '-'} / {data && data.t ? data.t.toFixed(1) : '-'} kWh
        </div>
    </div>
</div>