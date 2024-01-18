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
      "/data.json": "http://192.168.233.110",
      "/energyprice.json": "http://192.168.233.110",
      "/dayplot.json": "http://192.168.233.110",
      "/monthplot.json": "http://192.168.233.110",
      "/temperature.json": "http://192.168.233.110",
      "/sysinfo.json": "http://192.168.233.110",
      "/configuration.json": "http://192.168.233.110",
      "/tariff.json": "http://192.168.233.110",
      "/realtime.json": "http://192.168.233.110",
      "/priceconfig.json": "http://192.168.233.110",
      "/save": "http://192.168.233.110",
      "/reboot": "http://192.168.233.110",
      "/configfile": "http://192.168.233.110",
      "/upgrade": "http://192.168.233.110",
      "/mqtt-ca": "http://192.168.233.110",
      "/mqtt-cert": "http://192.168.233.110",
      "/mqtt-key": "http://192.168.233.110",
    }
  }
})
