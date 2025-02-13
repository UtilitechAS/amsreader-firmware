#!/bin/bash
set -e

# Upgrade pip
python -m pip install --upgrade pip

# Install Python packages
pip install -U platformio css_html_js_minify

# Navigate to the Svelte app directory
cd lib/SvelteUi/app

# Install npm dependencies and build the app
npm ci
npm run build

# Return to the previous directory
cd -