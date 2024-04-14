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
      "/data.json": "http://192.168.233.184",
      "/energyprice.json": "http://192.168.233.184",
      "/dayplot.json": "http://192.168.233.184",
      "/monthplot.json": "http://192.168.233.184",
      "/temperature.json": "http://192.168.233.184",
      "/sysinfo.json": "http://192.168.233.184",
      "/configuration.json": "http://192.168.233.184",
      "/tariff.json": "http://192.168.233.184",
      "/realtime.json": "http://192.168.233.184",
      "/priceconfig.json": "http://192.168.233.184",
      "/save": "http://192.168.233.184",
      "/reboot": "http://192.168.233.184",
      "/configfile": "http://192.168.233.184",
      "/upgrade": "http://192.168.233.184",
      "/mqtt-ca": "http://192.168.233.184",
      "/mqtt-cert": "http://192.168.233.184",
      "/mqtt-key": "http://192.168.233.184",
      "/logo.svg": "http://192.168.233.184",
    }
  }
})
