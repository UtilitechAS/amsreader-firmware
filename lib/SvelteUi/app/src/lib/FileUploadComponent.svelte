<script>
    export let action;
    export let title;

</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
    <div class="cnt">
        <strong>Upload {title}</strong>
        <p class="mb-4">Select a suitable file and click upload</p>
        <form action="{action}" enctype="multipart/form-data" method="post">
            <input name="file" type="file">
            <div class="w-full text-right mt-4">
                <button type="submit" class="btn-pri">Upload</button>
            </div>
        </form>
    </div>
</div>
