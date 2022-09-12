<script>

</script>

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
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            Security<br/>
            <select class="h-10 rounded-md shadow-sm border-gray-300">
                <option value="">None</option>
                <option value="">Only configuration</option>
                <option value="">Everything</option>
            </select>
        </div>
        <div class="my-3">
            Username<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            Password<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
    </div>
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">Meter</strong>
        <div class="my-3">
            <span>Serial configuration</span>
            <div class="flex">
                <select class="h-10 rounded-l-md shadow-sm border-r-0 border-gray-300">
                    <option value="">UART0</option>
                    <option value="">UART1</option>
                    <option value="">UART2</option>
                    <option value="">GPIO33</option>
                </select>
                <select class="h-10 shadow-sm border-gray-300">
                    <option value="2400">2400</option>
                    <option value="4800">4800</option>
                    <option value="9600">9600</option>
                    <option value="19200">19200</option>
                    <option value="38400">38400</option>
                    <option value="57600">57600</option>
                    <option value="115200">115200</option>
                </select>
                <select class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300">
                    <option value="2">7N1</option>
                    <option value="3">8N1</option>
                    <option value="10">7E1</option>
                    <option value="11">8E1</option>
                </select>
            </div>
            <label><input type="checkbox" class="rounded"/> inverted</label>
        </div>

        <div class="grid grid-cols-2">
            <div class="mb-1.5 col-span-2">
                Distribution<br/>
                <select class="h-10 rounded-md shadow-sm border-gray-300 w-full">
                    <option value="0"></option>
                    <option value="1">IT or TT (230V)</option>
                    <option value="2">TN (400V)</option>
                </select>
            </div>
            <div class="my-1.5 mr-2">
                Main fuse<br/>
                <label class="flex">
                    <input type="number" min="5" max="255" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    <div class="flex -mr-px">
                        <span class="flex items-center bg-gray-100 rounded-r-md border border-l-0 border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">A</span>
                    </div>                            
                </label>
            </div>
            <div class="my-1.5">
                Production<br/>
                <label class="flex">
                    <input type="number" min="0" max="255" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    <div class="flex">
                        <span class="flex items-center bg-gray-100 rounded-r-md border border-l-0 border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWp</span>
                    </div>                            
                </label>
            </div>
        </div>

        <div class="my-3">
            Encryption key<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            Authentication key<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        </div>
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">WiFi</strong>
        <div class="my-3">
            SSID<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            PSK<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div>
            Power<br/>
            <label class="flex">
                <input type="number" min="0" max="19.5" step="0.5" class="h-10 rounded-l-md shadow-sm border-gray-300"/>
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
                <select class="h-10 rounded-l-md shadow-sm border-r-0 border-gray-300">
                    <option>DHCP</option>
                    <option>Static</option>
                </select>
                <input type="text" class="h-10 shadow-sm border-gray-300 w-full"/>
                <select class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300">
                    <option>/24</option>
                </select>
            </div>
        </div>
        <div class="my-3">
            Gateway<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            DNS<br/>
            <div class="flex">
                <input type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                <input type="text" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300 w-full"/>
            </div>
            <label><input type="checkbox" class="rounded"/> enable mDNS</label>
        </div>
        <div class="my-3">
            NTP<br/>
            <div class="flex">
                <input type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                <input type="text" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300 w-full"/>
            </div>
            <label><input type="checkbox" class="rounded"/> obtain from DHCP</label>
            </div>
    </div>
    <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
        <strong class="text-sm">MQTT</strong>
        <div class="my-3">
            Server<br/>
            <div class="flex">
                <input type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                <input type="text" class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300 w-20"/>
            </div>
        </div>
        <div class="my-3">
            Username<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            Password<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            Client ID<br/>
            <input type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
        </div>
        <div class="my-3">
            Publish topic<br/>
            <div class="flex">
                <input type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                <select class="h-10 rounded-r-md shadow-sm border-l-0 border-gray-300">
                    <option>JSON</option>
                </select>
            </div>
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
