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
      "/data.json": "http://192.168.21.87",
      "/energyprice.json": "http://192.168.21.87",
      "/dayplot.json": "http://192.168.21.87",
      "/monthplot.json": "http://192.168.21.87",
      "/temperature.json": "http://192.168.21.87",
      "/sysinfo.json": "http://192.168.21.87",
      "/configuration.json": "http://192.168.21.87",
      "/tariff.json": "http://192.168.21.87",
      "/realtime.json": "http://192.168.21.87",
      "/priceconfig.json": "http://192.168.21.87",
      "/save": "http://192.168.21.87",
      "/reboot": "http://192.168.21.87",
      "/configfile": "http://192.168.21.87",
      "/upgrade": "http://192.168.21.87",
      "/mqtt-ca": "http://192.168.21.87",
      "/mqtt-cert": "http://192.168.21.87",
      "/mqtt-key": "http://192.168.21.87",
    }
  }
})
