<script>
    import { ampcol, zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let title;
    export let translations = {};

    let dark = document.documentElement.classList.contains('dark');

    let config = {};
    let max = 0;
    let min = 0;

    export let tariffData;
    export let realtime;

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];

        yTicks.push({
            value: 0,
            label: 0
        });

        console.log(realtime);

        if(tariffData && !isNaN(realtime?.h?.u)) {
            points.push({
                label: realtime.h.u.toFixed(2),
                value: realtime.h.u,
                title: realtime.h.u.toFixed(2) + ' kWh',
                color: ampcol(realtime.h.u/tariffData.c*100.0)
            });
            xTicks.push({
                label: translations.common?.now ?? "Now"
            });
        }

        if(tariffData && tariffData.p) {
            for(i = 0; i < tariffData.p.length; i++) {
                let peak = tariffData.p[i];
                let daylabel = peak.d > 0 ? zeropad(peak.d) + "." + (translations.months ? translations.months?.[new Date().getMonth()] : zeropad(new Date().getMonth()+1)) : "-";
                let title = daylabel;
                if(!isNaN(peak.h))
                    title = title + ' ' + zeropad(peak.h) + ':00';
                title = title + ': ' + peak.v.toFixed(2) + ' kWh';
                points.push({
                    label: peak.v.toFixed(2), 
                    value: peak.v, 
                    title: title,
                    color: dark ? '#5c2da5' : '#7c3aed'
                });
                xTicks.push({
                    label: daylabel
                })
                max = Math.max(max, peak.v);
            }
        }

        if(tariffData && tariffData.t) {
            for(i = 0; i < tariffData.t.length; i++) {
                let val = tariffData.t[i];
                if(val >= max) break;
                yTicks.push({
                    value: val,
                    label: val
                });
            }

            yTicks.push({
                label: tariffData.m.toFixed(1), 
                align: 'right',
                color: 'green',
                value: tariffData.m, 
            });
        }

        if(tariffData && tariffData.c) {
            yTicks.push({
                label: tariffData.c.toFixed(0), 
                color: 'orange',
                value: tariffData.c, 
            });
            max = Math.max(max, tariffData.c);
        }

        max = Math.ceil(max);

        config = {
            title: title,
            dark: document.documentElement.classList.contains('dark'),
            padding: { top: 20, right: 20, bottom: 20, left: 20 },
            y: {
                min: min,
                max: max,
                ticks: yTicks
            },
            x: {
                ticks: xTicks
            },
            points: points
        };

    }
</script>

<BarChart config={config} />
