<script>
    import { getConfiguration, configurationStore } from './ConfigurationStore'
    import { fetchWithTimeout, sysinfoStore } from './DataStores.js';
    import { translationsStore } from './TranslationService';
    import { wiki } from './Helpers.js';
    import UartSelectOptions from './UartSelectOptions.svelte';
    import Mask from './Mask.svelte'
    import Badge from './Badge.svelte';
    import CountrySelectOptions from './CountrySelectOptions.svelte';
    import { Link, navigate } from 'svelte-navigator';
    import SubnetOptions from './SubnetOptions.svelte';
    import QrCode from 'svelte-qrcode';
    import { scanForDevice } from './Helpers.js';

    export let basepath = "/";
    export let sysinfo = {};
  
    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let uiElements = [{
        name: 'Import gauge',
        key: 'i'
    },{
        name: 'Export gauge',
        key: 'e'
    },{
        name: 'Voltage',
        key: 'v'
    },{
        name: 'Amperage',
        key: 'a'
    },{
        name: 'Per phase',
        key: 'h'
    },{
        name: 'Power factor',
        key: 'f'
    },{
        name: 'Reactive',
        key: 'r'
    },{
        name: 'Realtime',
        key: 'c'
    },{
        name: 'Peaks',
        key: 't'
    },{
        name: 'Realtime plot',
        key: 'l'
    },{
        name: 'Price',
        key: 'p'
    },{
        name: 'Day plot',
        key: 'd'
    },{
        name: 'Month plot',
        key: 'm'
    },{
        name: 'Temperature plot',
        key: 's'
    },{
        name: 'Dark mode',
        key: 'k'
    }];

    let loading = true;
    let saving = false;

    let configuration = {
        g: {
            t: '', h: '', s: 0, u: '', p: ''
        },
        m: {
            b: 2400, p: 11, i: false, d: 0, f: 0, r: 0,
            e: { e: false, k: '', a: '' },
            m: { e: false, w: false, v: false, a: false, c: false }
        },
        w: { s: '', p: '', w: 0.0, z: 255, a: true, b: true },
        n: {
            m: '', i: '', s: '', g: '', d1: '', d2: '', d: false, n1: '', n2: '', h: false
        },
        q: {
            h: '', p: 1883, u: '', a: '', b: '',
            s: { e: false, c: false, r: true, k: false }
        },
        o: {
            e: '',
            c: '',
            u1: '',
            u2: '',
            u3: ''
        },
        t: {
            t: [0,0,0,0,0,0,0,0,0,0], h: 1
        },
        p: {
            e: false, t: '', r: '', c: '', m: 1.0, f: null
        },
        d: {
            s: false, t: false, l: 5
        },
        u: {
            i: 0, e: 0, v: 0, a: 0, r: 0, c: 0, t: 0, p: 0, d: 0, m: 0, s: 0, lang: 'en'
        },
        i: {
            h: { p: null, u: true },
            a: null,
            l: { p: null, i: false },
            r: { r: null, g: null, b: null, i: false },
            d: { d: null, b: 0 },
            t: { d: null, a: null },
            v: { p: null, d: { v: null, g: null }, o: null, m: null, b: null }
        },
        h: {
            t: '', h: '', n: ''
        },
        c: {
            e: false, i: null, es: null
        }
    };
    let languages = [];
    configurationStore.subscribe(update => {
        if(update.version) {
            configuration = update;
            loading = false;
            languages = [{ code: 'en', name: 'English'}];
            if(configuration.u.lang && configuration.u.lang != 'en') {
                languages.push({ code: configuration.u.lang, name: translations.language?.name ?? "Unknown"})
            }
            languages.push({ code: 'hub', name: 'Load from server'})
        }
    });
    getConfiguration();

    let isFactoryReset = false;
    let isFactoryResetComplete = false;
    async function factoryReset() {
        if(confirm("Factory reset?")) {
            isFactoryReset = true;
            const data = new URLSearchParams();
            data.append("perform", "true");
            const response = await fetch('reset', {
                method: 'POST',
                body: data
            });
            let res = (await response.json());
            isFactoryReset = false;
            isFactoryResetComplete = res.success;
        }
    }

    function updateSysinfo(url) {
        sysinfoStore.update(s => {
            s.trying = url;
            return s;
        });
    }

    async function handleSubmit(e) {
        saving = true;
		const formData = new FormData(e.target);
		const data = new URLSearchParams();
		for (let field of formData) {
			const [key, value] = field
			data.append(key, value)
		}

        const response = await fetch('save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())

        sysinfoStore.update(s => {
            s.hostname = formData.get('gh');
            s.usrcfg = res.success;
            s.booting = res.reboot;
            if(formData.get('nm') == 'static') {
                s.net.ip = formData.get('ni');
                s.net.mask = formData.get('nu');
                s.net.gw = formData.get('ng');
                s.net.dns1 = formData.get('nd');
            }
            setTimeout(scanForDevice, 5000, sysinfo, updateSysinfo);
            return s;
        });

        sysinfoStore.update(s => {
            s.booting = res.reboot;
            s.ui = configuration.u;
            return s;
        });

        saving = false;
        navigate(basepath);
	}

    async function reboot() {
      const response = await fetch('reboot', {
            method: 'POST'
        });
        let res = (await response.json())
    }

    const askReboot = function() {
      if(confirm('Reboot?')) {
        sysinfoStore.update(s => {
            s.booting = true;
            return s;
        });
        reboot();
      }
    }

    async function askDeleteCa() {
        if(confirm('Are you sure you want to delete CA?')) {
            const response = await fetch('mqtt-ca', {
                method: 'POST'
            });
            let res = (await response.text())
            configurationStore.update(c => {
                c.q.s.c = false;
                return c;
            });
        }
    }

    async function askDeleteCert() {
        if(confirm('Are you sure you want to delete cert?')) {
            const response = await fetch('mqtt-cert', {
                method: 'POST'
            });
            let res = (await response.text())
            configurationStore.update(c => {
                c.q.s.r = false;
                return c;
            });
        }
    }

    async function askDeleteKey() {
        if(confirm('Are you sure you want to delete key?')) {
            const response = await fetch('mqtt-key', {
                method: 'POST'
            });
            let res = (await response.text())
            configurationStore.update(c => {
                c.q.s.k = false;
                return c;
            });
        }
    }

    const updateMqttPort = function() {
        if(configuration.q.s.e) {
            if(configuration.q.p == 1883) configuration.q.p = 8883;
        } else {
            if(configuration.q.p == 8883) configuration.q.p = 1883;
        }
    }

    async function languageChanged() {
        if(configuration.u.lang == 'hub') {
            const response = await fetchWithTimeout("http://hub.amsleser.no/hub/language/list.json");
            languages = (await response.json())
        }
    }

    let gpioMax = 44;
    $: {
        gpioMax = sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39;
    }
</script>

<form on:submit|preventDefault={handleSubmit} autocomplete="off">
    <div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.general?.title ?? "General"}</strong>
            <a href="{wiki('General-configuration')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="g" value="true"/>
            <div class="my-1">
                <div class="flex">
                    <div>
                        {translations.conf?.general?.hostname ?? "Hostname"}<br/>
                        <input name="gh" bind:value={configuration.g.h} type="text" class="in-f w-full" pattern="[A-Za-z0-9-]+"/>
                    </div>
                    <div>
                        {translations.conf?.general?.timezone ?? "Time zone"}<br/>
                        <select name="gt" bind:value={configuration.g.t} class="in-l w-full">
                            <CountrySelectOptions/>
                        </select>
                    </div>
                </div>
            </div>
            <input type="hidden" name="p" value="true"/>
            <div class="my-1">
                <div class="flex">
                <div class="w-full">
                    {translations.conf?.price?.region ?? "Price region"}<br/>
                    <select name="pr" bind:value={configuration.p.r} class="in-f w-full">
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
                    {translations.conf?.price?.currency ?? "Currency"}<br/>
                    <select name="pc" bind:value={configuration.p.c} class="in-l">
                        {#each ["NOK","SEK","DKK","EUR","CHF"] as c}
                        <option value={c}>{c}</option>
                        {/each}
                    </select>
                </div>
            </div>
        </div>
            <div class="my-1">
                <Link to="/priceconfig" class="text-blue-600 hover:text-blue-800">{translations.conf?.price?.conf ?? "Configure"}</Link>
            </div>
            <div class="my-1">
                <label><input type="checkbox" name="pe" value="true" bind:checked={configuration.p.e} class="rounded mb-1"/> {translations.conf?.price?.enabled ?? "Enabled"}</label>
                {#if configuration.p.e && sysinfo.chip != 'esp8266'}
                <br/><input name="pt" bind:value={configuration.p.t} type="text" class="in-s" placeholder={translations.conf?.price?.api_key_placeholder ?? ""}/>
                {/if}
            </div>
            <div class="my-1">
                {translations.conf?.general?.security?.title ?? "Security"}<br/>
                <select name="gs" bind:value={configuration.g.s} class="in-s">
                    <option value={0}>{translations.conf?.general?.security?.none ?? "None"}</option>
                    <option value={1}>{translations.conf?.general?.security?.conf ?? "Conf"}</option>
                    <option value={2}>{translations.conf?.general?.security?.all ?? "All"}</option>
                </select>
            </div>
            {#if configuration.g.s > 0}
            <div class="my-1">
                {translations.conf?.general?.security?.username ?? "Username"}<br/>
                <input name="gu" bind:value={configuration.g.u} type="text" class="in-s" maxlength="36"/>
            </div>
            <div class="my-1">
                {translations.conf?.general?.security?.password ?? "Password"}<br/>
                <input name="gp" bind:value={configuration.g.p} type="password" class="in-s" maxlength="36"/>
            </div>
            {/if}
            <div class="my-1">
                {translations.conf?.general?.context ?? "Context"}<br/>
                <input name="gc" bind:value={configuration.g.c} type="text" pattern="[A-Za-z0-9]+" placeholder={translations.conf?.general?.context_placeholder ?? "/"} class="in-s" maxlength="36"/>
            </div>
        </div>
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.meter?.title ?? "Meter"}</strong>
            <a href="{wiki('Meter-configuration')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="m" value="true"/>
            <input type="hidden" name="mo" value="1"/>
            <div class="my-1">
                {translations.conf?.meter?.comm?.title ?? "Communication"}<br/>
                <select name="ma" bind:value={configuration.m.a} class="in-s">
                    <option value={0}>{translations.conf?.meter?.comm?.passive ?? "Passive"}</option>
                    <option value={2}>{translations.conf?.meter?.comm?.pulse ?? "Pulse"}</option>
                </select>
            </div>
            {#if configuration.m.a === 2}
                <div class="my-1">
                    <span>{translations.conf?.meter?.pulses ?? "Pulses per kWh"}</span>
                    <input name="mb" bind:value={configuration.m.b} class="in-s tr" type="number" min={1} max={3600}/>
                </div>
            {:else}
                <div class="my-1">
                    <span class="float-right">{translations.conf?.meter?.buffer ?? "Buffer size"}</span>
                    <span>{translations.conf?.meter?.serial ?? "Serial conf."}</span>
                    <label class="mt-2 ml-3 whitespace-nowrap"><input name="mi" value="true" bind:checked={configuration.m.i} type="checkbox" class="rounded mb-1"/> {translations.conf?.meter?.inverted ?? "inverted"}</label>
                    <div class="flex w-full">
                        <select name="mb" bind:value={configuration.m.b} class="in-f tr w-1/2">
                            <option value={0} disabled={configuration.m.b != 0}>Autodetect</option>
                            {#each [24,48,96,192,384,576,1152] as b}
                            <option value={b*100}>{b*100}</option>
                            {/each}
                        </select>
                        <select name="mp" bind:value={configuration.m.p} class="in-m" disabled={configuration.m.b == 0}>
                            <option value={0} disabled={configuration.m.b != 0}>-</option>
                            <option value={2}>7N1</option>
                            <option value={3}>8N1</option>
                            <option value={10}>7E1</option>
                            <option value={11}>8E1</option>
                        </select>
                        <input name="ms" type="number" bind:value={configuration.m.s} min={64} max={sysinfo.chip == 'esp8266' ? configuration.i.h.p == 3 || configuration.i.h.p == 113 ? 512 : 128 : 4096} step={64} class="in-l tr w-1/2">
                    </div>
                </div>
            {/if}
            <div class="my-1">
                {translations.common?.voltage ?? "Voltage"}<br/>
                <select name="md" bind:value={configuration.m.d} class="in-s">
                    <option value={2}>400V (TN)</option>
                    <option value={1}>230V (IT/TT)</option>
                </select>
            </div>
            <div class="my-1 flex">
                <div class="mx-1">
                    {translations.conf?.meter?.fuse ?? "Main fuse"}<br/>
                    <label class="flex">
                        <input name="mf" bind:value={configuration.m.f} type="number" min="5" max="65535" class="in-f tr w-full"/>
                        <span class="in-post">A</span>
                    </label>
                </div>
                <div class="mx-1">
                    {translations.conf?.meter?.prod ?? "Production"}<br/>
                    <label class="flex">
                        <input name="mr" bind:value={configuration.m.r} type="number" min="0" max="65535" class="in-f tr w-full"/>
                        <span class="in-post">kWp</span>
                    </label>
                </div>
            </div>
            <div class="my-1">
            </div>
            
            <div class="my-1">
                <label><input type="checkbox" name="me" value="true" bind:checked={configuration.m.e.e} class="rounded mb-1"/> {translations.conf?.meter?.encrypted ?? "Encrypted"}</label>
                {#if configuration.m.e.e}
                <br/><input name="mek" bind:value={configuration.m.e.k} type="text" class="in-s"/>
                {/if}
            </div>
            {#if configuration.m.e.e}
            <div class="my-1">
                {translations.conf?.meter?.authkey ?? "Authentication key"}<br/>
                <input name="mea" bind:value={configuration.m.e.a} type="text" class="in-s"/>
            </div>
            {/if}

            <label><input type="checkbox" name="mm" value="true" bind:checked={configuration.m.m.e} class="rounded mb-1"/> {translations.conf?.meter?.multipliers?.title ?? "Multipliers"}</label>
            {#if configuration.m.m.e}
            <div class="flex my-1">
                <div class="w-1/4">
                    {translations.conf?.meter?.multipliers?.watt ?? "Watt"}<br/>
                    <input name="mmw" bind:value={configuration.m.m.w} type="number" min="0.00" max="1000" step="0.001" class="in-f tr w-full"/>
                </div>
                <div class="w-1/4">
                    {translations.conf?.meter?.multipliers?.volt ?? "Volt"}<br/>
                    <input name="mmv" bind:value={configuration.m.m.v} type="number" min="0.00" max="1000" step="0.001" class="in-m tr w-full"/>
                </div>
                <div class="w-1/4">
                    {translations.conf?.meter?.multipliers?.amp ?? "Amp"}<br/>
                    <input name="mma" bind:value={configuration.m.m.a} type="number" min="0.00" max="1000" step="0.001" class="in-m tr w-full"/>
                </div>
                <div class="w-1/4">
                    {translations.conf?.meter?.multipliers?.kwh ?? "kWh"}<br/>
                    <input name="mmc" bind:value={configuration.m.m.c} type="number" min="0.00" max="1000" step="0.001" class="in-l tr w-full"/>
                </div>
            </div>
            {/if}
        </div>
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.connection?.title ?? "Connection"}</strong>
            <a href="{wiki('WiFi-configuration')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="w" value="true"/>
            <div class="my-1">
                <select name="nc" class="in-s" bind:value={configuration.n.c}>
                    <option value={1}>{translations.conf?.connection?.wifi ?? "WiFi"}</option>
                    <option value={2}>{translations.conf?.connection?.ap ?? "AP"}</option>
                    {#if sysinfo.if && sysinfo.if.eth}
                    <option value={3}>{translations.conf?.connection?.eth ?? "Ethernet"}</option>
                    {/if}
                </select>
            </div>
            {#if configuration.n.c == 1 || configuration.n.c == 2}
                <div class="my-1">
                    {translations.conf?.connection?.ssid ?? "SSID"}<br/>
                    <input name="ws" bind:value={configuration.w.s} type="text" class="in-s"/>
                </div>
                <div class="my-1">
                    {translations.conf?.connection?.psk ?? "Password"}<br/>
                    <input name="wp" bind:value={configuration.w.p} type="password" class="in-s"/>
                </div>
                <div class="my-1 flex">
                    <div class="w-1/2">
                        {translations.conf?.connection?.ps?.title ?? "Power saving"}<br/>
                        <select name="wz" bind:value={configuration.w.z} class="in-s">
                            <option value={255}>{translations.conf?.connection?.ps?.default ?? "Default"}</option>
                            <option value={0}>{translations.conf?.connection?.ps?.off ?? "Off"}</option>
                            <option value={1}>{translations.conf?.connection?.ps?.min ?? "Min"}</option>
                            <option value={2}>{translations.conf?.connection?.ps?.max ?? "Max"}</option>
                        </select>
                    </div>
                    <div class="ml-2 w-1/2">
                        {translations.conf?.connection?.pwr ?? "Power"}<br/>
                        <div class="flex">
                            <input name="ww" bind:value={configuration.w.w} type="number" min="0" max="20.5" step="0.5" class="in-f tr w-full"/>
                            <span class="in-post">dBm</span>
                        </div>
                    </div>
                </div>
                <div class="my-3">
                    <label><input type="checkbox" name="wb" value="true" bind:checked={configuration.w.b} class="rounded mb-1"/> {translations.conf?.connection?.tick_11b ?? "802.11b"}</label>
                </div>
            {/if}
        </div>
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.network?.title ?? "Network"}</strong>
            <a href="{wiki('Network-configuration')}" target="_blank" class="float-right">&#9432;</a>
            <div class="my-1">
                {translations.conf?.network?.ip ?? "IP"}<br/>
                <div class="flex">
                    <select name="nm" bind:value={configuration.n.m} class="in-f">
                        <option value="dhcp">{translations.conf?.network?.dhcp ?? "DHCP"}</option>
                        <option value="static">{translations.conf?.network?.static ?? "Static"}</option>
                    </select>
                    <input name="ni" bind:value={configuration.n.i} type="text" class="in-m w-full" disabled={configuration.n.m == 'dhcp'} required={configuration.n.m == 'static'}/>
                    <select name="ns" bind:value={configuration.n.s} class="in-l" disabled={configuration.n.m == 'dhcp'} required={configuration.n.m == 'static'}>
                        <SubnetOptions/>
                    </select>
                </div>
            </div>
            {#if configuration.n.m == 'static'}
            <div class="my-1">
                {translations.conf?.network?.gw ?? "Gateway"}<br/>
                <input name="ng" bind:value={configuration.n.g} type="text" class="in-s"/>
            </div>
            <div class="my-1">
                {translations.conf?.network?.dns ?? "DNS"}<br/>
                <div class="flex">
                    <input name="nd1" bind:value={configuration.n.d1} type="text" class="in-f w-full"/>
                    <input name="nd2" bind:value={configuration.n.d2} type="text" class="in-l w-full"/>
                </div>
            </div>
            {/if}
            <div class="my-1">
                <label><input name="nd" value="true" bind:checked={configuration.n.d} type="checkbox" class="rounded mb-1"/> {translations.conf?.network?.tick_mdns ?? "mDNS"}</label>
            </div>
            <input type="hidden" name="ntp" value="true"/>
            <div class="my-1">
                {translations.conf?.network?.ntp ?? "NTP"} <label class="ml-4"><input name="ntpd" value="true" bind:checked={configuration.n.h} type="checkbox" class="rounded mb-1"/> {translations.conf?.network?.tick_ntp_dhcp ?? "from DHCP"}</label><br/>
                <div class="flex">
                    <input name="ntph" bind:value={configuration.n.n1} type="text" class="in-s"/>
                </div>
            </div>
        </div>
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.mqtt?.title ?? "MQTT"}</strong>
            <a href="{wiki('MQTT-configuration')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="q" value="true"/>
            <div class="my-1">
                {translations.conf?.mqtt?.server ?? "Server"}
                {#if sysinfo.chip != 'esp8266'}
                <label class="float-right mr-3"><input type="checkbox" name="qs" value="true" bind:checked={configuration.q.s.e} class="rounded mb-1" on:change={updateMqttPort}/> SSL</label>
                {/if}
                <br/>
                <div class="flex">
                    <input name="qh" bind:value={configuration.q.h} type="text" class="in-f w-3/4"/>
                    <input name="qp" bind:value={configuration.q.p} type="number" min="1024" max="65535" class="in-l tr w-1/4"/>
                </div>
            </div>
            {#if configuration.q.s.e}
            <div class="my-1 flex">
                <span class="flex pr-2">
                    {#if configuration.q.s.c}
                    <span class="bd-on"><Link to="/mqtt-ca">{translations.conf?.mqtt?.ca_ok ?? "CA OK"}</Link></span>
                    <span class="bd-off"  on:click={askDeleteCa} on:keypress={askDeleteCa}>&#128465;</span>
                    {:else}
                    <Link to="/mqtt-ca"><Badge color="blue" text={translations.conf?.mqtt?.btn_ca_upload ?? "Upload CA"} title={translations.conf?.mqtt?.title_ca ?? ""}/></Link>
                    {/if}
                </span>

                <span class="flex pr-2">
                    {#if configuration.q.s.r}
                    <span class="bd-on"><Link to="/mqtt-cert">{translations.conf?.mqtt?.crt_ok ?? "Cert OK"}</Link></span>
                    <span class="bd-off" on:click={askDeleteCert} on:keypress={askDeleteCert}>&#128465;</span>
                    {:else}
                    <Link to="/mqtt-cert"><Badge color="blue" text={translations.conf?.mqtt?.btn_crt_upload ?? "Upload cert"} title={translations.conf?.mqtt?.title_crt ?? ""}/></Link>
                    {/if}
                </span>

                <span class="flex pr-2">
                    {#if configuration.q.s.k}
                    <span class="bd-on"><Link to="/mqtt-key">{translations.conf?.mqtt?.key_ok ?? "Key OK"}</Link></span>
                    <span class="bd-off" on:click={askDeleteKey} on:keypress={askDeleteKey}>&#128465;</span>
                    {:else}
                    <Link to="/mqtt-key"><Badge color="blue" text={translations.conf?.mqtt?.btn_key_upload ?? "Upload key"} title={translations.conf?.mqtt?.title_key ?? ""}/></Link>
                    {/if}
                </span>
            </div>
            {/if}
            <div class="my-1">
                {translations.conf?.mqtt?.user ?? "Username"}<br/>
                <input name="qu" bind:value={configuration.q.u} type="text" class="in-s"/>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.pass ?? "Password"}<br/>
                <input name="qa" bind:value={configuration.q.a} type="password" class="in-s"/>
            </div>
            <div class="my-1 flex">
                <div>
                    {translations.conf?.mqtt?.id ?? "Client ID"}<br/>
                    <input name="qc" bind:value={configuration.q.c} type="text" class="in-f w-full"/>
                </div>
                <div>
                    {translations.conf?.mqtt?.payload ?? "Payload"}<br/>
                    <select name="qm" bind:value={configuration.q.m} class="in-l">
                        <option value={0}>JSON</option>
                        <option value={1}>Raw (minimal)</option>
                        <option value={2}>Raw (full)</option>
                        <option value={3}>{translations.conf?.mqtt?.domoticz?.title ?? "Domoticz"}</option>
                        <option value={4}>{translations.conf?.mqtt?.ha?.title ?? "Home-Assistant"}</option>
                        <option value={255}>HEX dump</option>
                    </select>
                </div>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.publish ?? "Publish topic"}<br/>
                <input name="qb" bind:value={configuration.q.b} type="text" class="in-s"/>
            </div>
        </div>
        {#if configuration.q.m == 3}
            <div class="cnt">
                <strong class="text-sm">{translations.conf?.mqtt?.domoticz?.title ?? "Domoticz"}</strong>
                <a href="{wiki('MQTT-configuration#domoticz')}" target="_blank" class="float-right">&#9432;</a>
                <input type="hidden" name="o" value="true"/>
                <div class="my-1 flex">
                    <div class="w-1/2">
                        {translations.conf?.mqtt?.domoticz?.eidx ?? "Electricity IDX"}<br/>
                        <input name="oe" bind:value={configuration.o.e} type="text" class="in-f tr w-full"/>
                    </div>
                    <div class="w-1/2">
                        {translations.conf?.mqtt?.domoticz?.cidx ?? "Current IDX"}<br/>
                        <input name="oc" bind:value={configuration.o.c} type="text" class="in-l tr w-full"/>
                    </div>
                </div>
                <div class="my-1">
                    {translations.conf?.mqtt?.domoticz?.vidx ?? "Voltage IDX"}: L1, L2 & L3
                    <div class="flex">
                        <input name="ou1" bind:value={configuration.o.u1} type="text" class="in-f tr w-1/3"/>
                        <input name="ou2" bind:value={configuration.o.u2} type="text" class="in-m tr w-1/3"/>
                        <input name="ou3" bind:value={configuration.o.u3} type="text" class="in-l tr w-1/3"/>
                    </div>
                </div>
            </div>
        {/if}
        {#if configuration.q.m == 4}
            <div class="cnt">
                <strong class="text-sm">{translations.conf?.mqtt?.ha?.title ?? "Home-Assistant"}</strong>
                <a href="{wiki('MQTT-configuration#home-assistant')}" target="_blank" class="float-right">&#9432;</a>
                <input type="hidden" name="h" value="true"/>
                <div class="my-1">
                    {translations.conf?.mqtt?.ha?.discovery ?? "Discovery topic prefix"}<br/>
                    <input name="ht" bind:value={configuration.h.t} type="text" class="in-s" placeholder="homeassistant"/>
                </div>
                <div class="my-1">
                    {translations.conf?.mqtt?.ha?.hostname ?? "Hostname for URL"}<br/>
                    <input name="hh" bind:value={configuration.h.h} type="text" class="in-s" placeholder="{configuration.g.h}.local"/>
                </div>
                <div class="my-1">
                    {translations.conf?.mqtt?.ha?.tag ?? "Name tag"}<br/>
                    <input name="hn" bind:value={configuration.h.n} type="text" class="in-s"/>
                </div>
            </div>
        {/if}
        {#if configuration.c.es != null}
            <div class="cnt">
                <strong class="text-sm">{translations.conf?.cloud?.title ?? "Cloud connections"}</strong>
                <a href="{wiki('Cloud')}" target="_blank" class="float-right">&#9432;</a>
                <input type="hidden" name="c" value="true"/>
                <div class="my-1">
                    <label><input type="checkbox" name="ce" value="true" bind:checked={configuration.c.e} class="rounded mb-1"/> {translations.conf?.cloud?.ams ?? "AMS reader cloud"}</label>
                </div>
                <div class="my-1">
                    <label><input type="checkbox" class="rounded mb-1" name="ces" value="true" bind:checked={configuration.c.es}/> {translations.conf?.cloud?.es ?? "Energy Speedometer"}</label>
                    {#if configuration.c.es}
                        <div class="pl-5">MAC: {sysinfo.mac}</div>
                        <div class="pl-5">Meter ID: {sysinfo.meter.id ? sysinfo.meter.id : "missing, required"}</div>
                        {#if sysinfo.mac && sysinfo.meter.id}
                            <div class="pl-2">
                                <QrCode value='{'{'}"mac":"{sysinfo.mac}","meter":"{sysinfo.meter.id}"{'}'}'/>
                            </div>
                        {/if}
                    {/if}
                </div>
            </div>
        {/if}
        {#if configuration.p.r.startsWith("10YNO") || configuration.p.r.startsWith('10Y1001A1001A4')}
            <div class="cnt">
                <strong class="text-sm">{translations.conf?.thresholds?.title ?? "Thresholds"}</strong>
                <a href="{wiki('Threshold-configuration')}" target="_blank" class="float-right">&#9432;</a>
                <input type="hidden" name="t" value="true"/>
                <div class="flex flex-wrap my-1">
                    {#each {length: 9} as _, i}
                    <label class="flex w-40 m-1">
                        <span class="in-pre">{i+1}</span>
                        <input name="t{i}" bind:value={configuration.t.t[i]} type="number" min="0" max="65535" class="in-txt w-full"/>
                        <span class="in-post">kWh</span>
                    </label>
                    {/each}
                </div>
                <label class="flex m-1">
                    <span class="in-pre">{translations.conf?.thresholds?.avg ?? "Average of"}</span>
                    <input name="th" bind:value={configuration.t.h} type="number" min="0" max="255" class="in-txt tr w-full"/>
                    <span class="in-post">{translations.common?.hours ?? "hours"}</span>
                </label>
            </div>
        {/if}
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.ui?.title ?? "User interface"}</strong>
            <a href="{wiki('User-interface')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="u" value="true"/>
            <div class="flex flex-wrap">
                {#each uiElements as el}
                    <div class="w-1/2">
                        {translations.conf?.ui?.[el.key] ?? el.name}<br/>
                        <select name="u{el.key}" bind:value={configuration.u[el.key]} class="in-s">
                            <option value={0}>{translations.conf?.ui?.disabled ?? "Disabled"}</option>
                            <option value={1}>{translations.conf?.ui?.enabled ?? "Enabled"}</option>
                            <option value={2}>{translations.conf?.ui?.auto ?? "Auto"}</option>
                        </select>
                    </div>
                {/each}
                <div class="w-1/2">
                    {translations.conf?.ui?.lang ?? "Language"}
                    <select name="ulang" class="in-s" bind:value={configuration.u.lang} on:change={languageChanged}>
                        {#each languages as lang}
                            <option value={lang.code}>{lang.name}</option>
                        {/each}
                    </select>
                </div>
            </div>
        </div>
        {#if sysinfo.board > 20 || sysinfo.chip == 'esp8266' || configuration.i.d.d > 0}
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.hw?.title ?? "Hardware"}</strong>
            <a href="{wiki('GPIO-configuration')}" target="_blank" class="float-right">&#9432;</a>
            {#if sysinfo.board > 20}
            <input type="hidden" name="i" value="true"/>
            <div class="flex flex-wrap">
                <div class="w-1/3">
                    {translations.conf?.hw?.han?.rx ?? "HAN RX"}<br/>
                    <select name="ihp" bind:value={configuration.i.h.p} class="in-f w-full">
                        <UartSelectOptions chip={sysinfo.chip}/>
                    </select>
                </div>
                <div class="w-1/3">
                    {translations.conf?.hw?.han?.tx ?? "HAN TX"}<br/>
                    <select name="iht" bind:value={configuration.i.h.t} class="in-l w-full">
                        <UartSelectOptions chip={sysinfo.chip}/>
                    </select>
                </div>
                <div class="w-1/3">
                    <label class="ml-2"><input name="ihu" value="true" bind:checked={configuration.i.h.u} type="checkbox" class="rounded mb-1"/> {translations.conf?.hw?.han?.pullup ?? "pullup"}</label>
                </div>
            </div>
            <div class="flex flex-wrap">
                <div class="w-1/3">
                    {translations.conf?.hw?.ap_btn ?? "AP button"}<br/>
                    <input name="ia" bind:value={configuration.i.a} type="number" min="0" max={gpioMax} class="in-f tr w-full"/>
                </div>
                <div class="w-1/3">
                    {translations.conf?.hw?.led?.title ?? "LED"}<br/>
                    <div class="flex">
                        <input name="ilp" bind:value={configuration.i.l.p} type="number" min="0" max={gpioMax} class="in-l tr w-full"/>
                    </div>
                </div>
                <div class="w-1/3">
                    <label class="ml-4"><input name="ili" value="true" bind:checked={configuration.i.l.i} type="checkbox" class="rounded mb-1"/> {translations.conf?.hw?.led?.inverted ?? "inverted"}</label>
                </div>
                <div class="w-full">
                    {translations.conf?.hw?.led?.rgb ?? "RGB"}<label class="ml-4"><input name="iri" value="true" bind:checked={configuration.i.r.i} type="checkbox" class="rounded mb-1"/> {translations.conf?.hw?.led?.inverted ?? "inverted"}</label><br/>
                    <div class="flex">
                        <input name="irr" bind:value={configuration.i.r.r} type="number" min="0" max={gpioMax} class="in-f tr w-1/3"/>
                        <input name="irg" bind:value={configuration.i.r.g} type="number" min="0" max={gpioMax} class="in-m tr w-1/3"/>
                        <input name="irb" bind:value={configuration.i.r.b} type="number" min="0" max={gpioMax} class="in-l tr w-1/3"/>
                    </div>
                </div>
                <div class="w-full">
                    <div class="my-1 pr-1 w-1/3">
                        {translations.conf?.hw?.led?.disable ?? "LED dis. GPIO"}
                        <input name="idd" bind:value={configuration.i.d.d} type="number" min="0" max={gpioMax} class="in-s tr"/>
                    </div>
                </div>
                <div class="my-1 w-1/3">
                    {translations.conf?.hw?.temp ?? "Temperature"}<br/>
                    <input name="itd" bind:value={configuration.i.t.d} type="number" min="0" max={gpioMax} class="in-f tr w-full"/>
                </div>
                <div class="my-1 pr-1 w-1/3">
                    {translations.conf?.hw?.temp_analog ?? "Analog temp"}<br/>
                    <input name="ita" bind:value={configuration.i.t.a} type="number" min="0" max={gpioMax} class="in-l tr w-full"/>
                </div>
                {#if sysinfo.chip != 'esp8266'}
                <div class="my-1 pl-1 w-1/3">
                    {translations.conf?.hw?.vcc?.title ?? "Vcc"}<br/>
                    <input name="ivp" bind:value={configuration.i.v.p} type="number" min="0" max={gpioMax} class="in-s tr w-full"/>
                </div>
                {/if}
                {#if configuration.i.v.p > 0}
                <div class="my-1">
                    {translations.conf?.hw?.vcc?.divider ?? "Voltage divider"}<br/>
                    <div class="flex">
                        <input name="ivdv" bind:value={configuration.i.v.d.v} type="number" min="0" max="65535" class="in-f tr w-full" placeholder={translations.conf?.hw?.vcc?.div_vcc ?? "VCC"}/>
                        <input name="ivdg" bind:value={configuration.i.v.d.g} type="number" min="0" max="65535" class="in-l tr w-full" placeholder={translations.conf?.hw?.vcc?.div_gnd ?? "GND"}/>
                    </div>
                </div>
                {/if}
            </div> 
            {/if}
            {#if configuration.i.d.d > 0}
            <div class="my-1 w-full">
                {translations.conf?.hw?.led?.behaviour?.title ?? "LED behaviour"}
                <select name="idb" bind:value={configuration.i.d.b} class="in-s">
                    <option value={0}>{translations.conf?.hw?.led?.behaviour?.enabled ?? "Enabled"}</option>
                    <option value={1}>{translations.conf?.hw?.led?.behaviour?.disabled ?? "Disabled"}</option>
                </select>
            </div>
            {/if}
            {#if sysinfo.chip == 'esp8266'}
            <input type="hidden" name="iv" value="true"/>
            <div class="my-1 flex flex-wrap">
                <div class="w-1/3">
                    {translations.conf?.hw?.vcc?.offset ?? "Vcc offset"}<br/>
                    <input name="ivo" bind:value={configuration.i.v.o} type="number" min="0.0" max="3.5" step="0.01" class="in-f tr w-full"/>
                </div>
                <div class="w-1/3 pr-1">
                    {translations.conf?.hw?.vcc?.multiplier ?? "Multiplier"}<br/>
                    <input name="ivm" bind:value={configuration.i.v.m} type="number" min="0.1" max="10" step="0.01" class="in-l tr w-full"/>
                </div>
                {#if sysinfo.board == 2 || sysinfo.board == 100}
                <div class="w-1/3 pl-1">
                    {translations.conf?.hw?.vcc?.boot ?? "Boot limit"}<br/>
                    <input name="ivb" bind:value={configuration.i.v.b} type="number" min="2.5" max="3.5" step="0.1" class="in-s tr w-full"/>
                </div>
                {/if}
            </div>
            {/if}
        </div>
        {/if}
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.debug?.title ?? "Debugging"}</strong>
            <a href="https://amsleser.no/blog/post/24-telnet-debug" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="d" value="true"/>
            <div class="mt-3">
                <label><input type="checkbox" name="ds" value="true" bind:checked={configuration.d.s} class="rounded mb-1"/> {translations.conf?.debug?.enable ?? "Enable debugging"}</label>
            </div>
            {#if configuration.d.s}
            <div class="bd-red">{translations.conf?.debug?.danger ?? "Disable when done"}</div>
            <div class="my-1">
                <label><input type="checkbox" name="dt" value="true" bind:checked={configuration.d.t} class="rounded mb-1"/> {translations.conf?.debug?.telnet ?? "Enable telnet"}</label>
            </div>
            {#if configuration.d.t}
            <div class="bd-red">{translations.conf?.debug?.telnet_danger ?? "Disable when done"}</div>
            {/if}
            <div class="my-1">
                <select name="dl" bind:value={configuration.d.l} class="in-s">
                    <option value={1}>Verbose</option>
                    <option value={2}>Debug</option>
                    <option value={3}>Info</option>
                    <option value={4}>Warning</option>
                </select>
            </div>
            {/if}
        </div>
    </div>
    <div class="grid grid-cols-3 mt-3">
        <div>
            <button type="button" on:click={factoryReset} class="btn-red">{translations.conf?.btn_reset ?? "Factory reset"}</button>
        </div>
        <div class="text-center">
            <button type="button" on:click={askReboot} class="btn-yellow">{translations.btn?.reboot ?? "Reboot"}</button>
        </div>
        <div class="text-right">
            <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
        </div>
    </div>
</form>

<Mask active={loading} message={translations.conf?.mask?.loading ?? "Loading"}/>
<Mask active={saving} message={translations.conf?.mask?.saving ?? "Saving"}/>
<Mask active={isFactoryReset} message={translations.conf?.mask?.reset ?? "Factory reset"}/>
<Mask active={isFactoryResetComplete} message={translations.conf?.mask?.reset_done ?? "Done"}/>
