<script>
    import { zeropad, addHours, exportcol, niceAxis } from './Helpers.js';
    import BarChart from './BarChart.svelte';
    import { translationsStore } from './TranslationService.js';

    export let title;
    export let json;
    export let sysinfo;

    let translations = {};
    translationsStore.subscribe(update => {
        translations = update;
    });

    let config = {};
    let max;
    let min;

    let dark = document.documentElement.classList.contains('dark');

    $: {
        let i = 0;
        let yTicks = [];
        let xTicks = [];
        let points = [];
        min = max = 0;
        let cur = new Date();
        let clock_adjust = -cur.getTimezoneOffset()/60 - sysinfo.clock_offset;
        let currentHour = cur.getUTCHours();
        addHours(cur, -clock_adjust-24);
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
                color: dark ? '#5c2da5' : '#7c3aed',
                color2: dark ? '#27728e' : '#37829e',
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
                color: dark ? '#5c2da5' : '#7c3aed',
                color2: dark ? '#27728e' : '#37829e',
            });
            min = Math.max(min, exp*10);
            max = Math.max(max, imp*10);
            addHours(cur, 1);
        };

        min *= -1;
        let axis = niceAxis(min / 10, max / 10, 10);
        min = axis.min;
        max = axis.max;
        yTicks = axis.ticks;

        config = {
            title: title,
            dark: document.documentElement.classList.contains('dark'),
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
            points: points,
            link: {
                route: true,
                text: translations.dashboard?.edit_data ?? 'Edit data',
                url: '/edit-day'
            }
        };
    };

</script>

<BarChart config={config} />
