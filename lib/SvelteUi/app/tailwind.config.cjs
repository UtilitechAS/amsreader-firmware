const config = {
  content: ["./index.html", "./src/**/*.{html,js,svelte,ts}"],
  darkMode: 'class', // use 'class' for manual dark mode
  theme: {
    extend: {
      colors: {
        // NEAS brand colors
        'neas-green': '#0d3a2d',
        'neas-green-90': '#1b483a',
        'neas-green-100': '#225C4B',
        'neas-lightgreen': '#95c672',
        'neas-lightgreen-30': '#ecf4e5',
        'neas-gray': '#f1f6f5',
        'neas-gray-100': '#e0f0ed',
        'neas-gray-300': '#CCCCCC',
        'neas-red': '#c02b46',
        'neas-blue': '#385d76',
        'neas-yellow': '#f1e967',
        'neas-lightblue-new': '#c9e8f3'
      }
    },
  },
  plugins: [],
};

module.exports = config;
