<script>
    import { realtimeStore, getRealtime, isRealtimeFullyLoaded } from './RealtimeStore.js';

    export let title;

    let dark = document.documentElement.classList.contains('dark');

    let realtime = null;
    realtimeStore.subscribe(update => {
        realtime = update;
    });

    let visible = false;

    let max;
    let min;

    let width;
    let height;
    let heightAvailable;
    let widthAvailable;
    let points;

    let yScale = function(val, min, max) {
        return Math.ceil(heightAvailable - (((val-min)/(max - min)) * heightAvailable) ) - 25;
    };
    let xScale = function(val) {
        return 30 + Math.ceil((val/realtime.size) * (widthAvailable-35));
    };

    let yTicks;
    let xTicks;
    let barWidth;
    let unit

    $:{
        heightAvailable = parseInt(height) - 50;
        widthAvailable = width - 35;
	    barWidth = widthAvailable / realtime.size;

        min = 0;
        max = 0;

        if(realtime.data && heightAvailable > 10 && widthAvailable > 100 && barWidth > 0.1) {
            visible = true;
            for(let p in realtime.data) {
                let val = realtime.data[p];
                if(isNaN(val)) val = 0;
                max = Math.max(Math.ceil(val/1000.0)*1000, max);
                min = Math.min(Math.floor(val/1000.0)*1000, min);
            }
            let range = max - min;

            unit = max > 2500 ? 'kW' : 'W';
            
            yTicks = [];
            for(let i = min; i < max; i+= range/5) {
                yTicks.push({
                    value: i,
                    label: max > 2500 ? (i / 1000).toFixed(1) : i
                });
                if(yTicks.length > 6) break;
            }

            xTicks = [];
            for(let i = 0; i < realtime.size; i+=Math.round(realtime.size/Math.round(widthAvailable / 120))) {
                xTicks.push({
                    value: i,
                    label: '-'+Math.round((realtime.size - i) / 6)+' min'
                });
                if(xTicks.length > 12) break;
            }

            let i = realtime.size;
            points = xScale(realtime.size)+","+yScale(0, min, max)+" "+xScale(1)+","+yScale(0, min, max);
            for(let p in realtime.data) {
                if(i < 0) {
                    break;
                }
                let val = realtime.data[p];
                if(isNaN(val)) val = 0;
                points = xScale(i--)+","+yScale(val, min, max)+" "+points;
            }
        } else {
            visible = false;
        }
    }
</script>

<div class="chart" bind:clientWidth={width} bind:clientHeight={height}>
    {#if visible}
        <strong class="text-sm">{title} ({unit})</strong>
        {#if yTicks}
            <svg viewBox="0 0 {widthAvailable} {heightAvailable}"  height="100%">

                <!-- y axis -->
                <g class="axis y-axis">
                    {#each yTicks as tick}
                        {#if !isNaN(yScale(tick.value, min, max))}
                            <g class="tick tick-{tick.value}" transform="translate(0, {yScale(tick.value, min, max)})">
                                <line x2="100%"></line>
                                <text y="-4">{tick.label}</text>
                            </g>
                            {/if}
                    {/each}
                </g>

                <!-- x axis -->
                <g class="axis x-axis">
                    {#each xTicks as point, i}
                        {#if !isNaN(xScale(point.value))}
                            <g class="tick" transform="translate({xScale(point.value)},{heightAvailable})">
                                <text x="{barWidth/2}" y="-4">{point.label}</text>
                            </g>
                        {/if}
                    {/each}
                </g>

                <!-- Line -->
                <polyline
                opacity="0.9"
                    fill={dark ? '#5c2da5' : '#7c3aed'}
                    stroke={dark ? '#5c2da5' : '#7c3aed'}
                    stroke-width="1"
                    points={points}/>
            </svg>
        {/if}
    {:else}
        <strong class="text-sm">{title} not available</strong>
    {/if}
</div>
