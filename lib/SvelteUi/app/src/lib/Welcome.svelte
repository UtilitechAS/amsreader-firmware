<script>
  import { onDestroy, onMount } from "svelte";

  export let basepath = "/";

  let setupUrl = "http://192.168.4.1/setup";
  let autoRedirect;
  let redirectAttempted = false;
  let redirectBlocked = false;
  let isCaptivePortal = false;
  let copyState = "idle";
  let manualOpenAttempted = false;

  onMount(() => {
    setupUrl = resolveSetupUrl();

    isCaptivePortal = detectCaptivePortal();

    if (!isCaptivePortal) {
      autoRedirect = setTimeout(() => {
        redirectToBrowser(false);
      }, 600);
    }
  });

  onDestroy(() => {
    if (autoRedirect) {
      clearTimeout(autoRedirect);
    }
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

  function redirectToBrowser(allowSelfNavigation = false) {
    redirectAttempted = true;
    const target = setupUrl;
    const popup = window.open(target, "_blank", "noopener,noreferrer");
    const opened = popup && popup.closed === false;

    if (!opened) {
      redirectBlocked = true;
      if (allowSelfNavigation) {
        window.location.href = target;
      }
    } else {
      redirectBlocked = false;
      if (isCaptivePortal) {
        window.focus();
      }
    }
  }

  function handleManualClick(event) {
    event.preventDefault();
    manualOpenAttempted = true;
    redirectToBrowser(true);
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

    return patterns.some((pattern) => pattern.test(ua));
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
      setTimeout(() => {
        copyState = "idle";
      }, 3000);
    }
  }
</script>

<section class="mt-6">
  <div class="rounded-lg border border-slate-200 bg-white/90 p-6 shadow-md backdrop-blur dark:border-slate-700 dark:bg-slate-800/80">
    <h1 class="text-2xl font-semibold text-slate-800 dark:text-slate-100">
      Åpner oppsettet i nettleseren din
    </h1>
    <p class="mt-3 text-slate-600 dark:text-slate-300">
      Vi prøver automatisk å åpne hele konfigurasjonen i nettleseren. Hvis ingenting skjer,
      trykk på knappen under for å gå videre.
    </p>
    {#if redirectAttempted && redirectBlocked}
      <p class="mt-2 text-sm text-yellow-700 dark:text-yellow-200">
        Nettleseren blokkerte automatisk åpning. Trykk på knappen for å fortsette.
      </p>
    {/if}
    {#if isCaptivePortal}
      <div class="mt-3 rounded-md border border-blue-200 bg-blue-50 px-4 py-3 text-sm text-blue-900 dark:border-blue-900/60 dark:bg-blue-900/30 dark:text-blue-100">
        <p class="font-medium">Denne siden vises i et påloggingsvindu.</p>
        <p class="mt-2">
          Trykk <strong>Åpne i nettleser</strong>, og velg Safari, Chrome eller din vanlige nettleser når du får valget.
          Hvis ingenting åpner seg, kopier adressen og lim den inn i nettleseren selv.
        </p>
      </div>
    {/if}
    <p class="mt-4 text-sm text-slate-500 dark:text-slate-400">
      Adressen er <code class="rounded bg-slate-100 px-2 py-1 text-slate-800 dark:bg-slate-900 dark:text-slate-200">{setupUrl}</code> mens du er tilkoblet <strong>NEAS-WATTUP</strong>.<br />
      Du kan alltid skrive den inn manuelt i en nettleser hvis vinduet lukker seg.
    </p>

    <div class="mt-6">
      <a
        class="btn-pri inline-flex items-center justify-center"
        href={setupUrl}
        rel="noopener"
        target="_blank"
        on:click={handleManualClick}
      >
        Åpne i nettleser
      </a>
      <div class="mt-3 flex flex-wrap items-center gap-2 text-sm text-slate-500 dark:text-slate-400">
        <button
          class="rounded border border-slate-300 px-2 py-1 text-xs font-medium text-slate-600 transition hover:border-slate-400 hover:text-slate-700 dark:border-slate-700 dark:text-slate-200 dark:hover:border-slate-500"
          on:click={copySetupUrl}
        >
          {#if copyState === "pending"}
            Kopierer…
          {:else if copyState === "success"}
            Kopiert!
          {:else if copyState === "error"}
            Kunne ikke kopiere
          {:else}
            Kopier adressen
          {/if}
        </button>
        {#if manualOpenAttempted && redirectBlocked}
          <span>Velg «Åpne i nettleser» i dialogen hvis den vises.</span>
        {:else if copyState !== "success"}
          <span>Kopier eller skriv den inn manuelt dersom ingenting åpner seg.</span>
        {/if}
      </div>
    </div>
  </div>
</section>
