import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

// https://vitejs.dev/config/
export default defineConfig({
  build: {
    outDir: 'dist',
    assetsDir: '.',
    rollupOptions: {
      output: {
        assetFileNames: '[name][extname]',
        chunkFileNames: '[name].js',
        entryFileNames: '[name].js'
      }
    }
  },
  plugins: [svelte()],
  server: {
    proxy: {
      "/data.json": "http://192.168.233.154",
      "/energyprice.json": "http://192.168.233.154",
      "/dayplot.json": "http://192.168.233.154",
      "/monthplot.json": "http://192.168.233.154",
      "/temperature.json": "http://192.168.233.154",
      "/sysinfo.json": "http://192.168.233.154",
      "/configuration.json": "http://192.168.233.154",
      "/tariff.json": "http://192.168.233.154",
      "/realtime.json": "http://192.168.233.154",
      "/priceconfig.json": "http://192.168.233.154",
      "/translations.json": "http://192.168.233.154",
      "/cloudkey.json": "http://192.168.233.154",
      "/save": "http://192.168.233.154",
      "/reboot": "http://192.168.233.154",
      "/configfile": "http://192.168.233.154",
      "/upgrade": "http://192.168.233.154",
      "/mqtt-ca": "http://192.168.233.154",
      "/mqtt-cert": "http://192.168.233.154",
      "/mqtt-key": "http://192.168.233.154",
      "/logo.svg": "http://192.168.233.154",
    }
  }
})
