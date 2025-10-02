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
      "/data.json": "http://192.168.21.122",
      "/energyprice.json": "http://192.168.21.122",
      "/dayplot.json": "http://192.168.21.122",
      "/monthplot.json": "http://192.168.21.122",
      "/temperature.json": "http://192.168.21.122",
      "/sysinfo.json": "http://192.168.21.122",
      "/configuration.json": "http://192.168.21.122",
      "/tariff.json": "http://192.168.21.122",
      "/realtime.json": "http://192.168.21.122",
      "/priceconfig.json": "http://192.168.21.122",
      "/translations.json": "http://192.168.21.122",
      "/cloudkey.json": "http://192.168.21.122",
      "/wifiscan.json": "http://192.168.21.122",
      "/save": "http://192.168.21.122",
      "/reboot": "http://192.168.21.122",
      "/configfile": "http://192.168.21.122",
      "/upgrade": "http://192.168.21.122",
      "/mqtt-ca": "http://192.168.21.122",
      "/mqtt-cert": "http://192.168.21.122",
      "/mqtt-key": "http://192.168.21.122",
      "/logo.svg": "http://192.168.21.122",
    }
  }
})
