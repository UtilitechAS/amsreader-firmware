export let monthnames = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];

export function bcol(num) {
    return num === 1 ? 'green' : num === 2 ? 'yellow' : num === 3 ? 'red' : 'gray';
}

export function voltcol(volt) {
    if(volt > 218 && volt < 242) return '#32d900';
    if(volt > 212 && volt < 248) return '#b1d900';
    if(volt > 208 && volt < 252) return '#ffb800';
    return '#d90000';
};

export function ampcol(pct) {
    let col;
    if(pct > 90) col = '#d90000';
    else if(pct > 85) col = '#e32100';
    else if(pct > 80) col = '#ffb800';
    else if(pct > 75) col = '#dcd800';
    else col = '#32d900';

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
    }
}

export function hanError(err) {
    switch(err) {
        case -1: return "Parse error";
        case -2: return "Incomplete data received";
        case -3: return "Payload boundry flag missing";
        case -4: return "Header checksum error";
        case -5: return "Footer checksum error";
        case -9: return "Unknown data received, check meter config";
        case -41: return "Frame length not equal";
        case -51: return "Authentication failed";
        case -52: return "Decryption failed";
        case -53: return "Encryption key invalid";
        case 90: return "No HAN data received for at least 30s";
        case 91: return "Serial break";
        case 92: return "Serial buffer full";
        case 93: return "Serial FIFO overflow";
        case 94: return "Serial frame error";
        case 95: return "Serial parity error";
        case 96: return "RX error";
        case 98: return "Exception in code, debugging necessary";
        case 99: return "Autodetection failed";
    }
    if(err < 0) return "Unspecified error "+err;
    return "";
}

export function mqttError(err) {
    switch(err) {
        case -3: return "Connection failed";
        case -4: return "Network timeout";
        case -10: return "Connection denied";
        case -11: return "Failed to subscribe";
        case -13: return "Connection lost";
    }

    if(err < 0) return "Unspecified error "+err;
    return "";
}

export function priceError(err) {
    switch(err) {
        case 400:
            return "Unrecognized data in request";
        case 401:
        case 403:
            return "Unauthorized, check API key";
        case 404:
            return "Price unavailable, not found";
        case 425:
            return "Server says its too early";
        case 429:
            return "Exceeded API rate limit";
        case 500:
            return "Internal server error";
        case -1: return "Connection error";
        case -2: return "Incomplete data received";
        case -3: return "Invalid data, tag missing";
        case -51: return "Authentication failed";
        case -52: return "Decryption failed";
        case -53: return "Encryption key invalid";
    }

    if(err < 0) return "Unspecified error "+err;
    return "";
}

export function httpError(err) {
    switch(err) {
        case 255: return "Unable to start upgrade";
        case -1: return "Connection refused";
        case -2: return "Failed to send headers";
        case -3: return "Failed to send payload";
        case -4: return "Not connected";
        case -5: return "Connection lost";
        case -6: return "No stream";
        case -7: return "Not a HTTP server";
        case -8: return "Not enough memory";
        case -9: return "Encoding error";
        case -10: return "Stream write";
        case -11: return "Read timeout";
    }
    return "Unknown " + err;
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
        d = v < 10 ? 1 : 0;
    return v.toFixed(d);
}

export function addHours(date, hours) {
    date.setTime(date.getTime() + hours * 3600000);
    return date;
}

export function getResetReason(sysinfo) {
    if(sysinfo.chip == 'esp8266') {
        switch (sysinfo.boot_reason) {
            case 0: return "Normal";
            case 1: return "WDT reset";
            case 2: return "Exception reset";
            case 3: return "Soft WDT reset";
            case 4: return "Software restart";
            case 5: return "Deep sleep";
            case 6: return "External reset";
            default: return "Unknown (8266)";
        }
    } else {
        switch (sysinfo.boot_reason) {
            case 1 : return "Vbat power on reset";
            case 3 : return "Software reset";
            case 4 : return "WDT reset";
            case 5 : return "Deep sleep";
            case 6 : return "SLC reset";
            case 7 : return "Timer Group0 WDT reset";
            case 8 : return "Timer Group1 WDT reset";
            case 9 : return "RTC WDT reset";
            case 10: return "Instrusion test reset CPU";
            case 11: return "Time Group reset CPU";
            case 12: return "Software reset CPU";
            case 13: return "RTC WTD reset CPU";
            case 14: return "PRO CPU";
            case 15: return "Brownout";
            case 16: return "RTC reset";
            default: return "Unknown";
        }
    }
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
