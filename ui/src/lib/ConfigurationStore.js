import { writable } from 'svelte/store';

let configuration = {};
export const configurationStore = writable(configuration);

export async function getConfiguration() {
    const response = await fetch("configuration.json");
    configuration = (await response.json())
    configurationStore.set(configuration);
};

let priceConfig = {};
export const priceConfigStore = writable(priceConfig);

export async function getPriceConfig() {
    const response = await fetch("priceconfig.json");
    priceConfig = (await response.json())
    priceConfigStore.set(priceConfig);
}
