<script>
    import BarChart from './BarChart.svelte';
    import { ampcol, fmtnum } from './Helpers.js';

    export let u1;
    export let u2;
    export let u3;
    export let i1;
    export let i2;
    export let i3;
    export let max;

    let config = {};

    function point(v) {
        return {
            label: fmtnum(v) +'A', 
            value: isNaN(v) ? 0 : v, 
            color: ampcol(v ? (v)/(max)*100 : 0) 
        };
    };

    $: {
        let xTicks = [];
        let points = [];
        if(u1 > 0) {
            xTicks.push({ label: 'L1' });
            points.push(point(i1));
        }
        if(u2 > 0) {
            xTicks.push({ label: 'L2' });
            points.push(point(i2));
        }
        if(u3 > 0) {
            xTicks.push({ label: 'L3' });
            points.push(point(i3));
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
