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
        let cur = new Date();
        let lm = new Date();
        addHours(cur, sysinfo.clock_offset - ((24 + cur.getHours() - cur.getUTCHours())%24));
        addHours(lm, sysinfo.clock_offset - ((24 + lm.getHours() - lm.getUTCHours())%24));
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
                label: imp.toFixed(imp < 10 ? 1 : 0), 
                title: imp.toFixed(2) + ' kWh',
                value: imp, 
                label2: exp.toFixed(exp < 10 ? 1 : 0), 
                title2: exp.toFixed(2) + ' kWh',
                value2: exp,
                color: '#7c3aed',
                color2: '#37829E' 
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
                label: imp.toFixed(imp < 10 ? 1 : 0), 
                title: imp.toFixed(2) + ' kWh', 
                value: imp, 
                label2: exp.toFixed(exp < 10 ? 1 : 0), 
                title2: exp.toFixed(2) + ' kWh', 
                value2: exp,
                color: '#7c3aed',
                color2: '#37829E' 
            });
            min = Math.max(min, exp);
            max = Math.max(max, imp);
        }

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
                    label: val.toFixed(0)
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
                label: val.toFixed(0)
            });
        }

        config = {
            title: "Energy use last " + lm.getDate() + " days (kWh)",
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
