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
      "/data.json": "http://192.168.233.237",
      "/energyprice.json": "http://192.168.233.237",
      "/dayplot.json": "http://192.168.233.237",
      "/monthplot.json": "http://192.168.233.237",
      "/temperature.json": "http://192.168.233.237",
      "/sysinfo.json": "http://192.168.233.237",
      "/configuration.json": "http://192.168.233.237",
      "/tariff.json": "http://192.168.233.237",
      "/save": "http://192.168.233.237",
      "/reboot": "http://192.168.233.237",
      "/configfile": "http://192.168.233.237",
      "/upgrade": "http://192.168.233.237",
      "/mqtt-ca": "http://192.168.233.237",
      "/mqtt-cert": "http://192.168.233.237",
      "/mqtt-key": "http://192.168.233.237",
    }
  }
})
