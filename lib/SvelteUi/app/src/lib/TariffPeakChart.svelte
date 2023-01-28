<script>
    import { monthnames, zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';
    import { tariffStore, getTariff } from './DataStores';

    let config = {};
    let max = 0;
    let min = 0;

    let tariffData;
    tariffStore.subscribe(update => {
        tariffData = update;
    });
    getTariff();

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];

        yTicks.push({
            value: 0,
            label: 0
        });

        if(tariffData && tariffData.p) {
            for(i = 0; i < tariffData.p.length; i++) {
                let peak = tariffData.p[i];
                points.push({
                    label: peak.v.toFixed(2), 
                    value: peak.v, 
                    color: '#7c3aed' 
                });
                xTicks.push({
                    label: peak.d > 0 ? zeropad(peak.d) + "." + monthnames[new Date().getMonth()] : "-"
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
            title: "Tariff peaks",
            padding: { top: 20, right: 35, bottom: 20, left: 35 },
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
