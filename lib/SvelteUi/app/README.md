# SvelteUi App

Web interface for AMS Reader firmware built with Svelte 5 and Vite 6.

## Development Setup

### Prerequisites
- Node.js 20.x or 22.x LTS (required for Vite 6)
- npm

### Local Development Configuration

To develop against your AMS reader device, you need to configure the proxy target:

1. Copy the example config file:
   ```bash
   cp vite.config.local.example.js vite.config.local.js
   ```

2. Edit `vite.config.local.js` and update the IP address to match your device:
   ```javascript
   export default {
     proxyTarget: "http://192.168.1.100"  // Your device's IP
   }
   ```

3. The `vite.config.local.js` file is gitignored, so your personal settings won't be committed.

### Running Development Server

```bash
npm install
npm run dev
```

The dev server will proxy API requests to your configured device IP.

### Building for Production

```bash
npm run build
```

The build output will be in the `dist/` directory.

## Project Structure

- `src/` - Application source code
  - `routes/` - Page components using svelte-spa-router
  - `lib/` - Shared components and utilities
- `public/` - Static assets (favicon, etc.)
- `dist/` - Build output (not committed to git)

## Key Technologies

- **Svelte 5.17.0** - UI framework
- **Vite 6.0.7** - Build tool
- **svelte-spa-router 4.0.1** - Hash-based routing
- **Tailwind CSS** - Styling
