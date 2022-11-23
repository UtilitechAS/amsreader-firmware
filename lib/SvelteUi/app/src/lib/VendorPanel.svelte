<script>
    import { sysinfoStore } from './DataStores.js';
    import BoardTypeSelectOptions from './BoardTypeSelectOptions.svelte';
    import UartSelectOptions from './UartSelectOptions.svelte';
    import Mask from './Mask.svelte'

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
	}
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-3  md:grid-cols-2">
    <div class="bg-white m-2 p-3 rounded-md shadow-lg pb-4 text-gray-700">
        <form on:submit|preventDefault={handleSubmit}>
            <input type="hidden" name="v" value="true"/>
            <strong class="text-sm">Initial configuration</strong>
            <div class="my-3">
                Board type<br/>
                <select name="vb" bind:value={sysinfo.board} class="h-10 rounded-md shadow-sm border-gray-300 p-0 w-full">
                    <BoardTypeSelectOptions chip={sysinfo.chip}/>
                </select>
            </div>
            {#if sysinfo.board && sysinfo.board > 20}
                <div class="my-3">
                    HAN GPIO<br/>
                    <select name="vh" class="h-10 rounded-md shadow-sm border-gray-300">
                        <UartSelectOptions chip={sysinfo.chip}/>
                    </select>
                </div>
            {/if}
            <div class="my-3">
                <button type="submit" class="font-bold py-1 px-4 rounded bg-blue-500 text-white float-right">Save</button>
            </div>
            <span class="clear-both">&nbsp;</span>
        </form>
    </div>
</div>
<Mask active={loadingOrSaving} message="Saving device configuration" />
