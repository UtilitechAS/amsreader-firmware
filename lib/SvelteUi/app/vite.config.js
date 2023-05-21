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
      "/data.json": "http://192.168.28.100",
      "/energyprice.json": "http://192.168.28.100",
      "/dayplot.json": "http://192.168.28.100",
      "/monthplot.json": "http://192.168.28.100",
      "/temperature.json": "http://192.168.28.100",
      "/sysinfo.json": "http://192.168.28.100",
      "/configuration.json": "http://192.168.28.100",
      "/tariff.json": "http://192.168.28.100",
      "/save": "http://192.168.28.100",
      "/reboot": "http://192.168.28.100",
      "/configfile": "http://192.168.28.100",
      "/upgrade": "http://192.168.28.100"
    }
  }
})
