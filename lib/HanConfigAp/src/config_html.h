const char CONFIG_HTML[] PROGMEM = R"=="==(
<html>
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">

        <link rel="stylesheet" type="text/css" href="/style.css">

        <title>AMS2MQTT - configuration</title>
    </head>
    <body>
        <form method='post' action='/save'>
            <div class="wrapper">
                <div class="inner-wrapper">
                    <div>
                        <h2>WiFi</h2>
                    </div>
                    <div>
                        <input type='text' name='ssid' value="${config.ssid}" placeholder="SSID">
                    </div>
                    <div>
                        <input type='password' name='ssidPassword' value="${config.ssidPassword}" placeholder="Password">
                    </div>
                </div>
                <div class="inner-wrapper">
                    <div>
                        <h2>Meter Type</h2>
                    </div>
                    <div class="select-style">
                        <select name="meterType">
                            <option value="0" ${config.meterType0} disabled class="disabled-option"> SELECT TYPE </option>
                            <option value="1" ${config.meterType1}>Kaifa</option>
                            <option value="2" ${config.meterType2}>Aidon</option>
                            <option value="3" ${config.meterType3}>Kamstrup</option>
                        </select>
                    </div>
                </div>
                <div class="inner-wrapper">
                    <div>
                        <h2>MQTT</h2>
                    </div>
                    <div>
                        <label>Server & port:</label>
                        <input type='text' name='mqtt' value="${config.mqtt}" placeholder="server">
                        <input type='number' name='mqttPort' value="${config.mqttPort}" placeholder="port">
                    </div>
                    <div>
                        <label>Client ID:</label>
                        <input type='text' name='mqttClientID' value="${config.mqttClientID}" placeholder="client id">
                    </div>
                    <div>
                        <label>Publish topic: </label>
                        <input type='text' name='mqttPublishTopic' value="${config.mqttPublishTopic}" placeholder="topic">
                    </div>
                    <div>
                        <label>Username:</label>
                        <input type='text' name='mqttUser' value="${config.mqttUser}" placeholder="Blank for insecure">
                    </div>
                    <div>
                        <label>Password:</label>
                        <input type='password' name='mqttPass' value="${config.mqttPass}" placeholder="Blank for insecure">
                    </div>
                    <div>
                        <input class="submit-button" type='submit' value='save'>
                    </div>
                </div>
            </div>
        </form>
    <body>
</html>
)=="==";
