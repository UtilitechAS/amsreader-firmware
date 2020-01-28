const char INDEX_JS[] PROGMEM = R"=="==(
$(".GaugeMeter").gaugeMeter();

var wait = 500;
var nextrefresh = wait;
var fetch = function() {
    $.ajax({
        url: '/data.json',
        dataType: 'json',
    }).done(function(json) {
        $(".SimpleMeter").hide();
        var el = $(".GaugeMeter");
        el.show();
        var rate = 2500;
        if(json.data) {
            el.data('percent', json.pct);
            if(json.data.P) {
                var num = parseFloat(json.data.P);
                if(num > 1000) {
                    num = num / 1000;
                    el.data('text', num.toFixed(1));
                    el.data('append','kW');
                } else {
                    el.data('text', num);
                    el.data('append','W');
                }
            }
            el.gaugeMeter();

            for(var id in json.data) {
                var str = json.data[id];
                if(isNaN(str)) {
                    $('#'+id).html(str);
                } else {
                    var num = parseFloat(str);
                    $('#'+id).html(num.toFixed(1));
                }
            }

            if(json.data.U1 > 0) {
                $('#P1').show();
            }

            if(json.data.U2 > 0) {
                $('#P2').show();
            }

            if(json.data.U3 > 0) {
                $('#P3').show();
            }

            if(json.meterType == 3) {
                rate = 10000;
            }
            if(json.currentMillis && json.up) {
                nextrefresh = rate - ((json.currentMillis - json.up) % rate) + wait;
            } else {
                nextrefresh = 2500;
            }
        } else {
            el.data('percent', 0);
            el.data('text', '-');
            el.gaugeMeter();
            nextrefresh = 2500;
        }
        if(!nextrefresh || nextrefresh < 500) {
            nextrefresh = 2500;
        }
        setTimeout(fetch, nextrefresh);
    }).fail(function() {
        el.data('percent', 0);
        el.data('text', '-');
        el.gaugeMeter();
        nextrefresh = 10000;
        setTimeout(fetch, nextrefresh);
    });
}
setTimeout(fetch, nextrefresh);

)=="==";
