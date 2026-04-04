// Mermaid rendering for Material for MkDocs
// Renders fenced ```mermaid blocks processed by pymdownx.superfences (class="mermaid")

(function () {
  function initMermaid() {
    if (typeof window.mermaid === "undefined") return;

    // Initialize once
    if (!window.__jaam_mermaid_initialized) {
      window.mermaid.initialize({
        startOnLoad: false,
        // Don't auto-scale SVG to container width, keep text readable.
        flowchart: {
          useMaxWidth: false,
        },
        sequence: {
          useMaxWidth: false,
        },
      });
      window.__jaam_mermaid_initialized = true;
    }

    // Normalize legacy output (<pre class="mermaid"><code>...</code></pre>)
    // into <div class="mermaid">...</div> so Mermaid sees plain text.
    const legacyBlocks = document.querySelectorAll("pre.mermaid");
    legacyBlocks.forEach((pre) => {
      const code = pre.querySelector("code");
      if (!code) return;

      const div = document.createElement("div");
      div.className = "mermaid";
      div.textContent = code.textContent || "";
      pre.replaceWith(div);
    });

    const nodes = document.querySelectorAll(".mermaid");
    if (!nodes.length) return;

    try {
      // Mermaid v10+ API
      if (typeof window.mermaid.run === "function") {
        window.mermaid.run({ nodes: Array.from(nodes) });
      } else {
        // Fallback for older Mermaid
        window.mermaid.init(undefined, nodes);
      }
    } catch (e) {
      // Keep docs usable even if a diagram fails to render
      console.error("Mermaid render failed", e);
    }
  }

  // Material for MkDocs hook
  if (typeof window.document$ !== "undefined" && window.document$?.subscribe) {
    window.document$.subscribe(initMermaid);
  } else {
    // Fallback
    document.addEventListener("DOMContentLoaded", initMermaid);
  }
})();
