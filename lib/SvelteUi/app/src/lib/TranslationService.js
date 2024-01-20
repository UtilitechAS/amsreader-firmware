import { writable } from 'svelte/store';
import { fetchWithTimeout } from './DataStores';

let translations = false;
export const translationsStore = writable(translations);
async function getTranslations() {
    const response = await fetchWithTimeout("translations.json");
    translations = (await response.json())
    translationsStore.set(translations);
};
getTranslations();
