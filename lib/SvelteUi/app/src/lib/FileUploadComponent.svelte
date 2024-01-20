<script>
    import Mask from "./Mask.svelte";
    import { translationsStore } from "./TranslationService";

    export let action;
    export let title;

    let translations = {};
    translationsStore.subscribe(update => {
      translations = update;
    });

    let uploading = false;
</script>

<div class="grid xl:grid-cols-4 lg:grid-cols-2 md:grid-cols-2">
    <div class="cnt">
        <strong>{translations.upload?.title ?? "Upload"} {title}</strong>
        <p class="mb-4">{translations.upload?.desc ?? ""}</p>
        <form action="{action}" enctype="multipart/form-data" method="post" on:submit={() => uploading=true} autocomplete="off">
            <input name="file" type="file">
            <div class="w-full text-right mt-4">
                <button type="submit" class="btn-pri"><p class="mb-4">{translations.btn?.upload ?? "Upload"}</button>
            </div>
        </form>
    </div>
</div>
<Mask active={uploading} message={translations.upload?.mask ?? "Uploading"}/>
