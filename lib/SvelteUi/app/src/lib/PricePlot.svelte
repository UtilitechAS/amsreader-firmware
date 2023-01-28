<script>
    import { zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let json;

    let config = {};
    let max = 0;
    let min = 0;

    $: {
        let hour = new Date().getUTCHours();
        let i = 0;
        let val = 0;
        let h = 0;
        let d = json["20"] == null ? 2 : 1;
        let yTicks = [];
        let xTicks = [];
        let points = [];
        let cur = new Date();
        for(i = hour; i<24; i++) {
            cur.setUTCHours(i);
            val = json[zeropad(h++)];
            if(val == null) break;
            xTicks.push({
                label: zeropad(cur.getHours())
            });
            points.push({
                label: val > 0 ? val.toFixed(d) : '', 
                value: val > 0 ? Math.abs(val*100) : 0, 
                label2: val < 0 ? val.toFixed(d) : '', 
                value2: val < 0 ? Math.abs(val*100) : 0, 
                color: '#7c3aed' 
            });
            min = Math.min(min, val*100);
            max = Math.max(max, val*100);
        };
        for(i = 0; i < 24; i++) {
            cur.setUTCHours(i);
            val = json[zeropad(h++)];
            if(val == null) break;
            xTicks.push({
                label: zeropad(cur.getHours())
            });
            points.push({
                label: val > 0 ? val.toFixed(d) : '', 
                value: val > 0 ? Math.abs(val*100) : 0, 
                label2: val < 0 ? val.toFixed(d) : '', 
                value2: val < 0 ? Math.abs(val*100) : 0, 
                color: '#7c3aed' 
            });
            min = Math.min(min, val*100);
            max = Math.max(max, val*100);
        };

        max = Math.ceil(max);
        min = Math.floor(min);

        if(min < 0) {
            let yTickDistDown = min/4;
            for(i = 1; i < 5; i++) {
                let val = (yTickDistDown*i);
                yTicks.push({
                    value: val,
                    label: (val/100).toFixed(2)
                });
            }
        }

        let yTickDistUp = max/4;
        for(i = 0; i < 5; i++) {
            let val = (yTickDistUp*i);
            yTicks.push({
                value: val,
                label: (val/100).toFixed(2)
            });
        }

        config = {
            title: "Future energy price (" + json.currency + ")",
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
    };

</script>

<a href="https://transparency.entsoe.eu/" target="_blank" class="text-xs float-right z-40">Provided by ENTSO-E</a>
<BarChart config={config} />
