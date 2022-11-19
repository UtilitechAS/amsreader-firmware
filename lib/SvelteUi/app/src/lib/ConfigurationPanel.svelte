<script>
    import { getConfiguration, configurationStore } from './ConfigurationStore'
    import { sysinfoStore } from './DataStores.js';
    import BoardTypeSelectOptions from './BoardTypeSelectOptions.svelte';
    import UartSelectOptions from './UartSelectOptions.svelte';
    import Mask from './Mask.svelte'
    import { metertype } from './Helpers';
    import Badge from './Badge.svelte';

    export let sysinfo = {}

    let loadingOrSaving = true;

    let configuration = {
        g: {
            t: '', h: '', s: 0, u: '', p: ''
        },
        m: {
            b: 2400, p: 11, i: false, d: 0, f: 0, r: 0,
            e: { e: false, k: '', a: '' },
            m: { e: false, w: false, v: false, a: false, c: false }
        },
        w: { s: '', p: '', w: 0.0, z: 255 },
        n: {
            m: '', i: '', s: '', g: '', d1: '', d2: '', d: false, n1: '', n2: '', h: false
        },
        q: {
            h: '', p: 1883, u: '', a: '', b: '',
            s: { e: false, c: false, r: true, k: false }
        },
        t: {
            t: [0,0,0,0,0,0,0,0,0,0], h: 1
        },
        p: {
            e: false, t: '', r: '', c: '', m: 1.0
        },
        d: {
            s: false, t: false, l: 5
        },
        i: {
            h: null, a: null,
            l: { p: null, i: false },
            r: { r: null, g: null, b: null, i: false },
            t: { d: null, a: null },
            v: { p: null, d: { v: null, g: null }, o: null, m: null, b: null }
        }
    };
    configurationStore.subscribe(update => {
        if(update.version) {
            configuration = update;
            loadingOrSaving = false;
        }
    });
    getConfiguration();

    async function handleSubmit(e) {
        loadingOrSaving = true;
		const formData = new FormData(e.target)
		const data = new URLSearchParams()
		for (let field of formData) {
			const [key, value] = field
			data.append(key, value)
		}

        const response = await fetch('/save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())

        sysinfoStore.update(s => {
            s.booting = res.reboot;
            return s;
        });

        loadingOrSaving = false;
        getConfiguration();
	}
</script>

<form on:submit|preventDefault={handleSubmit}>
    <div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">General</strong>
            <input type="hidden" name="g" value="true"/>
            <div class="my-1">
                <div class="flex">
                    <div>
                        Hostname<br/>
                        <input name="gh" bind:value={configuration.g.h} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    </div>
                    <div>
                        Timezone<br/>
                        <select name="gt" bind:value={configuration.g.t} class="h-10 rounded-r-md border-l-0 shadow-sm border-gray-300">
                            <option value="UTC">UTC</option>
                            <option value="CET/CEST">CET/CEST</option>
                        </select>
                    </div>
                </div>
            </div>
            <input type="hidden" name="p" value="true"/>
            <div class="my-1">
                <div class="flex">
                    <div>
                        Price region<br/>
                        <select name="pr" bind:value={configuration.p.r} class="h-10 rounded-l-md shadow-sm border-gray-300">
                            <optgroup label="Norway">
                                <option value="10YNO-1--------2">NO1</option>
                                <option value="10YNO-2--------T">NO2</option>
                                <option value="10YNO-3--------J">NO3</option>
                                <option value="10YNO-4--------9">NO4</option>
                                <option value="10Y1001A1001A48H">NO5</option>
                            </optgroup>
                            <optgroup label="Sweden">
                                <option value="10Y1001A1001A44P">SE1</option>
                                <option value="10Y1001A1001A45N">SE2</option>
                                <option value="10Y1001A1001A46L">SE3</option>
                                <option value="10Y1001A1001A47J">SE4</option>
                                </optgroup>
                            <optgroup label="Denmark">
                                <option value="10YDK-1--------W">DK1</option>
                                <option value="10YDK-2--------M">DK2</option>
                            </optgroup>
                            <option value="10YAT-APG------L">Austria</option>
                            <option value="10YBE----------2">Belgium</option>
                            <option value="10YCZ-CEPS-----N">Czech Republic</option>
                            <option value="10Y1001A1001A39I">Estonia</option>
                            <option value="10YFI-1--------U">Finland</option>
                            <option value="10YFR-RTE------C">France</option>
                            <option value="10Y1001A1001A83F">Germany</option>
                            <option value="10YGB----------A">Great Britain</option>
                            <option value="10YLV-1001A00074">Latvia</option>
                            <option value="10YLT-1001A0008Q">Lithuania</option>
                            <option value="10YNL----------L">Netherland</option>
                            <option value="10YPL-AREA-----S">Poland</option>
                            <option value="10YCH-SWISSGRIDZ">Switzerland</option>
                        </select>
                    </div>
                    <div>
                        Currency<br/>
                        <select name="pc" bind:value={configuration.p.c} class="h-10 border-l-0 shadow-sm border-gray-300">
                            <option value="NOK">NOK</option>
                            <option value="SEK">SEK</option>
                            <option value="DKK">DKK</option>
                            <option value="EUR">EUR</option>
                        </select>
                    </div>
                    <div>
                        Multiplier<br/>
                        <input name="pm" bind:value={configuration.p.m} type="number" min="0.001" max="1000" step="0.001" class="h-10 rounded-r-md border-l-0 shadow-sm border-gray-300 w-24 text-right"/>
                    </div>
                </div>
            </div>
            {#if sysinfo.chip != 'esp8266'}
            <div class="my-1">
                <label><input type="checkbox" name="pe" bind:checked={configuration.p.e} class="rounded mb-1"/> ENTSO-E token</label>
                {#if configuration.p.e}
                <br/><input name="pt" bind:value={configuration.p.t} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
                {/if}
            </div>
            {/if}
            <div class="my-1">
                Security<br/>
                <select name="gs" bind:value={configuration.g.s} class="h-10 rounded-md shadow-sm border-gray-300">
                    <option value={0}>None</option>
                    <option value={1}>Only configuration</option>
                    <option value={2}>Everything</option>
                </select>
            </div>
            {#if configuration.g.s > 0}
            <div class="my-1">
                Username<br/>
                <input name="gu" bind:value={configuration.g.u} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-1">
                Password<br/>
                <input name="gp" bind:value={configuration.g.p} type="password" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            {/if}
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Meter</strong>
            <input type="hidden" name="m" value="true"/>
            <div class="my-1">
                <span>Serial configuration</span>
                <div class="flex">
                    <select name="mb" bind:value={configuration.m.b} class="h-10 rounded-l-md shadow-sm border-gray-300">
                        <option value={2400}>2400</option>
                        <option value={4800}>4800</option>
                        <option value={9600}>9600</option>
                        <option value={19200}>19200</option>
                        <option value={38400}>38400</option>
                        <option value={57600}>57600</option>
                        <option value={115200}>115200</option>
                    </select>
                    <select name="mp" bind:value={configuration.m.p} class="h-10 rounded-r-md border-l-0 shadow-sm  border-gray-300">
                        <option value={2}>7N1</option>
                        <option value={3}>8N1</option>
                        <option value={10}>7E1</option>
                        <option value={11}>8E1</option>
                    </select>
                    <label class="mt-2 ml-3"><input name="mi" value="true" bind:checked={configuration.m.i} type="checkbox" class="rounded mb-1"/> inverted</label>
                </div>
            </div>

            <div class="my-1 flex">
                <div class="w-32">
                    Distribution<br/>
                    <select name="md" bind:value={configuration.m.d} class="h-10 rounded-l-md shadow-sm border-gray-300 w-full">
                        <option value={0}></option>
                        <option value={1}>IT/TT</option>
                        <option value={2}>TN</option>
                    </select>
                </div>
                <div>
                    Main fuse<br/>
                    <label class="flex">
                        <input name="mf" bind:value={configuration.m.f} type="number" min="5" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                        <span class="flex items-center bg-gray-100 border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">A</span>
                    </label>
                </div>
                <div>
                    Production<br/>
                    <label class="flex">
                        <input name="mr" bind:value={configuration.m.r} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                        <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWp</span>
                    </label>
                </div>
            </div>
            <div class="my-1">
            </div>
            
            <div class="my-1">
                <label><input type="checkbox" name="me" value="true" bind:checked={configuration.m.e.e} class="rounded mb-1"/> Meter uses encryption</label>
                {#if configuration.m.e.e}
                <br/><input name="mek" bind:value={configuration.m.e.k} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
                {/if}
            </div>
            {#if configuration.m.e.e}
            <div class="my-1">
                Authentication key<br/>
                <input name="mea" bind:value={configuration.m.e.a} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            {/if}

            <label><input type="checkbox" name="mm" value="true" bind:checked={configuration.m.m.e} class="rounded mb-1"/> Multipliers</label>
            {#if configuration.m.m.e}
            <div class="flex my-1">
                <div class="w-1/4">
                    Instant<br/>
                    <input name="mmw" bind:value={configuration.m.m.w} type="number" min="0.00" max="655.35" step="0.01" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full text-right"/>
                </div>
                <div class="w-1/4">
                    Volt<br/>
                    <input name="mmv" bind:value={configuration.m.m.v} type="number" min="0.00" max="655.35" step="0.01" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                </div>
                <div class="w-1/4">
                    Amp<br/>
                    <input name="mma" bind:value={configuration.m.m.a} type="number" min="0.00" max="655.35" step="0.01" class="h-10 border-r-0 shadow-sm border-gray-300 w-full text-right"/>
                </div>
                <div class="w-1/4">
                    Acc.<br/>
                    <input name="mmc" bind:value={configuration.m.m.c} type="number" min="0.00" max="655.35" step="0.01" class="h-10 rounded-r-md shadow-sm border-gray-300 w-full text-right"/>
                </div>
            </div>
            {/if}
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">WiFi</strong>
            <input type="hidden" name="w" value="true"/>
            <div class="my-1">
                SSID<br/>
                <input name="ws" bind:value={configuration.w.s} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-1">
                PSK<br/>
                <input name="wp" bind:value={configuration.w.p} type="password" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-1 flex">
                <div>
                    Power saving<br/>
                    <select name="wz" bind:value={configuration.w.z} class="h-10 rounded-md shadow-sm border-gray-300">
                        <option value={255}>Default</option>
                        <option value={0}>Off</option>
                        <option value={1}>Minimum</option>
                        <option value={2}>Maximum</option>
                    </select>
                </div>
                <div class="ml-2">
                    Power<br/>
                    <label class="flex">
                        <input name="ww" bind:value={configuration.w.w} type="number" min="0" max="20.5" step="0.5" class="h-10 rounded-l-md shadow-sm border-gray-300 text-right"/>
                        <span class="flex items-center bg-gray-100 rounded-r-md border-l-0 border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">dBm</span>
                    </label>
                </div>
            </div>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Network</strong>
            <div class="my-1">
                IP<br/>
                <div class="flex">
                    <select name="nm" bind:value={configuration.n.m} class="h-10 rounded-l-md shadow-sm border border-gray-300">
                        <option value="dhcp">DHCP</option>
                        <option value="static">Static</option>
                    </select>
                    <input name="ni" bind:value={configuration.n.i} type="text" class="h-10 border-x-0 shadow-sm border-gray-300 w-full disabled:bg-gray-200" disabled={configuration.n.m == 'dhcp'}/>
                    <select name="ns" bind:value={configuration.n.s} class="h-10 rounded-r-md shadow-sm  border-gray-300 disabled:bg-gray-200" disabled={configuration.n.m == 'dhcp'}>
                        <option value="255.255.255.0">/24</option>
                        <option value="255.255.0.0">/16</option>
                        <option value="255.0.0.0">/8</option>
                    </select>
                </div>
            </div>
            {#if configuration.n.m == 'static'}
            <div class="my-1">
                Gateway<br/>
                <input name="ng" bind:value={configuration.n.g} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full disabled:bg-gray-200"/>
            </div>
            <div class="my-1">
                DNS<br/>
                <div class="flex">
                    <input name="nd1" bind:value={configuration.n.d1} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full disabled:bg-gray-200"/>
                    <input name="nd2" bind:value={configuration.n.d2} type="text" class="h-10 border-l-0 rounded-r-md shadow-sm border-gray-300 w-full disabled:bg-gray-200"/>
                </div>
            </div>
            {/if}
            <div class="my-1">
                <label><input name="nd" value="true" bind:checked={configuration.n.d} type="checkbox" class="rounded mb-1"/> enable mDNS</label>
            </div>
            <input type="hidden" name="ntp" value="true"/>
            <div class="my-1">
                NTP <label class="ml-4"><input name="ntpd" value="true" bind:checked={configuration.n.h} type="checkbox" class="rounded mb-1"/> obtain from DHCP</label><br/>
                <div class="flex">
                    <input name="ntph" bind:value={configuration.n.n1} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
                </div>
            </div>
        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">MQTT</strong>
            <input type="hidden" name="q" value="true"/>
            <div class="my-1">
                Server
                <label class="float-right mr-3"><input type="checkbox" name="qs" bind:checked={configuration.q.s.e} class="rounded mb-1"/> SSL</label>
                <br/>
                <div class="flex">
                    <input name="qh" bind:value={configuration.q.h} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                    <input name="qp" bind:value={configuration.q.p} type="number" min="1024" max="65535" class="h-10 border-l-0 rounded-r-md shadow-sm border-gray-300 w-20 text-right"/>
                </div>
            </div>
            {#if configuration.q.s.e}
            <div class="my-1">
                <div>
                    {#if configuration.q.s.c}
                    <Badge color="green" text="CA OK" title="Click here to replace CA"/>
                    {:else}
                    <Badge color="blue" text="Upload CA" title="Click here to upload CA"/>
                    {/if}

                    {#if configuration.q.s.r}
                    <Badge color="green" text="Cert OK" title="Click here to replace certificate"/>
                    {:else}
                    <Badge color="blue" text="Upload cert" title="Click here to upload certificate"/>
                    {/if}

                    {#if configuration.q.s.k}
                    <Badge color="green" text="Key OK" title="Click here to replace key"/>
                    {:else}
                    <Badge color="blue" text="Upload key" title="Click here to upload key"/>
                    {/if}
                </div>
            </div>
            {/if}
            <div class="my-1">
                Username<br/>
                <input name="qu" bind:value={configuration.q.u} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-1">
                Password<br/>
                <input name="qa" bind:value={configuration.q.a} type="password" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
            <div class="my-1 flex">
                <div>
                    Client ID<br/>
                    <input name="qc" bind:value={configuration.q.c} type="text" class="h-10 rounded-l-md shadow-sm border-gray-300 w-full"/>
                </div>
                <div>
                    Payload<br/>
                    <select name="qm" bind:value={configuration.q.m} class="h-10 border-l-0 rounded-r-md shadow-sm border-gray-300 w-36">
                        <option value={0}>JSON</option>
                        <option value={1}>Raw (minimal)</option>
                        <option value={2}>Raw (full)</option>
                        <option value={3}>Domoticz</option>
                        <option value={4}>HomeAssistant</option>
                        <option value={255}>Raw bytes</option>
                    </select>
                </div>
            </div>
            <div class="my-1">
                Publish topic<br/>
                <input name="qb" bind:value={configuration.q.b} type="text" class="h-10 rounded-md shadow-sm border-gray-300 w-full"/>
            </div>
        </div>
        {#if configuration.p.r.startsWith("10YNO") || configuration.p.r == '10Y1001A1001A48H'}
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Tariff thresholds</strong>
            <input type="hidden" name="t" value="true"/>
            <div class="flex flex-wrap my-1">
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">1</span>
                    <input name="t0" bind:value={configuration.t.t[0]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">2</span>
                    <input name="t1" bind:value={configuration.t.t[1]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">3</span>
                    <input name="t2" bind:value={configuration.t.t[2]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">4</span>
                    <input name="t3" bind:value={configuration.t.t[3]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">5</span>
                    <input name="t4" bind:value={configuration.t.t[4]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">6</span>
                    <input name="t5" bind:value={configuration.t.t[5]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">7</span>
                    <input name="t6" bind:value={configuration.t.t[6]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">8</span>
                    <input name="t7" bind:value={configuration.t.t[7]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
                <label class="flex w-40 m-1">
                    <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">9</span>
                    <input name="t8" bind:value={configuration.t.t[8]} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-full text-right"/>
                    <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">kWh</span>
                </label>
            </div>
            <label class="flex m-1">
                <span class="flex items-center bg-gray-100 rounded-l-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">Average of</span>
                <input name="th" bind:value={configuration.t.h} type="number" min="0" max="255" class="h-10 border-x-0 shadow-sm border-gray-300 w-24 text-right"/>
                <span class="flex items-center bg-gray-100 rounded-r-md border border-gray-300 px-3 whitespace-no-wrap text-grey-dark text-sm">hours</span>
            </label>
        </div>
        {/if}
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Cloud</strong>

        </div>
        <div class="bg-white m-2 p-2 rounded-md shadow-lg pb-4 text-gray-700">
            <strong class="text-sm">Hardware</strong>
            {#if sysinfo.board > 20}
            <input type="hidden" name="i" value="true"/>
            <div class="flex flex-wrap">
                <div>
                    HAN<br/>
                    <select name="ih" bind:value={configuration.i.h} class="h-10 rounded-l-md shadow-sm border-gray-300">
                        <UartSelectOptions chip={sysinfo.chip}/>
                    </select>
                </div>
                <div>
                    AP button<br/>
                    <input name="ia" bind:value={configuration.i.a} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 border-x-0 shadow-sm border-gray-300 text-right"/>
                </div>
                <div>
                    LED<label class="ml-4"><input name="ili" value="true" bind:checked={configuration.i.l.i} type="checkbox" class="rounded mb-1"/> inv</label><br/>
                    <div class="flex">
                        <input name="ilp" bind:value={configuration.i.l.p} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 rounded-r-md shadow-sm border-gray-300 text-right"/>
                    </div>
                </div>
                <div>
                    RGB<label class="ml-4"><input name="iri" value="true" bind:checked={configuration.i.r.i} type="checkbox" class="rounded mb-1"/> inverted</label><br/>
                    <div class="flex">
                        <input name="irr" bind:value={configuration.i.r.r} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 rounded-l-md shadow-sm border-gray-300 text-right"/>
                        <input name="irg" bind:value={configuration.i.r.g} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 border-x-0 shadow-sm border-gray-300 text-right"/>
                        <input name="irb" bind:value={configuration.i.r.b} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 rounded-r-md shadow-sm border-gray-300 text-right"/>
                    </div>
                </div>
                <div class="my-1">
                    Temperature<br/>
                    <input name="itd" bind:value={configuration.i.t.d} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 rounded-l-md shadow-sm border-gray-300 text-right"/>
                </div>
                <div class="my-1">
                    Analog temp<br/>
                    <input name="ita" bind:value={configuration.i.t.a} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 border-x-0 shadow-sm border-gray-300 text-right"/>
                </div>
                {#if sysinfo.chip != 'esp8266'}
                <div class="my-1">
                    Vcc<br/>
                    <input name="ivp" bind:value={configuration.i.v.p} type="number" min="0" max={sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39} class="h-10 rounded-r-md shadow-sm border-gray-300 text-right"/>
                </div>
                {/if}
                {#if configuration.i.v.p > 0}
                <div class="my-1">
                    Voltage divider<br/>
                    <div class="flex">
                        <input name="ivdv" bind:value={configuration.i.v.d.v} type="number" min="0" max="65535" class="h-10 rounded-l-md shadow-sm border-gray-300 text-right" placeholder="VCC"/>
                        <input name="ivdg" bind:value={configuration.i.v.d.g} type="number" min="0" max="65535" class="h-10 border-l-0 rounded-r-md shadow-sm border-gray-300 text-right" placeholder="GND"/>
                    </div>
                </div>
                {/if}
            </div> 
            {/if}
            {#if configuration.i.v.p > 0 || sysinfo.chip == 'esp8266'}
            <div class="my-1 flex flex-wrap">
                <div>
                    Vcc offset<br/>
                    <input name="ivo" bind:value={configuration.i.v.o} type="number" min="0.0" max="3.5" step="0.01" class="h-10 rounded-l-md shadow-sm border-gray-300 w-24 text-right"/>
                </div>
                <div>
                    multiplier<br/>
                    <input name="ivm" bind:value={configuration.i.v.m} type="number" min="0.1" max="10" step="0.01" class="h-10 border-x-0 shadow-sm border-gray-300 w-24 text-right"/>
                </div>
                <div>
                    boot limit<br/>
                    <input name="ivb" bind:value={configuration.i.v.b} type="number" min="2.5" max="3.5" step="0.1" class="h-10 rounded-r-md shadow-sm border-gray-300 w-24 text-right"/>
                </div>
            </div>
            {/if}
            <input type="hidden" name="d" value="true"/>
            <div class="mt-3">
                <label><input type="checkbox" name="ds" value="true" bind:checked={configuration.d.s} class="rounded mb-1"/> Enable debugging</label>
            </div>
            {#if configuration.d.s}
            <div class="my-auto bg-red-500 text-red-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">Debug can cause sudden reboots. Do not leave on!</div>
            <div class="my-1">
                <label><input type="checkbox" name="dt" value="true" bind:checked={configuration.d.t} class="rounded mb-1"/> Enable telnet</label>
            </div>
            {#if configuration.d.t}
            <div class="my-auto bg-red-500 text-red-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">Telnet is unsafe and should be off when not in use</div>
            {/if}
            <div class="my-1">
                <select name="dl" bind:value={configuration.d.l} class="form-control form-control-sh-10 rounded-md shadow-sm border-gray-300m">
                    <option value={1}>Verbose</option>
                    <option value={2}>Debug</option>
                    <option value={3}>Info</option>
                    <option value={4}>Warning</option>
                </select>
            </div>
            {/if}
        </div>
    </div>
    <button type="submit" class="font-bold py-2 px-4 rounded bg-blue-500 text-white float-right mr-3">Save</button>
</form>
<Mask active={loadingOrSaving} message="Loading configuration"/>
