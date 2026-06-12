<script>
    import { bcol } from './Helpers.js';

    export let services = [];
    export let translations = {};

    const defaultLabels = {
        mqtt: "MQTT",
        mqtt_c: "MQTT (custom)",
        mqtt_es: "Energy speedometer",
        price: "Price service",
        ntp: "NTP",
        cloud: "Cloud",
        zc: "ZmartCharge",
    };

    function label(svc) {
        return svc.n
            ?? translations.status?.services?.[svc.k]
            ?? translations.header?.[svc.k]
            ?? defaultLabels[svc.k]
            ?? svc.k;
    }

    function stateLabel(s) {
        return translations.status?.services?.state?.[s]
            ?? ({ 0: "Disabled", 1: "OK", 2: "Connecting", 3: "Error" }[s] ?? s);
    }

    const errorNamespace = {
        mqtt: "mqtt",
        mqtt_c: "mqtt",
        mqtt_es: "mqtt",
        price: "price",
        cloud: "http",
        zc: "http",
    };

    function errorLabel(svc) {
        if(!svc.e) return "";
        const ns = errorNamespace[svc.k];
        return translations.errors?.[ns]?.[svc.e] ?? svc.e;
    }
</script>

<div class="cnt">
    <strong class="text-sm">{translations.status?.services?.title ?? "Services"}</strong>
    {#if !services || services.length === 0}
        <div class="my-2 text-gray-400">{translations.status?.services?.none ?? "No services configured"}</div>
    {:else}
        {#each services as svc}
            <div class="my-2 flex items-center">
                <span class="bd-{bcol(svc.s)} mr-2 flex-none">&nbsp;</span>
                <span class="flex-1 flex items-baseline min-w-0">
                    <strong class="flex-none">{label(svc)}</strong>
                    {#if svc.d}<span class="text-gray-400 ml-2 text-xs truncate" title={svc.d}>{svc.d}</span>{/if}
                </span>
                <span class="text-xs ml-2 flex-none">{stateLabel(svc.s)}</span>
                {#if svc.e}<span class="bd-red ml-2 text-xs flex-none" title={svc.e}>{errorLabel(svc)}</span>{/if}
            </div>
        {/each}
    {/if}
</div>
