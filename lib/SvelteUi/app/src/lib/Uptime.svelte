<script>
    import { translationsStore } from "./TranslationService";

    export let epoch;

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

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
    {translations.header?.uptime ?? "Up"}
    {#if days > 1}
        {days} {translations.common?.days ?? "days"}
    {:else if days > 0}
        {days} {translations.common?.day ?? "day"}
    {:else if hours > 1}
        {hours} {translations.common?.hours ?? "hours"}
    {:else if hours > 0}
        {hours} {translations.common?.hour ?? "hour"}
    {:else if minutes > 1}
        {minutes} {translations.common?.minutes ?? "minutes"}
    {:else if minutes > 0}
        {minutes} {translations.common?.minute ?? "minute"}
    {:else}
        {epoch} {translations.common?.seconds ?? "seconds"}
    {/if}
{/if}
