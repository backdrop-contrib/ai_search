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
      $('form#ai-search-header-form', context).once('aiSearchHeaderCapture', function () {
        var $formEl = $(this);
        $formEl.on('submit', function () {
          var term = ($formEl.find('input[name="q"]').val() || '').trim();
          // Namespace the storage key by destination so the prefill only applies
          // to the intended AI Search page and does not leak to unrelated forms.
          var dest = (settings.ai_search_header && settings.ai_search_header.destination)
            ? settings.ai_search_header.destination
            : 'default';
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
      // Determine which destination this block serves so we read the matching key.
      var dest = (settings.ai_search_header && settings.ai_search_header.destination)
        ? settings.ai_search_header.destination
        : 'default';
      var storageKey = 'ai_search_prefill:' + dest;

      $('.search-api-ai-search-block-form', context).once('aiSearchHeaderAutoRun', function () {
        var formEl = this;
        var $formEl = $(formEl);

        var term = null;
        try {
          term = sessionStorage.getItem(storageKey);
          if (term) {
            // Consume immediately so no other form on this page reuses it.
            sessionStorage.removeItem(storageKey);
            // Also strip ?q from the URL so refresh doesn't retrigger via
            // the fallback path, while preserving any other query params.
            removeQParam();
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
                window._aiSearchQConsumed = true;
                // Remove only ?q, preserving any other query parameters.
                removeQParam();
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

        // Scroll to the AI search form
        smoothScrollTo(formEl);

        // Find the query input - Backdrop uses different selectors
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
