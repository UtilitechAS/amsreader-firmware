<script>
    import { zeropad, monthnames } from './Helpers.js';

    export let timestamp;
    export let fullTimeColor;
</script>

{#if Math.abs(new Date().getTime()-timestamp.getTime()) < 300000 }
{`${zeropad(timestamp.getDate())}. ${monthnames[timestamp.getMonth()]} ${zeropad(timestamp.getHours())}:${zeropad(timestamp.getMinutes())}`}
{:else}
<span class="{fullTimeColor}">{`${zeropad(timestamp.getDate())}.${zeropad(timestamp.getMonth())}.${timestamp.getFullYear()} ${zeropad(timestamp.getHours())}:${zeropad(timestamp.getMinutes())}`}</span>
{/if}
