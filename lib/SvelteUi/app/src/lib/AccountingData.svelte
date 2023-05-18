<script>
    import { fmtnum } from "./Helpers";

    export let sysinfo;
    export let data;
    export let currency;
    export let hasExport;

    let hasCost = false;
    let cols = 3
    $: {
        hasCost = data && data.h && (data.h.c > 0.01 || data.d.c > 0.01 || data.m.c > 0.01 || data.h.i > 0.01 || data.d.i > 0.01 || data.m.i > 0.01);
        cols = hasCost ? 3 : 2;
    }
</script>

<div class="mx-2 text-sm"><!-- EHorvat translated Real time calculation to german: Aktuelle Daten -->
    <strong>Aktuelle Daten</strong>
    <br/><br/>

    {#if data}
        {#if hasExport}<!-- EHorvat translated Import/Bezug Hour/Stunde Day/Tag Month/Monat Last mo./lzt.Monat-->
            <strong>Bezug</strong>
            <div class="grid grid-cols-{cols} mb-3">
                <div>Stunde</div>
                <div class="text-right">{fmtnum(data.h.u,2)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.h.c,2)} {currency}</div>{/if}
                <div>Tag</div>
                <div class="text-right">{fmtnum(data.d.u,1)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.d.c,1)} {currency}</div>{/if}
                <div>Monat</div>
                <div class="text-right">{fmtnum(data.m.u)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.m.c)} {currency}</div>{/if}
                <div>lzt.Monat </div>
                <div class="text-right">{fmtnum(sysinfo.last_month.u)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(sysinfo.last_month.c)} {currency}</div>{/if}
            </div><!-- EHorvat translated Real time calculation to german: Aktuelle Daten -->
            <strong>Export</strong>
            <div class="grid grid-cols-{cols}">
                <div>Stunde</div>
                <div class="text-right">{fmtnum(data.h.p,2)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.h.i,2)} {currency}</div>{/if}
                <div>Tag</div>
                <div class="text-right">{fmtnum(data.d.p,1)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.d.i,1)} {currency}</div>{/if}
                <div>Monat</div>
                <div class="text-right">{fmtnum(data.m.p)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.m.i)} {currency}</div>{/if}
                <div>lzt.Monat </div>
                <div class="text-right">{fmtnum(sysinfo.last_month.p)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(sysinfo.last_month.i)} {currency}</div>{/if}
            </div>
        {:else}<!-- EHorvat translated Consumption/Bezug Last month</Letztes Monat Cost/Kosten-->
            <strong>Consumption</strong>
            <div class="grid grid-cols-2 mb-3">
                <div>Stunde</div>
                <div class="text-right">{fmtnum(data.h.u,2)} kWh</div>
                <div>Tag</div>
                <div class="text-right">{fmtnum(data.d.u,1)} kWh</div>
                <div>Monat</div>
                <div class="text-right">{fmtnum(data.m.u)} kWh</div>
                <div>Letztes Monat</div>
                <div class="text-right">{fmtnum(sysinfo.last_month.u)} kWh</div>
            </div>
            {#if hasCost}
                <strong>Kosten</strong>
                <div class="grid grid-cols-2">
                    <div>Stunde</div>
                    <div class="text-right">{fmtnum(data.h.c,2)} {currency}</div>
                    <div>Tag</div>
                    <div class="text-right">{fmtnum(data.d.c,1)} {currency}</div>
                    <div>Monat</div>
                    <div class="text-right">{fmtnum(data.m.c)} {currency}</div>
                    <div>Letztes Monat</div>
                    <div class="text-right">{fmtnum(sysinfo.last_month.c)} {currency}</div>
                </div>
            {/if}
        {/if}
    {/if}
</div>