<script>
    import { zeropad, addHours, exportcol } from './Helpers.js';
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
                color: '#7c3aed',
                color2: '#37829E' 
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
                color: '#7c3aed',
                color2: '#37829E' 
            });
            min = Math.max(min, exp*10);
            max = Math.max(max, imp*10);
            addHours(cur, 1);
        };

        min *= -1;
        let range = Math.max(max, Math.abs(min));

        if(min < 0) {
            min = Math.min((range/4)*-1, min);
            let yTicksNum = Math.ceil((Math.abs(min)/range) * 4);
            let yTickDistDown = min/yTicksNum;
            for(i = 1; i < yTicksNum+1; i++) {
                let val = (yTickDistDown*i);
                yTicks.push({
                    value: val,
                    label: (val/10).toFixed(1)
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
