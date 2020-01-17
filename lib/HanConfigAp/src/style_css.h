const char STYLE_CSS[] PROGMEM = R"=="==(
body,div,input {
    font-family: "Roboto", Arial, Lucida Grande;
}
.wrapper {
    width: 500px;
    position: absolute;
    padding: 30px;
    background-color: #FFF;
    border-radius: 1px;
    color: #333;
    border-color: rgba(0, 0, 0, 0.03);
    box-shadow: 0 2px 2px rgba(0, 0, 0, .24), 0 0 2px rgba(0, 0, 0, .12);
    margin-left: 20px;
    margin-top: 20px;
}
div {
    padding-bottom: 5px;
}
label {
    font-family: "Roboto", "Helvetica Neue", sans-serif;
    font-size: 14px;
    line-height: 16px;
    width: 100px;
    display: inline-block;
}
input {
    font-family: "Roboto", "Helvetica Neue", sans-serif;
    font-size: 14px;
    line-height: 16px;
    bottom: 30px;
    border: none;
    border-bottom: 1px solid #d4d4d4;
    padding: 10px;
    background: transparent;
    transition: all .25s ease;
}
input[type=number] {
    width: 70px;
    margin-left: 5px;
}
input:focus {
    outline: none;
    border-bottom: 1px solid #3f51b5;
}
h2 {
    text-align: left;
    font-size: 20px;
    font-weight: bold;
    letter-spacing: 3px;
    line-height: 28px;
}
.submit-button {
    position: absolute;
    text-align: right;
    border-radius: 20px;
    border-bottom-right-radius: 0;
    border-top-right-radius: 0;
    background-color: #3f51b5;
    color: #FFF;
    padding: 12px 25px;
    display: inline-block;
    font-size: 12px;
    font-weight: bold;
    letter-spacing: 2px;
    right: 0px;
    bottom: 10px;
    cursor: pointer;
    transition: all .25s ease;
    box-shadow: 0 2px 2px rgba(0, 0, 0, .24), 0 0 2px rgba(0, 0, 0, .12);
    width: 100px;
}
.select-style {
    border-top: 10px solid white;
    border-bottom: 1px solid #d4d4d4;
    color: #ffffff;
    cursor: pointer;
    display: block;
    font-family: Roboto, "Helvetica Neue", sans-serif;
    font-size: 14px;
    font-weight: 400;
    height: 16px;
    line-height: 14px;
    min-width: 200px;
    padding-bottom: 7px;
    padding-left: 0px;
    padding-right: 0px;
    position: relative;
    text-align: left;
    width: 80%;
    -webkit-box-direction: normal;
    overflow: hidden;
    background: #ffffff url("data:image/png;base64,R0lGODlhDwAUAIABAAAAAP///yH5BAEAAAEALAAAAAAPABQAAAIXjI+py+0Po5wH2HsXzmw//lHiSJZmUAAAOw==") no-repeat 98% 50%;
}
.disabled-option {
    color: #d4d4d4;
}
.select-style select {
    padding: 5px 8px;
    width: 100%;
    border: none;
    box-shadow: none;
    background: transparent;
    background-image: none;
    -webkit-appearance: none;
}
.select-style select:focus {
    outline: none;
    border: none;
}
@media only screen and (max-width: 1000px) {
    .wrapper {
        width: 80%;
    }
}
@media only screen and (max-width: 300px) {
    .wrapper {
        width: 75%;
    }
}
@media only screen and (max-width: 600px) {
    .wrapper {
        width: 80%;
        margin-left: 0px;
        margin-top: 0px;
    }
    .submit-button {
        bottom: 0px;
        width: 70px;
    }
    input {
        width: 100%;
    }
}
)=="==";
