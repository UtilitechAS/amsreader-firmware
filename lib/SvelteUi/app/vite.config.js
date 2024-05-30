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
      "/data.json": "http://192.168.4.1",
      "/energyprice.json": "http://192.168.4.1",
      "/dayplot.json": "http://192.168.4.1",
      "/monthplot.json": "http://192.168.4.1",
      "/temperature.json": "http://192.168.4.1",
      "/sysinfo.json": "http://192.168.4.1",
      "/configuration.json": "http://192.168.4.1",
      "/tariff.json": "http://192.168.4.1",
      "/realtime.json": "http://192.168.4.1",
      "/priceconfig.json": "http://192.168.4.1",
      "/cloudkey.json": "http://192.168.4.1",
      "/save": "http://192.168.4.1",
      "/reboot": "http://192.168.4.1",
      "/configfile": "http://192.168.4.1",
      "/upgrade": "http://192.168.4.1",
      "/mqtt-ca": "http://192.168.4.1",
      "/mqtt-cert": "http://192.168.4.1",
      "/mqtt-key": "http://192.168.4.1",
      "/logo.svg": "http://192.168.4.1",
    }
  }
})
