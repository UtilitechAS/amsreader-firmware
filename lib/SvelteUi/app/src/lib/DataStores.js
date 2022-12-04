import { readable, writable } from 'svelte/store';

async function fetchWithTimeout(resource, options = {}) {
    const { timeout = 8000 } = options;
    
    const controller = new AbortController();
    const id = setTimeout(() => controller.abort(), timeout);
    const response = await fetch(resource, {
      ...options,
      signal: controller.signal  
    });
    clearTimeout(id);
    return response;
  }

let sysinfo = {
    version: '',
    chip: '',
    vndcfg: null,
    usrcfg: null,
    fwconsent: null,
    booting: false,
    upgrading: false
};
export const sysinfoStore = writable(sysinfo);
export async function getSysinfo() {
    const response = await fetchWithTimeout("/sysinfo.json");
    sysinfo = (await response.json())
    sysinfoStore.set(sysinfo);
};

let tries = 0;
let lastTemp = -127;
let lastPrice = null;
let data = {};
export const dataStore = readable(data, (set) => { 
    let timeout;
    async function getData() {
        fetchWithTimeout("/data.json")
            .then((res) => res.json())
            .then((data) => {
                set(data);
                if(lastTemp != data.t) {
                    lastTemp = data.t;
                    getTemperatures();
                }
                if(lastPrice != data.p) {
                    lastPrice = data.p;
                    getPrices();
                }
                if(sysinfo.upgrading) {
                    window.location.reload();
                } else if(sysinfo.booting) {
                    getSysinfo();
                }
                timeout = setTimeout(getData, 5000);
                tries = 0;
            })
            .catch((err) => {
                tries++;
                if(tries > 3) {
                    set({
                        em: 3,
                        hm: 0,
                        wm: 0,
                        mm: 0
                    });
                    timeout = setTimeout(getData, 15000);
                } else {
                    timeout = setTimeout(getData, 5000);
                }
            });
    }
    getData();
    return function stop() {
        clearTimeout(timeout);
    }
});

let prices = {};
export const pricesStore = writable(prices);
export async function getPrices(){
    const response = await fetchWithTimeout("/energyprice.json");
    prices = (await response.json())
    pricesStore.set(prices);
}

let dayPlot = {};
export const dayPlotStore = readable(dayPlot, (set) => { 
    async function getDayPlot(){
        const response = await fetchWithTimeout("/dayplot.json");
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
        const response = await fetchWithTimeout("/monthplot.json");
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
    const response = await fetchWithTimeout("/temperature.json");
    temperatures = (await response.json())
    temperaturesStore.set(temperatures);
}

export const temperaturesStore = writable(temperatures, (set) => { 
    getTemperatures();
    return function stop() {}
});

let tariff = {};
export async function getTariff() {
    const response = await fetchWithTimeout("/tariff.json");
    tariff = (await response.json())
    tariffStore.set(tariff);
    let date = new Date();
    setTimeout(getTariff, (61-date.getMinutes())*60000)
}

export const tariffStore = writable(tariff, (set) => { 
    return function stop() {}
});

let releases = [];
export const gitHubReleaseStore = writable(releases);

export async function getGitHubReleases() {
    const response = await fetchWithTimeout("https://api.github.com/repos/gskjold/AmsToMqttBridge/releases");
    releases = (await response.json())
    gitHubReleaseStore.set(releases);
};
