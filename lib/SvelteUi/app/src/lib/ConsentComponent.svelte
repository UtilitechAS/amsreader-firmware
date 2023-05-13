<script>
    import { sysinfoStore } from './DataStores.js';
    import Mask from './Mask.svelte'
    import { navigate } from 'svelte-navigator';
    import { wiki } from './Helpers';

    export let sysinfo = {}

    let loadingOrSaving = false;

    async function handleSubmit(e) {
        loadingOrSaving = true;
        const formData = new FormData(e.target)
        const data = new URLSearchParams()
        for (let field of formData) {
            const [key, value] = field
            data.append(key, value)
        }

        const response = await fetch('/save', {
            method: 'POST',
            body: data
        });
        let res = (await response.json())
        loadingOrSaving = false;

        sysinfoStore.update(s => {
            s.fwconsent = formData['sf'] === true ? 1 : formData['sf'] === false ? 2 : 0;
            s.booting = res.reboot;
            return s;
        });
        navigate("/");
    }
</script>

<div class="grid xl:grid-cols-3 lg:grid-cols-2">
    <div class="cnt">
        <form on:submit|preventDefault={handleSubmit} autocomplete="off">
            <div>
                Various permissions we need to do stuff:
            </div>
            <hr/>
            <div class="my-3">
                Enable one-click upgrade? (implies data collection)<br/>
                <a href="{wiki('Data-collection-on-one-click-firmware-upgrade')}" target="_blank" class="text-blue-600 hover:text-blue-800">Read more</a><br/>
                <label><input type="radio" name="sf" value={1} checked={sysinfo.fwconsent === 1} class="rounded m-2" required/> Yes</label><label><input type="radio" name="sf" value={2} checked={sysinfo.fwconsent === 2} class="rounded m-2" required/> No</label><br/>
            </div>
            <div class="my-3">
                <button type="submit" class="btn-pri">Save</button>
            </div>
        </form>
    </div>
</div>

<Mask active={loadingOrSaving} message="Saving preferences"/>
