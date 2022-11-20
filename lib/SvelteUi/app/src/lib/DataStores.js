import { readable, writable } from 'svelte/store';


let sysinfo = {
    version: '',
    chip: '',
    vndcfg: null,
    usrcfg: null,
    fwconsent: null,
    booting: true,
    upgrading: false
};
export const sysinfoStore = writable(sysinfo);

export async function getSysinfo() {
    const response = await fetch("/sysinfo.json");
    sysinfo = (await response.json())
    sysinfoStore.set(sysinfo);
};

let lastTemp = -127;
let data = {};
export const dataStore = readable(data, (set) => { 
    let timeout;
    async function getData() {
        fetch("/data.json")
            .then((res) => res.json())
            .then((data) => {
                set(data);
                if(lastTemp != data.t) {
                    lastTemp = data.t;
                    getTemperatures();
                }
                if(sysinfo.upgrading) {
                    window.location.reload();
                } else if(sysinfo.booting) {
                    getSysinfo();
                }
                timeout = setTimeout(getData, 5000);
            })
            .catch((err) => {
                data.em = 3;
                data.hm = 0;
                data.wm = 0;
                data.mm = 0;
                set(data);
                timeout = setTimeout(getData, 15000);
            });
    }
    getData();
    return function stop() {
        clearTimeout(timeout);
    }
});

let prices = {};
export const pricesStore = readable(prices, (set) => { 
    async function getPrices(){
        const response = await fetch("/energyprice.json");
        prices = (await response.json())
        set(prices);

        let date = new Date();
        setTimeout(getPrices, (61-date.getMinutes())*60000)
    }
    getPrices();
    return function stop() {}
});

let dayPlot = {};
export const dayPlotStore = readable(dayPlot, (set) => { 
    async function getDayPlot(){
        const response = await fetch("/dayplot.json");
        dayPlot = (await response.json())
        set(dayPlot);

        let date = new Date();
        setTimeout(getDayPlot, (61-date.getMinutes())*60000)
    }
    getDayPlot();
    return function stop() {}
});

let monthPlot = {};
export const monthPlotStore = readable(monthPlot, (set) => { 
    async function getmonthPlot(){
        const response = await fetch("/monthplot.json");
        monthPlot = (await response.json())
        set(monthPlot);

        let date = new Date();
        setTimeout(getmonthPlot, (24-date.getHours())*3600000)
    }
    getmonthPlot();
    return function stop() {}
});

let temperatures = {};
export async function getTemperatures() {
    const response = await fetch("/temperature.json");
    temperatures = (await response.json())
    temperaturesStore.set(temperatures);
}

export const temperaturesStore = writable(temperatures, (set) => { 
    getTemperatures();
    return function stop() {}
});

let releases = [];
export const gitHubReleaseStore = writable(releases);

export async function getGitHubReleases() {
    const response = await fetch("https://api.github.com/repos/gskjold/AmsToMqttBridge/releases");
    releases = (await response.json())
    gitHubReleaseStore.set(releases);
};
