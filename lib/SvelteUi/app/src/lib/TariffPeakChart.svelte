<script>
    import { zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    let monthnames = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];

    export let data;
    
    let config = {};
    let max = 0;
    let min = 0;

    let thresholds = [5,10,15,20,25,50,75,100,150];

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];

        yTicks.push({
            value: 0,
            label: 0
        });

        if(data && data.x) {
            for(i = 0; i < thresholds.length; i++) {
                let val = thresholds[i];
                if(val >= data.x) break;
                yTicks.push({
                    value: val,
                    label: val
                });
            }

            yTicks.push({
                label: data.x.toFixed(1), 
                align: 'right',
                color: 'green',
                value: data.x, 
            });
        }

        if(data && data.t) {
            yTicks.push({
                label: data.t.toFixed(1), 
                color: 'orange',
                value: data.t, 
            });
        }

        if(data && data.p) {
            for(i = 0; i < data.p.length; i++) {
                let val = data.p[i];
                points.push({
                    label: val.toFixed(2), 
                    value: val, 
                    color: '#7c3aed' 
                });
                xTicks.push({
                    label: monthnames[new Date().getMonth()]
                })
                max = Math.max(max, val);
            }
        }

        max = Math.ceil(max);

        config = {
            title: "Tariff peaks",
            padding: { top: 20, right: 15, bottom: 20, left: 35 },
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
