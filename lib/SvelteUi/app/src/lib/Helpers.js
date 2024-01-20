export function bcol(num) {
    return num === 1 ? 'green' : num === 2 ? 'yellow' : num === 3 ? 'red' : 'gray';
}

export function voltcol(volt, dark) {
    if(dark) {
        if(volt > 218 && volt < 242) return '#32c000';
        if(volt > 212 && volt < 248) return '#b1c000';
        if(volt > 208 && volt < 252) return '#ffa000';
        return '#d90000';
    } else {
        if(volt > 218 && volt < 242) return '#32d900';
        if(volt > 212 && volt < 248) return '#b1d900';
        if(volt > 208 && volt < 252) return '#ffb800';
        return '#d90000';
    }
};

export function ampcol(pct, dark) {
    let col;
    if(dark) {
        if(pct > 90) col = '#d90000';
        else if(pct > 85) col = '#e31000';
        else if(pct > 80) col = '#ffa900';
        else if(pct > 75) col = '#dcc300';
        else col = '#32c500';
    } else {
        if(pct > 90) col = '#d90000';
        else if(pct > 85) col = '#e32100';
        else if(pct > 80) col = '#ffb800';
        else if(pct > 75) col = '#dcd800';
        else col = '#32d900';
    }

    return col;
};

export function exportcol(pct) {
    if(pct > 75) return '#32d900';
    else if(pct > 50) return '#77d900';
    else if(pct > 25) return '#94d900';
    else return '#dcd800';
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
    if(isNaN(v)) return '-';
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
    if(code == "EOE") return "https://transparency.entsoe.eu/-E";
    if(code == "HKS") return "https://www.hvakosterstrommen.no/";
    if(code == "EDS") return "https://www.energidataservice.dk/";
    return "#";
}

let tries = 0; 
export function scanForDevice(sysinfo, updateFn) {
    var url = "";
    tries++;

    var retry = function() {
        setTimeout(scanForDevice, 1000, sysinfo, updateFn);
    };

    if(sysinfo.net.ip && tries%3 == 0) {
        if(!sysinfo.net.ip) {
            retry();
            return;
        };
        url = "http://" + sysinfo.net.ip;
    } else if(sysinfo.hostname && tries%3 == 1) {
        url = "http://" + sysinfo.hostname;
    } else if(sysinfo.hostname && tries%3 == 2) {
        url = "http://" + sysinfo.hostname + ".local";
    } else {
        url = "";
    }
    if(console) console.log("Trying url " + url);
    if(updateFn) updateFn(url);
    
    var xhr = new XMLHttpRequest();
    xhr.timeout = 5000;
    xhr.addEventListener('abort', retry);
    xhr.addEventListener('error', retry);
    xhr.addEventListener('timeout', retry);
    xhr.addEventListener('load', function(e) {
        window.location.href = url ? url : "/";
    });
    xhr.open("GET", url + "/is-alive", true);
    xhr.send();
};

export function capitalize(s) {
    return s.charAt(0).toUpperCase() + s.slice(1);
}

export function getBaseChip(chip) {
    if(chip.startsWith("esp32")) return "esp32";
    return chip;
}