<script>
    import BarChart from './BarChart.svelte';
    import { fmtnum } from './Helpers.js';

    export let title;
    export let unit = "";
    export let l1 = false;
    export let l2 = false;
    export let l2x = false;
    export let l3 = false;
    export let l1i = 0;
    export let l2i = 0;
    export let l3i = 0;
    export let l1e = 0;
    export let l2e = 0;
    export let l3e = 0;
    export let maxImport = 0;
    export let maxExport = 0;
    export let importColorFn;
    export let exportColorFn;

    let config = {};

    function point(p,q) {
        return {
            label: fmtnum(p > 900 ? p/1000.0 : p) + (p > 900 ? "k" : "") + unit,
            title: p.toFixed(2) + ' ' + unit,
            value: isNaN(p) ? 0 : p, 
            color: importColorFn(p ? (p)/(maxImport)*100 : 0, document.documentElement.classList.contains('dark')),
            
            label2: fmtnum(q > 900 ? q/1000.0 : q) + (q > 900 ? "k" : "") + unit,
            title2: q.toFixed(2) + ' ' + unit,
            value2: isNaN(q) ? 0 : q, 
            color2: exportColorFn(q ? (q)/(maxExport)*100 : 0),
        };
    };

    $: {
        let xTicks = [];
        let points = [];
        if(l1) {
            xTicks.push({ label: 'L1' });
            points.push(point(l1i, l1e));
        }
        if(l2) {
            if(l2x) {
                xTicks.push({ label: 'L2' });
                points.push({
                    label: 'Not available',
                    labelAngle: -90,
                    title: 'L2 current is not reported by your meter',
                    value: 0,
                    color: '#7c3aedcc' 
                });
            } else {
                xTicks.push({ label: 'L2' });
                points.push(point(l2i, l2e));
            }
        }
        if(l3) {
            xTicks.push({ label: 'L3' });
            points.push(point(l3i, l3e));
        }

        let ticks = [];
        if(maxExport) {
            ticks.push({ value: -maxExport, label: '-100%' });
            ticks.push({ value: (-maxExport/4)*3, label: '-75%' });
            ticks.push({ value: -maxExport/2, label: '-50%' });
            ticks.push({ value: -maxExport/4, label: '-25%' });
        }
        ticks.push({ value: 0, label: '0%' });
        if(maxImport) {
            ticks.push({ value: maxImport/4, label: '25%' });
            ticks.push({ value: maxImport/2, label: '50%' });
            ticks.push({ value: (maxImport/4)*3, label: '75%' });
            ticks.push({ value: maxImport, label: '100%' });
        }


        config = {
            title: title,
            padding: { top: 20, right: 15, bottom: 20, left: 35 },
            y: {
                min: -maxExport,
                max: maxImport,
                ticks: ticks
            },
            x: {
                ticks: xTicks
            },
            points: points
        };
    };
</script>
<BarChart config={config} />
