var im, em;
$(function() {
    im = $("#importMeter");
    if(im && im.gaugeMeter) {
        im.gaugeMeter({
            percent: 0,
            text: "-",
            append: "W"
        });
    }
    
    em = $("#exportMeter");
    if(em && em.gaugeMeter) {
        em.gaugeMeter({
            percent: 0,
            text: "-",
            append: "W"
        });
    }

    if($('.SimpleMeter').length > 0) {
        fetch();
    }

    // For config-mqtt
    $('#mqttEnable').on('change', function() {
        var inputs = $('.mqtt-config');
        inputs.prop('disabled', !$(this).is(':checked'));
    });
    
    $('#mqttPayloadFormat').on('change', function() {
        var val = parseInt($(this).val());
        if(val == 3) {
            $('.format-type-domoticz').show();
        } else {
            $('.format-type-domoticz').hide();
        }
    });

    $('#mqttEnable').trigger('change');
    $('#mqttPayloadFormat').trigger('change');

    // For config-meter
    $('.subtitute-dependent').on('change', function() {
        console.log("test");
        if($('#meterType').val() == 2 && $('#distributionSystem').val() == 1) {
            $('#substitute').show();
        } else {
            $('#substitute').hide();
        }
    });
    $('#meterType').trigger('change');

    // For config-wifi
    $('#wifiIpType').on('change', function() {
        if($(this).is(':checked')) {
            $('#staticIp').show();
        } else {
            $('#staticIp').hide();
        }
    });
    $('#wifiIpType').trigger('change');

    // For config-web
    $('#authSecurity').on('change', function() {
        var inputs = $('.auth-config');
        inputs.prop('disabled', $(this).val() == 0);
    });
    
    $('#authSecurity').trigger('change');

    // For file upload
    $('#fileUploadField').on('change',function(){
        var fileName = $(this).val();
        $(this).next('.custom-file-label').html(fileName);
    })

    switch(window.location.pathname) {
        case '/config-meter':
            $('#config-meter-link').addClass('active');
            break;
        case '/config-wifi':
            $('#config-wifi-link').addClass('active');
            break;
        case '/config-mqtt':
        case '/mqtt-ca':
        case '/mqtt-cert':
        case '/mqtt-key':
        case '/config-domoticz':
            $('#config-mqtt-link').addClass('active');
            break;
        case '/config-web':
            $('#config-web-link').addClass('active');
            break;
        case '/config-system':
        case '/firmware':
        case '/reset':
            $('#config-system-link').addClass('active');
            break;
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
        if(im && em) {
            $(".SimpleMeter").hide();
            im.show();
            em.show();
        }

        for(var id in json) {
            var str = json[id];
            if(typeof str === "object")
                continue;
            if(isNaN(str)) {
                $('#'+id).html(str);
            } else {
                var num = parseFloat(str);
                $('#'+id).html(num.toFixed(num < 0 ? 0 : num < 10 ? 2  : 1));
            }
        }

        if(window.moment) {
            $('#currentMillis').html(moment.duration(parseInt(json.uptime_seconds), 'seconds').humanize());
            $('#currentMillis').closest('.row').show();
        }

        if(json.status) {
            for(var id in json.status) {
                setStatus(id, json.status[id]);
            }
        }

        if(json.mqtt) {
            $('.mqtt-error').addClass('d-none');
            $('.mqtt-error'+json.mqtt.lastError).removeClass('d-none');
            $('#mqtt-lastError').html(json.mqtt.lastError);
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

            for(var id in json.data) {
                var str = json.data[id];
                if(isNaN(str)) {
                    $('#'+id).html(str);
                } else {
                    var num = parseFloat(str);
                    $('#'+id).html(num.toFixed(1));
                    $('#'+id+'-row').show();
                }
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
        }
        setTimeout(fetch, interval);
    }).fail(function() {
        setTimeout(fetch, interval*4);

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
        setStatus("mqtt", "secondary");
        setStatus("wifi", "secondary");
        setStatus("han", "secondary");
        setStatus("esp", "danger");
    });
}
