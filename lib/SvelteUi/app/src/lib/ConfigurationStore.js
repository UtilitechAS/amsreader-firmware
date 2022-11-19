import { writable } from 'svelte/store';

let configuration = {};
export const configurationStore = writable(configuration);

export async function getConfiguration() {
    const response = await fetch("/configuration.json");
    configuration = (await response.json())
    configurationStore.set(configuration);
};
