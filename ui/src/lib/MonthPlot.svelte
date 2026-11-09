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
        let lm = new Date();
        lm.setDate(0);
        lm.setHours(12);

        let clock_adjust = -cur.getTimezoneOffset()/60 - sysinfo.clock_offset;
        addHours(cur, -clock_adjust);
        addHours(lm, -clock_adjust);

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
                color: dark ? '#5c2da5' : '#7c3aed',
                color2: dark ? '#27728e' : '#37829e',
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
                color: dark ? '#5c2da5' : '#7c3aed',
                color2: dark ? '#27728e' : '#37829e',
            });
            min = Math.max(min, exp);
            max = Math.max(max, imp);
        }

        min *= -1;
        let axis = niceAxis(min, max, 1);
        min = axis.min;
        max = axis.max;
        yTicks = axis.ticks;

        config = {
            title: title.replace('{0}', lm.getDate().toFixed(0)) + " (kWh)",
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
                url: '/edit-month'
            }
        };
    };

</script>

<BarChart config={config} />
