<script>
    import { dataStore, realtimeStore, getRealtime, isRealtimeFullyLoaded } from './DataStores.js';

    export let title;

    let dark = document.documentElement.classList.contains('dark');

    let lastUp = 0;
    let lastValue = 0;
    let lastUpdate = 0;
    let updateCount = 0;

    let realtimeRequested = false;
    let realtime = null;
    realtimeStore.subscribe(update => {
        realtime = update;
        lastUpdate = lastUp;
        updateCount = 0;
    });

    let visible = false;

    function addValue() {
        if(updateCount == 60 || lastUpdate > lastUp || lastUpdate - lastUp > 300) {
            getRealtime();
        } else {
            while(lastUp > lastUpdate) {
                realtime.data.unshift(lastValue);
                realtime.data = realtime.data.slice(0,realtime.size);
                lastUpdate += 10;
                updateCount++;
            }
        }
    }

    dataStore.subscribe(update => {
        lastValue = update.i-update.e;
        lastUp = update.u;
        if(!realtimeRequested) {
            getRealtime();
            realtimeRequested = true;
            return;
        }
        if(!isRealtimeFullyLoaded()) return;
        addValue();
    });

    let max;
    let min;

    let width;
    let height;
    let heightAvailable;
    let widthAvailable;
    let points;
    let yScale;
    let xScale;

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

        console.log("\n--Realtime plot debug--")
        console.log("Data length: %d\nSize: %d", realtime?.data?.length, realtime?.size);
        console.log("Height: %d\nWidth: %d\nBar width: %s", heightAvailable, widthAvailable, barWidth);

        if(realtime.data && heightAvailable > 10 && widthAvailable > 100 && barWidth > 0.1) {
            visible = true;
            for(let p in realtime.data) {
                let val = realtime.data[p];
                if(isNaN(val)) val = 0;
                max = Math.max(Math.ceil(val/1000.0)*1000, max);
                min = Math.min(Math.ceil(val/1000.0)*1000, min);
            }

            unit = max > 2500 ? 'kW' : 'W';
            
            yTicks = [];
            for(let i = min; i < max; i+= max/5) {
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

            yScale = function(val) {
                return Math.ceil(heightAvailable - ((val/max)* heightAvailable) ) - 25;
            };
            xScale = function(val) {
                return 30 + Math.ceil((val/realtime.size) * (widthAvailable-35));
            };

            let i = realtime.size;
            points = xScale(realtime.size)+","+yScale(0)+" "+xScale(1)+","+yScale(0);
            for(let p in realtime.data) {
                if(i < 0) {
                    break;
                }
                let val = realtime.data[p];
                if(isNaN(val)) val = 0;
                points = xScale(i--)+","+yScale(val)+" "+points;
            }
        } else {
            visible = false;
        }
        console.log("Min: %d\nMax: %d\nShow: %s", min, max, visible);
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
                        {#if !isNaN(yScale(tick.value))}
                            <g class="tick tick-{tick.value}" transform="translate(0, {yScale(tick.value)})">
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
