<script>
    export let epoch;
    let days = 0;
    let hours = 0;
    let minutes = 0;
    $: {
        days = Math.round(epoch/86400);
        hours = Math.round(epoch/3600);
        minutes = Math.round(epoch/60);
    }
</script>
Up
{#if days > 1}
{days} days
{:else if days > 0}
{days} day
{:else if hours > 1}
{hours} hours
{:else if hours > 0}
{hours} hour
{:else if minutes > 1}
{minutes} minutes
{:else if minutes > 0}
{minutes} minute
{:else}
{epoch} seconds
{/if}
