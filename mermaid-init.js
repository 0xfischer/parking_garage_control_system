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

  function initMermaid() {
    if (!window.mermaid) return;
    try {
      window.mermaid.initialize({ startOnLoad: false, securityLevel: 'loose' });
      transformCodeFences();
      window.mermaid.init(undefined, document.querySelectorAll('.mermaid'));
    } catch (e) {
      console && console.warn && console.warn('Mermaid init failed:', e);
    }
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initMermaid);
  } else {
    initMermaid();
  }
})();
