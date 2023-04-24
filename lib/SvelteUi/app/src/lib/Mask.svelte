<script>
    export let active;
    export let message;
</script>

{#if active}
<div class="z-50" aria-modal="true">
  <div class="fixed inset-0 bg-gray-500 dark:bg-gray-900 bg-opacity-50 flex items-center justify-center">
    {#if message}
    <div class="bg-white dark:bg-gray-600 m-2 p-3 rounded-md shadow-lg pb-4 text-gray-700 dark:text-white w-96">{message}</div>
    {/if}
  </div>
</div>
{/if}
