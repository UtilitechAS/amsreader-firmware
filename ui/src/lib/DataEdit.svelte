<script>
    import { translationsStore } from './TranslationService';
    import { navigate } from 'svelte-navigator';
    import Mask from './Mask.svelte'

    export let prefix;
    export let data;
    export let url;
    export let basepath = "/";

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let hasExport = false;
    let importElements, exportElements;
    $: {
        importElements = []
        exportElements = []
        for(let key in data) {
            let elements = [];
            if(key.startsWith("i")) {
                elements = importElements;
            } else if(key.startsWith("e")) {
                elements = exportElements;
                if(data[key]) hasExport = true;
            }
            elements.push({
                key: key,
                name: prefix + " " + key.substring(1),
                value: data[key]
            });
        }
    }

    async function reset() {
        if(confirm("Clear all data?")) {
            for(let key in data) {
                if(key.startsWith("i") || key.startsWith("e")) data[key] = 0.0;
            }
        }
    }

    let saving = false;
    async function handleSubmit(e) {
        saving = true;
		const formData = new FormData(e.target);
		const params = new URLSearchParams();
		for (let field of formData) {
			const [key, value] = field
			params.append(key, value)
		}

        const response = await fetch(url, {
            method: 'POST',
            body: params
        });
        let res = (await response.json())

        saving = false;
        navigate(basepath);
	}
</script>
<form on:submit|preventDefault={handleSubmit} autocomplete="off">
    <div class="cnt">
        {#if importElements}
            <div class="text-sm font-bold">Import</div>
            <div class="flex flex-wrap my-1">
                {#each importElements as el}
                <label class="flex w-60 m-1">
                    <span class="in-pre">{el.name}</span>
                    <input name="{el.key}" bind:value={data[el.key]} type="number" step="0.01" class="in-txt w-full text-right"/>
                    <span class="in-post">kWh</span>
                </label>
                {/each}
            </div>
        {/if}
        {#if exportElements && hasExport}
            <div class="text-sm font-bold">Export</div>
            <div class="flex flex-wrap my-1">
                {#each exportElements as el}
                <label class="flex w-60 m-1">
                    <span class="in-pre">{el.name}</span>
                    <input name="{el.key}" bind:value={data[el.key]} type="number" step="0.01" class="in-txt w-full text-right"/>
                    <span class="in-post">kWh</span>
                </label>
                {/each}
            </div>
        {/if}
        <div class="grid grid-cols-2 mt-3">
            <div>
                <button type="button" on:click={reset} class="btn-red">Clear all</button>
            </div>
            <div class="text-right">
                <button type="submit" class="btn-pri">{translations.btn?.save ?? "Save"}</button>
            </div>
        </div>
    </div>
</form>
<Mask active={saving} message={translations.conf?.mask?.saving ?? "Saving"}/>
