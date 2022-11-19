<script>
    export let color;
    export let title;
    export let text;
</script>
{#if color == 'green'}
<span title={title} class="my-auto bg-green-500 text-green-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">{text}</span>
{:else if color === `yellow`}
<span title={title} class="my-auto bg-yellow-500 text-yellow-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">{text}</span>
{:else if color === `red`}
<span title={title} class="my-auto bg-red-500 text-red-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">{text}</span>
{:else if color === `blue`}
<span title={title} class="my-auto bg-blue-500 text-blue-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">{text}</span>
{:else if color === `gray`}
<span title={title} class="my-auto bg-gray-500 text-gray-100 text-xs font-semibold mr-2 px-2.5 py-0.5 rounded">{text}</span>
{/if}