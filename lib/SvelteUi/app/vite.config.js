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
      "/data.json": "http://192.168.233.172",
      "/energyprice.json": "http://192.168.233.172",
      "/dayplot.json": "http://192.168.233.172",
      "/monthplot.json": "http://192.168.233.172",
      "/temperature.json": "http://192.168.233.172",
      "/sysinfo.json": "http://192.168.233.172",
      "/configuration.json": "http://192.168.233.172",
      "/tariff.json": "http://192.168.233.172",
      "/priceconfig.json": "http://192.168.233.172",
      "/save": "http://192.168.233.172",
      "/reboot": "http://192.168.233.172",
      "/configfile": "http://192.168.233.172",
      "/upgrade": "http://192.168.233.172",
      "/mqtt-ca": "http://192.168.233.172",
      "/mqtt-cert": "http://192.168.233.172",
      "/mqtt-key": "http://192.168.233.172",
    }
  }
})
