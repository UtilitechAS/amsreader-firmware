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
      "/data.json": "http://192.168.21.192",
      "/energyprice.json": "http://192.168.21.192",
      "/dayplot.json": "http://192.168.21.192",
      "/monthplot.json": "http://192.168.21.192",
      "/temperature.json": "http://192.168.21.192",
      "/sysinfo.json": "http://192.168.21.192",
      "/configuration.json": "http://192.168.21.192",
      "/tariff.json": "http://192.168.21.192",
      "/realtime.json": "http://192.168.21.192",
      "/priceconfig.json": "http://192.168.21.192",
      "/translations.json": "http://192.168.21.192",
      "/cloudkey.json": "http://192.168.21.192",
      "/wifiscan.json": "http://192.168.21.192",
      "/save": "http://192.168.21.192",
      "/reboot": "http://192.168.21.192",
      "/configfile": "http://192.168.21.192",
      "/upgrade": "http://192.168.21.192",
      "/mqtt-ca": "http://192.168.21.192",
      "/mqtt-cert": "http://192.168.21.192",
      "/mqtt-key": "http://192.168.21.192",
      "/logo.svg": "http://192.168.21.192",
    }
  }
})
