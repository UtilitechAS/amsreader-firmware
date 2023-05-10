<script>
    export let chip;

    let gpioMax = 39;
    $: {
        switch (chip) {
            case 'esp8266': gpioMax = 16; break;
            case 'esp32s2': gpioMax = 44; break;
            case 'esp32c3': gpioMax = 19; break;
        }
    }
</script>

{#if chip == 'esp32c3'}
<option value={20}>UART0</option>
{:else}
<option value={3}>UART0</option>
{/if}
{#if chip == 'esp8266'}
<option value={113}>UART2</option>
{/if}
{#if chip == 'esp32' || chip == 'esp32solo'}
<option value={9}>UART1</option>
<option value={16}>UART2</option>
{/if}
{#if chip == 'esp32s2'}
<option value={18}>UART1</option>
{/if}

{#each {length: gpioMax+1} as _, i}
    {#if i > 3
        && !(chip == 'esp32' && (i == 9 || i == 16))
        && !(chip == 'esp32s2' && i == 18)
        && !(chip == 'esp8266' && (i == 3 || i == 113))
    }
        <option value={i}>GPIO{i}</option>
    {/if}
{/each}
