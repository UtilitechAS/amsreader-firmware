<script>
    import { sysinfoStore, networksStore, getNetworks } from '../lib/DataStores.js';
    import { translationsStore } from '../lib/TranslationService.js';
    import Mask from '../lib/Mask.svelte'
    import SubnetOptions from '../lib/SubnetOptions.svelte';
    import AddressBox from '../lib/AddressBox.svelte';
    import { scanForDevice, charAndNumPattern, asciiPatternExt, ipPattern } from '../lib/Helpers.js';

    const TEST_TIMEOUT = 30000; // Same as WIFI_TEST_TIMEOUT in the firmware
    const TEST_ERR_LOST = -1;   // Our own code: the phone lost contact with the device during the test

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });
    // Reassigned when translations arrive, so everything using it re-renders
    $: tr = (key, def, ...args) => {
        let s = key.split('.').reduce((o, k) => o?.[k], translations.setup) ?? def;
        args.forEach((a, i) => s = s.replace('{' + i + '}', a));
        return s;
    };

    let sysinfo = {};
    let hostname = '';
    let apName = '';
    sysinfoStore.subscribe(v => {
        sysinfo = v;
        // Read once only. The store refreshes every 10s and would overwrite what the user types.
        if(!hostname && v.hostname) {
            hostname = v.hostname;
            // The setup network has the device's unique name, unless a vendor has prepared it
            apName = v.vndcfg ? 'AMS2MQTT' : v.hostname;
        }
    });

    let networks = {};
    let ssid = '';
    let manual = false;
    networksStore.subscribe(update => {
        networks = update;
        manual = update?.c == 0;
        if(!update?.n?.some(n => n.s == ssid)) ssid = update?.n?.[0]?.s ?? '';
    });

    function rescan() {
        networksStore.set({c: -1, n: []});
        getNetworks().catch(() => networksStore.set({c: 0, n: []}));
    }

    let step = 'wifi'; // wifi -> testing -> address -> done
    let mode = 1;      // 1: WiFi, 2: standalone access point, 3: Ethernet
    let psk = '';
    let showPsk = false;
    let error = 0;
    let failures = 0;
    let verified = false;
    let elapsed = 0;
    let staticIp = false;
    let ip = '', mask = '255.255.255.0', gw = '', dns = '';
    let ack = false;
    let saving = false;
    let saveFailed = false;
    let trying = null;

    $: net = manual ? null : networks?.n?.find(n => n.s == ssid);
    $: wpa = net?.e?.includes('WPA');
    $: level = !net ? 0 : net.r >= -55 ? 4 : net.r >= -67 ? 3 : net.r >= -78 ? 2 : 1;
    $: variant = mode == 2 ? 'ap' : mode == 3 ? 'eth' : verified ? 'ok' : 'force';
    $: knownIp = staticIp ? ip : verified ? ip : '';
    $: mdnsUrl = 'http://' + hostname + '.local';
    $: url = variant == 'ap' ? 'http://192.168.4.1' : knownIp ? 'http://' + knownIp : mdnsUrl;
    $: altUrl = variant == 'ap' ? null : knownIp ? mdnsUrl : 'http://' + hostname;

    async function getJson(resource, options = {}) {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 5000);
        try {
            const response = await fetch(resource, { ...options, signal: controller.signal });
            return await response.json();
        } catch(e) {
            return null;
        } finally {
            clearTimeout(timer);
        }
    }

    function next() {
        error = 0;
        if(mode == 1) {
            startTest();
        } else {
            go('address');
        }
    }

    function go(s) {
        ack = false;
        step = s;
        window.scrollTo(0, 0);
    }

    async function startTest() {
        go('testing');
        verified = false;
        elapsed = 0;
        const started = Date.now();
        let running = false;
        while(true) {
            let res;
            if(running) {
                res = await getJson('wifitest.json');
            } else {
                res = await getJson('wifitest.json', { method: 'POST', body: new URLSearchParams({ ssid: ssid, psk: psk }) });
                running = res != null;
                res = null; // The answer to the start request says nothing about the result yet
            }
            elapsed = Date.now() - started;
            if(res && res.time == 0) {
                if(res.status == 3) {
                    verified = true;
                    ip = res.ip;
                    mask = res.mask;
                    gw = res.gw;
                    dns = res.dns;
                    go('address');
                } else {
                    fail(res.status || 4);
                }
                return;
            }
            // The device gives up after TEST_TIMEOUT, so no answer well beyond that means we lost it
            if(elapsed > TEST_TIMEOUT + 15000) {
                fail(TEST_ERR_LOST);
                return;
            }
            await new Promise(r => setTimeout(r, 1000));
        }
    }

    function fail(code) {
        error = code;
        failures++;
        go('wifi');
    }

    function modeChanged() {
        error = 0;
        failures = 0;
        psk = '';
        // The access point gets a name of its own, the device's name is a good start
        ssid = mode == 2 ? hostname : networks?.n?.[0]?.s ?? '';
    }

    function back() {
        verified = false;
        staticIp = false;
        go('wifi');
    }

    async function save() {
        saving = true;
        saveFailed = false;
        const data = new URLSearchParams({ s: 'true', sc: mode, sh: hostname });
        if(mode != 3) {
            data.append('ss', ssid);
            data.append('sp', psk);
        }
        if(staticIp) {
            data.append('sm', 'static');
            data.append('si', ip);
            data.append('su', mask);
            data.append('sg', gw);
            data.append('sd', dns);
        }
        const res = await getJson('save', { method: 'POST', body: data });
        saving = false;
        if(res?.success) {
            go('done');
            setTimeout(scanForDevice, 5000, { net: { ip: variant == 'ap' ? '192.168.4.1' : knownIp }, hostname: variant == 'ap' ? '' : hostname }, u => trying = u);
        } else {
            saveFailed = true;
        }
    }
</script>

<div class="max-w-lg mx-auto">
<div class="cnt p-4">
    <ol class="flex text-xs mb-4">
        {#each [['wifi', 'Wi-Fi'], ['address', 'Address'], ['done', 'Done']] as [key, label], i}
            {@const current = ['wifi', 'address', 'done'].indexOf(step == 'testing' ? 'wifi' : step)}
            <li class="flex-1 flex items-center {i <= current ? 'font-semibold' : 'text-gray-400'}">
                <span class="w-6 h-6 mr-2 rounded-full flex items-center justify-center text-white {i < current ? 'bg-green-600' : i == current ? 'bg-blue-600' : 'bg-gray-400'}">{i < current ? '✓' : i + 1}</span>
                {tr('steps.' + key, label)}
            </li>
        {/each}
    </ol>

    {#if step == 'wifi'}
        <form on:submit|preventDefault={next} autocomplete="off">
            <h2 class="text-lg font-semibold">{mode == 1 ? tr('wifi_title', 'Connect to your Wi-Fi') : mode == 2 ? tr('ap_title', 'Create a Wi-Fi network') : tr('eth_title', 'Use the network cable')}</h2>
            {#if mode == 1}
                <p class="text-sm text-gray-500 dark:text-gray-300">{tr('wifi_intro', 'Choose the network the meter reader should use. It only works with 2.4 GHz networks.')}</p>
            {/if}

            {#if mode == 1 || mode == 2}
                <div class="my-3">
                    <div class="flex justify-between">
                        <label for="ss">{tr('network', 'Network name')}</label>
                        {#if mode == 1}
                            <span class="text-sm">
                                {#if !manual}<button type="button" class="text-blue-600 dark:text-blue-400 mr-3" on:click={rescan}>{tr('rescan', 'Scan again')}</button>{/if}
                                <button type="button" class="text-blue-600 dark:text-blue-400" on:click={() => manual = !manual}>{manual ? tr('from_list', 'Choose from list') : tr('not_listed', 'Not listed?')}</button>
                            </span>
                        {/if}
                    </div>
                    {#if manual || mode == 2}
                        <input id="ss" bind:value={ssid} type="text" pattern={asciiPatternExt} maxlength="32" class="in-s" required/>
                    {:else}
                        <select id="ss" bind:value={ssid} class="in-s" required>
                            {#if networks?.c == -1}
                                <option value="" selected disabled>{tr('scanning', 'Looking for networks...')}</option>
                            {/if}
                            {#each networks?.n ?? [] as n}
                                <option value={n.s}>{n.e == 'open' ? '' : '🔒 '}{n.s}</option>
                            {/each}
                        </select>
                        {#if net}
                            <div class="text-sm mt-1 {level == 1 ? 'text-yellow-600 dark:text-yellow-400' : 'text-gray-500 dark:text-gray-300'}">
                                <span class="font-mono tracking-tighter">{'▂▄▆█'.slice(0, level)}<span class="opacity-30">{'▂▄▆█'.slice(level)}</span></span>
                                {tr('signal.' + level, ['', 'Weak signal', 'Fair signal', 'Good signal', 'Excellent signal'][level])} · {net.e}
                            </div>
                            {#if level == 1}
                                <div class="text-sm text-yellow-600 dark:text-yellow-400">{tr('weak_hint', 'The connection may be unreliable. If you can, move the meter reader or the router closer to each other.')}</div>
                            {:else if net.e == 'WPA2-EAP'}
                                <div class="text-sm text-red-600 dark:text-red-400">{tr('eap_hint', 'Enterprise networks (WPA2-EAP) are not supported.')}</div>
                            {/if}
                        {/if}
                    {/if}
                </div>
                {#if net?.e != 'open'}
                    <div class="my-3">
                        <label for="sp">{tr('password', 'Password')}</label>
                        <div class="flex">
                            <input id="sp" bind:value={psk} type={showPsk ? 'text' : 'password'} pattern={asciiPatternExt} minlength={wpa || mode == 2 ? 8 : undefined} maxlength="63" class="in-f w-full {error == 98 ? 'in-er' : ''}" required={wpa || mode == 2} autocomplete="off" autocapitalize="off" spellcheck="false"/>
                            <button type="button" on:click={() => showPsk = !showPsk} class="in-post" title={tr('show', 'Show password')}>{showPsk ? '🙈' : '👁️'}</button>
                        </div>
                        {#if mode == 2}
                            <div class="text-sm text-gray-500 dark:text-gray-300">{tr('ap_psk', 'At least 8 characters. You need this to join the meter reader\'s network.')}</div>
                        {/if}
                    </div>
                {/if}
            {:else}
                <p class="my-3">{tr('eth_intro', 'Plug in the network cable. The meter reader gets its address from your router.')}</p>
            {/if}

            {#if error}
                {@const code = [1, 98, 99, TEST_ERR_LOST].includes(error) ? error : 4}
                <div class="my-3 p-3 rounded border-l-4 border-red-500 bg-red-50 dark:bg-red-900/30" role="alert">
                    <div class="font-semibold">{tr('fail.' + code + '.t', {
                        1: 'Can\'t find "{0}"',
                        98: 'Wrong password',
                        99: 'No answer from "{0}"',
                        4: 'Could not connect to "{0}"',
                        [TEST_ERR_LOST]: 'Lost contact with the meter reader'
                    }[code], ssid)}</div>
                    <div class="text-sm">{tr('fail.' + code + '.h', {
                        1: 'The meter reader can\'t see this network from where it is. Check the name, and move the meter reader or the router closer. 5 GHz-only networks are not supported.',
                        98: '"{0}" did not accept the password. Passwords are case-sensitive, tap the eye to check what you typed.',
                        99: 'The meter reader did not get connected within 30 seconds. This is usually a weak signal or a wrong password. Some routers also block new devices until you allow them.',
                        4: 'The router refused the connection. Check the password, and that the router allows new devices.',
                        [TEST_ERR_LOST]: 'Your phone left the meter reader\'s network "{0}" during the test. Join it again, then try once more.'
                    }[code], code == TEST_ERR_LOST ? apName : ssid)}</div>
                </div>
            {/if}

            <button type="submit" class="btn-pri w-full mr-0 mt-2">{mode == 1 ? (error ? tr('retry', 'Try again') : tr('connect', 'Connect')) : tr('continue', 'Continue')}</button>

            {#if mode == 1 && failures >= 2}
                <div class="text-center mt-3 text-sm">
                    <button type="button" class="text-gray-500 dark:text-gray-300 underline" on:click={() => go('address')}>{tr('force_link', 'Still not working? Save without a confirmed connection')}</button>
                </div>
            {/if}

            <details class="mt-4 text-sm" open={sysinfo.if?.eth}>
                <summary class="cursor-pointer text-gray-500 dark:text-gray-300">{tr('more', 'Other ways to connect')}</summary>
                <select class="in-s mt-2" bind:value={mode} on:change={modeChanged}>
                    <option value={1}>{translations.conf?.connection?.wifi ?? "Connect to WiFi"}</option>
                    <option value={2}>{translations.conf?.connection?.ap ?? "Standalone access point"}</option>
                    {#if sysinfo.if?.eth}
                    <option value={3}>{translations.conf?.connection?.eth ?? "Ethernet"}</option>
                    {/if}
                </select>
            </details>
        </form>

    {:else if step == 'testing'}
        <div class="py-6 text-center">
            <div class="inline-block w-10 h-10 border-4 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
            <h2 class="text-lg font-semibold mt-3">{tr('testing', 'Connecting to "{0}"...', ssid)}</h2>
            <div class="h-1 my-3 bg-gray-200 dark:bg-gray-700 rounded overflow-hidden">
                <div class="h-1 bg-blue-600 transition-all duration-1000" style="width: {Math.min(100, elapsed * 100 / TEST_TIMEOUT)}%"></div>
            </div>
            <p class="text-sm text-gray-500 dark:text-gray-300">{tr('testing_hint', 'This takes up to 30 seconds. Your phone may lose contact with the meter reader for a moment, stay on this page.')}</p>
        </div>

    {:else if step == 'address'}
        <form on:submit|preventDefault={save} autocomplete="off">
            {#if variant == 'force'}
                <div class="mb-3 p-3 rounded border-l-4 border-yellow-500 bg-yellow-50 dark:bg-yellow-900/30" role="alert">
                    <div class="font-semibold">{tr('force_title', 'The connection is not confirmed')}</div>
                    <div class="text-sm">{tr('force_intro', 'The meter reader could not connect to "{0}" during the test. If you save anyway:', ssid)}</div>
                    <ul class="text-sm list-disc ml-5 mt-1">
                        <li>{tr('force_1', 'it may not connect at all')}</li>
                        <li>{tr('force_2', 'its IP address will be unknown, so you must find it by name')}</li>
                        <li>{tr('force_3', 'if it can\'t connect within 5 minutes, it goes back to setup and you can join "{0}" again', apName)}</li>
                    </ul>
                </div>
            {:else if variant == 'ok'}
                <h2 class="text-lg font-semibold"><span class="text-green-600">✓</span> {tr('ok_title', 'Connected to "{0}"', ssid)}</h2>
            {/if}

            <p>{variant == 'ap' ? tr('ap_intro', 'The meter reader will make its own Wi-Fi network "{0}". Join it with your password, then open:', ssid) : tr('find_at', 'After setup, you will find the meter reader at:')}</p>
            <AddressBox {url} alt={altUrl} {tr}/>
            {#if variant != 'ap' && !knownIp}
                <div class="text-sm -mt-2 mb-3 text-gray-500 dark:text-gray-300">{tr('ip', 'IP address')}: {tr('ip_unknown', 'unknown until the meter reader is connected')}</div>
            {/if}
            <p class="text-sm font-semibold">📸 {tr('screenshot', 'Take a screenshot or write it down. This page closes when your phone leaves the meter reader\'s network.')}</p>

            {#if variant != 'ap'}
                <div class="my-3">
                    <label for="sh">{tr('name', 'Device name')}</label>
                    <div class="flex">
                        <input id="sh" bind:value={hostname} type="text" class="in-f w-full" maxlength="32" pattern={charAndNumPattern} required autocomplete="off" autocapitalize="off" spellcheck="false"/>
                        <span class="in-post">.local</span>
                    </div>
                </div>
                <div class="my-3">
                    <label><input type="checkbox" bind:checked={staticIp} class="rounded mb-1"/> {tr('static_keep', 'Always use the same IP address (static IP)')}</label>
                    {#if staticIp}
                        <div class="text-sm text-gray-500 dark:text-gray-300">{tr('static_hint', 'Reserving the address in your router is usually the better choice.')}</div>
                        <div class="flex mt-1">
                            <input bind:value={ip} type="text" class="in-f w-full" required pattern={ipPattern} placeholder={tr('ip', 'IP address')}/>
                            <select bind:value={mask} class="in-l" required>
                                <SubnetOptions/>
                            </select>
                        </div>
                        <div class="flex mt-2">
                            <div class="w-1/2">
                                {translations.conf?.network?.gw ?? "Gateway"}
                                <input bind:value={gw} type="text" class="in-f w-full" required pattern={ipPattern}/>
                            </div>
                            <div class="w-1/2">
                                {translations.conf?.network?.dns ?? "DNS"}
                                <input bind:value={dns} type="text" class="in-l w-full" pattern={ipPattern}/>
                            </div>
                        </div>
                    {/if}
                </div>
            {/if}

            <label class="flex items-start my-4 font-semibold">
                <input type="checkbox" bind:checked={ack} class="rounded mt-1 mr-2"/>
                {variant == 'force' ? tr('force_ack', 'I understand that the meter reader may not connect') : tr('noted', 'I have saved the address')}
            </label>

            {#if saveFailed}
                <div class="my-3 p-3 rounded border-l-4 border-red-500 bg-red-50 dark:bg-red-900/30 text-sm" role="alert">{tr('save_failed', 'Could not save. Make sure your phone is still on the meter reader\'s network "{0}" and try again.', apName)}</div>
            {/if}

            <button type="submit" class="{variant == 'force' ? 'btn-yellow !ml-0' : 'btn-pri mr-0'} w-full disabled:opacity-50" disabled={!ack}>{variant == 'force' ? tr('force_save', 'Save anyway') : tr('finish', 'Finish setup')}</button>
            <div class="text-center mt-3 text-sm">
                <button type="button" class="text-blue-600 dark:text-blue-400" on:click={back}>‹ {tr('back', 'Back')}</button>
            </div>
        </form>

    {:else if step == 'done'}
        <h2 class="text-lg font-semibold"><span class="text-green-600">✓</span> {tr('done_title', 'Settings saved')}</h2>
        <p class="my-2">{({
            ok: tr('done_ok', 'The meter reader is restarting and joining "{0}".', ssid),
            force: tr('done_force', 'The meter reader is restarting and will try to join "{0}".', ssid),
            eth: tr('done_eth', 'The meter reader is restarting and connecting through the network cable.'),
            ap: tr('done_ap', 'The meter reader is restarting with its own Wi-Fi network.')
        })[variant]}</p>
        <ol class="list-decimal ml-5">
            <li>{variant == 'ap' ? tr('done_join_ap', 'Join the Wi-Fi network "{0}"', ssid) : variant == 'eth' ? tr('done_join_eth', 'Connect your phone to the same network as the meter reader') : tr('done_join', 'Connect your phone to "{0}" again, if it does not happen by itself', ssid)}</li>
            <li>{tr('done_open', 'Open this address:')}</li>
        </ol>
        <AddressBox {url} alt={altUrl} link={true} {tr}/>
        {#if variant == 'force' || variant == 'eth'}
            <p class="text-sm text-gray-500 dark:text-gray-300">{tr('done_router', 'If the name does not work, look for "{0}" in your router\'s list of connected devices.', hostname)}</p>
        {/if}
        {#if variant == 'force'}
            <p class="text-sm text-gray-500 dark:text-gray-300 mt-2">{tr('done_fallback', 'If it can\'t connect within 5 minutes, it goes back to setup. Join "{0}" again to fix the settings.', apName)}</p>
        {/if}
        <div class="flex items-center mt-4 text-sm text-gray-500 dark:text-gray-300">
            <div class="inline-block w-4 h-4 mr-2 border-2 border-blue-600 border-t-transparent rounded-full animate-spin"></div>
            {tr('searching', 'Looking for the meter reader. This page opens it when found.')}
        </div>
        {#if trying}<div class="text-xs text-gray-400 font-mono break-all">{trying}</div>{/if}
    {/if}
</div>
</div>

<Mask active={saving} message={tr('mask', 'Saving your configuration to the device')}/>
