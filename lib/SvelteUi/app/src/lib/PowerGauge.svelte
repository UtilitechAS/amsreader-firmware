<script>
    import PowerGaugeSvg from './PowerGaugeSvg.svelte';
    import { ampcol } from './Helpers.js';

    export let val;
    export let max;
    export let unit;
    export let label;
    export let sub = "";
    export let subunit = "";

    let pct = 0;
    $: {
        pct = (Math.min(val,max)/max) * 100
    }
</script>

<div class="pl-root">
    <PowerGaugeSvg pct={pct} color={ampcol(pct)}/>
    <span class="pl-ov">
        <span class="pl-lab">{label}</span>
        <br/>
        <span class="pl-val">{val}</span> 
        <span class="pl-unt">{unit}</span>
        {#if sub}
        <br/>
        <span class="pl-sub">{sub}</span>
        <span class="pl-snt">{subunit}/kWh</span>
        {/if}
    </span>
</div>
