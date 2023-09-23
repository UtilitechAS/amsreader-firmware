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
      "/data.json": "http://192.168.233.209",
      "/energyprice.json": "http://192.168.233.209",
      "/dayplot.json": "http://192.168.233.209",
      "/monthplot.json": "http://192.168.233.209",
      "/temperature.json": "http://192.168.233.209",
      "/sysinfo.json": "http://192.168.233.209",
      "/configuration.json": "http://192.168.233.209",
      "/tariff.json": "http://192.168.233.209",
      "/save": "http://192.168.233.209",
      "/reboot": "http://192.168.233.209",
      "/configfile": "http://192.168.233.209",
      "/upgrade": "http://192.168.233.209"
    }
  }
})
