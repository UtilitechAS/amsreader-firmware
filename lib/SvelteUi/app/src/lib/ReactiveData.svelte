<script>
    import { translationsStore } from "./TranslationService";

    export let importInstant;
    export let exportInstant;
    export let importTotal;
    export let exportTotal;

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });
</script>

<div class="mx-2 text-sm">
    <strong>{translations.reactive?.title ?? "Reactive"}</strong>
    
    <div class="grid grid-cols-2 mt-4">
        <div>{translations.reactive?.instant_in ?? "Instant in"}</div>
        <div class="text-right">{typeof importInstant !== 'undefined' ? importInstant.toFixed(0) : '-'} VAr</div>
        <div>{translations.reactive?.instant_out ?? "Instant out"}</div>
        <div class="text-right">{typeof exportInstant !== 'undefined' ? exportInstant.toFixed(0) : '-'} VAr</div>
    </div>
    
    <div class="grid grid-cols-2 mt-4">
        <div>{translations.reactive?.total_in ?? "Total in"}</div>
        <div class="text-right">{typeof importTotal !== 'undefined' ? importTotal.toFixed(1) : '-'} kVArh</div>
        <div>{translations.reactive?.total_out ?? "Total out"}</div>
        <div class="text-right">{typeof exportTotal !== 'undefined' ? exportTotal.toFixed(1) : '-'} kVArh</div>
    </div>
</div>