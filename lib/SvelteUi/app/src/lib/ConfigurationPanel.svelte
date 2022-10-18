<script>
    import { configurationStore } from './ConfigurationStore'

    let configuration = {
        general: {
            host: '',
            sec: 0,
            user: '',
            pass: ''
        },
        meter: {
            pin: 0,
            inv: false,
            dist: 0,
            fuse: 0,
            prod: 0,
            enc: '',
            auth: ''
        },
        wifi: {
            ssid: '',
            psk: '',
            pwr: 0.0
        },
        net: {
            mode: '',
            ip: '',
            gw: '',
            dns1: '',
            dns2: '',
            ntp1: '',
            ntp2: ''
        },
        mqtt: {
            host: '',
            post: 1883,
            user: '',
            pass: '',
            pub: ''
        }
    };
    let metersources = {};
    configurationStore.subscribe(update => {
        if(update.version) {

            switch(update.chip) {
                case "esp8266":
                    metersources = {
                        'UART0' : 3,
                        'UART2' : 113,
                        'GPIO4' : 4,
                        'GPIO5' : 5,
                        'GPIO9' : 9,
                        'GPIO10' : 10,
                        'GPIO12' : 12,
                        'GPIO13' : 13,
                        'GPIO14' : 14,
                        'GPIO15' : 15,
                        'GPIO16' : 16,
                    }
                    break;
                    case "esp32":
                    metersources = {
                        'UART0' : 3,
                        'UART1' : 9,
                        'UART2' : 16,
                        'GPIO4' : 4,
                        'GPIO5' : 5,
                        'GPIO6' : 6,
                        'GPIO7' : 7,
                        'GPIO8' : 8,
                        'GPIO10' : 10,
                        'GPIO11' : 11,
                        'GPIO12' : 12,
                        'GPIO13' : 13,
                        'GPIO14' : 14,
                        'GPIO15' : 15,
                        'GPIO17' : 17,
                        'GPIO18' : 18,
                        'GPIO19' : 19,
                        'GPIO21' : 21,
                        'GPIO22' : 22,
                        'GPIO23' : 23,
                        'GPIO25' : 25,
                        'GPIO32' : 32,
                        'GPIO33' : 33,
                        'GPIO34' : 34,
                        'GPIO35' : 35,
                        'GPIO36' : 36,
                        'GPIO39' : 39,
                    }
                    break;
                    case "esp32s2":
                    metersources = {
                        'UART0' : 3,
                        'UART1' : 18,
                        'GPIO4' : 4,
                        'GPIO5' : 5,
                        'GPIO6' : 6,
                        'GPIO7' : 7,
                        'GPIO8' : 8,
                        'GPIO9' : 9,
                        'GPIO10' : 10,
                        'GPIO11' : 11,
                        'GPIO12' : 12,
                        'GPIO13' : 13,
                        'GPIO14' : 14,
                        'GPIO15' : 15,
                        'GPIO16' : 16,
                        'GPIO17' : 17,
                        'GPIO19' : 19,
                        'GPIO21' : 21,
                        'GPIO22' : 22,
                        'GPIO23' : 23,
                        'GPIO25' : 25,
                        'GPIO32' : 32,
                        'GPIO33' : 33,
                        'GPIO34' : 34,
                        'GPIO35' : 35,
                        'GPIO36' : 36,
                        'GPIO39' : 39,
                        'GPIO40' : 40,
                        'GPIO41' : 41,
                        'GPIO42' : 42,
                        'GPIO43' : 43,
                        'GPIO44' : 44,
                    }
                    break;
                default:
                    metersources = {};
            }

            configuration = update;
        }
    });

    const handleSubmit = e => {
		const formData = new FormData(e.target)
		const data = new URLSearchParams()
		for (let field of formData) {
			const [key, value] = field
			data.append(key, value)
		}

        fetch('/save', {
            method: 'POST',
            body: data
        })			
	}
</script>

<form on:submit|preventDefault={handleSubmit}>
    <div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">General</strong>
            <div class="my-3">
                Timezone<br/>
                <select class="h-10 rounded-md shadow-sm border-gray-300">
                    <option value="Europe/Oslo">Europe/Oslo</option>
                </select>
            </div>
            <div class="my-3">
                Hostname<br/>
                <input name="general_host" bind:value={configuration.general.host} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Security<br/>
                <select name="general_sec" bind:value={configuration.general.sec} class="h-10 rounded-md shadow-sm border-gray-300">
                    <option value={0}>None</option>
                    <option value={1}>Only configuration</option>
                    <option value={2}>Everything</option>
                </select>
            </div>
            <div class="my-3">
                Username<br/>
                <input name="general_user" bind:value={configuration.general.user} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Password<br/>
                <input name="general_pass" bind:value={configuration.general.pass} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Meter</strong>
            <div class="my-3">
                <span>Serial configuration</span>
                <div class="flex">
                    <select name="meter_pin" bind:value={configuration.meter.pin} class="h-10 rounded-l-md shadow-sm border-r-0 border-gray-300">
                        {#each Object.entries(metersources) as [label, value] (label)}
                            <option value={value}>{label}</option>
                        {/each}
                    </select>
                    <select name="meter_baud" bind:value={configuration.meter.baud} class="h-10 shadow-sm border-gray-300">
                        <option value={2400}>2400</option>
                        <option value={4800}>4800</option>
                        <option value={9600}>9600</option>
                        <option value={19200}>19200</option>
                        <option value={38400}>38400</option>
                        <option value={57600}>57600</option>
                        <option value={115200}>115200</option>
                    </select>
                    <select name="meter_par" bind:value={configuration.meter.par} class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300">
                        <option value={2}>7N1</option>
                        <option value={3}>8N1</option>
                        <option value={10}>7E1</option>
                        <option value={11}>8E1</option>
                    </select>
                </div>
                <label><input name="meter_inv" value="true" bind:checked={configuration.meter.inv} type="checkbox" class="rounded"/> inverted</label>
            </div>

            <div class="grid grid-cols-2">
                <div class="mb-1.5 col-span-2">
                    Distribution<br/>
                    <select name="meter_dist" bind:value={configuration.meter.dist} class="h-10 rounded-md shadow-sm border-gray-300 w-full">
                        <option value={0}></option>
                        <option value={1}>IT or TT (230V)</option>
                        <option value={2}>TN (400V)</option>
                    </select>
                </div>
                <div class="my-1.5 mr-2">
                    Main fuse<br/>
                    <label class="flex">
                        <input name="meter_fuse" bind:value={configuration.meter.fuse} type="number" min="5" max="255" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                        <div class="flex -mr-px">
                            <span class="flex items-center bg-gray-100 rounded-r-md border border-l-0 border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">A</span>
                        </div>                            
                    </label>
                </div>
                <div class="my-1.5">
                    Production<br/>
                    <label class="flex">
                        <input name="meter_prod" bind:value={configuration.meter.prod} type="number" min="0" max="255" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                        <div class="flex">
                            <span class="flex items-center bg-gray-100 rounded-r-md border border-l-0 border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWp</span>
                        </div>                            
                    </label>
                </div>
            </div>

            <div class="my-3">
                Encryption key<br/>
                <input name="meter_enc" bind:value={configuration.meter.enc} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Authentication key<br/>
                <input name="meter_auth" bind:value={configuration.meter.auth} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">WiFi</strong>
            <div class="my-3">
                SSID<br/>
                <input name="wifi_ssid" bind:value={configuration.wifi.ssid} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                PSK<br/>
                <input name="wifi_psk" bind:value={configuration.wifi.psk} type="password" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div>
                Power<br/>
                <label class="flex">
                    <input name="wifi_pwr" bind:value={configuration.wifi.pwr} type="number" min="0" max="20.5" step="0.5" class="h-10 rounded-l-md shadow-sm border-gray-300"/>
                    <div class="flex -mr-px">
                        <span class="flex items-center bg-gray-100 rounded-r-md border border-l-0 border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">dBm</span>
                    </div>                            
                </label>
            </div>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Network</strong>
            <div class="my-3">
                IP<br/>
                <div class="flex">
                    <select name="net_ip" bind:value={configuration.net.mode} class="h-10 rounded-l-md shadow-sm border-r-0 border-gray-300">
                        <option value="d">DHCP</option>
                        <option value="s">Static</option>
                    </select>
                    <input type="text" class="h-10 shadow-sm border-gray-300 w-full"/>
                    <select class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300">
                        <option>/24</option>
                    </select>
                </div>
            </div>
            <div class="my-3">
                Gateway<br/>
                <input name="net_gw" bind:value={configuration.net.gw} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                DNS<br/>
                <div class="flex">
                    <input name="net_dns1" bind:value={configuration.net.dns1} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    <input name="net_dns2" bind:value={configuration.net.dns2} type="text" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300 w-full"/>
                </div>
                <label><input name="net_mdns" value="true" bind:checked={configuration.net.mdns} type="checkbox" class="rounded"/> enable mDNS</label>
            </div>
            <div class="my-3">
                NTP<br/>
                <div class="flex">
                    <input name="net_ntp1" bind:value={configuration.net.ntp1} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    <input name="net_ntp2" bind:value={configuration.net.ntp2} type="text" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300 w-full"/>
                </div>
                <label><input name="net_ntpdhcp" value="true" bind:checked={configuration.net.ntpdhcp} type="checkbox" class="rounded"/> obtain from DHCP</label>
                </div>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">MQTT</strong>
            <div class="my-3">
                Server<br/>
                <div class="flex">
                    <input name="mqtt_host" bind:value={configuration.mqtt.host} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    <input name="mqtt_port" bind:value={configuration.mqtt.port} type="text" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300 w-20"/>
                </div>
            </div>
            <div class="my-3">
                Username<br/>
                <input name="mqtt_user" bind:value={configuration.mqtt.user} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Password<br/>
                <input name="mqtt_pass" bind:value={configuration.mqtt.pass} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Client ID<br/>
                <input name="mqtt_clid" bind:value={configuration.mqtt.clid} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Publish topic<br/>
                <input name="mqtt_pub" bind:value={configuration.mqtt.pub} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-3">
                Payload<br/>
                <select name="mqtt_mode" bind:value={configuration.mqtt.mode} class="h-10 rounded-md shadow-sm border-gray-300">
                    <option value={0}>JSON</option>
                    <option value={1}>Raw values (minimal)</option>
                    <option value={2}>Raw values (full)</option>
                    <option value={3}>Domoticz</option>
                    <option value={4}>Home-Assistant</option>
                    <option value={255}>Raw data (bytes)</option>
                </select>
            </div>
            <div class="my-3">
                SSL
            </div>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Prices</strong>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Webhook</strong>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Backup and restore</strong>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Debugging</strong>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Vendor menu</strong>
            Board type<br/>
            GPIO<br/>
            Vcc<br/>
            Favico<br/>
        </div>
    </div>
    <button type="submit" class="font-bold py-2 px-4 rounded bg-blue-500 text-white">Save</button>
</form>