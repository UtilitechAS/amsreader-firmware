export function voltcol(pct) {
    if(pct > 85) return '#d90000';
    else if(pct > 75) return'#e32100';
    else if(pct > 70) return '#ffb800';
    else if(pct > 65) return '#dcd800';
    else if(pct > 35) return '#32d900';
    else if(pct > 25) return '#dcd800';
    else if(pct > 20) return '#ffb800';
    else if(pct > 15) return'#e32100';
    else return '#d90000';
};

export function ampcol(pct) {
    if(pct > 90) return '#d90000';
    else if(pct > 85) return'#e32100';
    else if(pct > 80) return '#ffb800';
    else if(pct > 75) return '#dcd800';
    else return '#32d900';
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
            return "Landis";
        case 10:
            return "Sagemcom";
        default:
            return "";
    }
}

export function zeropad(num) {
    num = num.toString();
    while (num.length < 2) num = "0" + num;
    return num;
}
