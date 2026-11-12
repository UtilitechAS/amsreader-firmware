<script>
    import { zeropad, addHours, addMinutes, getPriceSourceName, getPriceSourceUrl, formatCurrency, niceAxis } from './Helpers.js';
    import BarChart from './BarChart.svelte';
    import { onMount } from 'svelte';

    export let title;
    export let json;
    export let sysinfo;

    let config = {};
    let max;
    let min;

    let dark = document.documentElement.classList.contains('dark');

    let cur = new Date();

    onMount(() => {
        let timeout;
        function scheduleUpdate() {
            cur = new Date();
            timeout = setTimeout(() => {
                scheduleUpdate();
            }, (15 - (cur.getMinutes() % 15)) * 60000);
        }

        scheduleUpdate();

		return () => {
			clearTimeout(timeout);
		};
	});

    $: {
        if(json?.prices?.length > 0) {
            cur = new Date();
            let currency = json?.currency;
            let val = 0;
            let yTicks = [];
            let xTicks = [];
            let values = [];
            min = max = 0;
            addHours(cur, sysinfo.clock_offset - ((24 + cur.getHours() - cur.getUTCHours())%24));
            let i = json?.cursor ? json.cursor : 0;
            if(json?.fetched && json?.resolution) {
                // The cursor is from when the payload was fetched, the labels
                // are from now, so skip the points passed since. Price points
                // start on the whole hour in the price zone, which is a whole
                // number of hours from UTC, so epoch boundaries line up
                let pointMs = json.resolution * 60000;
                i += Math.floor(Date.now()/pointMs) - Math.floor(json.fetched/pointMs);
            }
            cur.setMinutes(Math.floor(cur.getMinutes()/json?.resolution)*json?.resolution,0,0);
            while(i < json?.prices?.length) {
                val = json.prices[i];
                if(val == null) break;
                xTicks.push({
                    label: values.length > 0 && json?.resolution < 60 && cur.getMinutes() != 0 ? '' : zeropad(cur.getHours())
                });
                values.push(val*100);
                min = Math.min(min, val*100);
                max = Math.max(max, val*100);
                addMinutes(cur, json?.resolution);
                i++;
            }

            let ret = formatCurrency(Math.max(Math.abs(min) / 100.0, Math.abs(max) / 100.0), currency);
            if(ret && ret[1] && ret[1] != currency) {
                currency = ret[1];
                min *= 100;
                max *= 100;
                for(i = 0; i < values.length; i++) {
                    values[i] *= 100;
                }
            }

            let points = [];
            for(i = 0; i < values.length; i++) {
                val = values[i];
                let disp = val * 0.01;
                let d = Math.abs(val) < 1000 ? 2 : 0;
                points.push({
                    label: disp >= 0 ? disp.toFixed(d) : '',
                    title: disp >= 0 ? disp.toFixed(2) + ' ' + currency : '',
                    value: val >= 0 ? Math.abs(val) : 0,
                    label2: disp < 0 ? disp.toFixed(d) : '',
                    title2: disp < 0 ? disp.toFixed(2) + ' ' + currency : '',
                    value2: val < 0 ? Math.abs(val) : 0,
                    color: dark ? '#5c2da5' : '#7c3aed'
                });
            }
            let axis = niceAxis(min / 100, max / 100, 100);
            min = axis.min;
            max = axis.max;
            yTicks = axis.ticks;

            config = {
                title: title + " (" + currency + ")",
                dark: document.documentElement.classList.contains('dark'),
                padding: { top: 20, right: 15, bottom: 20, left: 35 },
                y: {
                    min: min,
                    max: max,
                    ticks: yTicks
                },
                x: {
                    ticks: xTicks
                },
                points: points,
                link: {
                    text: "Provided by: " + getPriceSourceName(json.source),
                    url: getPriceSourceUrl(json.source),
                    target: '_blank'
                }
            };
        }
    };

</script>

{#if config.points && config.points.length > 0}
    <BarChart config={config} />
{/if}