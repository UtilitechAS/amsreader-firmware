<script>
    export let chip;

    let gpioMax = 39;
    $: {
        switch (chip) {
            case 'esp8266': gpioMax = 16; break;
            case 'esp32s2': gpioMax = 44; break;
            case 'esp32s3': gpioMax = 46; break;
            case 'esp32c3': gpioMax = 19; break;
        }
    }
</script>

{#if chip == 'esp8266'}
<option value={3}>UART0</option>
<option value={113}>UART2</option>
{/if}

{#each {length: gpioMax+1} as _, i}
    {#if i > 1 && !(chip == 'esp8266' && (i == 3 || i == 113))}
        <option value={i}>GPIO{i}</option>
    {/if}
{/each}
