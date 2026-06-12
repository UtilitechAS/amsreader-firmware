import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

// Try to import local config, fall back to default if not found
let localConfig = { proxyTarget: "http://192.168.4.1" };
try {
  const imported = await import('./vite.config.local.js');
  localConfig = imported.default;
} catch (e) {
  console.log('No vite.config.local.js found, using default proxy target:', localConfig.proxyTarget);
  console.log('Copy vite.config.local.example.js to vite.config.local.js to customize');
}

// https://vitejs.dev/config/
export default defineConfig({
  build: {
    outDir: 'dist',
    assetsDir: '.',
    minify: 'esbuild',
    target: 'es2020',
    rollupOptions: {
      output: {
        assetFileNames: '[name][extname]',
        chunkFileNames: '[name].js',
        entryFileNames: '[name].js',
        manualChunks: undefined
      }
    }
  },
  plugins: [svelte({
    compilerOptions: {
      dev: false
    }
  })],
  server: {
    proxy: {
      "/data.json": localConfig.proxyTarget,
      "/energyprice.json": localConfig.proxyTarget,
      "/importprice.json": localConfig.proxyTarget,
      "/exportprice.json": localConfig.proxyTarget,
      "/dayplot.json": localConfig.proxyTarget,
      "/monthplot.json": localConfig.proxyTarget,
      "/temperature.json": localConfig.proxyTarget,
      "/sysinfo.json": localConfig.proxyTarget,
      "/configuration.json": localConfig.proxyTarget,
      "/tariff.json": localConfig.proxyTarget,
      "/realtime.json": localConfig.proxyTarget,
      "/priceconfig.json": localConfig.proxyTarget,
      "/translations.json": localConfig.proxyTarget,
      "/cloudkey.json": localConfig.proxyTarget,
      "/wifiscan.json": localConfig.proxyTarget,
      "/save": localConfig.proxyTarget,
      "/reboot": localConfig.proxyTarget,
      "/configfile": localConfig.proxyTarget,
      "/upgrade": localConfig.proxyTarget,
      "/mqtt-ca": localConfig.proxyTarget,
      "/mqtt-cert": localConfig.proxyTarget,
      "/mqtt-key": localConfig.proxyTarget,
      "/logo.svg": localConfig.proxyTarget,
    }
  }
})
