var nextVersion;
var im, em, vm, am;
$(function() {
    im = $("#im");
    if(im && im.gaugeMeter) {
        im.gaugeMeter({
            percent: 0,
            text: "-",
            append: "W"
        });
    }
    
    em = $("#em");
    if(em && em.gaugeMeter) {
        em.gaugeMeter({
            percent: 0,
            text: "-",
            append: "W"
        });
    }
    
    vm = $("#vm");
    if(vm && vm.gaugeMeter) {
        vm.gaugeMeter({
            percent: 0,
            text: "-",
            append: "V"
        });
    }
    
    am = $("#am");
    if(am && am.gaugeMeter) {
        am.gaugeMeter({
            percent: 0,
            text: "-",
            append: "A"
        });
    }

    var meters = $('.SimpleMeter');

    if(meters.length > 0) {
        fetch();
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

    // For meter
    $('.sd').on('change', function() {
        if(($('#mt').val() == 2 || $('#mt').val() == 3) && $('#d').val() == 1) {
            $('#ss').show();
        } else {
            $('#ss').hide();
        }
    });

    $('#mt').on('change', function() {
        if($('#mt').val() == 4) {
            $('.enc').show();
        } else {
            $('.enc').hide();
        }
    });

    $('#mt').trigger('change');

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
        case '/reset':
            $('#system-link').addClass('active');
            break;
    }

    // Check for software upgrade
    var swv = $('#swVersion')
    if(meters.length > 0 && swv.length == 1 && swv.text() != "SNAPSHOT") {
        var v = swv.text().substring(1).split('.');
        var v_major = parseInt(v[0]);
        var v_minor = parseInt(v[1]);
        var v_patch = parseInt(v[2]);
        $.ajax({
            url: swv.data('url'),
            dataType: 'json'
        }).done(function(releases) {
            releases.reverse();
            var me;
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
            if(nextVersion) {
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

var setStatus = function(id, status) {
    var item = $('#'+id);
    item.removeClass('d-none');
    item.removeClass (function (index, className) {
        return (className.match (/(^|\s)badge-\S+/g) || []).join(' ');
    });
    item.addClass('badge badge-' + status);
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
            vm.show();
            am.show();
        }

        for(var id in json) {
            var str = json[id];
            if(typeof str === "object") {
                continue;
            }
            if(isNaN(str)) {
                $('#'+id).html(str);
            } else {
                var num = parseFloat(str);
                $('#'+id).html(num.toFixed(num < 0 ? 0 : num < 10 ? 2  : 1));
            }
        }

        if(window.moment) {
            $('.cs').html(moment.duration(parseInt(json.uptime_seconds), 'seconds').humanize());
            $('.cs').closest('.row').show();
        }

        if(json.status) {
            for(var id in json.status) {
                setStatus(id, json.status[id]);
            }
        }

        if(json.mqtt) {
            $('.me').addClass('d-none');
            $('.me'+json.mqtt.lastError).removeClass('d-none');
            $('#ml').html(json.mqtt.lastError);
        }

        if(json.wifi) {
            for(var id in json.wifi) {
                var str = json.wifi[id];
                dst = $('#'+id);
                if(isNaN(str)) {
                    dst.html(str);
                } else {
                    var num = parseFloat(str);
                    dst.html(num.toFixed(0));
                    $('#'+id+'-row').show();
                }
            }
        }

        if(json.data) {
            var p = 0;
            var p_pct = parseInt(json.p_pct);
            var p_append = "W";
            if(json.data.P) {
                p = parseFloat(json.data.P);
                if(p > 1000) {
                    p = (p/1000).toFixed(1);
                    p_append = "kW";
                }
            }
            if(im && im.gaugeMeter) {
                im.gaugeMeter({
                    percent: p_pct,
                    text: p,
                    append: p_append
                });
            }

            var po = 0;
            var po_pct = parseInt(json.po_pct);
            var po_append = "W";
            if(json.data.PO) {
                po = parseFloat(json.data.PO);
                if(po > 1000) {
                    po = (po/1000).toFixed(1);
                    po_append = "kW";
                }
            }
            if(em && em.gaugeMeter) {
                em.gaugeMeter({
                    percent: po_pct,
                    text: po,
                    append: po_append
                });
            }

            var v = parseFloat(json.v);
            if(v > 0) {
                var v_pct = parseInt(json.v_pct);

                if(vm && vm.gaugeMeter) {
                    vm.gaugeMeter({
                        percent: v_pct,
                        text: v.toFixed(1)
                    });
                }
            }

            var a = parseFloat(json.a);
            if(a > 0) {
                var a_pct = parseInt(json.a_pct);

                if(am && am.gaugeMeter) {
                    am.gaugeMeter({
                        percent: a_pct,
                        text: a.toFixed(1)
                    });
                }
            }

            for(var id in json.data) {
                var str = json.data[id];
                if(isNaN(str)) {
                    $('.'+id).html(str);
                } else {
                    var num = parseFloat(str);
                    $('.'+id).html(num.toFixed(1));
                    $('#'+id+'-row').show();
                    $('.'+id+'-row').show();
                }
            }

            var temp = parseInt(json.temp);
            if(temp == -127) {
                $('.temp').html("N/A");
            }
        } else {
            if(im && im.gaugeMeter) {
                im.gaugeMeter({
                    percent: 0,
                    text: "-",
                    append: "W"
                });
            }

            if(em && em.gaugeMeter) {
                em.gaugeMeter({
                    percent: 0,
                    text: "-",
                    append: "W"
                });
            }

            if(vm && vm.gaugeMeter) {
                vm.gaugeMeter({
                    percent: 0,
                    text: "-"
                });
            }

            if(am && am.gaugeMeter) {
                am.gaugeMeter({
                    percent: 0,
                    text: "-"
                });
            }
        }
        setTimeout(fetch, interval);
    }).fail(function() {
        setTimeout(fetch, interval*4);

        setStatus("mqtt", "secondary");
        setStatus("wifi", "secondary");
        setStatus("han", "secondary");
        setStatus("esp", "danger");
    });
}

var upgrade = function() {
    if(nextVersion) {
        if(confirm("Are you sure you want to perform upgrade to " + nextVersion.tag_name + "?")) {
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