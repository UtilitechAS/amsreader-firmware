import { readable, writable } from 'svelte/store';
import { isBusPowered, zeropad, scanForDevice } from './Helpers';
import fetchWithTimeout from './fetchWithTimeout';

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
export async function getSysinfo() {
    const response = await fetchWithTimeout("sysinfo.json?t=" + Math.floor(Date.now() / 1000));
    sysinfo = (await response.json())
    sysinfoStore.set(sysinfo);
};
export const sysinfoStore = writable(sysinfo, (set) => { 
    getSysinfo();
    return function stop() {}
});

function updateSysinfo(url) {
    sysinfoStore.update(s => {
        s.trying = url;
        return s;
    });
}

let tries = 0;
let lastTemp = -127;
let lastPrice = null;
let lastUp = 0;
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
                if(data.pe && data.p != lastPrice) {
                    lastPrice = data.p;
                    getPrices();
                }
                if(!sysinfo || !sysinfo.chip || sysinfo.booting || (tries > 1 && !isBusPowered(sysinfo.board))) {
                    getSysinfo();
                    if(dayPlotTimeout) clearTimeout(dayPlotTimeout);
                    dayPlotTimeout = setTimeout(getDayPlot, 2000);
                    if(monthPlotTimeout) clearTimeout(monthPlotTimeout);
                    monthPlotTimeout = setTimeout(getMonthPlot, 3000);
                }
                lastUp = data.u;
                if(!dayPlotTimeout) dayPlotTimeout = getDayPlot();
                if(!monthPlotTimeout) monthPlotTimeout = getMonthPlot();
                
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

let priceFetchTimeout;
let importPrices = {};
export const importPricesStore = writable(importPrices);
let exportprices = {};
export const exportPricesStore = writable(exportprices);

export async function getPrices() {
    if(priceFetchTimeout) {
        clearTimeout(priceFetchTimeout);
        priceFetchTimeout = 0;
    }
    {
        const response = await fetchWithTimeout("importprice.json");
        importPrices = (await response.json())
        importPricesStore.set(importPrices);
    }

    if(importPrices?.importExportPriceDifferent) {
        const response = await fetchWithTimeout("exportprice.json");
        exportprices = (await response.json())
        exportPricesStore.set(exportprices);
    }

    let date = new Date();
    priceFetchTimeout = setTimeout(getPrices, ((24-date.getHours())*3600000)+10)
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
    return function stop() {}
});

let temperatures = {};
export async function getTemperatures() {
    const response = await fetchWithTimeout("temperature.json");
    temperatures = (await response.json())
    temperaturesStore.set(temperatures);
}

export const temperaturesStore = writable(temperatures, (set) => { 
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

let networks = {c:-1,n:[]};
export async function getNetworks() {
    const response = await fetchWithTimeout("wifiscan.json", { timeout: 30000 });
    let res = (await response.json())
    let addedSsid = [];
    networks.c = 0;
    networks.n = [];
    for (let i = 0; i < res.n.length; i++) {
        let net = res.n[i];
        if(!addedSsid.includes(net.s)) {
            networks.n[networks.c++] = net;
            addedSsid.push(net.s);
        }
    }
    networksStore.set(networks);
}

export const networksStore = writable(networks, (set) => { 
    getNetworks();
    return function stop() {}
});
