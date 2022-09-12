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
      "/data.json": "http://192.168.233.235",
      "/energyprice.json": "http://192.168.233.235",
      "/dayplot.json": "http://192.168.233.235",
      "/monthplot.json": "http://192.168.233.235",
      "/temperature.json": "http://192.168.233.235",
      "/configuration.json": "http://192.168.233.244"
    }
  }
})
