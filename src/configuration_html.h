const char CONFIGURATION_HTML[] PROGMEM = R"=="==(
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>AMS reader - configuration</title>
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <link rel="stylesheet" type="text/css" href="/css/bootstrap.css"/>
    <link rel="stylesheet" type="text/css" href="/css/application.css"/>
</head>
<body class="bg-light">
<main role="main" class="container">
    <div class="d-flex align-items-center p-3 my-2 text-white-50 bg-purple rounded shadow">
        <div class="lh-100">
            <h6 class="mb-0 text-white lh-100">AMS reader - configuration</h6>
            <small>${version}</small>
        </div>
    </div>
    <form method="post" action="/save">
        <div class="row">
            <div class="col-md-6 col-lg-4">
                <div class="my-2 p-3 bg-white rounded shadow">
                    <h6 class="border-bottom border-gray pb-2 mb-4">WiFi</h6>
                    <div class="row form-group">
                        <label class="col-3">SSID</label>
                        <div class="col-9">
                            <input type="text" class="form-control" name="ssid" value="${config.ssid}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-3">Password</label>
                        <div class="col-9">
                            <input type="password" class="form-control" name="ssidPassword" value="${config.ssidPassword}"/>
                        </div>
                    </div>
                </div>
                <div class="my-3 p-3 bg-white rounded shadow">
                    <h6 class="border-bottom border-gray pb-2 mb-4">AMS meter</h6>
                    <div class="row form-group">
                        <label class="col-4">Meter type</label>
                        <div class="col-8">
                            <select class="form-control" name="meterType">
                                <option value="0" ${config.meterType0} disabled></option>
                                <option value="1" ${config.meterType1}>Kaifa</option>
                                <option value="2" ${config.meterType2}>Aidon</option>
                                <option value="3" ${config.meterType3}>Kamstrup</option>
                            </select>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Main fuse</label>
                        <div class="col-8">
                            <select class="form-control" name="fuseSize">
                                <option value="" ${config.fuseSize0}></option>
                                <option value="25" ${config.fuseSize25}>25A</option>
                                <option value="32" ${config.fuseSize32}>32A</option>
                                <option value="40" ${config.fuseSize40}>40A</option>
                                <option value="50" ${config.fuseSize50}>50A</option>
                                <option value="63" ${config.fuseSize63}>63A</option>
                            </select>
                        </div>
                    </div>
                </div>
            </div>
            <div class="col-md-6 col-lg-4">
                <div class="my-2 p-3 bg-white rounded shadow">
                    <h6 class="border-bottom border-gray pb-2 mb-4">MQTT</h6>
                    <div class="row form-group">
                        <label class="col-4">Hostname</label>
                        <div class="col-8">
                            <input type="text" class="form-control" name="mqtt" value="${config.mqtt}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Port</label>
                        <div class="col-8">
                            <input type="text" class="form-control" name="mqttPort" value="${config.mqttPort}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Client ID</label>
                        <div class="col-8">
                            <input type="text" class="form-control" name="mqttClientID" value="${config.mqttClientID}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Topic</label>
                        <div class="col-8">
                            <input type="text" class="form-control" name="mqttPublishTopic" value="${config.mqttPublishTopic}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Username</label>
                        <div class="col-8">
                            <input type="text" class="form-control" name="mqttUser" value="${config.mqttUser}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Password</label>
                        <div class="col-8">
                            <input type="password" class="form-control" name="mqttPass" value="${config.mqttPass}"/>
                        </div>
                    </div>
                </div>
            </div>
            <div class="col-md-6 col-lg-4">
                <div class="my-2 p-3 bg-white rounded shadow">
                    <h6 class="border-bottom border-gray pb-2 mb-4">Web server</h6>
                    <div class="row form-group">
                        <label class="col-4">Security</label>
                        <div class="col-8">
                            <select class="form-control" name="authSecurity">
                                <option value="0" ${config.authSecurity0}>None</option>
                                <option value="1" ${config.authSecurity1}>Only configuration</option>
                                <option value="2" ${config.authSecurity2}>Everything</option>
                            </select>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Username</label>
                        <div class="col-8">
                            <input type="text" class="form-control" name="authUser" value="${config.authUser}"/>
                        </div>
                    </div>
                    <div class="row form-group">
                        <label class="col-4">Password</label>
                        <div class="col-8">
                            <input type="password" class="form-control" name="authPass" value="${config.authPass}"/>
                        </div>
                    </div>
                </div>
            </div>
        </div>
        <hr/>
        <div class="row form-group">
            <div class="col-6">
                <a href="/" class="btn btn-outline-secondary">Back</a>
            </div>
            <div class="col-6 text-right">
                <button class="btn btn-primary">Save</button>
            </div>
        </div>
    </form>
</main>
</body>
</html>
)=="==";
