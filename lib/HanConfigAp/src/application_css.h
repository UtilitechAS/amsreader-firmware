const char APPLICATION_CSS[] PROGMEM = R"=="==(
.bg-purple {
    background-color: var(--purple);
}


.GaugeMeter {
    position: Relative;
    text-align: Center;
    overflow: Hidden;
    cursor: Default;
    display: inline-block;
}

.GaugeMeter SPAN, .GaugeMeter B {
    width: 54%;
    position: Absolute;
    text-align: Center;
    display: Inline-Block;
    color: RGBa(0,0,0,.8);
    font-weight: 100;
    font-family: "Open Sans", Arial;
    overflow: Hidden;
    white-space: NoWrap;
    text-overflow: Ellipsis;
    margin: 0 23%;
}

.GaugeMeter[data-style="Semi"] B {
    width: 80%;
    margin: 0 10%;
}

.GaugeMeter S, .GaugeMeter U {
    text-decoration: None;
    font-size: .60em;
    font-weight: 200;
    opacity: .6;
}

.GaugeMeter B {
    color: #000;
    font-weight: 200;
    opacity: .8;
}
)=="==";
