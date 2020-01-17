;
/*
 * AshAlom Gauge Meter.  Version 2.0.0
 * Copyright AshAlom.com  All rights reserved.
 * https://github.com/AshAlom/GaugeMeter <- Deleted!
 * https://github.com/githubsrinath/GaugeMeter <- Backup original.
 *
 * Original created by Dr Ash Alom
 *
 * This is a bug fixed and modified version of the AshAlom Gauge Meter.
 * Copyright 2018 Michael Wolf (Mictronics)
 * https://github.com/mictronics/GaugeMeter
 *
 */
!function ($) {
    $.fn.gaugeMeter = function (t) {
        var defaults = $.extend({
            id: "",
            percent: 0,
            used: null,
            min: null,
            total: null,
            size: 100,
            prepend: "",
            append: "",
            theme: "Red-Gold-Green",
            color: "",
            back: "RGBa(0,0,0,.06)",
            width: 3,
            style: "Full",
            stripe: "0",
            animationstep: 1,
            animate_gauge_colors: false,
            animate_text_colors: false,
            label: "",
            label_color: "Black",
            text: "",
            text_size: 0.22,
            fill: "",
            showvalue: false
        }, t);
        return this.each(function () {

            function getThemeColor(e) {
                var t = "#2C94E0";
                return e || (e = 1e-14),
                    "Red-Gold-Green" === option.theme && (e > 0 && (t = "#d90000"), e > 10 && (t = "#e32100"), e > 20 && (t = "#f35100"), e > 30 && (t = "#ff8700"), e > 40 && (t = "#ffb800"), e > 50 && (t = "#ffd900"), e > 60 && (t = "#dcd800"), e > 70 && (t = "#a6d900"), e > 80 && (t = "#69d900"), e > 90 && (t = "#32d900")),
                    "Green-Gold-Red" === option.theme && (e > 0 && (t = "#32d900"), e > 10 && (t = "#69d900"), e > 20 && (t = "#a6d900"), e > 30 && (t = "#dcd800"), e > 40 && (t = "#ffd900"), e > 50 && (t = "#ffb800"), e > 60 && (t = "#ff8700"), e > 70 && (t = "#f35100"), e > 80 && (t = "#e32100"), e > 90 && (t = "#d90000")),
                    "Green-Red" === option.theme && (e > 0 && (t = "#32d900"), e > 10 && (t = "#41c900"), e > 20 && (t = "#56b300"), e > 30 && (t = "#6f9900"), e > 40 && (t = "#8a7b00"), e > 50 && (t = "#a75e00"), e > 60 && (t = "#c24000"), e > 70 && (t = "#db2600"), e > 80 && (t = "#f01000"), e > 90 && (t = "#ff0000")),
                    "Red-Green" === option.theme && (e > 0 && (t = "#ff0000"), e > 10 && (t = "#f01000"), e > 20 && (t = "#db2600"), e > 30 && (t = "#c24000"), e > 40 && (t = "#a75e00"), e > 50 && (t = "#8a7b00"), e > 60 && (t = "#6f9900"), e > 70 && (t = "#56b300"), e > 80 && (t = "#41c900"), e > 90 && (t = "#32d900")),
                    "DarkBlue-LightBlue" === option.theme && (e > 0 && (t = "#2c94e0"), e > 10 && (t = "#2b96e1"), e > 20 && (t = "#2b99e4"), e > 30 && (t = "#2a9ce7"), e > 40 && (t = "#28a0e9"), e > 50 && (t = "#26a4ed"), e > 60 && (t = "#25a8f0"), e > 70 && (t = "#24acf3"), e > 80 && (t = "#23aff5"), e > 90 && (t = "#21b2f7")),
                    "LightBlue-DarkBlue" === option.theme && (e > 0 && (t = "#21b2f7"), e > 10 && (t = "#23aff5"), e > 20 && (t = "#24acf3"), e > 30 && (t = "#25a8f0"), e > 40 && (t = "#26a4ed"), e > 50 && (t = "#28a0e9"), e > 60 && (t = "#2a9ce7"), e > 70 && (t = "#2b99e4"), e > 80 && (t = "#2b96e1"), e > 90 && (t = "#2c94e0")),
                    "DarkRed-LightRed" === option.theme && (e > 0 && (t = "#d90000"), e > 10 && (t = "#dc0000"), e > 20 && (t = "#e00000"), e > 30 && (t = "#e40000"), e > 40 && (t = "#ea0000"), e > 50 && (t = "#ee0000"), e > 60 && (t = "#f30000"), e > 70 && (t = "#f90000"), e > 80 && (t = "#fc0000"), e > 90 && (t = "#ff0000")),
                    "LightRed-DarkRed" === option.theme && (e > 0 && (t = "#ff0000"), e > 10 && (t = "#fc0000"), e > 20 && (t = "#f90000"), e > 30 && (t = "#f30000"), e > 40 && (t = "#ee0000"), e > 50 && (t = "#ea0000"), e > 60 && (t = "#e40000"), e > 70 && (t = "#e00000"), e > 80 && (t = "#dc0000"), e > 90 && (t = "#d90000")),
                    "DarkGreen-LightGreen" === option.theme && (e > 0 && (t = "#32d900"), e > 10 && (t = "#33db00"), e > 20 && (t = "#34df00"), e > 30 && (t = "#34e200"), e > 40 && (t = "#36e700"), e > 50 && (t = "#37ec00"), e > 60 && (t = "#38f100"), e > 70 && (t = "#38f600"), e > 80 && (t = "#39f900"), e > 90 && (t = "#3afc00")),
                    "LightGreen-DarkGreen" === option.theme && (e > 0 && (t = "#3afc00"), e > 10 && (t = "#39f900"), e > 20 && (t = "#38f600"), e > 30 && (t = "#38f100"), e > 40 && (t = "#37ec00"), e > 50 && (t = "#36e700"), e > 60 && (t = "#34e200"), e > 70 && (t = "#34df00"), e > 80 && (t = "#33db00"), e > 90 && (t = "#32d900")),
                    "DarkGold-LightGold" === option.theme && (e > 0 && (t = "#ffb800"), e > 10 && (t = "#ffba00"), e > 20 && (t = "#ffbd00"), e > 30 && (t = "#ffc200"), e > 40 && (t = "#ffc600"), e > 50 && (t = "#ffcb00"), e > 60 && (t = "#ffcf00"), e > 70 && (t = "#ffd400"), e > 80 && (t = "#ffd600"), e > 90 && (t = "#ffd900")),
                    "LightGold-DarkGold" === option.theme && (e > 0 && (t = "#ffd900"), e > 10 && (t = "#ffd600"), e > 20 && (t = "#ffd400"), e > 30 && (t = "#ffcf00"), e > 40 && (t = "#ffcb00"), e > 50 && (t = "#ffc600"), e > 60 && (t = "#ffc200"), e > 70 && (t = "#ffbd00"), e > 80 && (t = "#ffba00"), e > 90 && (t = "#ffb800")),
                    "White" === option.theme && (t = "#fff"),
                    "Black" === option.theme && (t = "#000"),
                    t;
            }
            /* The label below gauge. */
            function createLabel(t, a) {
                if(t.children("b").length === 0){
                    $("<b></b>").appendTo(t).html(option.label).css({
                        "line-height": option.size + 5 * a + "px",
                        color: option.label_color
                    });
                }
            }
            /* Prepend and append text, the gauge text or percentage value. */
            function createSpanTag(t) {
                var fgcolor = "";
                if (option.animate_text_colors === true){
                    fgcolor = option.fgcolor;
                }
                var child = t.children("span");
                if(child.length !== 0){
                    child.html(r).css({color: fgcolor});
                    return;
                }
                if(option.text_size <= 0.0 || Number.isNaN(option.text_size)){
                    option.text_size = 0.22;
                }
                if(option.text_size > 0.5){
                    option.text_size = 0.5;
                }
                $("<span></span>").appendTo(t).html(r).css({
                    "line-height": option.size + "px",
                    "font-size": option.text_size * option.size + "px",
                    color: fgcolor
                });
            }
            /* Get data attributes as options from div tag. Fall back to defaults when not exists. */
            function getDataAttr(t) {
                $.each(dataAttr, function (index, element) {
                    if(t.data(element) !== undefined && t.data(element) !== null){
                        option[element] = t.data(element);
                    } else {
                        option[element] = $(defaults).attr(element);
                    }

                    if(element === "fill"){
                        s = option[element];
                    }

                    if((element === "size" ||
                        element === "width" ||
                        element === "animationstep" ||
                        element === "stripe"
                        ) && !Number.isInteger(option[element])){
                        option[element] = parseInt(option[element]);
                    }

                    if(element === "text_size"){
                        option[element] = parseFloat(option[element]);
                    }
                });
            }
            /* Draws the gauge. */
            function drawGauge(a) {
		if(M < 0) M = 0;
                if(M > 100) M = 100;
                var lw = option.width < 1 || isNaN(option.width) ? option.size / 20 : option.width;
                g.clearRect(0, 0, b.width, b.height);
                g.beginPath();
                g.arc(m, v, x, G, k, !1);
                if(s){
                    g.fillStyle = option.fill;
                    g.fill();
                }
                g.lineWidth = lw;
                g.strokeStyle = option.back;
                option.stripe > parseInt(0) ? g.setLineDash([option.stripe], 1) : g.lineCap = "round";
                g.stroke();
                g.beginPath();
                g.arc(m, v, x, -I, P * a - I, !1);
                g.lineWidth = lw;
                g.strokeStyle = option.fgcolor;
                g.stroke();
                c > M && (M += z, requestAnimationFrame(function(){
                    drawGauge(Math.min(M, c) / 100);
                }, p));
            }

            $(this).attr("data-id", $(this).attr("id"));
            var r,
                dataAttr = ["percent",
                    "used",
                    "min",
                    "total",
                    "size",
                    "prepend",
                    "append",
                    "theme",
                    "color",
                    "back",
                    "width",
                    "style",
                    "stripe",
                    "animationstep",
                    "animate_gauge_colors",
                    "animate_text_colors",
                    "label",
                    "label_color",
                    "text",
                    "text_size",
                    "fill",
                    "showvalue"],
                option = {},
                c = 0,
                p = $(this),
                s = false;
            p.addClass("gaugeMeter");
            getDataAttr(p);

            if(Number.isInteger(option.used) && Number.isInteger(option.total)){
                var u = option.used;
                var t = option.total;
                if(Number.isInteger(option.min)) {
                    if(option.min < 0) {
                        t -= option.min;
                        u -= option.min;
                    }
                }
                c = u / (t / 100);
            } else {
                if(Number.isInteger(option.percent)){
                    c = option.percent;
                } else {
                    c = parseInt(defaults.percent);
                }
            }
            if(c < 0) c = 0;
            if(c > 100) c = 100;

            if( option.text !== "" && option.text !== null && option.text !== undefined){
                if(option.append !== "" && option.append !== null && option.append !== undefined){
                    r = option.text + "<u>" + option.append + "</u>";
                } else {
                    r = option.text;
                }
                if(option.prepend !== "" && option.prepend !== null && option.prepend !== undefined){
                    r = "<s>" + option.prepend + "</s>" + r;
                }
            } else {
                if(defaults.showvalue === true || option.showvalue === true){
                    r = option.used;
                } else {
                    r = c.toString();
                }
                if(option.prepend !== "" && option.prepend !== null && option.prepend !== undefined){
                    r = "<s>" + option.prepend + "</s>" + r;
                }

                if(option.append !== "" && option.append !== null && option.append !== undefined){
                    r = r + "<u>" + option.append + "</u>";
                }
            }

            option.fgcolor = getThemeColor(c);
            if(option.color !== "" && option.color !== null && option.color !== undefined){
                option.fgcolor = option.color;
            }

            if(option.animate_gauge_colors === true){
                option.fgcolor = getThemeColor(c);
            }
            createSpanTag(p);

            if(option.style !== "" && option.style !== null && option.style !== undefined){
                createLabel(p, option.size / 13);
            }

            $(this).width(option.size + "px");

            var b = $("<canvas></canvas>").attr({width: option.size, height: option.size}).get(0),
                    g = b.getContext("2d"),
                    m = b.width / 2,
                    v = b.height / 2,
                    _ = 360 * option.percent,
                    x = (_ * (Math.PI / 180), b.width / 2.5),
                    k = 2.3 * Math.PI,
                    G = 0,
                    M = 0 === option.animationstep ? c : 0,
                    z = Math.max(option.animationstep, 0),
                    P = 2 * Math.PI,
                    I = Math.PI / 2,
                    R = option.style;
            var child = $(this).children("canvas");
            if(child.length !== 0){
                /* Replace existing canvas when new percentage was written. */
                child.replaceWith(b);
            } else {
                /* Initially create canvas. */
                $(b).appendTo($(this));
            }

            if ("Semi" === R){
                k = 2 * Math.PI;
                G = 3.13;
                P = 1 * Math.PI;
                I = Math.PI / .996;
            }
            if ("Arch" === R){
                k = 2.195 * Math.PI;
                G = 1, G = 655.99999;
                P = 1.4 * Math.PI;
                I = Math.PI / .8335;
            }
            drawGauge(M / 100);
        });
    };
}
(jQuery);
