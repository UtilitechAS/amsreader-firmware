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
      "/data.json": "http://192.168.233.69",
      "/energyprice.json": "http://192.168.233.69",
      "/dayplot.json": "http://192.168.233.69",
      "/monthplot.json": "http://192.168.233.69",
      "/temperature.json": "http://192.168.233.69",
      "/sysinfo.json": "http://192.168.233.69",
      "/configuration.json": "http://192.168.233.69",
      "/tariff.json": "http://192.168.233.69",
      "/realtime.json": "http://192.168.233.69",
      "/priceconfig.json": "http://192.168.233.69",
      "/cloudkey.json": "http://192.168.233.69",
      "/save": "http://192.168.233.69",
      "/reboot": "http://192.168.233.69",
      "/configfile": "http://192.168.233.69",
      "/upgrade": "http://192.168.233.69",
      "/mqtt-ca": "http://192.168.233.69",
      "/mqtt-cert": "http://192.168.233.69",
      "/mqtt-key": "http://192.168.233.69",
      "/logo.svg": "http://192.168.233.69",
    }
  }
})
