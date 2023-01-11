<script>
    export let config;

    let width;
    let height;
    let barWidth;
    let xScale;
    let yScale;
    let heightAvailable;
    let labelOffset;

    $: {
        heightAvailable = height-(config.title ? 20 : 0);
	    let innerWidth = width - (config.padding.left + config.padding.right);
	    barWidth = innerWidth / config.points.length;
        labelOffset = barWidth < 25 ? 28 : 17;

        let yPerUnit = (heightAvailable-config.padding.top-config.padding.bottom)/(config.y.max-config.y.min);

        xScale = function(i) {
            return (i*barWidth)+config.padding.left;
        };
        yScale = function(i) {
            let ret = 0;
            if(i > config.y.max) 
                ret = config.padding.bottom;
            else if(i < config.y.min)
                ret = heightAvailable-config.padding.bottom;
            else 
                ret = heightAvailable-config.padding.bottom-((i-config.y.min)*yPerUnit);
            return ret > heightAvailable || ret < 0.0 ? 0.0 : ret;
        };
    };
</script>

<div class="chart" bind:clientWidth={width} bind:clientHeight={height}>
    {#if config.title}
    <strong class="text-sm">{config.title}</strong>
    {/if}
    <svg height="{heightAvailable}">
        <!-- y axis -->
        <g class="axis y-axis">
            {#each config.y.ticks as tick}
                <g class="tick tick-{tick.value} tick-{tick.color}" transform="translate(0, {yScale(tick.value)})">
                    <line x2="100%"></line>
                    <text y="-4" x={tick.align == 'right' ? '85%' : ''}>{tick.label}</text>
                </g>
            {/each}
        </g>

        <!-- x axis -->
        <g class="axis x-axis">
            {#each config.x.ticks as point, i}
                <g class="tick" transform="translate({xScale(i)},{heightAvailable})">
                    <text x="{barWidth/2}" y="-4">{point.label}</text>
                </g>
            {/each}
        </g>

        <g class='bars'>
            {#each config.points as point, i}
                {#if point.value !== undefined}
                    <rect
                        x="{xScale(i) + 2}"
                        y="{yScale(point.value)}"
                        width="{barWidth - 4}"
                        height="{yScale(config.y.min) - yScale(Math.min(config.y.min, 0) + point.value)}"
                        fill="{point.color}"
                    />

                    {#if barWidth > 15}
                        <text 
                            y="{yScale(point.value) > yScale(0)-labelOffset ? yScale(point.value) - labelOffset : yScale(point.value)+10}" 
                            x="{xScale(i) + barWidth/2}" 
                            width="{barWidth - 4}"
                            dominant-baseline="middle"
                            text-anchor="{barWidth < 25 ? 'left' : 'middle'}"
                            fill="{yScale(point.value) > yScale(0)-labelOffset ? point.color : 'white'}"
                            transform="rotate({barWidth < 25 ? 90 : 0}, {xScale(i) + (barWidth/2)}, {yScale(point.value) > yScale(0)-labelOffset ? yScale(point.value) - labelOffset : yScale(point.value)+9})"
                        >{point.label}</text>
                    {/if}
                {/if}
                {#if point.value2 > 0.0001}
                    <rect
                        x="{xScale(i) + 2}"
                        y="{yScale(0)}"
                        width="{barWidth - 4}"
                        height="{yScale(config.y.min) - yScale(config.y.min + point.value2)}"
                        fill="{point.color}"
                    />
                    {#if barWidth > 15}
                        <text 
                            y="{yScale(-point.value2) < yScale(0)+12 ? yScale(-point.value2) + 12 : yScale(-point.value2)-10}" 
                            x="{xScale(i) + barWidth/2}" 
                            width="{barWidth - 4}"
                            dominant-baseline="middle"
                            text-anchor="{barWidth < 25 ? 'left' : 'middle'}"
                            fill="{yScale(-point.value2) < yScale(0)+12 ? point.color : 'white'}"
                            transform="rotate({barWidth < 25 ? 90 : 0}, {xScale(i) + (barWidth/2)}, {yScale(point.value2 - config.y.min) > yScale(0)-12 ? yScale(point.value2 - config.y.min) - 12 : yScale(point.value2 - config.y.min)+9})"
                        >{point.label2}</text>
                    {/if}
                {/if}
            {/each}
        </g>
    </svg>
</div>
