// wtop website: copy buttons and install tabs. No dependencies.
(function () {
  'use strict';

  function copyText(text) {
    if (navigator.clipboard && window.isSecureContext) {
      return navigator.clipboard.writeText(text);
    }
    // Fallback for file:// and plain http.
    return new Promise(function (resolve, reject) {
      var ta = document.createElement('textarea');
      ta.value = text;
      ta.setAttribute('readonly', '');
      ta.style.position = 'fixed';
      ta.style.opacity = '0';
      document.body.appendChild(ta);
      ta.select();
      try {
        document.execCommand('copy') ? resolve() : reject();
      } catch (e) {
        reject(e);
      } finally {
        document.body.removeChild(ta);
      }
    });
  }

  // Copies the target of data-copy, or the <code> next to the button in a
  // <pre>. Comment lines (# ...) are dropped so the result pastes cleanly.
  document.querySelectorAll('.copy').forEach(function (btn) {
    btn.addEventListener('click', function () {
      var sel = btn.getAttribute('data-copy');
      var code = sel ? document.querySelector(sel) : btn.parentElement.querySelector('code');
      if (!code) return;
      var text = code.innerText
        .split('\n')
        .filter(function (line) { return !/^\s*#/.test(line); })
        .join('\n')
        .replace(/\n{3,}/g, '\n\n')
        .trim();
      copyText(text).then(function () {
        var label = btn.textContent;
        btn.textContent = 'Copied!';
        btn.classList.add('done');
        setTimeout(function () {
          btn.textContent = label;
          btn.classList.remove('done');
        }, 1500);
      }, function () {
        btn.textContent = 'Press Ctrl+C';
      });
    });
  });

  // Accessible tabs (arrow keys, Home/End).
  document.querySelectorAll('[data-tabs]').forEach(function (root) {
    var tabs = Array.prototype.slice.call(root.querySelectorAll('[role="tab"]'));

    function select(tab) {
      tabs.forEach(function (t) {
        var on = t === tab;
        t.setAttribute('aria-selected', on ? 'true' : 'false');
        t.tabIndex = on ? 0 : -1;
        document.getElementById(t.getAttribute('aria-controls')).hidden = !on;
      });
      tab.focus();
    }

    tabs.forEach(function (tab, i) {
      tab.addEventListener('click', function () { select(tab); });
      tab.addEventListener('keydown', function (e) {
        var next = null;
        if (e.key === 'ArrowRight') next = tabs[(i + 1) % tabs.length];
        else if (e.key === 'ArrowLeft') next = tabs[(i - 1 + tabs.length) % tabs.length];
        else if (e.key === 'Home') next = tabs[0];
        else if (e.key === 'End') next = tabs[tabs.length - 1];
        if (next) { e.preventDefault(); select(next); }
      });
    });
  });

  // A poster can't vary by screen size, so swap in the one that matches the
  // mobile <source> (same media query as in index.html).
  if (window.matchMedia && window.matchMedia('(max-width: 640px)').matches) {
    document.querySelectorAll('video[data-poster-mobile]').forEach(function (video) {
      video.poster = video.getAttribute('data-poster-mobile');
    });
  }

  // Don't autoplay the demo video for visitors who prefer reduced motion;
  // the controls stay available to start it by hand.
  if (window.matchMedia && window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
    document.querySelectorAll('video[autoplay]').forEach(function (video) {
      video.removeAttribute('autoplay');
      video.pause();
    });
  }
})();
