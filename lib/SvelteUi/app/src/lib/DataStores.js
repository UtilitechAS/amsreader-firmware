import { readable, writable } from 'svelte/store';
import { isBusPowered, zeropad, scanForDevice } from './Helpers';

let queueMap = {}
let queue = [];

async function fetchWithTimeoutReal(resource, options = {}) {
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

let eatTimeout;
async function eatQueue() {
    if(queue.length) {
        let item = queue.shift();
        delete queueMap[item.resource];
        try {
            let response = await fetchWithTimeoutReal(item.resource, item.options);
            for(let i in item.callbacks) {
                item.callbacks[i](response.clone());
            }
        } catch(e) {
            console.error("Error calling " + item.resource, e);
            for(let i in item.callbacks) {
                item.callbacks[i]();
            }
        }
    }
    if(eatTimeout) clearTimeout(eatTimeout);
    eatTimeout = setTimeout(eatQueue, 100);
};
eatQueue();

export async function fetchWithTimeout(resource, options = {}) {
    let item;
    if(queueMap[resource]) {
        item = queueMap[resource];
    } else {
        item = {
            resource: resource,
            options: options,
            callbacks: []
        };
        queueMap[resource] = item;
        queue.push(item);
    }
    let promise = new Promise((cb) => item.callbacks.push(cb));
    return promise;
}

let sysinfo = {
    version: '',
    chip: '',
    mac: null,
    apmac: null,
    vndcfg: null,
    usrcfg: null,
    fwconsent: null,
    booting: false,
    upgrading: false,
    ui: {},
    security: 0,
    boot_reason: 0,
    upgrade: {
        x: -1,
        e: 0,
        f: null,
        t: null
    },
    trying: null,
    if: {
        eth: false
    }
};
export const sysinfoStore = writable(sysinfo);
export async function getSysinfo() {
    const response = await fetchWithTimeout("sysinfo.json?t=" + Math.floor(Date.now() / 1000));
    sysinfo = (await response.json())
    sysinfoStore.set(sysinfo);
};

function updateSysinfo(url) {
    sysinfoStore.update(s => {
        s.trying = url;
        return s;
    });
}

let tries = 0;
let lastTemp = -127;
let lastPrice = null;
let data = {};
export const dataStore = readable(data, (set) => { 
    let timeout;
    let scanTimeout;
    async function getData() {
        fetchWithTimeout("data.json")
            .then((res) => res.json())
            .then((data) => {
                set(data);
                if(lastTemp != data.t) {
                    lastTemp = data.t;
                    setTimeout(getTemperatures, 2000);
                }
                if(lastPrice == null && data.pe && data.p != null) {
                    lastPrice = data.p;
                    getPrices();
                }
                if(sysinfo.upgrading) {
                    window.location.reload();
                } else if(!sysinfo || !sysinfo.chip || sysinfo.booting || (tries > 1 && !isBusPowered(sysinfo.board))) {
                    getSysinfo();
                    if(dayPlotTimeout) clearTimeout(dayPlotTimeout);
                    dayPlotTimeout = setTimeout(getDayPlot, 2000);
                    if(monthPlotTimeout) clearTimeout(monthPlotTimeout);
                    monthPlotTimeout = setTimeout(getMonthPlot, 3000);
                }
                let to = 5000;
                if(isBusPowered(sysinfo.board) && data.v > 2.5) {
                    let diff = (3.3 - Math.min(3.3, data.v));
                    if(diff > 0) {
                        to = Math.max(diff, 0.1) * 10 * 5000;
                    }
                }
                if(to > 5000) console.log("Next in " + to + "ms");
                if(timeout) clearTimeout(timeout);
                timeout = setTimeout(getData, to);
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
                    if(!scanTimeout) scanTimeout = setTimeout(scanForDevice, 5000, sysinfo, updateSysinfo);
                } else {
                    timeout = setTimeout(getData, isBusPowered(sysinfo.board) ? 10000 : 5000);
                }
            });
    }
    getData();
    return function stop() {
        clearTimeout(timeout);
    }
});

let prices = {};
let priceShiftTimeout;
export const pricesStore = writable(prices);

export async function shiftPrices() {
    let fetchUpdate = false;
    pricesStore.update(p => {
        for(var i = 0; i < 36; i++) {
            if(p[zeropad(i)] == null) {
                fetchUpdate = i < 12;
                break;
            }
            p[zeropad(i)] = p[zeropad(i+1)];
        }
        return p;
    });
    if(fetchUpdate) {
        getPrices();
    } else {
        let date = new Date();
        priceShiftTimeout = setTimeout(shiftPrices, ((60-date.getMinutes())*60000))
    }
}

export async function getPrices() {
    if(priceShiftTimeout) {
        clearTimeout(priceShiftTimeout);
        priceShiftTimeout = 0;
    }
    const response = await fetchWithTimeout("energyprice.json");
    prices = (await response.json())
    pricesStore.set(prices);

    let date = new Date();
    priceShiftTimeout = setTimeout(shiftPrices, ((60-date.getMinutes())*60000))
}

let dayPlot = {};
let dayPlotTimeout;
export async function getDayPlot() {
    if(dayPlotTimeout) {
        clearTimeout(dayPlotTimeout);
        dayPlotTimeout = 0;
    }
    const response = await fetchWithTimeout("dayplot.json");
    dayPlot = (await response.json())
    dayPlotStore.set(dayPlot);

    let date = new Date();
    dayPlotTimeout = setTimeout(getDayPlot, ((60-date.getMinutes())*60000)+20)
}

export const dayPlotStore = writable(dayPlot, (set) => { 
    getDayPlot();
    return function stop() {}
});

let monthPlot = {};
let monthPlotTimeout;
export async function getMonthPlot() {
    if(monthPlotTimeout) {
        clearTimeout(monthPlotTimeout);
        monthPlotTimeout = 0;
    }
    const response = await fetchWithTimeout("monthplot.json");
    monthPlot = (await response.json())
    monthPlotStore.set(monthPlot);

    let date = new Date();
    monthPlotTimeout = setTimeout(getMonthPlot, ((24-date.getHours())*3600000)+40)
}

export const monthPlotStore = writable(monthPlot, (set) => { 
    getMonthPlot();
    return function stop() {}
});

let temperatures = {};
export async function getTemperatures() {
    const response = await fetchWithTimeout("temperature.json");
    temperatures = (await response.json())
    temperaturesStore.set(temperatures);
}

export const temperaturesStore = writable(temperatures, (set) => { 
    getTemperatures();
    return function stop() {}
});

let tariff = {};
let tariffTimeout;
export async function getTariff() {
    if(tariffTimeout) {
        clearTimeout(tariffTimeout);
        tariffTimeout = 0;
    }
    const response = await fetchWithTimeout("tariff.json");
    tariff = (await response.json())
    tariffStore.set(tariff);
    let date = new Date();
    tariffTimeout = setTimeout(getTariff, ((60-date.getMinutes())*60000)+30)
}

export const tariffStore = writable(tariff, (set) => { 
    return function stop() {}
});

let releases = [];
export const gitHubReleaseStore = writable(releases);

export async function getGitHubReleases() {
    const response = await fetchWithTimeout("https://api.github.com/repos/UtilitechAS/amsreader-firmware/releases");
    releases = (await response.json())
    gitHubReleaseStore.set(releases);
};

let realtimeOffset = 0;
let realtime = { data: [] };
export async function getRealtime() {
    const response = await fetchWithTimeout(realtimeOffset < 0 ? "realtime.json" : "realtime.json?offset="+realtimeOffset);
    let res = (await response.json())
    realtimeStore.update(current => {
        for(let i = 0; i < res.size; i++) {
            current.data[res.offset+i] = res.data[i];
        }
        current.size = current.data.length;
        return current;
    });
    if(realtimeOffset >= 0) {
        realtimeOffset += res.size;
        if(realtimeOffset < res.total) {
            setTimeout(getRealtime, 2000);
        } else {
            realtimeOffset = -1;
        }
    }
}

export function isRealtimeFullyLoaded() {
    return realtimeOffset == -1;
}

export const realtimeStore = writable(realtime);
