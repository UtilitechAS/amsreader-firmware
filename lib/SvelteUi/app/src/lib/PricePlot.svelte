<script>
    import { zeropad, addHours, getPriceSourceName, getPriceSourceUrl } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let json;
    export let sysinfo;

    let config = {};
    let max;
    let min;

    $: {
        let currency = json.currency;
        let hour = new Date().getUTCHours();
        let i = 0;
        let val = 0;
        let h = 0;
        let yTicks = [];
        let xTicks = [];
        let values = [];
        min = max = 0;
        let cur = new Date();
        addHours(cur, sysinfo.clock_offset - ((24 + cur.getHours() - cur.getUTCHours())%24));
        for(i = hour; i<24; i++) {
            val = json[zeropad(h++)];
            if(val == null) break;
            xTicks.push({
                label: zeropad(cur.getHours())
            });
            values.push(val*100);
            min = Math.min(min, val*100);
            max = Math.max(max, val*100);
            addHours(cur, 1);
        };
        for(i = 0; i < 24; i++) {
            val = json[zeropad(h++)];
            if(val == null) break;
            xTicks.push({
                label: zeropad(cur.getHours())
            });
            values.push(val*100);
            min = Math.min(min, val*100);
            max = Math.max(max, val*100);
            addHours(cur, 1);
        };

        if(min > -100 && max < 100) {
            switch(currency) {
                case 'NOK':
                case 'DKK':
                    currency = 'øre';
                    break;
                case 'SEK':
                    currency = 'öre';
                    break;
                case 'EUR':
                    currency = 'cent';
                    break;
                case 'CHF':
                    currency = 'rp.';
                    break;
                default:
                    currency = currency+'/100';
            }
            min *= 100;
            max *= 100;
            for(i = 0; i < values.length; i++) {
                values[i] *= 100;
            }
        }

        let points = [];
        for(i = 0; i < values.length; i++) {
            val = values[i];
            let disp = val * 0.01;
            let d = Math.abs(val) < 1000 ? 2 : 0;
            points.push({
                label: disp >= 0 ? disp.toFixed(d) : '',
                title: disp >= 0 ? disp.toFixed(2) + ' ' + currency : '',
                value: val >= 0 ? Math.abs(val) : 0,
                label2: disp < 0 ? disp.toFixed(d) : '',
                title2: disp < 0 ? disp.toFixed(2) + ' ' + currency : '',
                value2: val < 0 ? Math.abs(val) : 0,
                color: '#7c3aed'
            });
        }
        let range = Math.max(max, Math.abs(min));

        if(min < 0) {
            min = Math.min((range/4)*-1, min);
            let yTicksNum = Math.ceil((Math.abs(min)/range) * 4);
            let yTickDistDown = min/yTicksNum;
            for(i = 1; i < yTicksNum+1; i++) {
                let val = (yTickDistDown*i);
                yTicks.push({
                    value: val,
                    label: (val/100).toFixed(2)
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
                label: (val/100).toFixed(2)
            });
        }

        config = {
            title: "Future energy price (" + currency + ")",
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

<a href="{getPriceSourceUrl(json.source)}" target="_blank" class="text-xs float-right z-40">Provided by: {getPriceSourceName(json.source)}</a>
<BarChart config={config} />
