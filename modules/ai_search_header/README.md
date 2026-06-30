# AI Search Header

A Backdrop CMS module that provides a header search form which redirects to an AI Search page and automatically runs the search.

## Features

- Provides a "AI Search Header" block that can be placed in any region (typically in the header/navigation)
- Captures the user's search query
- Redirects to a configured page that contains the AI Search Block
- Automatically fills in and submits the search to the AI Search Block
- Uses sessionStorage to pass the query between pages
- Smooth scrolls to the AI search form before auto-submitting

## Requirements

- Backdrop CMS 1.x
- AI Search Search Block module (ai_search_search_block)

## Installation

1. This module ships as a submodule of ai_search (`modules/ai_search_header`)
2. Enable the module: `bee pm-enable ai_search_header`
3. Or enable via the UI at `admin/modules`

## Configuration

1. Go to your layout configuration (e.g., `admin/structure/layouts/manage/default`)
2. Add the "AI Search Header" block to your desired region (typically in the header)
3. Configure the block:
   - **Destination path**: Enter the path to the page that contains your AI Search Block (e.g., `chatbot` or `/chatbot`)
4. Save the layout

## How It Works

1. User enters a search term in the header search form
2. On submit, the term is stored in sessionStorage and carried on the redirect as ?q (fallback)
3. User is redirected to the AI Search page
4. JavaScript detects the stored search term
5. The term is automatically filled into the AI Search Block query field
6. The AI Search Block form is automatically submitted
7. User sees both AI results and database search results
8. Page smoothly scrolls to show the results

## Styling

The block uses classes compatible with core search block styling:
- `.search-block-form`
- `.search-form`
- `.form-search`
- `.form-control`

You can add custom CSS in `css/header-search.css` to further customize the appearance.

## Technical Details

- **Form class**: `.ai-search-header-form` (no fixed ID — Backdrop generates a unique ID per instance)
- **Input name**: `q` (matches web standards)
- **Storage**: Uses browser sessionStorage (cleared after use)
- **JavaScript behaviors**:
  - `aiSearchHeaderCapture`: Captures the search term on form submit
  - `aiSearchHeaderAutoRun`: Auto-runs the search on the destination page

## Troubleshooting

**Search doesn't auto-run:**
- Check that the destination path is correct
- Ensure the AI Search Block exists on the destination page
- Check browser console for JavaScript errors
- Verify sessionStorage is enabled in the browser

**Form doesn't redirect:**
- Verify the destination path in block configuration
- Check that the path exists and is accessible

**Styling issues:**
- The module inherits core search block styles
- Add custom CSS to override if needed
- Check that your theme supports inline forms (`.container-inline`)

## JavaScript Console Logs

The module emits one browser warning:
- `AI Search Header: query input not found on AI form.` — logged when the auto-run behavior attaches to a `.search-api-ai-search-block-form` element but cannot find an `input[name="query"]` inside it.

Enable the browser console to see this message for debugging.

## Credits

Converted from Drupal 11 to Backdrop CMS.
