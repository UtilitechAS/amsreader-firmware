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
      "/data.json": "http://192.168.233.244",
      "/energyprice.json": "http://192.168.233.244",
      "/dayplot.json": "http://192.168.233.244",
      "/monthplot.json": "http://192.168.233.244",
      "/temperature.json": "http://192.168.233.244",
      "/sysinfo.json": "http://192.168.233.244",
      "/configuration.json": "http://192.168.233.244",
      "/tariff.json": "http://192.168.233.244",
      "/save": "http://192.168.233.244",
      "/reboot": "http://192.168.233.244",
      "/configfile": "http://192.168.233.244",
      "/upgrade": "http://192.168.233.244"
    }
  }
})
