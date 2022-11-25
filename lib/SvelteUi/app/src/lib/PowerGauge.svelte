<script>
    import PowerGaugeSvg from './PowerGaugeSvg.svelte';
    import { ampcol } from './Helpers.js';

    export let val;
    export let max;
    export let unit;
    export let label;
</script>

<div class="pl-root">
    <PowerGaugeSvg pct={val/max * 100} color={ampcol(val/max * 100)}/>
    <span class="pl-ov">
        <span class="pl-val">{val}</span> 
        <span class="pl-unt">{unit}</span>
        <br/>
        <span class="pl-lab">{label}</span>
    </span>
</div>
