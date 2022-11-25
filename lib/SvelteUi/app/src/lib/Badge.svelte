<script>
    export let color;
    export let title;
    export let text;
</script>
{#if color == 'green'}
<span title={title} class="bd-grn">{text}</span>
{:else if color === `yellow`}
<span title={title} class="bd-ylo">{text}</span>
{:else if color === `red`}
<span title={title} class="bd-red">{text}</span>
{:else if color === `blue`}
<span title={title} class="bd-blu">{text}</span>
{:else if color === `gray`}
<span title={title} class="bd-gry">{text}</span>
{/if}