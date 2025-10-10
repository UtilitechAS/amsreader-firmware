<script>
    import { getConfiguration, configurationStore } from './ConfigurationStore'
    import { sysinfoStore, networksStore } from './DataStores.js';
    import fetchWithTimeout from './fetchWithTimeout';
    import { translationsStore } from './TranslationService';
    import { wiki, ipPattern, asciiPattern, asciiPatternExt, charAndNumPattern, hexPattern, numPattern, wifiStateFromRssi } from './Helpers.js';
    import UartSelectOptions from './UartSelectOptions.svelte';
    import Mask from './Mask.svelte'
    import Badge from './Badge.svelte';
    import CountrySelectOptions from './CountrySelectOptions.svelte';
    import { Link, navigate } from 'svelte-navigator';
    import SubnetOptions from './SubnetOptions.svelte';
    import QrCode from 'svelte-qrcode';
    import WifiLowIcon from "./../assets/wifi-low-light.svg";
    import WifiMediumIcon from "./../assets/wifi-medium-light.svg";
    import WifiHighIcon from "./../assets/wifi-high-light.svg";
    import WifiOffIcon from "./../assets/wifi-off-light.svg";

    export let basepath = "/";
    export let sysinfo = {};
    export let data;
  
    const WIFI_ICON_MAP = {
        high: WifiHighIcon,
        medium: WifiMediumIcon,
        low: WifiLowIcon,
        off: WifiOffIcon
    };

    let wifiIcon = WIFI_ICON_MAP.off;
    let wifiTitle = "Wi-Fi offline";
  
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

    let cloudenabled = false;

    let configuration;
    let languages = [];
    configurationStore.subscribe(update => {
        if(update.version) {
            cloudenabled = update?.c?.e;
            configuration = update;
            loading = false;
            languages = [{ code: 'en', name: 'English'}];
            if(!configuration?.fw) {
                configuration = {
                    ...configuration,
                    fw: {
                        a: false,
                        s: 2,
                        e: 3
                    }
                };
            } else {
                configuration.fw = {
                    a: !!configuration.fw.a,
                    s: Number(configuration.fw.s ?? 2),
                    e: Number(configuration.fw.e ?? 3)
                };
            }
            if(configuration?.u?.lang && configuration.u.lang != 'en') {
                languages.push({ code: configuration.u.lang, name: translations.language?.name ?? "Unknown"})
            }
            languages.push({ code: 'hub', name: 'Load from server'})
        }
    });
    getConfiguration();

    let manual = true;
    let networks = {};
    let networkSignalInfos = [];
    networksStore.subscribe(update => {
        manual = true;
        for (let i = 0; i < update.n.length; i++) {
            let net = update.n[i];
            if(net.s == configuration?.w?.s) {
                manual = false;
                break;
            }
        }
        networks = update;
    });

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
            configuration.u.lang = translations.language.code;
        }
    }

    async function enablePriceFetch() {
        configuration.p.e = true;
    }

    function formatHour(hour) {
        let value = Number(hour ?? 0);
        if(!Number.isFinite(value)) {
            value = parseInt(hour ?? 0, 10);
        }
        if(!Number.isFinite(value)) {
            value = 0;
        }
        const normalized = ((value % 24) + 24) % 24;
        return `${normalized.toString().padStart(2, '0')}:00`;
    }

    let gpioMax = 44;
    $: {
        gpioMax = sysinfo.chip == 'esp8266' ? 16 : sysinfo.chip == 'esp32s2' ? 44 : 39;
    }

    async function cloudBind() {
        const response = await fetchWithTimeout("cloudkey.json");
        if(response.status == 200) {
            let data = await response.json();
            window.open("https://www.amsleser.cloud/device/" + data.seed);
        } else {
            alert("Not able to bind to cloud");
        }
    }

    const _global = (window || global);
    _global.bindToCloud = function() {
        console.log("BIND CALLED");
    }
        $: {
                const { level, label } = wifiStateFromRssi(data?.r);
                wifiIcon = WIFI_ICON_MAP[level] ?? WIFI_ICON_MAP.off;
                wifiTitle = label;
        }

        $: networkSignalInfos = Array.isArray(networks?.n)
                ? networks.n.map((net) => {
                        const { level, label } = wifiStateFromRssi(net?.r);
                        return {
                                icon: WIFI_ICON_MAP[level] ?? WIFI_ICON_MAP.off,
                                title: label
                        };
                })
                : [];
</script>

<form on:submit|preventDefault={handleSubmit} autocomplete="off">
    <div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
        {#if configuration?.g}
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.general?.title ?? "General"}</strong>
            <a href="{wiki('General-configuration')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="g" value="true"/>
            <div class="my-1">
                <div class="flex">
                    <div>
                        {translations.conf?.general?.hostname ?? "Hostname"}<br/>
                        <input name="gh" bind:value={configuration.g.h} type="text" class="in-f w-full" pattern={charAndNumPattern}/>
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
                        <select name="pr" bind:value={configuration.p.r} on:change={enablePriceFetch} class="in-f w-full">
                            <optgroup label="Norway">
                                {#if !configuration.p.t}
                                    <option value="NO1S">NO1 with support</option>
                                    <option value="NO2S">NO2 with support</option>
                                    <option value="NO3S">NO3 with support</option>
                                    <option value="NO4S">NO4 with support</option>
                                    <option value="NO5S">NO5 with support</option>
                                {/if}
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
			                <option value="10YSI-ELES-----O">Slovenia</option>
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
                <br/><input name="pt" bind:value={configuration.p.t} type="text" class="in-s" placeholder={translations.conf?.price?.api_key_placeholder ?? ""} pattern={charAndNumPattern}/>
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
                <input name="gu" bind:value={configuration.g.u} type="text" class="in-s" maxlength="36" pattern={asciiPattern}/>
            </div>
            <div class="my-1">
                {translations.conf?.general?.security?.password ?? "Password"}<br/>
                <input name="gp" bind:value={configuration.g.p} type="password" class="in-s" maxlength="36" pattern={asciiPattern}/>
            </div>
            {/if}
            <div class="my-1">
                {translations.conf?.general?.context ?? "Context"}<br/>
                <input name="gc" bind:value={configuration.g.c} type="text" pattern={charAndNumPattern} placeholder={translations.conf?.general?.context_placeholder ?? "/"} class="in-s" maxlength="36"/>
            </div>
        </div>
        {/if}
        {#if configuration?.fw}
        <div class="cnt">
            <strong class="text-sm">Firmware updates</strong>
            <input type="hidden" name="fw" value="true"/>
            <div class="my-1">
                <label>
                    <input type="checkbox" name="fwa" value="true" bind:checked={configuration.fw.a} class="rounded mb-1"/>
                    Enable nightly auto-updates
                </label>
            </div>
            <div class="my-1 grid grid-cols-2 gap-2">
                <div>
                    Start hour<br/>
                    <input name="fws" type="number" min="0" max="23" class="in-s w-full" bind:value={configuration.fw.s} disabled={!configuration.fw.a}/>
                </div>
                <div>
                    End hour<br/>
                    <input name="fwe" type="number" min="0" max="23" class="in-s w-full" bind:value={configuration.fw.e} disabled={!configuration.fw.a}/>
                </div>
            </div>
            <div class="my-1 text-xs text-gray-500">
                When enabled, the device will install available updates once per night between {formatHour(configuration.fw.s)} and {formatHour(configuration.fw.e)} using its local time zone.
            </div>
            <div class="my-1 text-xs">
                {#if sysinfo?.upgrade?.m === true}
                    <span class="text-green-600">Latest firmware already installed.</span>
                {:else if sysinfo?.upgrade?.n}
                    Latest available: {sysinfo.upgrade.n}
                {:else}
                    Checking for updates…
                {/if}
            </div>
        </div>
        {/if}
        {#if configuration?.m}
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
                    {#if sysinfo?.features?.includes('kmp')}
                    <option value={9}>KMP</option>
                    {/if}
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
                            {#each [3,12,24,48,96,192,384,576,1152] as b}
                            <option value={b*100}>{b*100}</option>
                            {/each}
                        </select>
                        <select name="mp" bind:value={configuration.m.p} class="in-m" disabled={configuration.m.b == 0}>
                            <option value={0} disabled={configuration.m.b != 0}>-</option>
                            <option value={2}>7N1</option>
                            <option value={3}>8N1</option>
                            <option value={7}>8N2</option>
                            <option value={10}>7E1</option>
                            <option value={11}>8E1</option>
                        </select>
                        <input name="ms" type="number" bind:value={configuration.m.s} min={64} max={sysinfo.chip == 'esp8266' ? configuration.i.h.p == 3 || configuration.i.h.p == 113 ? 512 : 256 : 4096} step={64} class="in-l tr w-1/2">
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
                <br/><input name="mek" bind:value={configuration.m.e.k} type="text" class="in-s" pattern={hexPattern}/>
                {/if}
            </div>
            {#if configuration.m.e.e}
            <div class="my-1">
                {translations.conf?.meter?.authkey ?? "Authentication key"}<br/>
                <input name="mea" bind:value={configuration.m.e.a} type="text" class="in-s" pattern={hexPattern}/>
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
        {/if}
        {#if configuration?.w}
        <div class="cnt">
            <strong class="text-sm">{translations.conf?.connection?.title ?? "Connection"}</strong>
            <a href="{wiki('Network-connection')}" target="_blank" class="float-right">&#9432;</a>
            <input type="hidden" name="w" value="true"/>
            {#if configuration.n.c == 1 || configuration.n.c == 2}
                <div class="my-1">
                    {translations.conf?.connection?.ssid ?? "Network name (SSID)"}
                    <br/>
                    {#if networks?.c == -1}
                        <div class="text-sm italic text-slate-500">{translations.conf?.connection?.searching ?? "Scanning for networks..."}</div>
                    {/if}
                    {#if networks?.n?.length}
                        <div class="mt-2 space-y-2">
                            {#each networks.n as network, index (network.s ?? index)}
                                <label class="group flex items-center justify-between gap-4 rounded-xl border border-slate-200 bg-white/80 px-3 py-2 shadow-sm transition hover:border-blue-400 hover:bg-blue-50 hover:shadow-md dark:border-slate-700 dark:bg-slate-800/70 dark:hover:border-blue-300 dark:hover:bg-slate-800">
                                    <span class="flex items-center gap-3">
                                        <input
                                            type="radio"
                                            class="h-4 w-4 text-blue-600 focus:ring-blue-500"
                                            name="ws"
                                            value={network.s}
                                            bind:group={configuration.w.s}/>
                                        <span class="flex flex-col">
                                            <span class="font-medium text-slate-800 dark:text-slate-100">{network.s || (translations.conf?.connection?.hidden_ssid ?? "Hidden network")}</span>
                                            <span class="text-xs text-slate-500 dark:text-slate-400">{networkSignalInfos[index]?.title ?? (translations.conf?.connection?.wifi_offline ?? "Signal unavailable")}</span>
                                        </span>
                                    </span>
                                    <div class="flex items-center gap-2">
                                        <img class="h-6 w-6 opacity-80 group-hover:opacity-100" src={networkSignalInfos[index]?.icon ?? WIFI_ICON_MAP.off} alt={networkSignalInfos[index]?.title ?? 'Wi-Fi offline'} title={networkSignalInfos[index]?.title ?? 'Wi-Fi offline'}/>
                                        {#if configuration.w.s === network.s}
                                            <span class="rounded-full bg-blue-100 px-2 py-0.5 text-xs font-medium text-blue-600 group-hover:bg-blue-200 dark:bg-blue-400/20 dark:text-blue-200">{translations.conf?.connection?.selected ?? "Selected"}</span>
                                        {/if}
                                    </div>
                                </label>
                            {/each}
                        </div>
                    {:else if networks?.c != -1}
                        <div class="text-sm italic text-slate-500">{translations.conf?.connection?.no_networks ?? "No networks found"}</div>
                    {/if}
                </div>
                <div class="my-1">
                    {translations.conf?.connection?.psk ?? "Password"}<br/>
                    <input name="wp" bind:value={configuration.w.p} type="password" class="in-s" pattern={asciiPatternExt}/>
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
        {/if}
        {#if configuration?.q}
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
                    <input name="qh" bind:value={configuration.q.h} type="text" class="in-f w-2/3" pattern={asciiPattern}/>
                    <input name="qp" bind:value={configuration.q.p} type="number" min="1024" max="65535" class="in-l tr w-1/3"/>
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
                <input name="qu" bind:value={configuration.q.u} type="text" class="in-s" pattern={asciiPatternExt}/>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.pass ?? "Password"}<br/>
                <input name="qa" bind:value={configuration.q.a} type="password" class="in-s" pattern={asciiPatternExt}/>
            </div>
            <div class="my-1 flex">
                <div>
                    {translations.conf?.mqtt?.id ?? "Client ID"}<br/>
                    <input name="qc" bind:value={configuration.q.c} type="text" class="in-f w-full" required={configuration.q.h} pattern={charAndNumPattern}/>
                </div>
                <div>
                    {translations.conf?.mqtt?.payload ?? "Payload"}<br/>
                    <select name="qm" bind:value={configuration.q.m} class="in-l">
                        <option value={1}>Raw (minimal)</option>
                        <option value={2}>Raw (full)</option>
                        <option value={3}>Domoticz</option>
                        <option value={4}>Home-Assistant</option>
                        <option value={0}>JSON (classic)</option>
                        <option value={5}>JSON (multi topic)</option>
                        <option value={6}>JSON (flat)</option>
                        <option value={255}>HEX dump</option>
                    </select>
                </div>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.publish ?? "Publish topic"}<br/>
                <input name="qb" bind:value={configuration.q.b} type="text" class="in-s" pattern={asciiPattern}/>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.subscribe ?? "Subscribe topic"}<br/>
                <input name="qr" bind:value={configuration.q.r} type="text" class="in-s" pattern={asciiPattern} placeholder="{configuration.q.b}/command"/>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.update ?? "Update method"}
                <span class="float-right">{translations.conf?.mqtt?.interval ?? "Interval"}</span>
                <div class="flex">
                    <select name="qt" bind:value={configuration.q.t} class="in-f w-1/2">
                        <option value={0}>{translations.conf?.mqtt?.realtime ?? "Real time"}</option>
                        <option value={1}>{translations.conf?.mqtt?.interval ?? "Interval"}</option>
                    </select>
                    <input name="qd" bind:value={configuration.q.d} type="number" min="1" max="3600" class="in-l tr w-1/2" disabled={configuration?.q?.t != 1}/>
                </div>
            </div>
            <div class="my-1">
                {translations.conf?.mqtt?.timeout ?? "Timeout"}
                <span class="float-right">{translations.conf?.mqtt?.keepalive ?? "Keep-alive"}</span>
                <div class="flex">
                    <input name="qi" bind:value={configuration.q.i} type="number" min="500" max="10000" class="in-f tr w-1/2"/>
                    <input name="qk" bind:value={configuration.q.k} type="number" min="5" max="180" class="in-l tr w-1/2"/>
                </div>
            </div>
        </div>
        {/if}
        {#if configuration?.q?.m == 3}
            <div class="cnt">
                <strong class="text-sm">{translations.conf?.mqtt?.domoticz?.title ?? "Domoticz"}</strong>
                <a href="{wiki('MQTT-configuration#domoticz')}" target="_blank" class="float-right">&#9432;</a>
                <input type="hidden" name="o" value="true"/>
                <div class="my-1 flex">
                    <div class="w-1/2">
                        {translations.conf?.mqtt?.domoticz?.eidx ?? "Electricity IDX"}<br/>
                        <input name="oe" bind:value={configuration.o.e} type="text" class="in-f tr w-full" pattern={numPattern}/>
                    </div>
                    <div class="w-1/2">
                        {translations.conf?.mqtt?.domoticz?.cidx ?? "Current IDX"}<br/>
                        <input name="oc" bind:value={configuration.o.c} type="text" class="in-l tr w-full" pattern={numPattern}/>
                    </div>
                </div>
                <div class="my-1">
                    {translations.conf?.mqtt?.domoticz?.vidx ?? "Voltage IDX"}: L1, L2 & L3
                    <div class="flex">
                        <input name="ou1" bind:value={configuration.o.u1} type="text" class="in-f tr w-1/3" pattern={numPattern}/>
                        <input name="ou2" bind:value={configuration.o.u2} type="text" class="in-m tr w-1/3" pattern={numPattern}/>
                        <input name="ou3" bind:value={configuration.o.u3} type="text" class="in-l tr w-1/3" pattern={numPattern}/>
                    </div>
                </div>
            </div>
        {/if}
        {#if configuration?.q?.m == 4}
            <div class="cnt">
                <strong class="text-sm">{translations.conf?.mqtt?.ha?.title ?? "Home-Assistant"}</strong>
                <a href="{wiki('MQTT-configuration#home-assistant')}" target="_blank" class="float-right">&#9432;</a>
                <input type="hidden" name="h" value="true"/>
                <div class="my-1">
                    {translations.conf?.mqtt?.ha?.discovery ?? "Discovery topic prefix"}<br/>
                    <input name="ht" bind:value={configuration.h.t} type="text" class="in-s" placeholder="homeassistant" pattern={asciiPattern}/>
                </div>
                <div class="my-1">
                    {translations.conf?.mqtt?.ha?.hostname ?? "Hostname for URL"}<br/>
                    <input name="hh" bind:value={configuration.h.h} type="text" class="in-s" placeholder="{configuration.g.h}.local" pattern={asciiPattern}/>
                </div>
                <div class="my-1">
                    {translations.conf?.mqtt?.ha?.tag ?? "Name tag"}<br/>
                    <input name="hn" bind:value={configuration.h.n} type="text" class="in-s" pattern={asciiPattern}/>
                </div>
            </div>
        {/if}
        {#if configuration?.p?.r?.startsWith("NO") || configuration?.p?.r?.startsWith("10YNO") || configuration?.p?.r?.startsWith('10Y1001A1001A4')}
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
        {#if configuration?.u}
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
        {/if}
    </div>
    <div class="grid grid-cols-3 mt-3">
        {#if data?.a}
        <div>
            <button type="button" on:click={factoryReset} class="btn-red">{translations.conf?.btn_reset ?? "Factory reset"}</button>
        </div>
        <div class="text-center">
            <button type="button" on:click={askReboot} class="btn-yellow">{translations.btn?.reboot ?? "Reboot"}</button>
        </div>
        {/if}
        {#if configuration}
        <div class="text-right">
            <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
        </div>
        {/if}
    </div>
</form>

<Mask active={loading} message={translations.conf?.mask?.loading ?? "Loading"}/>
<Mask active={saving} message={translations.conf?.mask?.saving ?? "Saving"}/>
<Mask active={isFactoryReset} message={translations.conf?.mask?.reset ?? "Factory reset"}/>
<Mask active={isFactoryResetComplete} message={translations.conf?.mask?.reset_done ?? "Done"}/>