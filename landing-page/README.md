# QEFI Entry Manager Landing Page

A modern, responsive landing page for the QEFI Entry Manager project, deployed to GitHub Pages.

## Features

- Modern, clean design with gradient accents
- Fully responsive layout (mobile, tablet, desktop)
- Download links for all supported platforms
- Screenshot gallery
- Feature showcase
- Build from source instructions
- Smooth scrolling and animations
- Dark code blocks with copy functionality

## Local Development

To view the landing page locally:

1. Simply open `index.html` in a web browser, or
2. Use a local server for better testing:

```bash
# Python 3
cd landing-page
python -m http.server 8000

# Node.js (if you have http-server installed)
npx http-server landing-page

# PHP
php -S localhost:8000 -t landing-page
```

Then visit `http://localhost:8000`

## Deployment

The landing page is automatically deployed to GitHub Pages using the workflow in `.github/workflows/deploy-landing-page.yml`.

### Manual Deployment

The workflow runs automatically on pushes to the `master` or `landing-page` branches. You can also trigger it manually from the Actions tab in GitHub.

### GitHub Pages Configuration

To enable GitHub Pages:

1. Go to your repository Settings
2. Navigate to Pages
3. Under "Build and deployment", select "Source" as "GitHub Actions"
4. The workflow will automatically deploy the `landing-page` directory

## Customization

### Colors

Edit the CSS variables in `assets/css/style.css`:

```css
:root {
    --color-primary: #4CAF50;
    --color-secondary: #2196F3;
    --color-accent: #FF9800;
    /* ... more variables */
}
```

### Content

Edit `index.html` to modify:
- Hero section text
- Feature descriptions
- Download links
- Screenshots
- Build instructions

### Screenshots

Place screenshots in the `.github/` directory (referenced with relative paths like `../.github/screenshot.png`).

## Structure

```
landing-page/
├── index.html              # Main HTML file
├── assets/
│   ├── css/
│   │   └── style.css      # Main stylesheet
│   └── js/
│       └── main.js        # Interactive functionality
└── README.md              # This file
```

## Browser Support

- Chrome/Edge (latest)
- Firefox (latest)
- Safari (latest)
- Mobile browsers (iOS Safari, Chrome Mobile)

## License

Same as the main project (GPL-3.0)
