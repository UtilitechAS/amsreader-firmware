export function bcol(num) {
    return num === 1 ? 'green' : num === 2 ? 'yellow' : num === 3 ? 'red' : 'gray';
}

export function voltcol(volt, dark) {
    if(volt > 218 && volt < 242) return '#23ac05';
    if(volt > 212 && volt < 248) return '#b1d900';
    if(volt > 208 && volt < 252) return '#a3b200';
    return '#b20000';
};

export function ampcol(pct, dark) {
    let col;
    if(pct > 90) col = '#b20000';
    else if(pct > 85) col = '#b19601';
    else if(pct > 80) col = '#a3b200';
    else if(pct > 75) col = '#569f12';
    else col = '#23ac05';
    return col;
};

export function exportcol(pct) {
    if(pct > 75) return '#23ac05';
    else if(pct > 50) return '#77d900';
    else if(pct > 25) return '#94d900';
    else return '#569f12';
};

export function metertype(mt) {
    switch(mt) {
        case 1:
            return "Aidon";
        case 2:
            return "Kaifa";
        case 3:
            return "Kamstrup";
        case 8:
            return "Iskra";
        case 9:
            return "Landis+Gyr";
        case 10:
            return "Sagemcom";
        default:
            return "Unknown";
    }
}

export function zeropad(num) {
    num = num.toString();
    while (num.length < 2) num = "0" + num;
    return num;
}

export function wifiStateFromRssi(rawValue) {
    const rssi = typeof rawValue === 'string' ? Number(rawValue) : rawValue;
    if(typeof rssi !== 'number' || Number.isNaN(rssi)) {
        return { level: 'off', label: 'Wi-Fi offline', rssi: rawValue };
    }

    if(rssi >= -50) {
        return { level: 'high', label: `Wi-Fi strong (${rssi} dBm)`, rssi };
    }
    if(rssi >= -60) {
        return { level: 'medium', label: `Wi-Fi medium (${rssi} dBm)`, rssi };
    }
    if(rssi >= -75) {
        return { level: 'low', label: `Wi-Fi weak (${rssi} dBm)`, rssi };
    }
    return { level: 'off', label: `Wi-Fi very weak/offline (${rssi} dBm)`, rssi };
}

export function boardtype(c, b) {
    switch(b) {
        case 5:
            switch(c) {
                case 'esp8266':
                    return "Pow-K (GPIO12)";
                case 'esp32s2':
                    return "Pow-K+";
            }
        case 7:
            switch(c) {
                case 'esp8266':
                    return "Pow-U (GPIO12)";
                case 'esp32s2':
                    return "Pow-U+";
            }
        case 6:
            return "Pow-P1";
        case 51:
            return "Wemos S2 mini";
        case 50:
            return "Generic ESP32-S2";
        case 201:
            return "Wemos LOLIN D32";
        case 202:
            return "Adafruit HUZZAH32";
        case 203:
            return "DevKitC";
        case 241:
            return "LilyGO T-ETH-POE";
        case 242:
            return "M5 PoESP32";
        case 243:
            return "WT32-ETH01";
        case 245:
            return "wESP32";
        case 200:
            return "Generic ESP32";
        case 2:
            return "HAN Reader 2.0 by Max Spencer";
        case 0:
            return "Custom hardware by Roar Fredriksen";
        case 1:
            return "Kamstrup module by Egil Opsahl";
        case 8:
            return "µHAN mosquito by dbeinder"
        case 3:
            return "Pow-K (UART0)";
        case 4:
            return "Pow-U (UART0)";
        case 101:
            return "Wemos D1 mini";
        case 100:
            return "Generic ESP8266";
        case 70:
            return "Generic ESP32-C3";
        case 71:
            return "ESP32-C3-DevKitM-1";
        case 80:
            return "Generic ESP32-S3";
    }
    return "Unknown";
}

export function isBusPowered(boardType) {
    switch(boardType) {
        case 2:
        case 4:
        case 7:
            return true;
    }
    return false;
}

export function uiVisibility(choice, state) {
    return choice == 1 || (choice == 2 && state);
}

export function wiki(page) {
    return "https://github.com/UtilitechAS/amsreader-firmware/wiki/" + page;
}

export function fmtnum(v,d) {
    if(v == null || isNaN(v)) return '-';
    if(isNaN(d))
        d = v < 1.0 ? 2 : v < 10.0 ? 1 : 0;
    return v.toFixed(d);
}

export function addHours(date, hours) {
    date.setTime(date.getTime() + hours * 3600000);
    return date;
}

export function getPriceSourceName(code) {
    if(code == "EOE") return "ENTSO-E";
    if(code == "HKS") return "hvakosterstrommen.no";
    if(code == "EDS") return "Energi Data Service";
    if(code == "MIX") return "Mixed sources";
    return "Unknown (" + code + ")";
}

export function getPriceSourceUrl(code) {
    if(code == "EOE") return "https://transparency.entsoe.eu/";
    if(code == "HKS") return "https://www.hvakosterstrommen.no/";
    if(code == "EDS") return "https://www.energidataservice.dk/";
    return "#";
}

let tries = 0; 
export function scanForDevice(sysinfo, updateFn) {
    tries++;

    const targets = buildScanTargets(sysinfo);
    if(!targets.length) {
        if(updateFn) updateFn("");
        setTimeout(scanForDevice, 1500, sysinfo, updateFn);
        return;
    }

    const target = targets[(tries - 1) % targets.length];
    if(!target) {
        setTimeout(scanForDevice, 1000, sysinfo, updateFn);
        return;
    }

    const url = normalizeTarget(target);
    if(console) console.log("Trying url " + url);
    if(updateFn) updateFn(url);

    const retry = function() {
        setTimeout(scanForDevice, 1000, sysinfo, updateFn);
    };

    const xhr = new XMLHttpRequest();
    xhr.timeout = 5000;
    xhr.addEventListener('abort', retry);
    xhr.addEventListener('error', retry);
    xhr.addEventListener('timeout', retry);
    xhr.addEventListener('load', function() {
        window.location.href = url ? url : "/";
    });
    const healthUrl = url.replace(/\/$/, '') + "/is-alive";
    xhr.open("GET", healthUrl, true);
    xhr.send();
};

function buildScanTargets(sysinfo = {}) {
    const manualTargets = Array.isArray(sysinfo.targets) ? sysinfo.targets : [];
    const fallbackTargets = [];

    if(sysinfo.net && sysinfo.net.ip) {
        fallbackTargets.push(sysinfo.net.ip);
    }
    if(sysinfo.hostname) {
        fallbackTargets.push(sysinfo.hostname);
        const looksLikeHost = sysinfo.hostname.indexOf('.') === -1 && sysinfo.hostname.indexOf(':') === -1;
        if(looksLikeHost) {
            fallbackTargets.push(`${sysinfo.hostname}.local`);
        }
    }

    const candidates = [...manualTargets, ...fallbackTargets];
    const deduped = [];
    for(const value of candidates) {
        if(!value) continue;
        const trimmed = value.toString().trim();
        if(!trimmed) continue;
        if(!deduped.includes(trimmed)) deduped.push(trimmed);
    }
    return deduped;
}

function normalizeTarget(target) {
    if(!target) return "";
    const trimmed = target.toString().trim();
    if(trimmed.startsWith("http://") || trimmed.startsWith("https://")) {
        return trimmed;
    }
    return `http://${trimmed}`;
}

export function capitalize(s) {
    return s.charAt(0).toUpperCase() + s.slice(1);
}

export function getBaseChip(chip) {
    if(chip.startsWith("esp32")) return "esp32";
    return chip;
}

export function formatUnit(val, unit) {
    let ret = [val, unit];
    if(typeof val === 'undefined') {
        ret[0] = "-";
        ret[1] = unit;
    } else if(val >= 1_000_000_000) { // Over 1000 M
        ret[0] = (val / 1_000_000).toFixed(val > 10_000_000_000 ? 0 : 1);
        ret[1] = "M"+unit;
    } else if(val > 10_000) { // Over 10 k
        ret[0] = (val / 1000).toFixed(val > 1_000_000 ? 0 : val > 100_000 ? 1 : 2);
        ret[1] = "k"+unit;
    } else {
        ret[0] = val.toFixed(0);
        ret[1] = unit;
    }
    return ret;
}

export function formatCurrency(val, currency) {
    let ret = [fmtnum(val, 2), currency];
    if(typeof val === 'undefined') {
        ret[0] = "-";
        ret[1] = currency;
    } else if(Math.abs(val * 100) < 100) {
        ret[0] = fmtnum(val * 100.0, 2);
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
        ret[1] = currency;
    }
    return ret;
}

export let ipPattern = "((25[0-5]|(2[0-4]|1\\d|[1-9]|)\\d)\\.?\\b){4}";
export let asciiPattern = "[\\x20-\\x7E]+";
export let asciiPatternExt = "[\\x20-\\xFF]+";
export let charAndNumPattern = "[A-Za-z0-9_\\-]+";
export let hexPattern = "[0-9A-Fa-f]+";
export let numPattern = "[0-9]+";

