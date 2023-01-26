<script>
    import BarChart from './BarChart.svelte';
    import { voltcol } from './Helpers.js';

    export let u1;
    export let u2;
    export let u3;
    export let ds;

    let min = 200;
    let max = 260;
    let config = {};

    $: {
        let xTicks = [];
        let points = [];
        if(u1 > 0) {
            xTicks.push({ label: ds === 1 ? 'L1-L2' : 'L1' });
            points.push({
                label: u1 ? u1.toFixed(0) + 'V' : '-', 
                value: u1 ? u1 : 0, 
                color: voltcol(u1 ? u1 : 0) 
            });
        }
        if(u2 > 0) {
            xTicks.push({ label: ds === 1 ? 'L1-L3' : 'L2' });
            points.push({ 
                label: u2 ? u2.toFixed(0) + 'V' : '-', 
                value: u2 ? u2 : 0, 
                color: voltcol(u2 ? u2 : 0) 
            });
        }
        if(u3 > 0) {
            xTicks.push({ label: ds === 1 ? 'L2-L3' : 'L3' });
            points.push({ 
                label: u3 ? u3.toFixed(0) + 'V' : '-', 
                value: u3 ? u3 : 0, 
                color: voltcol(u3 ? u3 : 0) 
            });
        }
        config = {
            padding: { top: 20, right: 15, bottom: 20, left: 35 },
            y: {
                min: min,
                max: max,
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
