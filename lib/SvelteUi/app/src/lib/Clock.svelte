<script>
    import { zeropad, monthnames, addHours } from './Helpers.js';

    export let timestamp;
    export let fullTimeColor;
    export let offset;

    let showFull;
    $:{
        showFull = Math.abs(new Date().getTime()-timestamp.getTime()) < 300000;
        if(!isNaN(offset))
            addHours(timestamp, offset - ((24 + timestamp.getHours() - timestamp.getUTCHours())%24));
    }
</script>

{#if showFull }
{`${zeropad(timestamp.getDate())}. ${monthnames[timestamp.getMonth()]} ${zeropad(timestamp.getHours())}:${zeropad(timestamp.getMinutes())}`}
{:else}
<span class="{fullTimeColor}">{`${zeropad(timestamp.getDate())}.${zeropad(timestamp.getMonth()+1)}.${timestamp.getFullYear()} ${zeropad(timestamp.getHours())}:${zeropad(timestamp.getMinutes())}`}</span>
{/if}
