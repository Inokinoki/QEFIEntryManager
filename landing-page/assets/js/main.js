// Smooth scroll for navigation links
document.querySelectorAll('a[href^="#"]').forEach(anchor => {
    anchor.addEventListener('click', function (e) {
        e.preventDefault();
        const target = document.querySelector(this.getAttribute('href'));
        if (target) {
            target.scrollIntoView({
                behavior: 'smooth',
                block: 'start'
            });
        }
    });
});

// Copy code functionality
const copyButtons = document.querySelectorAll('.copy-btn');
copyButtons.forEach(button => {
    button.addEventListener('click', async () => {
        const codeId = button.getAttribute('data-code');
        const codeElement = document.getElementById(codeId);

        if (codeElement) {
            try {
                await navigator.clipboard.writeText(codeElement.textContent);
                button.textContent = 'Copied!';
                button.style.background = 'rgba(76, 175, 80, 0.3)';

                setTimeout(() => {
                    button.textContent = 'Copy';
                    button.style.background = 'transparent';
                }, 2000);
            } catch (err) {
                console.error('Failed to copy:', err);
                button.textContent = 'Failed';
                setTimeout(() => {
                    button.textContent = 'Copy';
                }, 2000);
            }
        }
    });
});

// Add animation on scroll
const observerOptions = {
    threshold: 0.1,
    rootMargin: '0px 0px -50px 0px'
};

const observer = new IntersectionObserver((entries) => {
    entries.forEach(entry => {
        if (entry.isIntersecting) {
            entry.target.style.opacity = '1';
            entry.target.style.transform = 'translateY(0)';
        }
    });
}, observerOptions);

// Observe all cards
document.querySelectorAll('.feature-card, .download-card, .screenshot-card').forEach(card => {
    card.style.opacity = '0';
    card.style.transform = 'translateY(20px)';
    card.style.transition = 'opacity 0.6s ease, transform 0.6s ease';
    observer.observe(card);
});

// Mobile menu toggle (if needed in future)
const setupMobileMenu = () => {
    const nav = document.querySelector('.nav-menu');
    // Could add hamburger menu here if needed
};

// Add active state to navigation based on scroll position
const sections = document.querySelectorAll('section[id]');
const navLinks = document.querySelectorAll('.nav-link');

window.addEventListener('scroll', () => {
    let current = '';
    sections.forEach(section => {
        const sectionTop = section.offsetTop;
        const sectionHeight = section.clientHeight;
        if (scrollY >= sectionTop - 200) {
            current = section.getAttribute('id');
        }
    });

    navLinks.forEach(link => {
        link.style.color = '';
        if (link.getAttribute('href') === `#${current}`) {
            link.style.color = 'var(--color-primary)';
        }
    });
});

// Add loading animation
window.addEventListener('load', () => {
    document.body.style.opacity = '0';
    setTimeout(() => {
        document.body.style.transition = 'opacity 0.3s ease';
        document.body.style.opacity = '1';
    }, 100);
});

// Fetch GitHub repository stats
async function fetchGitHubStats() {
    const repo = 'Inokinoki/QEFIEntryManager';
    const starsElement = document.getElementById('github-stars');
    const forksElement = document.getElementById('github-forks');

    try {
        const response = await fetch(`https://api.github.com/repos/${repo}`);
        if (!response.ok) throw new Error('API request failed');

        const data = await response.json();

        // Format numbers (e.g., 1234 -> 1.2k)
        const formatNumber = (num) => {
            if (num >= 1000) {
                return (num / 1000).toFixed(1) + 'k';
            }
            return num.toString();
        };

        if (starsElement) {
            starsElement.textContent = formatNumber(data.stargazers_count);
        }

        if (forksElement) {
            forksElement.textContent = formatNumber(data.forks_count);
        }
    } catch (error) {
        console.error('Failed to fetch GitHub stats:', error);
        // Set default values on error
        if (starsElement) starsElement.textContent = '★';
        if (forksElement) forksElement.textContent = '⑂';
    }
}

// Fetch stats when page loads
fetchGitHubStats();

