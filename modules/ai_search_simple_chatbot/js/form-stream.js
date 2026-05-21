(function ($, Backdrop) {
  'use strict';

  function escapeHtml(s) {
    return s
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  // Auto-link bare http/https URLs, but keep trailing punctuation out of the link.
  function autolink(s) {
    return s.replace(
      /\bhttps?:\/\/[^\s<>"']+[^\s<>"'.,!?;:)\]}]/g,
      function (url) {
        // Strip off trailing punctuation separately
        var match = url.match(/^(.*?)([.,!?;:)\]}]+)?$/);
        var clean = match[1];
        var trail = match[2] || '';
        return '<a href="' + clean + '" target="_blank" rel="nofollow noopener">' +
          clean +
          '</a>' + trail;
      }
    );
  }


  // Replace your current streamText with this version.
  function streamText($el, text, speed, done) {
    $el.html('');               // write HTML, not text
    var i = 0;

    (function next() {
      if (i < text.length) {
        // Escape the partial slice, then autolink URLs so they're clickable.
        var slice = text.slice(0, i + 1);
        var html  = autolink(escapeHtml(slice));
        $el.html(html);
        i++;
        setTimeout(next, speed);
      } else if (typeof done === 'function') {
        done();
      }
    })();
  }

  Backdrop.behaviors.searchApiAiStream = {
    attach: function (context) {
      var $context = $(context);

      $context.find('.chat-history-scroll').once('chat-history-a11y').each(function () {
        var $history = $(this);
        $history.attr({
          'role': 'log',
          'aria-live': 'polite',
          'aria-relevant': 'additions text',
          'aria-atomic': 'false'
        });

        if (!$history.siblings('.chat-history-status').length) {
          $history.after('<div class="chat-history-status visually-hidden" role="status" aria-live="polite" aria-atomic="true"></div>');
        }
      });

      // Stream any element marked for streaming.
      $context.find('.chat-stream-text').once('chat-stream').each(function () {
        var $textEl   = $(this);
        var original  = $textEl.text();
        var $sources  = $textEl.siblings('.chat-source-link');

        if ($sources.length) $sources.hide();
        if (original && original.length) {
          $textEl.closest('form').find('.chat-history-status').text(Backdrop.t('Assistant response loading.'));
          streamText($textEl, original, 20, function () {
            if ($sources.length) $sources.show();
            $textEl.closest('form').find('.chat-history-status').text(Backdrop.t('Assistant response complete.'));
          });
        }
      });

      // Enter-to-submit (Shift+Enter = newline)
      $context.find('.chat-form-query').once('chat-enter-submit').each(function () {
        var $input = $(this);
        $input.on('keydown', function (e) {
          if (e.isComposing || e.keyCode === 229) return;
          var isEnter = (e.key === 'Enter' || e.which === 13);
          if (isEnter && !e.shiftKey) {
            e.preventDefault();
            var $form = $input.closest('form');
            var $btn  = $form.find('.chat-form-send:enabled:visible').first();
            if ($btn.length) {
              $btn.trigger('mousedown').trigger('click');
            } else {
              $form.trigger('submit');
            }
          }
        });
      });

      // Topbar Clear History -> click hidden submit inside the form (no validation)
      $context.find('.chatbot-topbar .chat-form-clear-history')
        .once('chat-clear-history')
        .on('click', function (e) {
          e.preventDefault();
          var $wrapper = $(this).closest('.chatbot-wrapper');
          var $proxy = $wrapper.find('.chat-form-clear-history-proxy:enabled').first();
          if ($proxy.length) {
            $proxy.trigger('mousedown').trigger('click');
          }
        });
    }
  };
})(jQuery, Backdrop);

(function ($, Backdrop) {
  'use strict';
  Backdrop.behaviors.chatToggleBehavior = {
    attach: function (context) {
      $(context).find('.chat-toggle-button').once('chat-toggle').each(function () {
        var $button = $(this);
        var $block  = $button.closest('.block-search-api-ai-simple-chatbot-search-api-ai-chat-form');
        var $panel = $block.find('.chatbot-inner').first();

        if (!$panel.length) {
          return;
        }

        if (!$panel.attr('id')) {
          $panel.attr('id', 'ai-search-chatbot-panel');
        }
        $button.attr('aria-controls', $panel.attr('id'));

        var isCollapsed = localStorage.getItem('chatbot-collapsed') === 'true';
        $block.toggleClass('collapsed', isCollapsed);
        $panel.prop('hidden', isCollapsed);
        $button.attr('aria-expanded', isCollapsed ? 'false' : 'true');
        $button.text(isCollapsed ? '💬 Open Chat' : '💬 Chat');

        $button.on('click', function () {
          $block.toggleClass('collapsed');
          var now = $block.hasClass('collapsed');
          localStorage.setItem('chatbot-collapsed', now);
          $panel.prop('hidden', now);
          $button.attr('aria-expanded', now ? 'false' : 'true');
          $button.text(now ? '💬 Open Chat' : '💬 Chat');
        });
      });
    }
  };
})(jQuery, Backdrop);
