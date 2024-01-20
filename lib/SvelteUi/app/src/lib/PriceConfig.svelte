<script>
    import { priceConfigStore, getPriceConfig } from './ConfigurationStore'
    import { translationsStore } from './TranslationService';
    import { wiki } from './Helpers.js';
    import Mask from './Mask.svelte'
    import HelpIcon from './HelpIcon.svelte';
    import TrashIcon from './TrashIcon.svelte';
    import {  navigate } from 'svelte-navigator';

    export let basepath = "/";

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let days = ['mo','tu','we','th','fr','sa','su'];

    let configuration = {};
    let loading = true;
    let saving = false;

    priceConfigStore.subscribe(update => {
        if(update.o) {
            configuration = update;
            loading = false;
        }
    });

    getPriceConfig();

    async function handleSubmit(e) {
        saving = true;
		const data = new URLSearchParams();
        data.append("r", "true");
        data.append("rc", configuration.o.length);
        configuration.o.forEach(function(e,i) {
            data.append("rt"+i, e.t);
            data.append("rn"+i, e.n);
            data.append("rd"+i, e.d);
            data.append("ra"+i, e.a);
            data.append("rh"+i, e.h);
            data.append("rv"+i, e.v);

            data.append("rsm"+i, e.s.m);
            data.append("rsd"+i, e.s.d);
            data.append("rem"+i, e.e.m);
            data.append("red"+i, e.e.d);
        });

        const response = await fetch('save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())

        saving = false;
        navigate(basepath + "configuration");
	}

    let toggleDay = function(arr, day) {
        if(arr.includes(day)) {
            arr = arr.filter(function(i) {
                return i !== day
            });
        } else {
            arr.push(day);
        }
        return arr;
    };

    let addRow = function() {
        let arr = configuration.o;
        arr.push({
            t: 1,
            n: '',
            d: 3,
            a: [0,1,2,3,4,5,6,7],
            h: [0,1,2,3,4,5,6,7,8,9, 10,11,12,13,14,15,16,17,18,19, 20,21,22,23],
            v: 0.001,
            s: {m:0,d:0},
            e: {m:0,d:0}
        });
        configuration.o = arr
    };

    let deleteRow = function(rn) {
        let arr = configuration.o;
        arr.splice(rn, 1);
        configuration.o = arr
    };
</script>
<div class="cnt">
    <strong class="text-sm">{translations.conf?.price?.title ?? "Price configuration"}</strong>
    <a href="{wiki('Price-configuration')}" target="_blank" class="float-right"><HelpIcon/></a>
    <hr class="m-3"/>
    <form on:submit|preventDefault={handleSubmit} autocomplete="off">
        <input type="hidden" name="r" value="true"/>
        {#if configuration.o}
            {#each configuration.o as c,rn}
                <div class="flex flex-wrap">
                    <div class="mr-3">
                        <input name="n" type="text" class="in-s" bind:value={c.n}/>
                    </div>
                    <div class="flex mr-3">
                        <select name="rd" class="in-f" bind:value={c.d}>
                            <option value={1}>{translations.conf?.common?.import ?? "Import"}</option>
                            <option value={2}>{translations.conf?.common?.export ?? "Export"}</option>
                            <option value={3}>{translations.conf?.price?.both ?? "Both"}</option>
                        </select>
                        <select name="rt" class="in-m" bind:value={c.t}>
                            <option value={0}>{translations.conf?.price?.fixed ?? "Fixed"}</option>
                            <option value={1}>+</option>
                            <option value={2}>%</option>
                        </select>
                        <input name="rv" type="number" class="in-l tr" style="width: 100px;" min="0.0001" max="99.9999" step="0.0001" bind:value={c.v}/>
                    </div>
                    <div class="flex flex-wrap mr-3">
                        <span class="mr-2">{translations.common?.days ?? "Days"}:</span>
                        <div>
                            {#each {length: 7} as _,i}
                                <span class={c.a.includes(i) ? 'bd-on' : 'bd-off'} on:click={() => c.a = toggleDay(c.a, i)}>{days[i]}</span>
                            {/each}
                        </div>
                    </div>
                    <div class="flex flex-wrap mr-3">
                        <span class="mr-2">{translations.common?.hours ?? "Hours"}:</span>
                        <div>
                            <div>
                                {#each {length: 8} as _,i}
                                    <span class={c.h.includes(i) ? 'bd-on' : 'bd-off'} on:click={() => c.h = toggleDay(c.h, i)}>{i.toString().padStart(2,'0')}</span>
                                {/each}
                            </div>
                            <div>
                                {#each {length: 8} as _,i}
                                    <span class={c.h.includes(i+8) ? 'bd-on' : 'bd-off'} on:click={() => c.h = toggleDay(c.h, i+8)}>{(i+8).toString().padStart(2,'0')}</span>
                                {/each}
                            </div>
                            <div>
                                {#each {length: 8} as _,i}
                                    <span class={c.h.includes(i+16) ? 'bd-on' : 'bd-off'} on:click={() => c.h = toggleDay(c.h, i+16)}>{(i+16).toString().padStart(2,'0')}</span>
                                {/each}
                            </div>
                        </div>
                    </div>
                    <div class="flex flex-wrap mr-3">
                        <select name="rsm" class="in-f" bind:value={c.s.m}>
                            <option value={0}>-</option>
                            {#each {length: 31} as _,i}
                                <option value={i+1}>{i+1}</option>
                            {/each}
                        </select>
                        <select name="rsd" class="in-m" bind:value={c.s.d}>
                            <option value={0}>-</option>
                            {#each {length: 12} as _,i}
                                <option value={i+1}>{translations.months?.[i]}</option>
                            {/each}
                        </select>
                        <input class="in-m" disabled value="to" style="width: 20px;color:#888;"/>
                        <select name="rem" class="in-m" bind:value={c.e.m}>
                            <option value={0}>-</option>
                            {#each {length: 31} as _,i}
                                <option value={i+1}>{i+1}</option>
                            {/each}
                        </select>
                        <select name="red" class="in-l" bind:value={c.e.d}>
                            <option value={0}>-</option>
                            {#each {length: 12} as _,i}
                                <option value={i+1}>{translations.months?.[i]}</option>
                            {/each}
                        </select>
                    </div>
                    
                    <div class="mt-1.5 ml-3">
                        <span class="text-red-500 text-xs" on:click={() => deleteRow(rn)} on:keypress={() => deleteRow(rn)}><TrashIcon/></span>
                    </div>
                </div>
                <hr class="m-3"/>
            {/each}
        {/if}
        <div class="grid grid-cols-3">
            <div>
                <button type="button" on:click={addRow} class="btn-pri">{translations.conf?.price?.btn_add ?? "Add"}</button>
            </div>
            <div class="text-center">
            </div>
            <div class="text-right">
                <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
            </div>
        </div>
    </form>
</div>

<Mask active={loading} message={translations.conf?.price?.mask_loading ?? "Loading"}/>
<Mask active={saving} message={translations.conf?.price?.mask_loading ?? "Saving"}/>
