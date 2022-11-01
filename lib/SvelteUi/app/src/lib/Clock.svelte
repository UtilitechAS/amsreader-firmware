<script>
    import { zeropad } from './Helpers.js';

    export let timestamp;
    let monthnames = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"];
</script>

{#if Math.abs(new Date().getTime()-timestamp.getTime()) < 300000 }
{`${zeropad(timestamp.getDate())}. ${monthnames[timestamp.getMonth()]} ${zeropad(timestamp.getHours())}:${zeropad(timestamp.getMinutes())}`}
{:else}
<span class="text-red-500">{`${zeropad(timestamp.getDate())}.${zeropad(timestamp.getMonth())}.${timestamp.getFullYear()} ${zeropad(timestamp.getHours())}:${zeropad(timestamp.getMinutes())}`}</span>
{/if}
