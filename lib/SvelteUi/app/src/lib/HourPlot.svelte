<script>
// Generate a plot for one hour duration
// If export is existing it shows up as bars towards "down". Autoscale is done
//
// EHorvat
//
    import { zeropad } from './Helpers.js';
    import BarChart from './BarChart.svelte';

    export let json;

    let config = {};
    let max = 0;
    let min = 0;    
    let no_of_datapoints = 179;   //Number of points-1 in Hour Plot (180 points ...3 per minute)
    let max_exp = 0;
    let min_exp = 0;

    $: {
        let i = 0;        
        let yTicks = [];
        let xTicks = [];
        let points = [];
        let exp = 0;
//        for(i = no_of_datapoints; i>-1; i--) {
        for(i = no_of_datapoints; i>-1; i--) {
            let imp = json["i"+zeropad(i)];     //JSON contains (import-export) Watt reading
            if(imp === undefined) imp = 0;   
            if(imp < 0) {                       //if import = 0...there might be export
                exp = -1 * imp;                 //convert export to  positive number
                imp = 0;
            } else {
                exp = 0;
            }
            if (i == no_of_datapoints) {                      //Set base for min values at start of for loop
                max = Math.ceil(imp/10)*10;
                min = Math.floor(imp/10)*10;
                max_exp = Math.ceil(exp/10)*10;
                min_exp = Math.floor(exp/10)*10;
            }
            if (i%3 == 0) {                     //show x ticks not on every data point
                xTicks.push({
                    label: zeropad(-1*i/3)      //show "-" Minutes
                });
            } else {
                xTicks.push({
                    label: " "
                });  
                }
                points.push({
                    label: imp,
                    title: imp + ' W',
                    value: imp, 
                    label2: exp * -1, 
                    title2: exp * -1+ ' W',
                    value2: exp,
                    color2: '#407038',
                    color: '#7c3aed' 
                }); 
            min = Math.min(min, imp);
            max = Math.max(max, imp);
            min_exp = Math.min(min_exp, exp);
            max_exp = Math.max(max_exp, exp);
        }   // ++++++++++++++++++++++++++++++++++ for loop end    
        if (max_exp > 0) {                         //There is some export....
            max_exp = Math.ceil(max_exp/10)*10;  
            min_exp = Math.floor(min_exp/10)*10;
            if (max > 0) {                         //There is also some import....
                min = 0;                       
                min_exp = 0;                   
            }    
        } else {
            max_exp=0;
            min_exp=0;
        }
        if (max > 0) {min_exp = 0} // If there is some import set scale min for export to 0
//  
        max_exp = max_exp * -1;  //Invert export data ..... now we have negative values for export
        min_exp = min_exp * -1;  //Invert export data ..... now we have negative values for export
//
        if(max_exp < 0) {      //There is some export....
            let yTickDistDown = (max_exp-min_exp)/4;
            if (yTickDistDown > -1) yTickDistDown=-1;
            for(i = 0; i < 5; i++) {
                let val = (yTickDistDown*i) + min_exp;
                yTicks.push({
                    value: val,
                    label: val.toFixed(0)
                });
            }
        }
//     
        max = Math.ceil(max/10)*10;        
        if (min > 0) {
            min = Math.floor(min/10)*10;
        }   
        if (max > 0) {                      //There is some import....
            let yTickDistUp = (max-min)/4;
            if (yTickDistUp < 1) yTickDistUp=1;
            for(i = 0; i < 5; i++) {
             let val = (yTickDistUp*i)+min;
                yTicks.push({
                    value: val,
                    label: val.toFixed(0)
                });
            }
        }
        if(max_exp < 0) {      //There is some export....
            min = max_exp;
            if  (max < 0) {     //There is no import
                max = min_exp; 
                min = max_exp;
            }
        }

        config = {
            title: "Electrical power last hour (Watt)",
//            title: "Elektrische Leistung letzte Stunde (Watt)",  //EHorvat translated: "Electrical power last hour (Watt)
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
