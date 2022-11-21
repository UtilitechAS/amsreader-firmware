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
        let imp = 0;
        let exp = 0;
        let h = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];
        let cur = new Date();
        let lm = new Date();
        lm.setDate(0);

        for(i = cur.getDate(); i<=lm.getDate(); i++) {
            imp = json["i"+zeropad(i)];
            exp = json["e"+zeropad(i)];
            val = imp-exp;
            if(!val) val = 0;

            xTicks.push({
                label: zeropad(i)
            });
            points.push({
                label: val.toFixed(0), 
                value: val, 
                color: '#7c3aed' 
            });
            min = Math.min(min, val);
            max = Math.max(max, val);
        }
        for(i = 1; i < cur.getDate(); i++) {
            imp = json["i"+zeropad(i)];
            exp = json["e"+zeropad(i)];
            val = imp-exp;
            if(!val) val = 0;

            xTicks.push({
                label: zeropad(i)
            });
            points.push({
                label: val.toFixed(0), 
                value: val, 
                color: '#7c3aed' 
            });
            min = Math.min(min, val);
            max = Math.max(max, val);
        }

        max = Math.ceil(max/10)*10;
        min = Math.floor(min/10)*10;
        let range = max;
        if(min < 0) range += Math.abs(min);
        let yTickDist = range/4;
        for(i = 0; i < 5; i++) {
            val = min + (yTickDist*i);
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
