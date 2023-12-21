<script>
    import { sysinfoStore } from './DataStores.js';
    import BoardTypeSelectOptions from './BoardTypeSelectOptions.svelte';
    import UartSelectOptions from './UartSelectOptions.svelte';
    import Mask from './Mask.svelte'
    import { navigate } from 'svelte-navigator';

    export let sysinfo = {}

    let loadingOrSaving = false;
    async function handleSubmit(e) {
        loadingOrSaving = true;
		const formData = new FormData(e.target)
		const data = new URLSearchParams()
		for (let field of formData) {
			const [key, value] = field
			data.append(key, value)
		}

        const response = await fetch('/save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())
        loadingOrSaving = false;

        sysinfoStore.update(s => {
            s.vndcfg = res.success;
            s.booting = res.reboot;
            return s;
        });
        navigate(sysinfo.usrcfg ? "/" : "/setup");
	}

    let cc = false;
    sysinfoStore.subscribe(update => {
      sysinfo = update;
      if(update.fwconsent === 1) {
        cc = !sysinfo.usrcfg;
      }
    });
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-3 md:grid-cols-2">
    <div class="cnt">
        <form on:submit|preventDefault={handleSubmit} autocomplete="off">
            <input type="hidden" name="v" value="true"/>
            <strong class="text-sm">Initial configuration</strong>
            {#if sysinfo.usrcfg}
            <div class="bd-red">WARNING: Changing this configuration will affect basic configuration of your device. Only make changes here if instructed by vendor</div>
            {/if}
            <div class="my-3">
                Board type<br/>
                <select name="vb" bind:value={sysinfo.board} class="in-s">
                    <BoardTypeSelectOptions chip={sysinfo.chip}/>
                </select>
            </div>
            {#if sysinfo.board && sysinfo.board > 20}
                <div class="my-3">
                    HAN GPIO<br/>
                    <select name="vh" class="in-s">
                        <UartSelectOptions chip={sysinfo.chip}/>
                    </select>
                </div>
            {/if}
            <div class="my-3">
                <label><input type="checkbox" name="vr" value="true" class="rounded mb-1" bind:checked={cc} /> Clear all other configuration</label>
            </div>
            <div class="my-3">
                <button type="submit" class="btn-pri">Save</button>
            </div>
            <span class="clear-both">&nbsp;</span>
        </form>
    </div>
</div>
<Mask active={loadingOrSaving} message="Saving device configuration" />
