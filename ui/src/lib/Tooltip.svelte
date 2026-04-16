<script>
	export let title;
	export let x;
	export let y;

    let width;
    let height;
</script>
<div 
    class="tooltip" 
    style="top: {y - height - 10}px; left: {x - (width / 2)}px;"
    bind:clientHeight={height}
    bind:clientWidth={width}
>{title}</div>
