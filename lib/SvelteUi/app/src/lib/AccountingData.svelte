<script>
    export let data;
    export let currency;

    let hasExport = data && (data.om || data.e > 0);
</script>

<div class="mx-2 text-sm">
    <strong>Real time calculation</strong>
    
    <div class="flex">
        <div>Hour</div>
        <div class="flex-auto text-right">{data && data.h && data.h.u ? data.h.u.toFixed(2) : '-'} kWh {#if currency && (hasExport)}/ {data && data.h && data.h.c ? data.h.c.toFixed(2) : '-'} {currency}{/if}</div>
    </div>
    <div class="flex">
        <div>Day</div>
        <div class="flex-auto text-right">{data && data.d && data.d.u ? data.d.u.toFixed(1) : '-'} kWh {#if currency && (hasExport)}/ {data && data.d && data.d.c ? data.d.c.toFixed(2) : '-'} {currency}{/if}</div>
    </div>
    <div class="flex">
        <div>Month</div>
        <div class="flex-auto text-right">{data && data.m && data.m.u ? data.m.u.toFixed(0) : '-'} kWh {#if currency && (hasExport)}/ {data && data.m && data.m.c ? data.m.c.toFixed(2) : '-'} {currency}{/if}</div>
    </div>
    <div class="mt-4">
        {#if hasExport}
        <div class="flex">
            <div>Hour</div>
            <div class="flex-auto text-right">{data && data.h && data.h.p ? data.h.p.toFixed(2) : '-'} kWh {#if currency}/ {data && data.h && data.h.pc ? data.h.pc.toFixed(2) : '-'} {currency}{/if}</div>
        </div>
        <div class="flex">
            <div>Day</div>
            <div class="flex-auto text-right">{data && data.d && data.d.p ? data.d.p.toFixed(1) : '-'} kWh {#if currency}/ {data && data.d && data.d.pc ? data.d.pc.toFixed(2) : '-'} {currency}{/if}</div>
        </div>
        <div class="flex">
            <div>Month</div>
            <div class="flex-auto text-right">{data && data.m && data.m.p ? data.m.p.toFixed(0) : '-'} kWh {#if currency}/ {data && data.m && data.m.pc ? data.m.pc.toFixed(2) : '-'} {currency}{/if}</div>
        </div>
        {:else}
        <div class="flex">
            <div>Hour</div>
            <div class="flex-auto text-right">{data && data.h && data.h.c ? data.h.c.toFixed(2) : '-'} {currency}</div>
        </div>
        <div class="flex">
            <div>Day</div>
            <div class="flex-auto text-right">{data && data.d && data.d.c ? data.d.c.toFixed(2) : '-'} {currency}</div>
        </div>
        <div class="flex">
            <div>Month</div>
            <div class="flex-auto text-right">{data && data.m && data.m.c ? data.m.c.toFixed(2) : '-'} {currency}</div>
        </div>
        {/if}
    </div>
</div>