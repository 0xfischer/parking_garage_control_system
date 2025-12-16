// Initialize Mermaid and transform fenced code blocks (```mermaid) into <div class="mermaid"> blocks
(function () {
  function transformCodeFences() {
    var codes = document.querySelectorAll('pre > code.language-mermaid, pre > code.mermaid');
    codes.forEach(function (code) {
      var pre = code.parentElement;
      var container = document.createElement('div');
      container.className = 'mermaid';
      container.textContent = code.textContent;
      pre.parentElement.replaceChild(container, pre);
    });
  }

  function transformDoxygenFragments() {
    // Doxygen often wraps code blocks as div.fragment with div.line children
    var fragments = document.querySelectorAll('div.fragment');
    fragments.forEach(function (frag) {
      var txt = (frag.textContent || '').trim();
      // Heuristic detection of Mermaid content
      var looksLikeMermaid = /^(graph|flowchart|sequenceDiagram|classDiagram|stateDiagram|erDiagram|journey|gantt|pie)\b/i.test(txt)
        || /^stateDiagram-v2\b/i.test(txt);
      if (looksLikeMermaid) {
        var div = document.createElement('div');
        div.className = 'mermaid';
        div.textContent = txt;
        frag.parentNode.replaceChild(div, frag);
      }
    });
  }

  function initMermaid() {
    if (!window.mermaid) return;
    try {
      // Transform before initializing Mermaid
      transformCodeFences();
      transformDoxygenFragments();

      window.mermaid.initialize({ startOnLoad: false, securityLevel: 'loose' });
      window.mermaid.run();
    } catch (e) {
      if (typeof console !== 'undefined' && console.warn) {
        console.warn('Mermaid init failed:', e);
      }
    }
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initMermaid);
  } else {
    initMermaid();
  }
})();
