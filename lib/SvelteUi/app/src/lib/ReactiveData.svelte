<script>
    import { formatUnit } from "./Helpers";

    export let importInstant;
    export let exportInstant;
    export let importTotal;
    export let exportTotal;
    export let translations = {};

    let ii,ei,it,et;
    $: {
        ii = formatUnit(importInstant, "VAr");
        ei = formatUnit(exportInstant, "VAr");
        it = formatUnit(importTotal*1000, "VArh");
        et = formatUnit(exportTotal*1000, "VArh");
    }
</script>

<div class="mx-2 text-sm">
    <strong>{translations.reactive?.title ?? "Reactive"}</strong>
    
    <div class="grid grid-cols-2 mt-4">
        <div>{translations.reactive?.instant_in ?? "Instant in"}</div>
        <div class="text-right">{ii[0]} {ii[1]}</div>
        <div>{translations.reactive?.instant_out ?? "Instant out"}</div>
        <div class="text-right">{ei[0]} {ei[1]}</div>
    </div>
    
    <div class="grid grid-cols-2 mt-4">
        <div>{translations.reactive?.total_in ?? "Total in"}</div>
        <div class="text-right">{it[0]} {it[1]}</div>
        <div>{translations.reactive?.total_out ?? "Total out"}</div>
        <div class="text-right">{et[0]} {et[1]}</div>
    </div>
</div>