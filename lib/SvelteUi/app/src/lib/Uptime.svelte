<script>
    export let epoch;
    let days = Math.round(epoch/86400);
    let hours = Math.round(epoch/3600);
    let minutes = Math.round(epoch/60);
</script>
Up
{#if days > 0}
{days} days
{:else if hours > 0}
{hours} hours
{:else if minutes > 0}
{minutes} minutes
{:else}
{epoch} seconds
{/if}
