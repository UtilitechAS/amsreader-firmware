<script>
    import { zeropad, monthnames, addHours } from './Helpers.js';

    export let timestamp;
    export let fullTimeColor;
    export let offset;

    let showFull;
    let adjusteTimestamp;
    $:{
        showFull = Math.abs(new Date().getTime()-timestamp.getTime()) < 300000;
        addHours(timestamp, offset);
    }
</script>

{#if showFull }
{`${zeropad(timestamp.getDate())}. ${monthnames[timestamp.getMonth()]} ${zeropad(timestamp.getUTCHours())}:${zeropad(timestamp.getMinutes())}`}
{:else}
<span class="{fullTimeColor}">{`${zeropad(timestamp.getDate())}.${zeropad(timestamp.getMonth()+1)}.${timestamp.getFullYear()} ${zeropad(timestamp.getUTCHours())}:${zeropad(timestamp.getMinutes())}`}</span>
{/if}
