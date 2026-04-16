import { readable, writable } from 'svelte/store';
import fetchWithTimeout from './fetchWithTimeout';

let realtimeOffset = 0;
let realtime = { data: [] };
export async function getRealtime() {
    const response = await fetchWithTimeout(realtimeOffset < 0 ? "realtime.json" : "realtime.json?offset="+realtimeOffset);
    let res = (await response.json())
    realtimeStore.update(current => {
        for(let i = 0; i < res.size; i++) {
            current.data[res.offset+i] = res.data[i];
        }
        current.size = current.data.length;
        return current;
    });
    if(realtimeOffset >= 0) {
        realtimeOffset += res.size;
        if(realtimeOffset < res.total) {
            setTimeout(getRealtime, 2000);
        } else {
            realtimeOffset = -1;
        }
    }
}

export function isRealtimeFullyLoaded() {
    return realtimeOffset == -1;
}

export const realtimeStore = writable(realtime);

let lastUp = 0;
let lastValue = 0;
let lastUpdate = 0;
let updateCount = 0;

let realtimeRequested = false;

function addValue() {
    if(updateCount == 60 || lastUpdate > lastUp || lastUpdate - lastUp > 300) {
        getRealtime();
        updateCount = 0;
    } else {
        realtimeStore.update(s => {
            if(s.lastUpdate) {
                while(lastUp > s.lastUpdate) {
                    s.data.unshift(lastValue);
                    s.data = s.data.slice(0,s.size);
                    s.lastUpdate += 10;
                    updateCount++;
                }
            } else {
                s.lastUpdate = lastUp;
            }
            return s;
        });
    }
}

export function updateRealtime(update) {
    lastValue = update.i-update.e;
    lastUp = update.u;
    if(!realtimeRequested) {
        getRealtime();
        realtimeRequested = true;
        lastUpdate = lastUp;
        return;
    }
    if(!isRealtimeFullyLoaded()) return;
    addValue();
}
