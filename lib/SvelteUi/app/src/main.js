import "./app.postcss";
import App from "./App.svelte";

if (localStorage.theme === 'dark'
    // Automatic darkmode:
    //|| (!('theme' in localStorage) && window.matchMedia('(prefers-color-scheme: dark)').matches)
    ) {
      document.documentElement.classList.add('dark')
  } else {
      document.documentElement.classList.remove('dark')
}

const app = new App({
  target: document.getElementById("app"),
});

export default app;
