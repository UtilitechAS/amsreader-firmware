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
      "/data.json": "http://192.168.233.187",
      "/energyprice.json": "http://192.168.233.187",
      "/dayplot.json": "http://192.168.233.187",
      "/monthplot.json": "http://192.168.233.187",
      "/temperature.json": "http://192.168.233.187",
      "/sysinfo.json": "http://192.168.233.187",
      "/configuration.json": "http://192.168.233.187",
      "/tariff.json": "http://192.168.233.187",
      "/realtime.json": "http://192.168.233.187",
      "/priceconfig.json": "http://192.168.233.187",
      "/save": "http://192.168.233.187",
      "/reboot": "http://192.168.233.187",
      "/configfile": "http://192.168.233.187",
      "/upgrade": "http://192.168.233.187",
      "/mqtt-ca": "http://192.168.233.187",
      "/mqtt-cert": "http://192.168.233.187",
      "/mqtt-key": "http://192.168.233.187",
    }
  }
})
