<script>
    import { fmtnum, capitalize, formatUnit } from "./Helpers";

    export let sysinfo;
    export let data;
    export let currency;
    export let hasExport;
    export let translations = {};

    let rih, rid, rim, ril, reh, red, rem, rel;
    let hasCost = false;
    let cols = 3;
    $: {
        hasCost =
            data &&
            data.h &&
            (Math.abs(data.h.c) > 0.01 ||
                Math.abs(data.d.c) > 0.01 ||
                Math.abs(data.m.c) > 0.01 ||
                Math.abs(data.h.i) > 0.01 ||
                Math.abs(data.d.i) > 0.01 ||
                Math.abs(data.m.i) > 0.01);
        cols = hasCost ? 3 : 2;

        rih = formatUnit(data?.h?.u * 1000, "Wh");
        rid = formatUnit(data?.d?.u * 1000, "Wh");
        rim = formatUnit(data?.m?.u * 1000, "Wh");
        ril = formatUnit(sysinfo?.last_month?.u * 1000, "Wh");

        reh = formatUnit(data?.h?.p * 1000, "Wh");
        red = formatUnit(data?.d?.p * 1000, "Wh");
        rem = formatUnit(data?.m?.p * 1000, "Wh");
        rel = formatUnit(sysinfo?.last_month?.p * 1000, "Wh");
    }
</script>

<div class="mx-2 text-sm neas-green">
    <strong>{translations.realtime?.title ?? "Real time calculations"}</strong>
    <br /><br />

    {#if data}
        {#if hasExport}
            <strong>{translations.common?.import ?? "Import"}</strong>
            <div class="grid grid-cols-{cols} mb-3">
                <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                <div class="text-right">{rih[0]} {rih[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(data.h.c, 2)}
                        {currency}
                    </div>{/if}
                <div>{capitalize(translations.common?.day ?? "Day")}</div>
                <div class="text-right">{rid[0]} {rid[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(data.d.c, 1)}
                        {currency}
                    </div>{/if}
                <div>{capitalize(translations.common?.month ?? "Month")}</div>
                <div class="text-right">{rim[0]} {rim[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(data.m.c)}
                        {currency}
                    </div>{/if}
                <div>{translations.realtime?.last_mo ?? "Last mo."}</div>
                <div class="text-right">{ril[0]} {ril[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(sysinfo.last_month?.c)}
                        {currency}
                    </div>{/if}
            </div>
            <strong>{translations.common?.export ?? "Export"}</strong>
            <div class="grid grid-cols-{cols}">
                <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                <div class="text-right">{reh[0]} {reh[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(data.h.i, 2)}
                        {currency}
                    </div>{/if}
                <div>{capitalize(translations.common?.day ?? "Day")}</div>
                <div class="text-right">{red[0]} {red[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(data.d.i, 1)}
                        {currency}
                    </div>{/if}
                <div>{capitalize(translations.common?.month ?? "Month")}</div>
                <div class="text-right">{rem[0]} {rem[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(data.m.i)}
                        {currency}
                    </div>{/if}
                <div>{translations.realtime?.last_mo ?? "Last mo."}</div>
                <div class="text-right">{rel[0]} {rel[1]}</div>
                {#if hasCost}<div class="text-right">
                        {fmtnum(sysinfo.last_month?.i)}
                        {currency}
                    </div>{/if}
            </div>
        {:else}
            <strong
                >{translations.realtime?.consumption ?? "Consumption"}</strong
            >
            <div class="grid grid-cols-2 mb-3">
                <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                <div class="text-right">{rih[0]} {rih[1]}</div>
                <div>{capitalize(translations.common?.day ?? "Day")}</div>
                <div class="text-right">{rid[0]} {rid[1]}</div>
                <div>{capitalize(translations.common?.month ?? "Month")}</div>
                <div class="text-right">{rim[0]} {rim[1]}</div>
                <div>{translations.realtime?.last_month ?? "Last month"}</div>
                <div class="text-right">{ril[0]} {ril[1]}</div>
            </div>
            {#if hasCost}
                <strong>{translations.realtime?.cost ?? "Cost"}</strong>
                <div class="grid grid-cols-2">
                    <div>{capitalize(translations.common?.hour ?? "Hour")}</div>
                    <div class="text-right">
                        {fmtnum(data.h.c, 2)}
                        {currency}
                    </div>
                    <div>{capitalize(translations.common?.day ?? "Day")}</div>
                    <div class="text-right">
                        {fmtnum(data.d.c, 1)}
                        {currency}
                    </div>
                    <div>
                        {capitalize(translations.common?.month ?? "Month")}
                    </div>
                    <div class="text-right">{fmtnum(data.m.c)} {currency}</div>
                    <div>
                        {translations.realtime?.last_month ?? "Last month"}
                    </div>
                    <div class="text-right">
                        {fmtnum(sysinfo.last_month?.c)}
                        {currency}
                    </div>
                </div>
            {/if}
        {/if}
    {/if}
</div>
