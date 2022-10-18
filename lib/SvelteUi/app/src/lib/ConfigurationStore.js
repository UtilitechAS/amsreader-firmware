import { readable } from 'svelte/store';

let configuration = {};
export const configurationStore = readable(configuration, (set) => { 
    async function getConfiguration(){
        const response = await fetch("/configuration.json");
        configuration = (await response.json())
        set(configuration);
    }
    getConfiguration();
    return function stop() {}
});
