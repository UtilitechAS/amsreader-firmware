<script>
    import {boardtype} from './Helpers.js'

    export let chip;
</script>

<option value={-1}></option>
{#if chip == 'esp8266'}
    <optgroup label="amsleser.no">
        <option value={7}>{boardtype(chip, 7)}</option>
        <option value={5}>{boardtype(chip, 5)}</option>
        <option value={4}>{boardtype(chip, 4)}</option>
        <option value={3}>{boardtype(chip, 3)}</option>
    </optgroup>
    <optgroup label="Custom hardware">
        <option value={2}>{boardtype(chip, 2)}</option>
        <option value={1}>{boardtype(chip, 1)}</option>
        <option value={0}>{boardtype(chip, 0)}</option>
    </optgroup>
    <optgroup label="Generic hardware">
        <option value={101}>{boardtype(chip, 101)}</option>
        <option value={100}>{boardtype(chip, 100)}</option>
    </optgroup>
{/if}
{#if chip == 'esp32'}
    <optgroup label="Generic hardware">
        <option value={201}>{boardtype(chip, 201)}</option>
        <option value={202}>{boardtype(chip, 202)}</option>
        <option value={203}>{boardtype(chip, 203)}</option>
        <option value={200}>{boardtype(chip, 200)}</option>
    </optgroup>
{/if}
{#if chip == 'esp32s2'}
    <optgroup label="amsleser.no">
        <option value={7}>{boardtype(chip, 7)}</option>
        <option value={6}>{boardtype(chip, 6)}</option>
        <option value={5}>{boardtype(chip, 5)}</option>
    </optgroup>
    <optgroup label="Generic hardware">
        <option value={51}>{boardtype(chip, 51)}</option>
        <option value={50}>{boardtype(chip, 50)}</option>
    </optgroup>
{/if}
{#if chip == 'esp32solo'}
    <optgroup label="Generic hardware">
        <option value={200}>{boardtype(chip, 200)}</option>
    </optgroup>
{/if}
