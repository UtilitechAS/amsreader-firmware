import { readable, writable } from 'svelte/store';


let sysinfo = {
    version: '',
    chip: '',
    vndcfg: null,
    usrcfg: null,
    fwconsent: null,
    booting: false
};
export const sysinfoStore = writable(sysinfo);

export async function getSysinfo() {
    const response = await fetch("/sysinfo.json");
    sysinfo = (await response.json())
    sysinfoStore.set(sysinfo);
};

let data = {};
export const dataStore = readable(data, (set) => { 
    async function getData(){
        const response = await fetch("/data.json");
        data = (await response.json())
        set(data);
        if(sysinfo.booting) {
            getSysinfo();
        }
    }
    const interval = setInterval(getData, 5000);
    getData();
    return function stop() {
        clearInterval(interval);
    }
});

let prices = {};
export const pricesStore = readable(prices, (set) => { 
    async function getPrices(){
        const response = await fetch("/energyprice.json");
        prices = (await response.json())
        set(prices);
    }
    const date = new Date();
    const timeout = setTimeout(getPrices, (61-date.getMinutes())*60000)
    getPrices();
    return function stop() {
        clearTimeout(timeout);
    }
});

let dayPlot = {};
export const dayPlotStore = readable(dayPlot, (set) => { 
    async function getDayPlot(){
        const response = await fetch("/dayplot.json");
        dayPlot = (await response.json())
        set(dayPlot);
    }
    const date = new Date();
    const timeout = setTimeout(getDayPlot, (61-date.getMinutes())*60000)
    getDayPlot();
    return function stop() {
        clearTimeout(timeout);
    }
});

let monthPlot = {};
export const monthPlotStore = readable(monthPlot, (set) => { 
    async function getmonthPlot(){
        const response = await fetch("/monthplot.json");
        monthPlot = (await response.json())
        set(monthPlot);
    }
    const date = new Date();
    const timeout = setTimeout(getmonthPlot, (24-date.getHours())*3600000)
    getmonthPlot();
    return function stop() {
        clearTimeout(timeout);
    }
});

let temperatures = {};
export const temperaturesStore = readable(temperatures, (set) => { 
    async function getTemperatures(){
        const response = await fetch("/temperature.json");
        temperatures = (await response.json())
        set(temperatures);
    }
    const interval = setInterval(getTemperatures, 60000);
    getTemperatures();
    return function stop() {
        clearTimeout(interval);
    }
});
