<script>
    export let url;
    export let alt = null;
    export let link = false;
    export let tr;

    let copied = false;

    // navigator.clipboard only exists on https, so fall back to a selected input
    function copy() {
        if(navigator.clipboard) {
            navigator.clipboard.writeText(url);
        } else {
            const input = document.createElement('input');
            input.value = url;
            document.body.appendChild(input);
            input.select();
            input.setSelectionRange(0, url.length);
            document.execCommand('copy');
            input.remove();
        }
        copied = true;
        setTimeout(() => copied = false, 2000);
    }
</script>

<div class="my-3 p-3 rounded bg-gray-100 dark:bg-gray-700">
    <div class="flex items-center">
        {#if link}
            <a href={url} class="font-mono text-lg font-semibold break-all flex-1 text-blue-600 dark:text-blue-400">{url}</a>
        {:else}
            <span class="font-mono text-lg font-semibold break-all flex-1 select-all">{url}</span>
        {/if}
        <button type="button" class="btn-pri-sm ml-2 mr-0" on:click={copy}>{copied ? tr('copied', 'Copied') : tr('copy', 'Copy')}</button>
    </div>
    {#if alt}
        <div class="text-sm text-gray-500 dark:text-gray-300 font-mono break-all">{tr('or', 'or')} {alt}</div>
    {/if}
</div>
