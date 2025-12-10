<script>
    import { zeropad, addHours, addMinutes, getPriceSourceName, getPriceSourceUrl, formatCurrency } from './Helpers.js';
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
            let i = Math.floor(((cur.getHours()*60) + cur.getMinutes()) / json?.resolution);
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
            let range = Math.max(max, Math.abs(min));

            if(min < 0) {
                min = Math.min((range/4)*-1, min);
                let yTicksNum = Math.ceil((Math.abs(min)/range) * 4);
                let yTickDistDown = min/yTicksNum;
                for(i = 1; i < yTicksNum+1; i++) {
                    let val = (yTickDistDown*i);
                    yTicks.push({
                        value: val,
                        label: (val/100).toFixed(2)
                    });
                }
            }

            max = Math.max((range/4), max);
            let xTicksNum = Math.ceil((max/range) * 4);
            let yTickDistUp = max/xTicksNum;
            for(i = 0; i < xTicksNum+1; i++) {
                let val = (yTickDistUp*i);
                yTicks.push({
                    value: val,
                    label: (val/100).toFixed(2)
                });
            }

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