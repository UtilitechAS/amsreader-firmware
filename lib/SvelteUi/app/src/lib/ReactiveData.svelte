<script>
    export let importInstant;
    export let exportInstant;
    export let importTotal;
    export let exportTotal;
</script>

<div class="mx-2 text-sm"> <!-- EHorvat translated to german- ... Reactive/Blindleistung   Instant/Aktuell Total/Summe in/Bez.  out/Exp.-->
    <strong>Blindleistung</strong>
    
    <div class="grid grid-cols-2 mt-4">
        <div>Aktuell Bez.</div>
        <div class="text-right">{typeof importInstant !== 'undefined' ? importInstant.toFixed(0) : '-'} VAr</div>
        <div>Aktuell Exp.</div>
        <div class="text-right">{typeof exportInstant !== 'undefined' ? exportInstant.toFixed(0) : '-'} VAr</div>
    </div>
    
    <div class="grid grid-cols-2 mt-4">
        <div>Total Bez.</div>
        <div class="text-right">{typeof importTotal !== 'undefined' ? importTotal.toFixed(1) : '-'} kVArh</div>
        <div>Total Exp.</div>
        <div class="text-right">{typeof exportTotal !== 'undefined' ? exportTotal.toFixed(1) : '-'} kVArh</div>
    </div>
</div>