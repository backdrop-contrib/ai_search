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
          try {
            sessionStorage.setItem('ai_search_prefill', term);
          } catch (err) {
            // Storage unavailable (private browsing etc.); the ?q query
            // parameter on the redirect covers this case.
          }
        });
      });
    }
  };

  Backdrop.behaviors.aiSearchHeaderAutoRun = {
    attach: function (context, settings) {
      $('.search-api-ai-search-block-form', context).once('aiSearchHeaderAutoRun', function () {
        var formEl = this;
        var $formEl = $(formEl);
        
        var term = null;
        try {
          term = sessionStorage.getItem('ai_search_prefill');
        } catch (e) {
          // Storage unavailable; fall through to the ?q parameter.
        }

        // Fallback: read ?q from the URL (set by the header form redirect).
        // Also makes /chatbot?q=... links shareable.
        // Use a page-level flag so only the first form on the page consumes it.
        if (!term) {
          try {
            if (!window._aiSearchQConsumed) {
              term = new URLSearchParams(window.location.search).get('q');
              if (term) {
                window._aiSearchQConsumed = true;
                // Replace the URL without ?q so back/refresh don't re-trigger.
                try {
                  var cleanUrl = window.location.pathname + window.location.hash;
                  window.history.replaceState(null, '', cleanUrl);
                } catch (e) {
                  // replaceState unavailable; leave URL as-is.
                }
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

        // Clear from storage
        try {
          sessionStorage.removeItem('ai_search_prefill');
        } catch (e) {
          // Ignore; worst case the next visit re-runs the same search.
        }

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
