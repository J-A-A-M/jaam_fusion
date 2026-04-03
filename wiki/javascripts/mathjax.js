// MathJax configuration for Material for MkDocs
// Works with pymdownx.arithmatex (generic: true), which outputs \( .. \) and \[ .. \]

window.MathJax = {
  tex: {
    inlineMath: [['\\(', '\\)']],
    displayMath: [['\\[', '\\]']],
    processEscapes: true,
    processEnvironments: true,
  },
  options: {
    // Skip code blocks and other tags where backslashes are common
    skipHtmlTags: ['script', 'noscript', 'style', 'textarea', 'pre', 'code'],
  },
};

// Re-render math when navigating (Material's instant loading)
// Fallbacks included for non-Material contexts.
function typesetMath() {
  if (!window.MathJax || !window.MathJax.typesetPromise) return;
  window.MathJax.typesetPromise();
}

if (typeof document$ !== 'undefined' && document$ && document$.subscribe) {
  document$.subscribe(() => {
    typesetMath();
  });
} else {
  document.addEventListener('DOMContentLoaded', () => {
    typesetMath();
  });
}
