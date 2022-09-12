const config = {
  content: ["./index.html","./src/**/*.{html,js,svelte,ts}"],

  theme: {
    extend: {},
  },

  plugins: [
    require('@tailwindcss/forms')
  ],
};

module.exports = config;
