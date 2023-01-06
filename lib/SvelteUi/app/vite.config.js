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
      "/data.json": "https://ams2mqtt.no23.cc",
      "/energyprice.json": "https://ams2mqtt.no23.cc",
      "/dayplot.json": "https://ams2mqtt.no23.cc",
      "/monthplot.json": "https://ams2mqtt.no23.cc",
      "/temperature.json": "https://ams2mqtt.no23.cc",
      "/sysinfo.json": "https://ams2mqtt.no23.cc",
      "/configuration.json": "http://192.168.233.229",
      "/tariff.json": "https://ams2mqtt.no23.cc",
      "/save": "http://192.168.233.229",
      "/reboot": "http://192.168.233.229",
      "/configfile": "http://192.168.233.229",
      "/upgrade": "http://192.168.233.229"
    }
  }
})
