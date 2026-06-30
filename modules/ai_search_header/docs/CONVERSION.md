# AI Search Header - Drupal 11 to Backdrop Conversion

## Conversion Summary

This document outlines the changes made to convert the ai_search_header module from Drupal 11 to Backdrop CMS.

## Files Converted

### Created/Modified:
1. **ai_search_header.info** (NEW) - Backdrop module info file
2. **ai_search_header.module** (NEW) - Backdrop hook implementations
3. **js/autorun.js** (MODIFIED) - Updated to use Backdrop.behaviors and jQuery .once()
4. **css/header-search.css** (KEPT) - No changes needed
5. **README.md** (NEW) - Documentation for Backdrop version

### Removed:
1. **ai_search_header.info.yml** - Drupal's YAML info file
2. **ai_search_header.libraries.yml** - Drupal's library definition (JS/CSS attached differently in Backdrop)
3. **src/Form/AiSearchHeaderForm.php** - Drupal's Form API class (converted to hook_form)
4. **src/Plugin/Block/AiSearchHeaderBlock.php** - Drupal's Block Plugin (converted to hook_block_*)

## Key Changes

### 1. Module Info File
**Before (Drupal):** `.info.yml` with YAML structure
```yaml
name: AI Search Header
type: module
core_version_requirement: ^10 || ^11
```

**After (Backdrop):** `.info` with INI structure
```ini
name = AI Search Header
type = module
backdrop = 1.x
```

### 2. Block Implementation
**Before (Drupal):** Plugin system with annotations
- Class: `AiSearchHeaderBlock extends BlockBase implements ContainerFactoryPluginInterface`
- Used dependency injection for FormBuilder
- Annotation: `@Block(id = "ai_search_header_block", ...)`

**After (Backdrop):** Hook-based system
- `hook_block_info()` - Define block
- `hook_block_configure()` - Configuration form
- `hook_block_save()` - Save configuration
- `hook_block_view()` - Render block

### 3. Form Implementation
**Before (Drupal):** Form API class
- Class: `AiSearchHeaderForm extends FormBase`
- Method: `buildForm()` with typed parameters
- Method: `submitForm()` with typed parameters
- Used `Url::fromUserInput()` for redirects

**After (Backdrop):** Hook-based form
- Function: `ai_search_header_form()` - Form builder
- Function: `ai_search_header_form_submit()` - Submit handler
- Uses `$form_state['redirect']` for redirects

### 4. JavaScript Changes
**Before (Drupal):**
```javascript
(function ($, Drupal, drupalSettings, once) {
  Drupal.behaviors.aiSearchHeaderCapture = {
    attach(context) {
      once('aiSearchHeaderCapture', 'form#ai-search-header-form', context)
        .forEach(function (formEl) { ... });
    }
  };
})(jQuery, Drupal, drupalSettings, once);
```

**After (Backdrop):**
```javascript
(function ($, Backdrop) {
  Backdrop.behaviors.aiSearchHeaderCapture = {
    attach: function (context, settings) {
      $('form.ai-search-header-form', context).once('aiSearchHeaderCapture', function () {
        var $formEl = $(this);
        ...
      });
    }
  };
})(jQuery, Backdrop);
```

Key JS changes:
- `Drupal` → `Backdrop`
- `drupalSettings` → `settings` (passed as parameter)
- `once()` function → jQuery `.once()` method
- Removed Drupal-specific AJAX execution logic (not needed in Backdrop)
- Changed toolbar selector from `#toolbar-bar` to `#admin-bar, #toolbar`

### 5. Asset Loading
**Before (Drupal):** Library system via `.libraries.yml`
```php
$form['#attached']['library'][] = 'ai_search_header/autorun';
```

**After (Backdrop):** Direct file attachment
```php
backdrop_add_js(backdrop_get_path('module', 'ai_search_header') . '/js/autorun.js');
backdrop_add_css(backdrop_get_path('module', 'ai_search_header') . '/css/header-search.css');
```

### 6. Dependency Injection
**Before (Drupal):** Constructor injection via ContainerFactoryPluginInterface
```php
public function __construct(
  array $configuration,
  $plugin_id,
  $plugin_definition,
  protected FormBuilderInterface $formBuilder,
) { ... }
```

**After (Backdrop):** Direct function calls
```php
$form = backdrop_get_form('ai_search_header_form', $destination);
```

### 7. Type Declarations
**Before (Drupal):** Strict typing with `declare(strict_types=1)`
```php
public function buildForm(array $form, FormStateInterface $form_state, ?string $destination_path = null): array
```

**After (Backdrop):** No type declarations (PHP 5.x compatibility)
```php
function ai_search_header_form($form, &$form_state, $destination_path = 'chatbot')
```

## Testing Checklist

- [x] Module enables without errors
- [x] Block appears in block configuration UI
- [x] Block configuration form works
- [x] Block can be placed in a layout
- [ ] Search form renders correctly
- [ ] Form submission captures search term
- [ ] Redirect to destination page works
- [ ] Auto-run functionality works on destination page
- [ ] Results appear and scroll works

## Backdrop-Specific Considerations

1. **No Namespace/Autoloading**: Backdrop doesn't use PSR-4 autoloading, so all code is in the .module file
2. **Hook System**: Everything uses hooks instead of plugins/services
3. **Form State**: Uses `$form_state['redirect']` instead of `$form_state->setRedirectUrl()`
4. **jQuery .once()**: Backdrop uses jQuery's .once() plugin, not Drupal's once() function
5. **Settings**: `backdrop_add_js(..., 'setting')` instead of drupalSettings
6. **Form Elements**: Uses `#title_display` instead of implementing custom rendering

## Compatibility Notes

- Backdrop 1.x (PHP 5.6+)
- Requires ai_search_search_block module
- Works with core jQuery and jQuery .once() plugin (included in Backdrop core)
- Compatible with standard Backdrop layouts and blocks system

## Known Limitations

- No typed function parameters (Backdrop limitation)
- No dependency injection (uses global functions)
- No service container
- CSS file is empty (relies on theme/core styles)

## Future Enhancements

- Add more configuration options (placeholder text, button text, etc.)
- Add admin UI to select destination from available layouts/pages
- Add custom styling options
- Add support for multiple AI search blocks on destination page
