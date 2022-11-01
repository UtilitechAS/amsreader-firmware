<script>
    export let active;
</script>

{#if active}
<div class="z-10" aria-modal="true">
  <div class="fixed inset-0 bg-gray-500 bg-opacity-50"></div>
</div>

{/if}
