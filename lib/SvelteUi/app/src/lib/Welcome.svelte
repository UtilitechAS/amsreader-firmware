<script>
  import { onDestroy, onMount } from "svelte";

  export let basepath = "/";

  let setupUrl = "http://192.168.4.1/setup";
  let isCaptivePortal = false;
  let isIosPortal = false;
  let isMacPortal = false;
  let isAndroidPortal = false;
  let copyState = "idle";
  let closeInstructionsVisible = false;
  let closeAttempted = false;
  let closeSucceeded = false;

  onMount(() => {
    setupUrl = resolveSetupUrl();

    detectCaptivePortal();
  });

  onDestroy(() => {
  });

  function resolveSetupUrl() {
    const fallbackSetup = "http://192.168.4.1/setup";

    try {
      const base = new URL(basepath || "/", window.location.href);
      const setup = new URL("./setup", base).href;
      return setup;
    } catch (err) {
      return fallbackSetup;
    }
  }

  function detectCaptivePortal() {
    if (typeof navigator === "undefined") return false;
    const ua = navigator.userAgent || "";
    const patterns = [
      /CaptiveNetworkSupport/i,
      /CaptivePortalLogin/i,
      /WISPr/i,
      /Microsoft\s?NCSI/i,
      /MiniBrowser/i,
      /PortalApp/i,
      /CaptiveNetworkWebSheet/i,
    ];

    isIosPortal = /iPad|iPhone|iPod/.test(ua) && !window.MSStream;
    isMacPortal = /Macintosh|Mac OS X/.test(ua);
    isAndroidPortal = /Android/.test(ua);

    isCaptivePortal = patterns.some((pattern) => pattern.test(ua));
    isIosPortal = isCaptivePortal && isIosPortal;
    isMacPortal = isCaptivePortal && isMacPortal && !isIosPortal;
    isAndroidPortal = isCaptivePortal && isAndroidPortal;

    return isCaptivePortal;
  }

  async function copySetupUrl(event) {
    event?.preventDefault?.();
    copyState = "pending";

    const fallbackCopy = () => {
      try {
        const textarea = document.createElement("textarea");
        textarea.value = setupUrl;
        textarea.setAttribute("readonly", "");
        textarea.style.position = "absolute";
        textarea.style.left = "-9999px";
        document.body.appendChild(textarea);
        textarea.select();
        const success = document.execCommand("copy");
        document.body.removeChild(textarea);
        return success;
      } catch (err) {
        return false;
      }
    };

    try {
      if (navigator.clipboard?.writeText) {
        await navigator.clipboard.writeText(setupUrl);
        copyState = "success";
      } else if (fallbackCopy()) {
        copyState = "success";
      } else {
        copyState = "error";
      }
    } catch (error) {
      if (fallbackCopy()) {
        copyState = "success";
      } else {
        copyState = "error";
      }
    }

    if (copyState === "success") {
      if (isCaptivePortal) {
        attemptCloseCaptiveWindow();
        closeInstructionsVisible = true;
      }

      setTimeout(() => {
        copyState = "idle";
      }, 3000);
    }
  }

  function attemptCloseCaptiveWindow() {
    if (!isCaptivePortal || closeAttempted) {
      return;
    }

    closeAttempted = true;

    let wasClosed = false;

    try {
      window.close();
      wasClosed = window.closed;
    } catch (err) {
      wasClosed = false;
    }

    if (!wasClosed) {
      // Try a common workaround where the captive window allows replacing itself first.
      try {
        window.open("", "_self");
        window.close();
        wasClosed = window.closed;
      } catch (err) {
        wasClosed = false;
      }
    }

    if (wasClosed) {
      closeSucceeded = true;
      return;
    }

    setTimeout(() => {
      if (window.closed || document.visibilityState === "hidden") {
        closeSucceeded = true;
      }
    }, 400);
  }
</script>

<section class="mt-6">
  <div class="rounded-lg border border-slate-200 bg-white/90 p-6 shadow-md backdrop-blur dark:border-slate-700 dark:bg-slate-800/80">
    <h1 class="text-2xl font-semibold text-slate-800 dark:text-slate-100">
      Gå til oppsettet i nettleseren din
    </h1>
    <p class="mt-3 text-slate-600 dark:text-slate-300">
      Dette vinduet er bare tilkoblingshjelp. Kopier adressen under, lukk vinduet, og lim den inn i nettleseren du vil bruke.
    </p>

    <div class="mt-5 space-y-5">
      <div class="rounded-md border border-slate-200 bg-slate-50 p-4 text-sm text-slate-700 dark:border-slate-700 dark:bg-slate-900 dark:text-slate-200">
        <p class="font-medium text-slate-800 dark:text-slate-100">1. Kopier adressen</p>
        <p class="mt-2">
          Adressen er <code class="rounded bg-white px-2 py-1 font-mono text-sm text-slate-900 dark:bg-slate-800 dark:text-slate-100">{setupUrl}</code> og virker mens du er tilkoblet <strong>NEAS-WATTUP</strong>.
        </p>
        <div class="mt-3 flex flex-wrap items-center gap-2">
          <button
            class="rounded border border-slate-300 bg-white px-3 py-1 text-xs font-semibold uppercase tracking-wide text-slate-700 transition hover:border-slate-400 hover:text-slate-900 dark:border-slate-600 dark:bg-slate-800 dark:text-slate-200 dark:hover:border-slate-500"
            on:click={copySetupUrl}
          >
            {#if copyState === "pending"}
              Kopierer…
            {:else if copyState === "success"}
              Kopiert!
            {:else if copyState === "error"}
              Kopier mislyktes
            {:else}
              Kopier til utklippstavlen
            {/if}
          </button>
          <span class="text-xs text-slate-500 dark:text-slate-400">
            {#if copyState === "success"}
              Lim adressen inn i nettleserens adresselinje.
            {:else if copyState === "error"}
              Marker adressen og kopier manuelt (for eksempel med ⌘+C).
            {:else}
              Trykk knappen, eller marker og kopier adressen manuelt.
            {/if}
          </span>
          {#if closeAttempted && !closeSucceeded}
            <span class="text-xs text-slate-500 dark:text-slate-400">
              Vi forsøker å lukke vinduet automatisk. Hvis det fortsatt er åpent, lukk det selv og følg stegene under.
            </span>
          {/if}
          {#if closeSucceeded}
            <span class="text-xs text-slate-500 dark:text-slate-400">
              Vinduet skal lukke seg automatisk om et øyeblikk.
            </span>
          {/if}
        </div>
      </div>

      <div class="rounded-md border border-blue-200 bg-blue-50 p-4 text-sm text-blue-900 dark:border-blue-900/60 dark:bg-blue-900/30 dark:text-blue-100">
        <p class="font-medium">2. Åpne den i nettleseren</p>
        <div class="mt-2 space-y-2">
          {#if isIosPortal}
            <ol class="list-decimal space-y-1 pl-5">
              <li>Kopier adressen.</li>
              <li>Trykk <strong>Ferdig</strong> øverst til høyre.</li>
              <li>Velg <strong>Behold tilkoblingen</strong>.</li>
              <li>Åpne Safari og lim inn adressen i adresselinjen.</li>
            </ol>
          {:else if isMacPortal}
            <ol class="list-decimal space-y-1 pl-5">
              <li>Kopier adressen.</li>
              <li>Lukk vinduet.</li>
              <li>Når dialogen dukker opp, velg <strong>Fortsett uten internett</strong>.</li>
              <li>Åpne Safari og lim inn adressen (⌘+V).</li>
            </ol>
          {:else if isAndroidPortal}
            <ol class="list-decimal space-y-1 pl-5">
              <li>Kopier adressen.</li>
              <li>Trykk tilbakeknappen for å lukke vinduet.</li>
              <li>Åpne Chrome (eller ønsket nettleser) og lim inn adressen.</li>
            </ol>
          {:else}
            <p>Kopier adressen, lukk dette vinduet, velg <strong>Fortsett uten nettverk/internet (Continue without network)</strong> og lim adressen inn i nettleseren du foretrekker.</p>
          {/if}
        </div>
      </div>

      {#if isCaptivePortal && closeInstructionsVisible}
        <div class="rounded-md border border-amber-300 bg-amber-50 p-4 text-sm text-amber-900 dark:border-amber-900/60 dark:bg-amber-900/20 dark:text-amber-100">
          <p class="font-medium">Tips når du lukker vinduet</p>
          <p class="mt-2">
            Velg <strong>Behold tilkoblingen</strong> eller <strong>Fortsett uten internett</strong> dersom du får spørsmål. Unngå «Koble fra nettverket», ellers mister du forbindelsen til enheten.
          </p>
        </div>
      {/if}
    </div>
  </div>
</section>
