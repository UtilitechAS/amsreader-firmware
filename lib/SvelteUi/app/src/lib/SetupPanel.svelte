<script>
    import { onMount, onDestroy } from 'svelte';
    import { sysinfoStore, networksStore } from './DataStores.js';
    import { get } from 'svelte/store';
    import { configurationStore, getConfiguration } from './ConfigurationStore';
    import { translationsStore } from './TranslationService.js';
    import Mask from './Mask.svelte'
    import SubnetOptions from './SubnetOptions.svelte';
    import { scanForDevice, charAndNumPattern, asciiPatternExt, ipPattern, wifiStateFromRssi } from './Helpers.js';
    import WifiLowIcon from "./../assets/wifi-low-light.svg";
    import WifiMediumIcon from "./../assets/wifi-medium-light.svg";
    import WifiHighIcon from "./../assets/wifi-high-light.svg";
    import WifiOffIcon from "./../assets/wifi-off-light.svg";
    import { meterPresets, getMeterPresetById, buildMeterStateFromPreset, createMeterStateFromConfiguration, describePresetSummary } from './meterPresets.js';
    import NeasLogo from "./../assets/neas_logotype_white.svg";

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

    let configuration;
    let meterState;
    let meterStateInitialized = false;
    let selectedMeterPresetId = '';
    let selectedMeterPreset = null;

    const unsubscribeConfiguration = configurationStore.subscribe(update => {
        configuration = update;
        if(update?.m && !meterStateInitialized) {
            meterState = createMeterStateFromConfiguration(update.m);
            meterStateInitialized = true;
        }
    });

    onMount(() => {
        getConfiguration();
    });

    onDestroy(() => {
        if (typeof unsubscribeConfiguration === 'function') {
            unsubscribeConfiguration();
        }
    });

    function ensureMeterState() {
        if(!meterStateInitialized && configuration?.m) {
            meterState = createMeterStateFromConfiguration(configuration.m);
            meterStateInitialized = true;
        }
        if(!meterState && configuration?.m) {
            meterState = createMeterStateFromConfiguration(configuration.m);
        }
    }

    function handlePresetSelection(presetId) {
        ensureMeterState();
        selectedMeterPresetId = presetId;
        const preset = getMeterPresetById(presetId);
        selectedMeterPreset = preset ?? null;
        if(preset) {
            meterState = buildMeterStateFromPreset(meterState, preset);
        } else if(configuration?.m) {
            meterState = createMeterStateFromConfiguration(configuration.m);
        }
    }

    function clearPresetSelection() {
        selectedMeterPresetId = '';
        selectedMeterPreset = null;
        if(configuration?.m) {
            meterState = createMeterStateFromConfiguration(configuration.m);
        }
    }

    function buildMeterPayload() {
        ensureMeterState();
        const state = meterState ?? createMeterStateFromConfiguration(configuration?.m);
        if(!state) {
            return null;
        }
        const cfg = configuration?.m ?? {};
        return {
            source: state.source ?? cfg.o ?? 1,
            parser: state.parser ?? cfg.a ?? 0,
            baud: state.baud ?? cfg.b ?? 0,
            parity: state.parity ?? cfg.p ?? 3,
            invert: state.invert ?? cfg.i ?? false,
            distributionSystem: state.distributionSystem ?? cfg.d ?? 2,
            mainFuse: state.mainFuse ?? cfg.f ?? 0,
            production: state.production ?? cfg.r ?? 0,
            buffer: state.buffer ?? cfg.s ?? 256,
            encrypted: state.encrypted ?? cfg?.e?.e ?? false,
            encryptionKey: state.encryptionKey ?? cfg?.e?.k ?? '',
            authenticationKey: state.authenticationKey ?? cfg?.e?.a ?? '',
            multipliers: {
                watt: state.multipliers?.watt ?? cfg?.m?.w ?? 1,
                volt: state.multipliers?.volt ?? cfg?.m?.v ?? 1,
                amp: state.multipliers?.amp ?? cfg?.m?.a ?? 1,
                kwh: state.multipliers?.kwh ?? cfg?.m?.c ?? 1
            }
        };
    }

    let manual = false;
    let networks = {};
    networksStore.subscribe(update => {
        networks = update;
    });

    export let sysinfo = {}
    export let data = {}

    let staticIp = false;
    let connectionMode = 1;
    let loadingOrSaving = false;
    let reconnectTargets = [];
    let networkSignalInfos = [];
    let selectedSsid = '';
    let autoUpdateChoice = 'false';
    let lastAutoFlag;

    function updateSysinfo(url) {
        sysinfoStore.update(s => {
            s.trying = url;
            return s;
        });
    }

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target);
        const data = new URLSearchParams();
        for (let field of formData) {
            const [key, value] = field;
			data.append(key, typeof value === 'string' ? value : String(value))
        }

        const meterPayload = buildMeterPayload();
        if(meterPayload) {
            data.set('m', 'true');
            data.set('mo', String(meterPayload.source ?? 1));
            data.set('ma', String(meterPayload.parser ?? 0));
            data.set('mb', String(meterPayload.baud ?? 0));
            data.set('mp', String(meterPayload.parity ?? 3));
            data.set('mi', meterPayload.invert ? 'true' : 'false');
            data.set('md', String(meterPayload.distributionSystem ?? 2));
            data.set('mf', String(meterPayload.mainFuse ?? 0));
            data.set('mr', String(meterPayload.production ?? 0));
            data.set('ms', String(meterPayload.buffer ?? 256));
            if(meterPayload.encrypted) {
                data.set('me', 'true');
                data.set('mek', meterPayload.encryptionKey ?? '');
                data.set('mea', meterPayload.authenticationKey ?? '');
            }
            data.set('mmw', String(meterPayload.multipliers?.watt ?? 1));
            data.set('mmv', String(meterPayload.multipliers?.volt ?? 1));
            data.set('mma', String(meterPayload.multipliers?.amp ?? 1));
            data.set('mmc', String(meterPayload.multipliers?.kwh ?? 1));
        }

        const autoValue = formData.get('fwa');
        const autoDecision = autoValue === 'true';

        const response = await fetch('save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())
        loadingOrSaving = false;

    const hostFromForm = String(formData.get('sh') ?? '').trim();
        const message = typeof res.message === 'string' ? res.message : '';
        const hintParts = message.split('|').map(part => part.trim());
        const hintHost = hintParts[0] ?? '';
        const hintMdns = hintParts[1] ?? '';
        const hintIp = hintParts[2] ?? '';
        const fallbackHostname = hintHost || hostFromForm || sysinfo.hostname || (sysinfo?.chipId ? `ams-${sysinfo.chipId}` : 'ams-reader');
        const fallbackMdns = hintMdns || (fallbackHostname && fallbackHostname.indexOf('.') === -1 && fallbackHostname.indexOf(':') === -1 ? `${fallbackHostname}.local` : fallbackHostname);
    const staticIpValue = staticIp ? String(formData.get('si') ?? '').trim() : hintIp;
        const uniqueTargets = Array.from(new Set([staticIpValue, fallbackHostname, fallbackMdns].filter(val => val && val.length > 0)));
        reconnectTargets = res.reboot ? [...uniqueTargets] : [];

        sysinfoStore.update(s => {
            if(!s.net) s.net = {};
            const computedHostname = fallbackHostname || s.hostname || hostFromForm;
            s.hostname = computedHostname;
            if(!s.upgrade || typeof s.upgrade !== 'object') {
                s.upgrade = { x: -1, e: 0, f: null, t: null, m: false };
            }
            s.upgrade.auto = autoDecision;
            if(staticIp) {
                s.net.ip = staticIpValue;
                s.net.mask = formData.get('su');
                s.net.gw = formData.get('sg');
                s.net.dns1 = formData.get('sd');
            } else if(hintIp) {
                s.net.ip = hintIp;
            }
            s.targets = [...uniqueTargets];
            s.usrcfg = res.success;
            s.booting = res.reboot;
            return s;
        });

        const latestSysinfo = get(sysinfoStore);
        sysinfo = latestSysinfo;
        if(res.reboot) {
            setTimeout(() => scanForDevice(latestSysinfo, updateSysinfo), 5000);
        }
    }


    $: {
        const autoFlag = sysinfo?.upgrade?.auto;
        if(!loadingOrSaving && autoFlag !== lastAutoFlag) {
            if(autoFlag === true) {
                autoUpdateChoice = 'true';
            } else {
                autoUpdateChoice = 'false';
            }
            lastAutoFlag = autoFlag;
        }
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


<div class="min-h-screen bg-neas-green flex flex-col items-center p-4">
    <!-- Neas Logo -->
    <div class="mb-8">
        <svg class="w-24 h-24 text-blue-600 dark:text-blue-400" viewBox="0 0 100 100" fill="currentColor">
           <img alt="Neas logo" src={NeasLogo} class="w-full h-full" />
        </svg>
    </div>
    
    <!-- Main Card -->
    <div class="bg-neas-green dark:neas-green rounded-2xl max-w-md w-full p-8">
        <form on:submit|preventDefault={handleSubmit}>
            <input type="hidden" name="s" value="true"/>
            <input type="hidden" name="fw" value="true"/>
            
            <!-- Title -->
            <div class="text-center mb-8">
                <h1 class="text-2xl font-semibold text-slate-900 dark:text-slate-100 mb-2">
                    {translations.setup?.title ?? "WiFi Setup"}
                </h1>
                <p class="text-sm text-slate-600 dark:text-slate-400">
                    Connect your device to the internet
                </p>
            </div>

                <!-- WiFi Network Selection -->
                <div class="mb-6">
                    <div class="flex items-center justify-between mb-3">
                        <span class="block text-sm font-medium text-slate-900 dark:text-slate-100">
                            {translations.conf?.connection?.ssid ?? "Select Network"}
                        </span>
                        <button type="button" 
                                class="text-xs text-blue-600 hover:text-blue-700 dark:text-blue-400 dark:hover:text-blue-300 font-medium"
                                on:click={() => manual = !manual}>
                            {manual ? "Show Networks" : "Manual Entry"}
                        </button>
                    </div>
                    
                    {#if manual}
                        <input name="ss" 
                               type="text" 
                               pattern={asciiPatternExt} 
                               placeholder="Enter network name (SSID)"
                               class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                               required={connectionMode == 1 || connectionMode == 2}/>
                    {:else}
                        {#if networks?.c == -1}
                            <div class="flex items-center justify-center py-8 text-slate-500 dark:text-slate-400">
                                <div class="animate-spin rounded-full h-6 w-6 border-b-2 border-blue-600 mr-3"></div>
                                <span class="text-sm">{translations.conf?.connection?.searching ?? "Scanning for networks..."}</span>
                            </div>
                        {/if}
                        {#if networks?.n?.length}
                            <div class="space-y-2 max-h overflow-y-auto">
                                {#each networks.n as network, index (network.s ?? index)}
                                    <label class="group flex items-center justify-between gap-4 rounded-lg border border-slate-200 bg-slate-50/50 px-4 py-3 shadow-sm transition hover:border-blue-400 hover:bg-blue-50 hover:shadow-md dark:border-slate-700 dark:bg-slate-800/50 dark:hover:border-blue-300 dark:hover:bg-slate-700 cursor-pointer">
                                        <span class="flex items-center gap-3 flex-1 min-w-0">
                                            <input
                                                type="radio"
                                                class="h-4 w-4 text-blue-600 focus:ring-blue-500"
                                                name="ss"
                                                value={network.s}
                                                bind:group={selectedSsid}
                                                required={connectionMode == 1 || connectionMode == 2}/>
                                            <span class="flex flex-col min-w-0 flex-1">
                                                <span class="font-medium text-slate-900 dark:text-slate-100 truncate">
                                                    {network.s || (translations.conf?.connection?.hidden_ssid ?? "Hidden network")}
                                                </span>
                                                <span class="text-xs text-slate-500 dark:text-slate-400">
                                                    {networkSignalInfos[index]?.title ?? (translations.conf?.connection?.wifi_offline ?? "Signal unavailable")}
                                                </span>
                                            </span>
                                        </span>
                                        <div class="flex items-center gap-2 flex-shrink-0">
                                            <img class="h-5 w-5 opacity-70 group-hover:opacity-100" 
                                                 src={networkSignalInfos[index]?.icon ?? WIFI_ICON_MAP.off} 
                                                 alt={networkSignalInfos[index]?.title ?? 'Wi-Fi offline'} 
                                                 title={networkSignalInfos[index]?.title ?? 'Wi-Fi offline'}/>
                                            {#if selectedSsid === network.s}
                                                <div class="w-2 h-2 bg-blue-600 rounded-full"></div>
                                            {/if}
                                        </div>
                                    </label>
                                {/each}
                            </div>
                        {:else if networks?.c != -1}
                            <div class="text-center py-8 text-slate-500 dark:text-slate-400">
                                <svg class="w-12 h-12 mx-auto mb-3 opacity-50" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                                    <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M8.111 16.404a5.5 5.5 0 017.778 0M12 20h.01m-7.08-7.071c3.904-3.905 10.236-3.905 14.141 0M1.394 9.393c5.857-5.857 15.355-5.857 21.213 0"/>
                                </svg>
                                <p class="text-sm">{translations.conf?.connection?.no_networks ?? "No networks found"}</p>
                                <button type="button" 
                                        class="mt-2 text-xs text-blue-600 hover:text-blue-700 dark:text-blue-400 dark:hover:text-blue-300"
                                        on:click={() => manual = true}>
                                    Enter network manually
                                </button>
                            </div>
                        {/if}
                    {/if}
                </div>
                <!-- WiFi Password -->
                <div class="mb-6">
                    <label for="wifi-password" class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2">
                        {translations.conf?.connection?.psk ?? "WiFi Password"}
                    </label>
                    <input id="wifi-password"
                           name="sp" 
                           type="password" 
                           pattern={asciiPatternExt} 
                           class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                           placeholder="Enter WiFi password"
                           autocomplete="off" 
                           required={connectionMode == 2}/>
                </div>
            
            <!-- Device Hostname -->
            <div class="mb-6">
                <label for="hostname-input" class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2">
                    {translations.conf?.general?.hostname ?? "Device Name"}
                </label>
                <input id="hostname-input"
                       name="sh" 
                       bind:value={sysinfo.hostname} 
                       type="text" 
                       class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                       maxlength="32" 
                       pattern={charAndNumPattern} 
                       placeholder="e.g., ams-reader-01" 
                       autocomplete="off"/>
                <p class="mt-1 text-xs text-slate-500 dark:text-slate-400">Optional: Give your device a custom name</p>
            </div>
            <!-- Advanced Network Settings Toggle -->
            <div class="mb-6">
                <button type="button" 
                        class="flex items-center justify-between w-full p-3 text-left bg-slate-50 dark:bg-slate-700/50 rounded-lg border border-slate-200 dark:border-slate-600 hover:bg-slate-100 dark:hover:bg-slate-700 transition-colors"
                        on:click={() => staticIp = !staticIp}>
                    <span class="text-sm font-medium text-slate-900 dark:text-slate-100">
                        Advanced Network Settings
                    </span>
                    <svg class="w-4 h-4 text-slate-500 dark:text-slate-400 transform transition-transform {staticIp ? 'rotate-180' : ''}" 
                         fill="none" stroke="currentColor" viewBox="0 0 24 24">
                        <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7"/>
                    </svg>
                </button>
                
                {#if staticIp}
                    <div class="mt-4 space-y-4 p-4 bg-slate-50/50 dark:bg-slate-800/50 rounded-lg border border-slate-200 dark:border-slate-600">
                        <!-- Static IP -->
                        <div>
                            <label for="static-ip" class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2">
                                Static IP Address
                            </label>
                            <div class="flex space-x-2">
                                <input id="static-ip"
                                       name="si" 
                                       type="text" 
                                       class="flex-1 px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                                       required={staticIp} 
                                       pattern={ipPattern}
                                       placeholder="192.168.1.100"/>
                                <select name="su" 
                                        class="px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                                        required={staticIp}>
                                    <SubnetOptions/>
                                </select>
                            </div>
                        </div>
                        
                        <!-- Gateway and DNS -->
                        <div class="grid grid-cols-2 gap-4">
                            <div>
                                <label for="gateway" class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2">
                                    {translations.conf?.network?.gw ?? "Gateway"}
                                </label>
                                <input id="gateway"
                                       name="sg" 
                                       type="text" 
                                       class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                                       pattern={ipPattern}
                                       placeholder="192.168.1.1"/>
                            </div>
                            <div>
                                <label for="dns" class="block text-sm font-medium text-slate-900 dark:text-slate-100 mb-2">
                                    {translations.conf?.network?.dns ?? "DNS Server"}
                                </label>
                                <input id="dns"
                                       name="sd" 
                                       type="text" 
                                       class="w-full px-3 py-2 border border-slate-300 dark:border-slate-600 rounded-lg bg-white dark:bg-slate-700 text-slate-900 dark:text-slate-100 focus:ring-2 focus:ring-blue-500 focus:border-blue-500 transition-colors" 
                                       pattern={ipPattern}
                                       placeholder="8.8.8.8"/>
                            </div>
                        </div>
                    </div>
                {/if}
            </div>
            
            <!-- Submit Button -->
            <div class="text-center">
                <button type="submit" 
                        class="w-full bg-neas-lightgreen hover:bg-neas-lightgreen-30 disabled:bg-slate-400 disabled:cursor-not-allowed text-white font-medium py-3 px-6 rounded-lg transition-colors focus:outline-none focus:ring-2 focus:ring-blue-500 focus:ring-offset-2 dark:focus:ring-offset-slate-800">
                    {translations.btn?.save ?? "Connect & Continue"}
                </button>
            </div>
            {#if reconnectTargets.length}
                <div class="mt-6 p-4 bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 rounded-lg">
                    <div class="flex items-start">
                        <svg class="w-5 h-5 text-green-600 dark:text-green-400 mt-0.5 mr-3 flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                            <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M9 12l2 2 4-4m6 2a9 9 0 11-18 0 9 9 0 0118 0z"/>
                        </svg>
                        <div>
                            <h4 class="text-sm font-medium text-green-800 dark:text-green-200 mb-2">
                                {translations.setup?.reconnect?.title ?? "Setup Complete!"}
                            </h4>
                            <p class="text-sm text-green-700 dark:text-green-300 mb-3">
                                {translations.setup?.reconnect?.info ?? "Device is rebooting. You can reconnect using these addresses:"}
                            </p>
                            <div class="space-y-1">
                                {#each reconnectTargets as target}
                                    <div class="flex items-center">
                                        <code class="text-xs bg-green-100 dark:bg-green-800/50 text-green-800 dark:text-green-200 px-2 py-1 rounded font-mono">
                                            {target.startsWith('http://') || target.startsWith('https://') ? target : `http://${target}`}
                                        </code>
                                    </div>
                                {/each}
                            </div>
                        </div>
                    </div>
                </div>
            {/if}
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message={translations.setup?.mask ?? "Connecting..."}/>
