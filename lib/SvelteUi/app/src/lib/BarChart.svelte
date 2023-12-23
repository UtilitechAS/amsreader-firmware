<script>
    export let config;

    let width;
    let height;
    let barWidth;
    let xScale;
    let yScale;
    let heightAvailable;
    let labelOffset;
    let vertSwitch = 30;
    let titleHeight = 0;

    $: {
        heightAvailable = height-titleHeight;
	    let innerWidth = width - (config.padding.left + config.padding.right);
	    barWidth = innerWidth / config.points.length;
        labelOffset = barWidth < vertSwitch ? 30 : 15;

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
    {#if config.x.ticks && config.points && heightAvailable}
        {#if config.title}
            <div class="text-sm font-bold" bind:clientHeight={titleHeight}>{config.title}</div>
        {/if}
        <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {width} {heightAvailable}">
            <!-- y axis -->
            <g class="axis y-axis">
                {#each config.y.ticks as tick}
                    {#if !isNaN(yScale(tick.value))}
                        <g class="tick tick-{tick.value} tick-{tick.color}" transform="translate(0, {yScale(tick.value)})">
                            <line x2="100%"></line>
                            <text y="-4" x={tick.align == 'right' ? '85%' : ''}>{tick.label}</text>
                        </g>
                        {/if}
                {/each}
            </g>

            <!-- x axis -->
            <g class="axis x-axis">
                {#each config.x.ticks as point, i}
                    {#if !isNaN(xScale(i))}
                        <g class="tick" transform="translate({xScale(i)},{heightAvailable})">
                            {#if barWidth > 20 || i%2 == 0}
                            <text x="{barWidth/2}" y="-4">{point.label}</text>
                            {/if}
                        </g>
                        {/if}
                {/each}
            </g>

            <g class='bars'>
                {#each config.points as point, i}
                    {#if !isNaN(xScale(i)) && !isNaN(yScale(point.value))}
                        <g>
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
                                    width="{barWidth - 4}"
                                    dominant-baseline="middle"
                                    text-anchor="{barWidth < vertSwitch || point.labelAngle ? 'left' : 'middle'}"
                                    fill="{yScale(point.value) > yScale(0)-labelOffset ? point.color : 'white'}"
                                    transform="translate({xScale(i) + barWidth/2} {yScale(point.value) > yScale(0) - labelOffset ? yScale(point.value) - labelOffset : yScale(point.value) + 10}) rotate({point.labelAngle ? point.labelAngle : barWidth < vertSwitch ? 90 : 0})"
                                    
                                >{point.label}</text>
                                {#if point.title}
                                <title>{point.title}</title>
                                {/if}
                            {/if}
                        {/if}
                        </g>
                        <g>
                        {#if point.value2 > 0.0001}
                            <rect
                                x="{xScale(i) + 2}"
                                y="{yScale(0)}"
                                width="{barWidth - 4}"
                                height="{yScale(config.y.min) - yScale(config.y.min + point.value2)}"
                                fill="{point.color2 ? point.color2 : point.color}"
                            />
                            {#if barWidth > 15}
                                <text 
                                    width="{barWidth - 4}"
                                    dominant-baseline="middle"
                                    text-anchor="{'middle'}"
                                    fill="{yScale(-point.value2) < yScale(0) + 15 ? point.color2 ? point.color2 : point.color : 'white'}"
                                    transform="translate({xScale(i) + (barWidth/2)} {yScale(-point.value2) < yScale(0) + 15 ? yScale(-point.value2) + 15 : yScale(-point.value2) - 14}) rotate({barWidth < vertSwitch ? 90 : 0})"
                                >{point.label2}</text>
                                {#if point.title2}
                                <title>{point.title2}</title>
                                {/if}
                            {/if}
                        {/if}
                        </g>
                    {/if}
                {/each}
            </g>
        </svg>
    {/if}
</div>
