<script>
    import BarChart from './BarChart.svelte';
    import { fmtnum, voltcol } from './Helpers.js';

    export let u1;
    export let u2;
    export let u3;
    export let ds;

    let config = {};

    function point(v) {
        return {
            label: fmtnum(v) + 'V', 
            value: isNaN(v) ? 0 : v, 
            color: voltcol(v ? v : 0) 
        };
    };

    $: {
        let xTicks = [];
        let points = [];
        if(u1 > 0) {
            xTicks.push({ label: ds === 1 ? 'L1-L2' : 'L1' });
            points.push(point(u1));
        }
        if(u2 > 0) {
            xTicks.push({ label: ds === 1 ? 'L1-L3' : 'L2' });
            points.push(point(u2));
        }
        if(u3 > 0) {
            xTicks.push({ label: ds === 1 ? 'L2-L3' : 'L3' });
            points.push(point(u3));
        }
        config = {
            padding: { top: 20, right: 15, bottom: 20, left: 35 },
            y: {
                min: 200,
                max: 260,
                ticks: [
                    { value: 207, label: '-10%' },
                    { value: 230, label: '230v' },
                    { value: 253, label: '+10%' }
                ]
            },
            x: {
                ticks: xTicks
            },
            points: points
        };
    }
</script>
<BarChart config={config} />
