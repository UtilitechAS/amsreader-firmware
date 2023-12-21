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
      "/data.json": "http://192.168.233.49",
      "/energyprice.json": "http://192.168.233.49",
      "/dayplot.json": "http://192.168.233.49",
      "/monthplot.json": "http://192.168.233.49",
      "/temperature.json": "http://192.168.233.49",
      "/sysinfo.json": "http://192.168.233.49",
      "/configuration.json": "http://192.168.233.49",
      "/tariff.json": "http://192.168.233.49",
      "/save": "http://192.168.233.49",
      "/reboot": "http://192.168.233.49",
      "/configfile": "http://192.168.233.49",
      "/upgrade": "http://192.168.233.49",
      "/mqtt-ca": "http://192.168.233.49",
      "/mqtt-cert": "http://192.168.233.49",
      "/mqtt-key": "http://192.168.233.49",
    }
  }
})
