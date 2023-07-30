<script>
    import { zeropad, addHours } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let json;
    export let sysinfo;

    let config = {};
    let max;
    let min;

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];
        min = max = 0;
        let cur = addHours(new Date(), -24);
        let currentHour = new Date().getUTCHours();
        addHours(cur, sysinfo.clock_offset - ((24 + cur.getHours() - cur.getUTCHours())%24));
        for(i = currentHour; i<24; i++) {
            let imp = json["i"+zeropad(i)];
            let exp = json["e"+zeropad(i)];
            if(imp === undefined) imp = 0;
            if(exp === undefined) exp = 0;

            xTicks.push({
                label: zeropad(cur.getHours())
            });
            points.push({
                label: imp.toFixed(1), 
                title: imp.toFixed(2) + ' kWh',
                value: imp*10, 
                label2: exp.toFixed(1), 
                title2: exp.toFixed(2) + ' kWh',
                value2: exp*10,
                color: '#7c3aed' 
            });
            min = Math.max(min, exp*10);
            max = Math.max(max, imp*10);
            addHours(cur, 1);
        };
        for(i = 0; i < currentHour; i++) {
            let imp = json["i"+zeropad(i)];
            let exp = json["e"+zeropad(i)];
            if(imp === undefined) imp = 0;
            if(exp === undefined) exp = 0;

            xTicks.push({
                label: zeropad(cur.getHours())
            });
            points.push({
                label: imp.toFixed(1), 
                title: imp.toFixed(2) + ' kWh',
                value: imp*10, 
                label2: exp.toFixed(1), 
                title2: exp.toFixed(2) + ' kWh',
                value2: exp*10,
                color: '#7c3aed' 
            });
            min = Math.max(min, exp*10);
            max = Math.max(max, imp*10);
            addHours(cur, 1);
        };

        let boundary = Math.ceil(Math.max(min, max));

        max = boundary;
        min = min == 0 ? 0 : boundary*-1;

        if(min < 0) {
            let yTickDistDown = min/4;
            for(i = 1; i < 5; i++) {
                let val = (yTickDistDown*i);
                yTicks.push({
                    value: val,
                    label: (val/10).toFixed(1)
                });
            }
        }

        let yTickDistUp = max/4;
        for(i = 0; i < 5; i++) {
            let val = (yTickDistUp*i);
            yTicks.push({
                value: val,
                label: (val/10).toFixed(1)
            });
        }

        config = {
            title: "Energy use last 24 hours (kWh)",
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
