const char INDEX_HTML[] PROGMEM = R"=="==(
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>AMS reader</title>
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <link rel="stylesheet" type="text/css" href="/css/bootstrap.css"/>
    <link rel="stylesheet" type="text/css" href="/css/application.css"/>
    <script src="/js/jquery.js"></script>
    <script src="/js/gaugemeter.js"></script>
</head>
<body class="bg-light">
<main role="main" class="container">
    <div class="d-flex align-items-center p-3 my-2 text-white-50 bg-purple rounded shadow">
        <div class="lh-100">
            <h6 class="mb-0 text-white lh-100">AMS reader</h6>
            <small>${version}</small>
        </div>
    </div>
    <div class="my-3 p-3 bg-white rounded shadow">
        <h6 class="border-bottom border-gray pb-2 mb-4">Current meter values</h6>
        <div class="row">
            <div class="col-md-4">
                <div class="text-center">
                    <div class="GaugeMeter rounded"
                         data-size="200px"
                         data-text_size="0.11"
                         data-width="25"
                         data-style="Arch"
                         data-theme="Green-Gold-Red"
                         data-animationstep="0"
                         data-animate_gauge_colors="1"

                         data-percent="0"
                         data-text="-"
                         data-label="Consumption"
                         data-append="W"
                    ></div>
                </div>
            </div>
            <div class="col-md-4">
                <div id="P1" class="row" style="display: none;">
                    <div class="col-2">P1</div>
                    <div class="col-5 text-right"><span id="U1">-</span> V</div>
                    <div class="col-5 text-right"><span id="I1">-</span> A</div>
                </div>
                <div id="P2" class="row" style="display: none;">
                    <div class="col-2">P2</div>
                    <div class="col-5 text-right"><span id="U2">-</span> V</div>
                    <div class="col-5 text-right"><span id="I2">-</span> A</div>
                </div>
                <div id="P3" class="row" style="display: none;">
                    <div class="col-2">P3</div>
                    <div class="col-5 text-right"><span id="U3">-</span> V</div>
                    <div class="col-5 text-right"><span id="I3">-</span> A</div>
                </div>
            </div>
        </div>
    </div>
    <hr/>
    <div class="row form-group">
        <div class="col-6">
            <a href="https://github.com/gskjold/AmsToMqttBridge/releases" class="btn btn-outline-secondary">Release notes</a>
        </div>
        <div class="col-6 text-right">
            <a href="/configuration" class="btn btn-primary">Configuration</a>
        </div>
    </div>
</main>
<script src="/js/index.js"></script>
</body>
</html>
)=="==";
