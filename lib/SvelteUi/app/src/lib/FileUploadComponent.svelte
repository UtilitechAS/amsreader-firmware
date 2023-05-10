<script>
    import Mask from "./Mask.svelte";

    export let action;
    export let title;

    let uploading = false;
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
    <div class="cnt">
        <strong>Upload {title}</strong>
        <p class="mb-4">Select a suitable file and click upload</p>
        <form action="{action}" enctype="multipart/form-data" method="post" on:submit={() => uploading=true} autocomplete="off">
            <input name="file" type="file">
            <div class="w-full text-right mt-4">
                <button type="submit" class="btn-pri">Upload</button>
            </div>
        </form>
    </div>
</div>
<Mask active={uploading} message="Uploading file, please wait"/>
