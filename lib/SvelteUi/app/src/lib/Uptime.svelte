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
        {days} {translations.common?.days ?? "d"}
    {:else if days > 0}
        {days} {translations.common?.day ?? "d"}
    {:else if hours > 1}
        {hours} {translations.common?.hours ?? "h"}
    {:else if hours > 0}
        {hours} {translations.common?.hour ?? "h"}
    {:else if minutes > 1}
        {minutes} {translations.common?.minutes ?? "m"}
    {:else if minutes > 0}
        {minutes} {translations.common?.minute ?? "m"}
    {:else}
        {epoch} {translations.common?.seconds ?? "s"}
    {/if}
{/if}
