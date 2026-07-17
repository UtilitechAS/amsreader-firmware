<script>
    export let cfg;
    export let item;
    export let translations = {};
</script>
<div class="w-1/2">
    {translations.conf?.ui?.[item.key] ?? item.name}<br/>
    <select name="u{item.key}" bind:value={cfg[item.key]} class="in-s">
        <option value={0}>{translations.conf?.ui?.disabled ?? "Disabled"}</option>
        <option value={1}>{translations.conf?.ui?.enabled ?? "Enabled"}</option>
        <option value={2}>{translations.conf?.ui?.auto ?? "Auto"}</option>
    </select>
</div>
