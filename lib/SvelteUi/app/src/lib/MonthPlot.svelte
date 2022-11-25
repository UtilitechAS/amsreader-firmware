<script>
    import { zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let json;

    let config = {};
    let max = 0;
    let min = 0;

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];
        let cur = new Date();
        let lm = new Date();
        lm.setDate(0);

        for(i = cur.getDate(); i<=lm.getDate(); i++) {
            let imp = json["i"+zeropad(i)];
            let exp = json["e"+zeropad(i)];
            if(imp === undefined) imp = 0;
            if(exp === undefined) exp = 0;

            xTicks.push({
                label: zeropad(i)
            });
            points.push({
                label: imp.toFixed(0), 
                value: imp, 
                label2: exp.toFixed(0), 
                value2: exp,
                color: '#7c3aed' 
            });
            min = Math.max(min, exp);
            max = Math.max(max, imp);
        }
        for(i = 1; i < cur.getDate(); i++) {
            let imp = json["i"+zeropad(i)];
            let exp = json["e"+zeropad(i)];
            if(imp === undefined) imp = 0;
            if(exp === undefined) exp = 0;

            xTicks.push({
                label: zeropad(i)
            });
            points.push({
                label: imp.toFixed(0), 
                value: imp, 
                label2: exp.toFixed(0), 
                value2: exp,
                color: '#7c3aed' 
            });
            min = Math.max(min, exp);
            max = Math.max(max, imp);
        }

        let boundary = Math.ceil(Math.max(min, max)/10)*10;

        max = boundary;
        min = min == 0 ? 0 : boundary*-1;

        if(min < 0) {
            let yTickDistDown = min/4;
            for(i = 0; i < 5; i++) {
                let val = (yTickDistDown*i);
                yTicks.push({
                    value: val,
                    label: val.toFixed(0)
                });
            }
        }

        let yTickDistUp = max/4;
        for(i = 0; i < 5; i++) {
            let val = (yTickDistUp*i);
            yTicks.push({
                value: val,
                label: val.toFixed(0)
            });
        }

        config = {
            title: "Energy use last month (kWh)",
            height: 226,
            width: 1520,
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

<BarChart config={config} />
