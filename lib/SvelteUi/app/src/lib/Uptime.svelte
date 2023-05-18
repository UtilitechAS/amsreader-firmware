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
<!-- EHorvat translated to german- ... Up/sLauft days/Tage day/Tag hours/Stunden hour/Stunde minutes/Minuten minute/Minute seconds/Sekunden-->
{#if epoch}
    sLauft
    {#if days > 1}
        {days} Tage
    {:else if days > 0}
        {days} Tag
    {:else if hours > 1}
        {hours} Stunden
    {:else if hours > 0}
        {hours} Stunde
    {:else if minutes > 1}
        {minutes} Minuten
    {:else if minutes > 0}
        {minutes} Minute
    {:else}
        {epoch} Sekunden
    {/if}
{/if}

