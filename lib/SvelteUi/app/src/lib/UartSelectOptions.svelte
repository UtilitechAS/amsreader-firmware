<script>
    export let chip;
</script>

<option value={3}>UART0</option>
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
<option value={4}>GPIO4</option>
<option value={5}>GPIO5</option>
{#if chip.startsWith('esp32')}
<option value={6}>GPIO6</option>
<option value={7}>GPIO7</option>
<option value={8}>GPIO8</option>
{/if}
{#if chip == 'esp8266'}
<option value={9}>GPIO9</option>
{/if}
<option value={10}>GPIO10</option>
{#if chip.startsWith('esp32')}
<option value={11}>GPIO11</option>
{/if}
<option value={12}>GPIO12</option>
<option value={13}>GPIO13</option>
<option value={14}>GPIO14</option>
<option value={15}>GPIO15</option>

{#if chip.startsWith('esp32')}
<option value={17}>GPIO17</option>
{#if chip != 'esp32s2'}
<option value={18}>GPIO18</option>
{/if}
<option value={19}>GPIO19</option>
<option value={21}>GPIO21</option>
<option value={22}>GPIO22</option>
<option value={23}>GPIO23</option>
<option value={25}>GPIO25</option>
<option value={32}>GPIO32</option>
<option value={33}>GPIO33</option>
<option value={34}>GPIO34</option>
<option value={35}>GPIO35</option>
<option value={36}>GPIO36</option>
<option value={39}>GPIO39</option>
{/if}
{#if chip == 'esp32s2'}
<option value={40}>GPIO40</option>
<option value={41}>GPIO41</option>
<option value={42}>GPIO42</option>
<option value={43}>GPIO43</option>
<option value={44}>GPIO44</option>
{/if}
