<script>
    import BarChart from './BarChart.svelte';
    import { ampcol } from './Helpers.js';

    export let u1;
    export let u2;
    export let u3;
    export let i1;
    export let i2;
    export let i3;
    export let max;

    let config = {};

    $: {
        let xTicks = [];
        let points = [];
        if(u1 > 0) {
            xTicks.push({ label: 'L1' });
            points.push({
                label: i1 ? (i1 > 10 ? i1.toFixed(0) : i1.toFixed(1)) + 'A' : '-', 
                value: i1 ? i1 : 0, 
                color: ampcol(i1 ? (i1)/(max)*100 : 0) 
            });
        }
        if(u2 > 0) {
            xTicks.push({ label: 'L2' });
            points.push({
                label: i2 ? (i2 > 10 ? i2.toFixed(0) : i2.toFixed(1)) + 'A' : '-', 
                value: i2 ? i2 : 0, 
                color: ampcol(i2 ? (i2)/(max)*100 : 0) 
            });
        }
        if(u3 > 0) {
            xTicks.push({ label: 'L3' });
            points.push({
                label: i3 ? (i3 > 10 ? i3.toFixed(0) : i3.toFixed(1)) + 'A' : '-', 
                value: i3 ? i3 : 0, 
                color: ampcol(i3 ? (i3)/(max)*100 : 0) 
            });
        }
        config = {
            padding: { top: 20, right: 15, bottom: 20, left: 35 },
            y: {
                min: 0,
                max: max,
                ticks: [
                    { value: 0, label: '0%' },
                    { value: max/4, label: '25%' },
                    { value: max/2, label: '50%' },
                    { value: (max/4)*3, label: '75%' },
                    { value: max, label: '100%' }
                ]
            },
            x: {
                ticks: xTicks
            },
            points: points
        };
    };
</script>
<BarChart config={config} />
