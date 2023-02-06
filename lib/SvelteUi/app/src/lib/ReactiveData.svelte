<script>
    export let importInstant;
    export let exportInstant;
    export let importTotal;
    export let exportTotal;
</script>

<div class="mx-2 text-sm">
    <strong>Reactive</strong>
    
    <div class="grid grid-cols-2 mt-4">
        <div>Instant in</div>
        <div class="text-right">{typeof importInstant !== 'undefined' ? importInstant.toFixed(0) : '-'} VAr</div>
        <div>Instant out</div>
        <div class="text-right">{typeof exportInstant !== 'undefined' ? exportInstant.toFixed(0) : '-'} VAr</div>
    </div>
    
    <div class="grid grid-cols-2 mt-4">
        <div>Total in</div>
        <div class="text-right">{typeof importTotal !== 'undefined' ? importTotal.toFixed(1) : '-'} kVArh</div>
        <div>Total out</div>
        <div class="text-right">{typeof exportTotal !== 'undefined' ? exportTotal.toFixed(1) : '-'} kVArh</div>
    </div>
</div>