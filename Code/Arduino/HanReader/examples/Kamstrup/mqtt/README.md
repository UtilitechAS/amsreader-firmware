# Setup

1. Copy AmsToMqttBridge\Code\Arduino\HanReader\src to Arduino\libraries
2. Download the following libraries and put them in Arduino\libraries
   - ESP8266WiFi
   - PubSubClient
   - ArduinoJson
3. **Set MQTT_MAX_PACKET_SIZE in PubSubClient.h to at least 512 (i used 1024)**
4. Edit the following variables in the project:
   - ssid
   - password
   - mqtt_server
   - mqtt_topic
   - device_name

## Output example:
### List 1
```
{
	"dn": "espams",
	"up": 1475902,
	"data": {
		"ls": 25,
		"lvi": "Kamstrup_V0001",
		"mid": "5706567274389702",
		"mt": "6841121BN243101040",
		"t": 1510088840,
		"aip": 3499,
		"aep": 0,
		"rip": 0,
		"rep": 424,
		"al1": 10.27,
		"al2": 6.37,
		"al3": 11.79,
		"vl1": 231,
		"vl2": 226,
		"vl3": 231
	}
}
```
### List 2
```
{
	"dn": "espams",
	"up": 1041212,
	"data": {
		"ls": 35,
		"lvi": "Kamstrup_V0001",
		"mid": "5706567274389702",
		"mt": "6841121BN243101040",
		"t": 1510088405,
		"aip": 4459,
		"aep": 0,
		"rip": 0,
		"rep": 207,
		"al1": 14.72,
		"al2": 6.39,
		"al3": 15.02,
		"vl1": 231,
		"vl2": 227,
		"vl3": 231,
		"cl": 1510088405,
		"caie": 588500,
		"caee": 0,
		"crie": 93,
		"cree": 80831
	}
}
```

### List 1 and 2 fields
- dn = Device Name
- up = MS since last reboot
- ls = List Size
- lvi = List Version Identifier
- mid = Meter ID
- mt = Meter Type
- t = Time
- aie = Active Import Power
- aep = Active Export Power
- rip = Reactive Import Power
- rep = Reactive Export Power
- al1 = Current L1
- al2 = Current L2
- al3 = Current L3
- cl1 = Voltage L1
- cl2 = Voltage L2
- cl3 = Voltage L3

### List 2 additional fields
- cl = Meter Clock
- caie = Cumulative Active Import Energy
- caee = Cumulative Active Export Energy
- crie = Cumulative Reactive Import Energy
- cree = Cumulative Reactive Export Energy