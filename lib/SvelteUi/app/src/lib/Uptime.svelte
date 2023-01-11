<script>
    export let epoch;
    let days = 0;
    let hours = 0;
    let minutes = 0;
    $: {
        days = Math.floor(epoch/86400);
        hours = Math.floor(epoch/3600);
        minutes = Math.floor(epoch/60);
    }
</script>
{#if epoch}
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
{/if}
