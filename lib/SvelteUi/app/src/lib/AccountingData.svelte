<script>
    import { fmtnum, capitalize } from "./Helpers";
    import { translationsStore } from "./TranslationService";

    export let sysinfo;
    export let data;
    export let currency;
    export let hasExport;

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let hasCost = false;
    let cols = 3
    $: {
        hasCost = data && data.h && (Math.abs(data.h.c) > 0.01 || Math.abs(data.d.c) > 0.01 || Math.abs(data.m.c) > 0.01 || Math.abs(data.h.i) > 0.01 || Math.abs(data.d.i) > 0.01 || Math.abs(data.m.i) > 0.01);
        cols = hasCost ? 3 : 2;
    }
</script>

<div class="mx-2 text-sm">
    <strong>{translations.realtime?.title ?? "Real time calculations"}</strong>
    <br/><br/>

    {#if data}
        {#if hasExport}
            <strong>{translations.common?.import ?? "Import"}</strong>
            <div class="grid grid-cols-{cols} mb-3">
                <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                <div class="text-right">{fmtnum(data.h.u,2)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.h.c,2)} {currency}</div>{/if}
                <div>{capitalize(translations.common?.day ?? "Day")}</div>
                <div class="text-right">{fmtnum(data.d.u,1)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.d.c,1)} {currency}</div>{/if}
                <div>{capitalize(translations.common?.month ?? "Month")}</div>
                <div class="text-right">{fmtnum(data.m.u)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.m.c)} {currency}</div>{/if}
                <div>{translations.realtime?.last_mo ?? "Last mo."}</div>
                <div class="text-right">{fmtnum(sysinfo.last_month.u)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(sysinfo.last_month.c)} {currency}</div>{/if}
            </div>
            <strong>{translations.common?.export ?? "Export"}</strong>
            <div class="grid grid-cols-{cols}">
                <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                <div class="text-right">{fmtnum(data.h.p,2)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.h.i,2)} {currency}</div>{/if}
                <div>{capitalize(translations.common?.day ?? "Day")}</div>
                <div class="text-right">{fmtnum(data.d.p,1)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.d.i,1)} {currency}</div>{/if}
                <div>{capitalize(translations.common?.month ?? "Month")}</div>
                <div class="text-right">{fmtnum(data.m.p)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(data.m.i)} {currency}</div>{/if}
                <div>{translations.realtime?.last_mo ?? "Last mo."}</div>
                <div class="text-right">{fmtnum(sysinfo.last_month.p)} kWh</div>
                {#if hasCost}<div class="text-right">{fmtnum(sysinfo.last_month.i)} {currency}</div>{/if}
            </div>
        {:else}
            <strong>{translations.realtime?.consumption ?? "Consumption"}</strong>
            <div class="grid grid-cols-2 mb-3">
                <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                <div class="text-right">{fmtnum(data.h.u,2)} kWh</div>
                <div>{capitalize(translations.common?.day ?? "Day")}</div>
                <div class="text-right">{fmtnum(data.d.u,1)} kWh</div>
                <div>{capitalize(translations.common?.month ?? "Month")}</div>
                <div class="text-right">{fmtnum(data.m.u)} kWh</div>
                <div>{translations.realtime?.last_month ?? "Last month"}</div>
                <div class="text-right">{fmtnum(sysinfo.last_month.u)} kWh</div>
            </div>
            {#if hasCost}
                <strong>{translations.realtime?.cost ?? "Cost"}</strong>
                <div class="grid grid-cols-2">
                    <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                    <div class="text-right">{fmtnum(data.h.c,2)} {currency}</div>
                    <div>{capitalize(translations.common?.day ?? "Day")}</div>
                    <div class="text-right">{fmtnum(data.d.c,1)} {currency}</div>
                    <div>{capitalize(translations.common?.month ?? "Month")}</div>
                    <div class="text-right">{fmtnum(data.m.c)} {currency}</div>
                    <div>{translations.realtime?.last_month ?? "Last month"}</div>
                    <div class="text-right">{fmtnum(sysinfo.last_month.c)} {currency}</div>
                </div>
            {/if}
        {/if}
    {/if}
</div>