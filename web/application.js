var nextVersion;
var im, em;

// Price plot
var pp;
var pa;
var po = {
    title: 'Future energy price',
    titleTextStyle: {
        fontSize: 14
    },
    bar: { groupWidth: '90%' },
    legend: { position: 'none' },
    vAxis: {
        viewWindowMode: 'maximized'
    },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
};
var pl = null; // Last price

// Day plot
var ep;
var ea;
var eo = {
    title: 'Energy use last 24 hours',
    titleTextStyle: {
        fontSize: 14
    },
    bar: { groupWidth: '90%' },
    legend: { position: 'none' },
    vAxis: {
        title: 'kWh',
        viewWindowMode: 'maximized'
    },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
};

// Month plot
var mp;
var ma;
var mo = {
    title: 'Energy use last month',
    titleTextStyle: {
        fontSize: 14
    },
    bar: { groupWidth: '90%' },
    legend: { position: 'none' },
    vAxis: {
        title: 'kWh',
        viewWindowMode: 'maximized'
    },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
};

// Voltage plot
var vp;
var va;
var vo = {
    title: 'Phase voltage',
    titleTextStyle: {
        fontSize: 14
    },
    bar: { groupWidth: '90%' },
    vAxis: {
        minValue: 200,
        maxValue: 260,
        ticks: [
            { v: 207, f: '-10%'},
            { v: 230, f: '230V'},
            { v: 253, f: '+10%'}
        ]
    },
    legend: { position: 'none' },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
};

// Amperage plot
var ap;
var aa;
var ao = {
    title: 'Phase current',
    titleTextStyle: {
        fontSize: 14
    },
    bar: { groupWidth: '90%' },
    vAxis: {
        minValue: 0,
        maxValue: 100,
        ticks: [
            { v: 25, f: '25%'},
            { v: 50, f: '50%'},
            { v: 75, f: '75%'},
            { v: 100, f: '100%'}
        ]
    },
    legend: { position: 'none' },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
};

// Import plot
var ip;
var ia;
var io = {
    legend: 'none',
    pieHole: 0.6,
    pieSliceText: 'none',
    pieStartAngle: 216,
    slices: {
        0: { color: 'green' },
        1: { color: '#eee' },
        2: { color: 'transparent' }
    },
    legend: { position: 'none' },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
    chartArea: {
        left: 0,
        top: 0,
        width: '100%',
        height: '100%'
    }
};

// Export plot
var xp;
var xa;
var xo = {
    legend: 'none',
    pieHole: 0.6,
    pieSliceText: 'none',
    pieStartAngle: 216,
    slices: {
        0: { color: 'green' },
        1: { color: '#eee' },
        2: { color: 'transparent' }
    },
    legend: { position: 'none' },
    tooltip: { trigger: 'none'},
    enableInteractivity: false,
    chartArea: {
        left: 0,
        top: 0,
        width: '100%',
        height: '100%'
    }
};

$(function() {
    var meters = $('.plot1');

    if(meters.length > 0) {
        // Chart
        google.charts.load('current', {'packages':['corechart']});
        google.charts.setOnLoadCallback(setupChart);
    }

    // For mqtt
    $('#m').on('change', function() {
        var inputs = $('.mc');
        inputs.prop('disabled', !$(this).is(':checked'));
    });
    
    $('#f').on('change', function() {
        var val = parseInt($(this).val());
        if(val == 3) {
            $('.f3-s').show();
        } else {
            $('.f3-s').hide();
        }
    });

    $('#s').on('change', function() {
        var port = $('#p');
        if($(this).is(':checked')) {
            if(port.val() == 1883) {
                port.val(8883);
            }
        } else {
            if(port.val() == 8883) {
                port.val(1883);
            }
        }
    });

    $('#m').trigger('change');
    $('#f').trigger('change');

    // For wifi
    $('#st').on('change', function() {
        if($(this).is(':checked')) {
            $('#i').show();
        } else {
            $('#i').hide();
        }
    });
    $('#st').trigger('change');

    // For web
    $('#as').on('change', function() {
        var inputs = $('.ac');
        inputs.prop('disabled', $(this).val() == 0);
    });
    
    $('#as').trigger('change');

    // For file upload
    $('#fileUploadField').on('change',function(){
        var fileName = $(this).val();
        $(this).next('.custom-file-label').html(fileName);
    })
    $('.upload-form').on('submit', function(i, form) {
        $('#loading-indicator').show();
    });

    // For NTP
    $('#n').on('change', function() {
        var inputs = $('.nc');
        inputs.prop('disabled', !$(this).is(':checked'));
    });
    $('#n').trigger('change');

    // Navbar
    switch(window.location.pathname) {
        case '/temperature':
            $('#temp-link').addClass('active');
            break;
        case '/price':
            $('#price-link').addClass('active');
            break;
        case '/meter':
        case '/wifi':
        case '/mqtt':
        case '/mqtt-ca':
        case '/mqtt-cert':
        case '/mqtt-key':
        case '/domoticz':
        case '/web':
        case '/ntp':
        case '/entsoe':
            $('#config-link').addClass('active');
            break;
        case '/gpio':
        case '/debugging':
        case '/firmware':
            $('#firmware-warn').show();
        case '/reset':
            $('#system-link').addClass('active');
            break;
    }

    // Check for software upgrade
    var swv = $('#swVersion');
    var fwl = $('#fwLink');
    if((meters.length > 0 || fwl.length > 0) && swv.length == 1) {
        var v = swv.text().substring(1).split('.');
        var v_major = parseInt(v[0]);
        var v_minor = parseInt(v[1]);
        var v_patch = parseInt(v[2]);
        $.ajax({
            url: swv.data('url'),
            dataType: 'json'
        }).done(function(releases) {
            if(/^v\d{1,2}\.\d{1,2}\.\d{1,2}$/.test(swv.text()) && fwl.length == 0) {
                releases.reverse();
                var next_patch;
                var next_minor;
                var next_major;
                $.each(releases, function(i, release) {
                    var ver2 = release.tag_name;
                    var v2 = ver2.substring(1).split('.');
                    var v2_major = parseInt(v2[0]);
                    var v2_minor = parseInt(v2[1]);
                    var v2_patch = parseInt(v2[2]);

                    if(v2_major == v_major) {
                        if(v2_minor == v_minor) {
                            if(v2_patch > v_patch) {
                                next_patch = release;
                            }
                        } else if(v2_minor == v_minor+1) {
                            next_minor = release;
                        }
                    } else if(v2_major == v_major+1) {
                        if(next_major) {
                            var mv = next_major.tag_name.substring(1).split('.');
                            var mv_major = parseInt(mv[0]);
                            var mv_minor = parseInt(mv[1]);
                            var mv_patch = parseInt(mv[2]);
                            if(v2_minor == mv_minor) {
                                next_major = release;
                            }
                        } else {
                            next_major = release;
                        }
                    }
                });
                if(next_minor) {
                    nextVersion = next_minor;
                } else if(next_major) {
                    nextVersion = next_major;
                } else if(next_patch) {
                    nextVersion = next_patch;
                }
            } else {
                nextVersion = releases[0];
            }
            if(nextVersion) {
                if(fwl.length > 0) {
                    var chipset = fwl.data('chipset').toLowerCase();
                    $.each(releases, function(i, release) {
                        if(release.tag_name == nextVersion.tag_name) {
                            $.each(release.assets, function(i, asset) {
                                if(asset.name.includes(chipset) && !asset.name.includes("partitions")) {
                                    fwl.prop('href', asset.browser_download_url);
                                    $('#fwDownload').show();
                                    return false;
                                }
                            });
                        }
                    });
                };
                $('#newVersionTag').text(nextVersion.tag_name);
                $('#newVersionUrl').prop('href', nextVersion.html_url);
                $('#newVersion').removeClass('d-none');
            }
        });
    }

    // Temperature
    var tt = $('#temp-template');
    if(tt.length > 0) {
        setTimeout(loadTempSensors, 500);
    }
});

var resizeTO;
$( window ).resize(function() {
    if(resizeTO) clearTimeout(resizeTO);
    resizeTO = setTimeout(function() {
        $(this).trigger('resizeEnd');
    }, 250);
});

$(window).on('resizeEnd', function() {
    redraw();
});

var zeropad = function(num) {
    num = num.toString();
    while (num.length < 2) num = "0" + num;
    return num;
}

var setupChart = function() {
    pp = new google.visualization.ColumnChart(document.getElementById('pp'));
    ep = new google.visualization.ColumnChart(document.getElementById('ep'));
    mp = new google.visualization.ColumnChart(document.getElementById('mp'));
    vp = new google.visualization.ColumnChart(document.getElementById('vp'));
    ap = new google.visualization.ColumnChart(document.getElementById('ap'));
    ip = new google.visualization.PieChart(document.getElementById('ip'));
    xp = new google.visualization.PieChart(document.getElementById('xp'));
    fetch();
    drawDay();
    drawMonth();
};

var redraw = function() {
    if(pl != null) {
        pp.draw(pa, po);
    }
    ep.draw(ea, eo);
    mp.draw(ma, mo);
    vp.draw(va, vo);
    ap.draw(aa, ao);
    ip.draw(ia, io);
    xp.draw(xa, xo);
};

var drawPrices = function() {
    $('#ppc').show();
    $.ajax({
        url: '/energyprice.json',
        timeout: 30000,
        dataType: 'json',
    }).done(function(json) {
        data = [['Hour',json.currency + '/kWh', { role: 'style' }, { role: 'annotation' }]];
        var r = 1;
        var hour = moment.utc().hours();
        var offset = moment().utcOffset()/60;
        var min = 0;
        var h  = 0;
        var d = json["20"] == null ? 2 : 1;
        for(var i = hour; i<24; i++) {
            var val = json[zeropad(h++)];
            if(val == null) break;
            data[r++] = [zeropad((i+offset)%24), val, "color: #6f42c1;opacity: 0.9;", val == null ? "" : val.toFixed(d)];
            Math.min(0, val);
        };
        for(var i = 0; i < 24; i++) {
            var val = json[zeropad(h++)];
            if(val == null) break;
            data[r++] = [zeropad((i+offset)%24), val, "color: #6f42c1;opacity: 0.9;", val == null ? "" : val.toFixed(d)];
            Math.min(0, val);
        };
        pa = google.visualization.arrayToDataTable(data);
        po.vAxis.title = json.currency;
        if(min == 0)
            po.vAxis.minValue = 0;
        pp.draw(pa, po);
    });
}

var drawDay = function() {
    $.ajax({
        url: '/dayplot.json',
        timeout: 30000,
        dataType: 'json',
    }).done(function(json) {
        data = [['Hour','kWh', { role: 'style' }, { role: 'annotation' }]];
        var r = 1;
        var hour = moment.utc().hours();
        var offset = moment().utcOffset()/60;
        var min = 0;
        for(var i = hour; i<24; i++) {
            var val = json["h"+zeropad(i)];
            data[r++] = [zeropad((i+offset)%24), val, "color: #6f42c1;opacity: 0.9;", val.toFixed(1)];
            Math.min(0, val);
        };
        for(var i = 0; i < hour; i++) {
            var val = json["h"+zeropad(i)];
            data[r++] = [zeropad((i+offset)%24), val, "color: #6f42c1;opacity: 0.9;", val.toFixed(1)];
            Math.min(0, val);
        };
        ea = google.visualization.arrayToDataTable(data);
        if(min == 0)
            eo.vAxis.minValue = 0;
        ep.draw(ea, eo);

        setTimeout(drawDay, (61-moment().minute())*60000);
    });
};

var drawMonth = function() {
    $.ajax({
        url: '/monthplot.json',
        timeout: 30000,
        dataType: 'json',
    }).done(function(json) {
        data = [['Day','kWh', { role: 'style' }, { role: 'annotation' }]];
        var r = 1;
        var day = moment().date();
        var eom = moment().subtract(1, 'months').endOf('month').date();
        var min = 0;
        for(var i = day; i<=eom; i++) {
            var val = json["d"+zeropad(i)];
            data[r++] = [zeropad((i)), val, "color: #6f42c1;opacity: 0.9;", val.toFixed(0)];
            Math.min(0, val);
        }
        for(var i = 1; i < day; i++) {
            var val = json["d"+zeropad(i)];
            data[r++] = [zeropad((i)), val, "color: #6f42c1;opacity: 0.9;", val.toFixed(0)];
            Math.min(0, val);
        }
        ma = google.visualization.arrayToDataTable(data);
        if(min == 0)
            mo.vAxis.minValue = 0;
        mp.draw(ma, mo);

        setTimeout(drawMonth, (24-moment().hour())*60000);
    });
};

var setStatus = function(id, sid) {
    var item = $('#'+id);
    item.removeClass('d-none');
    item.removeClass (function (index, className) {
        return (className.match (/(^|\s)badge-\S+/g) || []).join(' ');
    });
    var status;
    if(sid == 0) {
        status = "secondary";
    } else if(sid == 1) {
        status = "success";
    } else if(sid == 2) {
        status = "warning";
    } else {
        status = "danger";
    }
    item.addClass('badge badge-' + status);
};

var voltcol = function(pct) {
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

var ampcol = function(pct) {
    if(pct > 85) return '#d90000';
    else if(pct > 75) return'#e32100';
    else if(pct > 70) return '#ffb800';
    else if(pct > 65) return '#dcd800';
    else return '#32d900';
};

var interval = 5000;
var fetch = function() {
    $.ajax({
        url: '/data.json',
        timeout: 10000,
        dataType: 'json',
    }).done(function(json) {
        if(im) {
            $(".SimpleMeter").hide();
            im.show();
            em.show();

        }

        for(var id in json) {
            var str = json[id];
            if(typeof str === "object") {
                continue;
            }
            if(isNaN(str)) {
                $('.j'+id).html(str);
            } else {
                var num = parseFloat(str);
                $('.j'+id).html(num.toFixed(num < 0 ? 0 : num < 10 ? 2  : 1));
            }
            $('.r'+id).show();
        }

        if(window.moment) {
            $('.ju').html(moment.duration(parseInt(json.u), 'seconds').humanize());
        }

        var kib = parseInt(json.m)/1000;
        $('.jm').html(kib.toFixed(1));
        if(kib > 32) {
            $('.ssl-capable').removeClass('d-none');
        }

        setStatus("esp", json.em);
        setStatus("han", json.hm);
        setStatus("wifi", json.wm);
        setStatus("mqtt", json.mm);


        if(ip) {
            var v = parseInt(json.i);
            var pct = (v*100)/parseInt(json.im);
            var append = "W";
            if(v > 1000) {
                v = (v/1000).toFixed(1);
                append = "kW";
            }
            $('.ipo').html(v);
            $('.ipoa').html(append);
            var arr = [
                ['Slice', 'Value'],
                ['', (pct*2.88)],
                ['', ((100-pct)*2.88)],
                ['', 72],
            ];
            io.slices[0].color = ampcol(pct);
            ia = google.visualization.arrayToDataTable(arr);
            ip.draw(ia, io);
        }

        if(xp) {
            var v = parseInt(json.e);
            var pct = (v*100)/(parseInt(json.om)*1000);
            var append = "W";
            if(v > 1000) {
                v = (v/1000).toFixed(1);
                append = "kW";
            }
            $('.epo').html(v);
            $('.epoa').html(append);
            var arr = [
                ['Slice', 'Value'],
                ['', (pct*2.88)],
                ['', ((100-pct)*2.88)],
                ['', 72],
            ];
            xo.slices[0].color = ampcol(pct);
            xa = google.visualization.arrayToDataTable(arr);
            xp.draw(xa, xo);
        }

        if(vp) {
            var c = 0;
            var t = 0;
            var r = 1;
            var arr = [['Phase', 'Voltage', { role: 'style' }, { role: 'annotation' }]];
            if(json.u1) {
                var u1 = parseFloat(json.u1);
                t += u1;
                c++;
                var pct = (Math.max(parseFloat(json.u1)-195.5, 1)*100/69);
                arr[r++] = ['L1', u1, "color: " + voltcol(pct) + ";opacity: 0.9;", u1 + "V"];
            }
            if(json.u2) {
                var u2 = parseFloat(json.u2);
                t += u2;
                c++;
                var pct = (Math.max(parseFloat(json.u2)-195.5, 1)*100/69);
                arr[r++] = ['L2', u2, "color: " + voltcol(pct) + ";opacity: 0.9;", u2 + "V"];
            }
            if(json.u3) {
                var u3 = parseFloat(json.u3);
                t += u3;
                c++;
                var pct = (Math.max(parseFloat(json.u3)-195.5, 1)*100/69);
                arr[r++] = ['L3', u3, "color: " + voltcol(pct) + ";opacity: 0.9;", u3 + "V"];
            }
            v = t/c;
            if(v > 0) {
                va = google.visualization.arrayToDataTable(arr);
                vp.draw(va, vo);
            }
        }

        if(ap && json.mf) {
            var a = 0;
            var r = 1;
            var arr = [['Phase', 'Amperage', { role: 'style' }, { role: 'annotation' }]];
            if(json.i1) {
                var i1 = parseFloat(json.i1);
                a = Math.max(a, i1);
                var pct = (parseFloat(json.i1)/parseInt(json.mf))*100;
                arr[r++] = ['L1', pct, "color: " + ampcol(pct) + ";opacity: 0.9;", i1 + "A"];
            }
            if(json.i2) {
                var i2 = parseFloat(json.i2);
                a = Math.max(a, i2);
                var pct = (parseFloat(json.i2)/parseInt(json.mf))*100;
                arr[r++] = ['L2', pct, "color: " + ampcol(pct) + ";opacity: 0.9;", i2 + "A"];
            }
            if(json.i3) {
                var i3 = parseFloat(json.i3);
                a = Math.max(a, i3);
                var pct = (parseFloat(json.i3)/parseInt(json.mf))*100;
                arr[r++] = ['L3', pct, "color: " + ampcol(pct) + ";opacity: 0.9;", i3 + "A"];
            }
            if(a > 0) {
                aa = google.visualization.arrayToDataTable(arr);
                ap.draw(aa, ao);
            }
        }

        if(json.me) {
            $('.me').addClass('d-none');
            $('.me'+json.me).removeClass('d-none');
            $('#ml').html(json.me);
        }

        var temp = parseInt(json.t);
        if(temp == -127) {
            $('.jt').html("N/A");
            $('.rt').hide();
        } else {
            $('.rt').show();
        }

        var vcc = parseFloat(json.v);
        if(vcc > 0.0) {
            $('.rv').show();
        } else {
            $('.rv').hide();
        }

        var unixtime = moment().unix();
        var ts = parseInt(json.c);
        if(Math.abs(unixtime-ts) < 300) {
            $('.jc').html(moment(ts * 1000).format('DD. MMM HH:mm'));
            $('.jc').removeClass('text-danger');
        } else  {
            $('.jc').html(moment(ts * 1000).format('DD.MM.YYYY HH:mm'));
            $('.jc').addClass('text-danger');
        }

        setTimeout(fetch, interval);

        var price = parseFloat(json.p);
        if(price && price != pl) {
            pl = price;
            drawPrices();
        }
    }).fail(function(x, text, error) {
        console.log("Failed request");
        console.log(text);
        console.log(error);
        setTimeout(fetch, interval*4);

        setStatus("mqtt", 0);
        setStatus("wifi", 0);
        setStatus("han", 0);
        setStatus("esp", 3);
    });
}

var upgrade = function() {
    if(nextVersion) {
        if(confirm("Are you sure you want to perform upgrade to " + nextVersion.tag_name + "?")) {
            $('#loading-indicator').show();
            window.location.href="/upgrade?version=" + nextVersion.tag_name;
        }
    }
}

var loadTempSensors = function() {
    $.ajax({
        url: '/temperature.json',
        timeout: 10000,
        dataType: 'json',
    }).done(function(json) {
        if($('#loading').length > 0) {
            $('#loading').hide();

            var list = $('#sensors');
            if(json.c > 0) {
                list.empty();
                var temp = $.trim($('#temp-template').html());
                $.each(json.s, function(i, o) {
                    var item = temp.replace(/{{index}}/ig, o.i);
                    var item = item.replace(/{{address}}/ig, o.a);
                    var item = item.replace(/{{name}}/ig, o.n);
                    var item = item.replace(/{{value}}/ig, o.v > -50 && o.v < 127 ? o.v : "N/A");
                    var item = item.replace(/{{common}}/ig, o.c ? "checked" : "");
                    list.append(item);
                });
            } else {
                $('#notemp').show();
            }
        } else {
            $.each(json.s, function(i, o) {
                $('#temp-'+o.i).html(o.v > -50 && o.v < 127 ? o.v : "N/A");
            });
        }
        setTimeout(loadTempSensors, 10000);
    }).fail(function() {
        setTimeout(loadTempSensors, 60000);
        $('#loading').hide();
        $('#error').show();
    });
}