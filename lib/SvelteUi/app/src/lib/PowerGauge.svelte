<script>
    import PowerGaugeSvg from './PowerGaugeSvg.svelte';
    import { formatUnit } from './Helpers';

    export let val;
    export let max;
    export let unit;
    export let label;
    export let sub = "";
    export let subunit = "";
    export let colorFn;

    let arr;
    let pct = 0;
    $: {
        arr = formatUnit(val, unit);
        pct = (Math.min(val,max)/max) * 100
    }
</script>

<div class="pl-root">
    <PowerGaugeSvg pct={pct} color={colorFn(pct, document.documentElement.classList.contains('dark'))}/>
    <span class="pl-ov">
        <span class="pl-lab">{label}</span>
        <br/>
        <span class="pl-val">{arr[0]}</span> 
        <span class="pl-unt">{arr[1]}</span>
        {#if sub}
        <br/>
        <span class="pl-sub">{sub}</span>
        <span class="pl-snt">{subunit}/kWh</span>
        {/if}
    </span>
</div>
