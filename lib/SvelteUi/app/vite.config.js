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
      "/data.json": "http://192.168.233.229",
      "/energyprice.json": "http://192.168.233.229",
      "/dayplot.json": "http://192.168.233.229",
      "/monthplot.json": "http://192.168.233.229",
      "/temperature.json": "http://192.168.233.229",
      "/sysinfo.json": "http://192.168.233.229",
      "/configuration.json": "http://192.168.233.229",
      "/tariff.json": "http://192.168.233.229",
      "/save": "http://192.168.233.229",
      "/reboot": "http://192.168.233.229",
      "/upgrade": "http://192.168.233.229"
    }
  }
})
