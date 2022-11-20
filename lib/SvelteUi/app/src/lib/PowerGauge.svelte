<script>
    import PowerGaugeSvg from './PowerGaugeSvg.svelte';
    import { ampcol } from './Helpers.js';

    export let val;
    export let max;
    export let unit;
    export let label;
</script>

<div class="overlay-plot">
    <PowerGaugeSvg pct={val/max * 100} color={ampcol(val/max * 100)}/>
    <span class="plot-overlay">
        <span class="plot-value">{val}</span> 
        <span class="plot-unit">{unit}</span>
        <br/>
        <span class="plot-label">{label}</span>
    </span>
</div>

<style>
.overlay-plot {
    position: relative;
}
.plot-overlay {
    position: absolute;
    top: 35%;
    left: 25%;
    width: 50%;
    text-align: center;
}
.plot-value {
    font-size: 1.7rem;
    //cursor: pointer;
}
.plot-unit {
    font-size: 1.0rem;
    color: grey;
}
.plot-label {
    font-size: 1.0rem;
}
</style>
