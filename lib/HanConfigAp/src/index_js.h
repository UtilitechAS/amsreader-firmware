const char INDEX_JS[] PROGMEM = R"=="==(
$(function() {
    $(".GaugeMeter").gaugeMeter();

    $('.update').on('click', function() {
        var el = $(".GaugeMeter");
        el.data('percent', 75);
        el.data('text', '33.8');
        el.gaugeMeter();
        console.log(el);
    });
});
)=="==";
