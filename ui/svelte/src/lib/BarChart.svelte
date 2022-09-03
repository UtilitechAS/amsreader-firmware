<script>
    export let config;

    let barWidth;
    let xScale;
    let yScale;

    $: {
	    let innerWidth = config.width - (config.padding.left + config.padding.right);
	    barWidth = innerWidth / config.points.length;

        let yPerUnit = (config.height-config.padding.top-config.padding.bottom)/(config.y.max-config.y.min);

        xScale = function(i) {
            return (i*barWidth)+config.padding.left;
        };
        yScale = function(i) {
            if(!i) return config.height-config.padding.bottom;
            if(i > config.y.max) return config.height;
            let ret = config.height-config.padding.bottom-((i-config.y.min)*yPerUnit);
            return ret > config.height || ret < 0 ? 0 : ret;
        };
    };
</script>

<div class="chart" bind:clientWidth={config.width} bind:clientHeight={config.height}>
    <svg height="{config.height}">
        <!-- y axis -->
        <g class="axis y-axis">
            {#each config.y.ticks as tick}
                <g class="tick tick-{tick.value}" transform="translate(0, {yScale(tick.value)})">
                    <line x2="100%"></line>
                    <text y="-4">{tick.label}</text>
                </g>
            {/each}
        </g>

        <!-- x axis -->
        <g class="axis x-axis">
            {#each config.x.ticks as point, i}
                <g class="tick" transform="translate({xScale(i)},{config.height})">
                    <text x="{barWidth/2}" y="-4">{point.label}</text>
                </g>
            {/each}
        </g>

        <g class='bars'>
            {#each config.points as point, i}
                <rect
                    x="{xScale(i) + 2}"
                    y="{yScale(point.value)}"
                    width="{barWidth - 4}"
                    height="{yScale(0) - yScale(point.value)}"
                    fill="{point.color}"
                />
                <text 
                    y="{yScale(point.value) > yScale(0)-15 ? yScale(point.value) - 12 : yScale(point.value)+10}" 
                    x="{xScale(i) + barWidth/2}" 
                    width="{barWidth - 4}"
                    dominant-baseline="middle"
                    text-anchor="{barWidth < 25 ? 'left' : 'middle'}"
                    fill="{yScale(point.value) > yScale(0)-15 ? point.color : 'white'}"
                    transform="rotate({barWidth < 25 ? 90 : 0}, {xScale(i) + (barWidth/2)}, {yScale(point.value) > yScale(0)-12 ? yScale(point.value) - 12 : yScale(point.value)+10})"
                >{point.label}</text>
            {/each}
        </g>
    </svg>
</div>

<style>
	h2 {
		text-align: center;
	}

	.chart {
		width: 100%;
		margin: 0 auto;
	}

	svg {
		position: relative;
		width: 100%;
	}

	.tick {
		font-family: Helvetica, Arial;
		font-size: .725em;
		font-weight: 200;
	}

	.tick line {
		stroke: #e2e2e2;
		stroke-dasharray: 2;
	}

	.tick text {
		fill: #999;
		text-anchor: start;
	}

	.tick.tick-0 line {
		stroke-dasharray: 0;
	}

	.x-axis .tick text {
		text-anchor: middle;
	}

	.bars rect {
		stroke: rgb(0,0,0);
        stroke-opacity: 0.25;
		opacity: 0.9;
	}

    .bars text {
		font-family: Helvetica, Arial;
		font-size: .725em;
        display: block;
        text-align: center;
    }
</style>
