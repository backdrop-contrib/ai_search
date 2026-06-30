(function ($, Backdrop) {
  'use strict';

  function getScrollOffset() {
    var extra = 0;
    var $toolbar = $('#admin-bar:visible, #toolbar:visible');
    if ($toolbar.length) extra += $toolbar.outerHeight();
    var $fixedHeader = $('.site-header.is-fixed:visible, header.fixed:visible');
    if ($fixedHeader.length) extra += $fixedHeader.outerHeight();
    return extra + 12;
  }

  function smoothScrollTo(el) {
    if (!el) return;
    var $el = $(el);
    if (!$el.length) return;
    var top = Math.max(0, $el.offset().top - getScrollOffset());
    $('html, body').stop(true).animate({ scrollTop: top }, 250);
    $el.attr('tabindex', '-1').focus();
  }

  Backdrop.behaviors.aiSearchHeaderCapture = {
    attach: function (context, settings) {
      $('form.ai-search-header-form', context).once('aiSearchHeaderCapture', function () {
        var $formEl = $(this);
        $formEl.on('submit', function () {
          var term = ($formEl.find('input[name="q"]').val() || '').trim();
          // Read destination from the per-form data attribute so each block
          // instance uses its own configured path as the storage namespace.
          var dest = $formEl.attr('data-ai-search-destination') || 'default';
          try {
            sessionStorage.setItem('ai_search_prefill:' + dest, term);
          } catch (err) {
            // Storage unavailable (private browsing etc.); the ?q query
            // parameter on the redirect covers this case.
          }
        });
      });
    }
  };

  // Remove only the `q` parameter from the current URL, preserving all others.
  function removeQParam() {
    try {
      var params = new URLSearchParams(window.location.search);
      if (params.has('q')) {
        params.delete('q');
        var qs = params.toString();
        var cleanUrl = window.location.pathname + (qs ? '?' + qs : '') + window.location.hash;
        window.history.replaceState(null, '', cleanUrl);
      }
    } catch (e) {
      // replaceState or URLSearchParams unavailable; leave URL as-is.
    }
  }

  Backdrop.behaviors.aiSearchHeaderAutoRun = {
    attach: function (context, settings) {
      // Use the current page path as the storage key — it matches what the
      // capture behavior wrote when the user submitted on a page whose block
      // destination pointed here.
      var dest = (settings.ai_search_header && settings.ai_search_header.page_path)
        ? settings.ai_search_header.page_path
        : 'default';
      var storageKey = 'ai_search_prefill:' + dest;

      $('.search-api-ai-search-block-form', context).once('aiSearchHeaderAutoRun', function () {
        var formEl = this;
        var $formEl = $(formEl);

        // Read term from sessionStorage without consuming it yet — we only
        // consume after confirming the form input exists and the value is applied.
        var term = null;
        var termFromStorage = false;
        var termFromUrl = false;
        try {
          term = sessionStorage.getItem(storageKey);
          if (term) {
            termFromStorage = true;
          }
        } catch (e) {
          // Storage unavailable; fall through to the ?q parameter.
        }

        // Fallback: read ?q from the URL (set by the header form redirect).
        // Also makes /chatbot?q=... links shareable.
        // Use a page-level flag so only the first matching form consumes it.
        if (!term) {
          try {
            if (!window._aiSearchQConsumed) {
              term = new URLSearchParams(window.location.search).get('q');
              if (term) {
                termFromUrl = true;
              }
            }
          } catch (e) {
            term = null;
          }
        }
        if (term) {
          term = String(term).trim();
        }

        if (!term) return;

        // Find the query input before consuming any stored state.
        var input = formEl.querySelector('input[name="query"]');
        if (!input) {
          console.warn('AI Search Header: query input not found on AI form.');
          return;
        }

        // Check if already auto-run
        if ($formEl.data('ai-autorun-done')) {
          return;
        }
        $formEl.data('ai-autorun-done', true);

        // Input confirmed — now consume the stored term and clean the URL.
        if (termFromStorage) {
          try { sessionStorage.removeItem(storageKey); } catch (e) {}
          // Strip ?q so a refresh doesn't retrigger via the fallback path.
          removeQParam();
        }
        if (termFromUrl) {
          window._aiSearchQConsumed = true;
          removeQParam();
        }

        // Scroll to the AI search form
        smoothScrollTo(formEl);

        // Fill in the search term
        input.value = term;
        $(input).trigger('input').trigger('change');

        // Submit the form
        setTimeout(function() {
          var submitBtn = formEl.querySelector('input[type="submit"], button[type="submit"]');
          if (submitBtn) {
            $(submitBtn).click();
          } else {
            $formEl.trigger('submit');
          }
        }, 300);
      });
    }
  };

})(jQuery, Backdrop);
