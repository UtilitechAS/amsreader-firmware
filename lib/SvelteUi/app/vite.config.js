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
      "/data.json": "http://192.168.233.115",
      "/energyprice.json": "http://192.168.233.115",
      "/dayplot.json": "http://192.168.233.115",
      "/monthplot.json": "http://192.168.233.115",
      "/temperature.json": "http://192.168.233.115",
      "/sysinfo.json": "http://192.168.233.115",
      "/configuration.json": "http://192.168.233.115",
      "/tariff.json": "http://192.168.233.115",
      "/realtime.json": "http://192.168.233.115",
      "/priceconfig.json": "http://192.168.233.115",
      "/cloudkey.json": "http://192.168.233.115",
      "/save": "http://192.168.233.115",
      "/reboot": "http://192.168.233.115",
      "/configfile": "http://192.168.233.115",
      "/upgrade": "http://192.168.233.115",
      "/mqtt-ca": "http://192.168.233.115",
      "/mqtt-cert": "http://192.168.233.115",
      "/mqtt-key": "http://192.168.233.115",
      "/logo.svg": "http://192.168.233.115",
    }
  }
})
