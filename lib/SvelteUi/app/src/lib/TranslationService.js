import { writable } from 'svelte/store';
import { fetchWithTimeout } from './DataStores';

let translations = false;
export const translationsStore = writable(translations);
export async function getTranslations(lang) {
    const response = await fetchWithTimeout("translations.json" + (lang ? "?lang=" + lang : ""));
    translations = (await response.json())
    translationsStore.set(translations);
};
getTranslations();
