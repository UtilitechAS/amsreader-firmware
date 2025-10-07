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

// import { defineConfig } from 'vite'
// import { svelte } from '@sveltejs/vite-plugin-svelte'

// const ip = "http://10.22.22.158";

// // https://vitejs.dev/config/
// export default defineConfig({
//   build: {
//     outDir: 'dist',
//     assetsDir: '.',
//     rollupOptions: {
//       output: {
//         assetFileNames: '[name][extname]',
//         chunkFileNames: '[name].js',
//         entryFileNames: '[name].js'
//       }
//     }
//   },
//   plugins: [svelte()],
//   server: {
//     proxy: {
//       "/data.json": ip + "/data.json",
//       "/energyprice.json": ip + "/energyprice.json",
//       "/dayplot.json": ip + "/dayplot.json",
//       "/monthplot.json": ip + "/monthplot.json",
//       "/temperature.json": ip + "/temperature.json",
//       "/sysinfo.json": ip + "/sysinfo.json",
//       "/configuration.json": ip + "/configuration.json",
//       "/tariff.json": ip + "/tariff.json",
//       "/realtime.json": ip + "/realtime.json",
//       "/priceconfig.json": ip + "/priceconfig.json",
//       "/translations.json": ip + "/translations.json",
//       "/cloudkey.json": ip + "/cloudkey.json",
//       "/wifiscan.json": ip + "/wifiscan.json",
//       "/save": ip + "/save",
//       "/reboot": ip + "/reboot",
//       "/configfile": ip + "/configfile",
//       "/upgrade": ip + "/upgrade",
//       "/mqtt-ca": ip + "/mqtt-ca",
//       "/mqtt-cert": ip + "/mqtt-cert",
//       "/mqtt-key": ip + "/mqtt-key",
//       "/logo.svg": ip + "/logo.svg",
//     },
//     port: 5173,
//     strictPort: true,
//     hmr: {
//       protocol: 'ws',
//       host: 'localhost',
//       port: 5173
//     }
//   }
// })

// import { defineConfig } from 'vite'
// import { svelte } from '@sveltejs/vite-plugin-svelte'

// // Allow overriding device IP via environment variable (e.g. VITE_DEVICE_IP=http://192.168.1.50)
// const ip = process.env.VITE_DEVICE_IP || "http://10.22.22.158";

// // https://vitejs.dev/config/
// export default defineConfig({
//   build: {
//     outDir: 'dist',
//     assetsDir: '.',
//     rollupOptions: {
//       output: {
//         assetFileNames: '[name][extname]',
//         chunkFileNames: '[name].js',
//         entryFileNames: '[name].js'
//       }
//     }
//   },
//   plugins: [svelte()],
//   server: {
//     proxy: {
//       "/data.json": ip + "/data.json",
//       "/energyprice.json": ip + "/energyprice.json",
//       "/dayplot.json": ip + "/dayplot.json",
//       "/monthplot.json": ip + "/monthplot.json",
//       "/temperature.json": ip + "/temperature.json",
//       "/sysinfo.json": ip + "/sysinfo.json",
//       "/configuration.json": ip + "/configuration.json",
//       "/tariff.json": ip + "/tariff.json",
//       "/realtime.json": ip + "/realtime.json",
//       "/priceconfig.json": ip + "/priceconfig.json",
//       "/translations.json": ip + "/translations.json",
//       "/cloudkey.json": ip + "/cloudkey.json",
//       "/wifiscan.json": ip + "/wifiscan.json",
//       "/save": ip + "/save",
//       "/reboot": ip + "/reboot",
//       "/configfile": ip + "/configfile",
//       "/upgrade": ip + "/upgrade",
//       "/mqtt-ca": ip + "/mqtt-ca",
//       "/mqtt-cert": ip + "/mqtt-cert",
//       "/mqtt-key": ip + "/mqtt-key",
//       "/logo.svg": ip + "/logo.svg",
//     },
//     port: 5173,
//     strictPort: true,
//     hmr: {
//       protocol: 'ws',
//       host: 'localhost',
//       port: 5173
//     }
//   }
// })