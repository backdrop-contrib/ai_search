<?php

/**
 * Helper functions for Search API AI backends (engines + utilities).
 *
 * Preferred path: discover embedding-capable models directly from ai_provider
 * modules. Legacy hook_embedding_engine_info() support remains for compatibility.
 */

/**
 * Sets the engine configuration onto a backend using the shared trait slot.
 */
function ai_search_set_engine_configuration(&$backend, array $configuration) {
  $backend->traitConfiguration = $configuration;
}

/**
 * Load an embeddings engine include file.
 *
 * Registry entries store `.inc` paths, but module_load_include() wants the
 * basename.
 */
function ai_search_load_engine_include($module, $file) {
  $include = preg_replace('/\.inc$/', '', $file);
  module_load_include('inc', $module, $include);
}

/**
 * Parse provider/model IDs without assuming AI core helpers are available.
 */
function ai_search_parse_model_id($model_id) {
  if (function_exists('ai_parse_model_id')) {
    return ai_parse_model_id($model_id);
  }

  if (strpos($model_id, '/') !== FALSE) {
    return explode('/', $model_id, 2);
  }

  return [NULL, $model_id];
}

/**
 * Build a normalized embedding engine definition.
 */
function ai_search_build_embedding_engine_definition($provider_id, $model_id, $model_name = NULL) {
  $label = !empty($model_name) ? $model_name : $model_id;
  return [
    'label' => $label,
    'class' => 'AiSearchEmbeddingsEngine',
    'file' => 'includes/AiSearchEmbeddingsEngine.inc',
    'module' => 'ai_search',
    'provider_id' => $provider_id,
    'model_id' => $model_id,
    'model_name' => $label,
  ];
}

/**
 * Return normalized embedding engine definitions.
 *
 * @param string|null $provider_filter
 *   Optional provider ID filter.
 * @param bool $include_legacy_hooks
 *   Merge legacy hook_embedding_engine_info() results.
 *
 * @return array
 *   Engine definitions keyed by engine ID.
 */
function ai_search_get_embedding_engine_definitions($provider_filter = NULL, $include_legacy_hooks = TRUE) {
  $engines = [];

  if (function_exists('ai_get_all_models_for_method')) {
    $grouped = ai_get_all_models_for_method('getEmbeddingModels');
    foreach ($grouped as $provider_label => $provider_models) {
      foreach ($provider_models as $engine_id => $model_name) {
        list($provider_id, $model_id) = ai_search_parse_model_id($engine_id);
        if (empty($provider_id) || empty($model_id)) {
          continue;
        }
        if (!empty($provider_filter) && $provider_filter !== $provider_id) {
          continue;
        }
        $engines[$engine_id] = ai_search_build_embedding_engine_definition($provider_id, $model_id, $model_name);
      }
    }
  }

  if ($include_legacy_hooks) {
    $legacy_engines = module_invoke_all('embedding_engine_info');
    foreach ($legacy_engines as $engine_id => $def) {
      if (isset($engines[$engine_id])) {
        continue;
      }
      if (!empty($provider_filter)) {
        list($provider_id) = ai_search_parse_model_id($engine_id);
        if ($provider_id !== $provider_filter) {
          continue;
        }
      }
      $engines[$engine_id] = $def;
    }
  }

  ksort($engines);
  return $engines;
}

/**
 * Return a single embedding engine definition.
 */
function ai_search_get_embedding_engine_definition($engine_id, $include_legacy_hooks = TRUE) {
  if (empty($engine_id)) {
    return NULL;
  }

  $engines = ai_search_get_embedding_engine_definitions(NULL, $include_legacy_hooks);
  return !empty($engines[$engine_id]) ? $engines[$engine_id] : NULL;
}

/**
 * Default engine configuration.
 * Default to 1536 (e.g. text-embedding-3-small).
 */
function ai_search_default_engine_configuration() {
  return [
    'embeddings_engine' => NULL,
    'embeddings_engine_configuration' => [
      'dimension' => 1536,
    ],
  ];
}

/**
 * Build the embedding engine configuration form.
 * Dimension is read-only and driven by the selected engine.
 */
function ai_search_engine_configuration_form($form, &$form_state, $configuration) {
  $options = ai_search_get_embedding_engines_options();

  $form['embeddings_engine'] = [
    '#type' => 'select',
    '#title' => t('Embeddings Engine'),
    '#options' => $options,
    '#required' => TRUE,
    '#default_value' => $configuration['embeddings_engine'] ?? '',
    '#description' => t('Provider used to generate embeddings. Changing this requires reindexing.'),
    '#ajax' => [
      'callback' => 'ai_search_update_embedding_configuration_form',
      'wrapper'  => 'embedding-configuration-wrapper',
      'method'   => 'replace',
      'effect'   => 'fade',
    ],
  ];

  $form['embeddings_engine_configuration'] = [
    '#type' => 'fieldset',
    '#title' => t('Embeddings Engine Configuration'),
    '#prefix' => '<div id="embedding-configuration-wrapper">',
    '#suffix' => '</div>',
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  ];

  $dimension_value = isset($configuration['embeddings_engine_configuration']['dimension'])
    ? (int) $configuration['embeddings_engine_configuration']['dimension']
    : 1536;

  $form['embeddings_engine_configuration']['dimension'] = [
    '#type' => 'number',
    '#title' => t('Dimensions'),
    '#description' => t('Fixed by the selected embedding model.'),
    '#default_value' => $dimension_value,
    '#required' => TRUE,
    '#attributes' => ['readonly' => 'readonly'],
  ];

  return $form;
}

/**
 * Load an embeddings engine instance for the given configuration.
 */
function ai_search_load_embeddings_engine($configuration) {
  $plugin_id     = $configuration['embeddings_engine'] ?? NULL;
  $plugin_config = $configuration['embeddings_engine_configuration'] ?? [];

  if (!$plugin_id) {
    return NULL;
  }

  $def = ai_search_get_embedding_engine_definition($plugin_id, TRUE);
  if (empty($def)) {
    return NULL;
  }
  if (empty($def['class']) || empty($def['file']) || empty($def['module'])) {
    watchdog('ai_search', 'Invalid engine definition for @id', ['@id' => $plugin_id], WATCHDOG_ERROR);
    return NULL;
  }

  ai_search_load_engine_include($def['module'], $def['file']);
  if (!class_exists($def['class'])) {
    watchdog('ai_search', 'Engine class @c not found', ['@c' => $def['class']], WATCHDOG_ERROR);
    return NULL;
  }

  try {
    // Merge engine definition into config so model_id/model_name are passed to the class
    list($parsed_provider, $parsed_model) = ai_search_parse_model_id($plugin_id);
    $full_config = array_merge($plugin_config, [
      'provider_id' => !empty($def['provider_id']) ? $def['provider_id'] : ($parsed_provider ?: ai_default_provider_id()),
      'model_id' => $def['model_id'] ?? $parsed_model,
      'model_name' => $def['model_name'] ?? $parsed_model,
      'model_dimension' => $def['model_dimension'] ?? ($plugin_config['dimension'] ?? 1536),
    ]);

    return new $def['class']($full_config, NULL);
  }
  catch (\Exception $e) {
    watchdog('ai_search', 'Failed to initialize embedding engine: @message', ['@message' => $e->getMessage()], WATCHDOG_ERROR);
    return NULL;
  }
}

/**
 * Build the options list for embedding engines.
 * Avoid creating network clients here; rely on static/cheap checks.
 */
function ai_search_get_embedding_engines_options() {
  $options = [];
  $engines = ai_search_get_embedding_engine_definitions(NULL, TRUE);

  // Group engines by provider for optgroup support
  $grouped = [];

  foreach ($engines as $id => $def) {
    if (empty($def['class']) || empty($def['file']) || empty($def['module'])) {
      continue;
    }
    ai_search_load_engine_include($def['module'], $def['file']);
    if (!class_exists($def['class'])) {
      continue;
    }

    // If the engine exposes a static availability check, use it.
    $available = TRUE;
    if (method_exists($def['class'], 'isAvailableStatic')) {
      try {
        $available = call_user_func([$def['class'], 'isAvailableStatic']);
      }
      catch (\Exception $e) {
        $available = FALSE;
      }
    }

    if ($available) {
      $label = $def['label'] ?? $id;

      // Try to determine provider from model ID (e.g., "ollama/model-name")
      $provider_label = 'Other';
      if (strpos($id, '/') !== FALSE) {
        // Provider/model format.
        $parts = explode('/', $id, 2);
        $provider_id = $parts[0];

        if (function_exists('ai_get_provider_label')) {
          $provider_label = ai_get_provider_label($provider_id);
        }
        else {
          $provider_label = ucfirst($provider_id);
        }
      }
      // Detect common unprefixed embedding models.
      elseif (preg_match('/^(text-embedding|gpt|davinci|curie|babbage|ada)/i', $id)) {
        $provider_label = 'AI API';
      }
      if (!isset($grouped[$provider_label])) {
        $grouped[$provider_label] = [];
      }
      $grouped[$provider_label][$id] = $label;
    }
  }

  // Sort providers alphabetically
  ksort($grouped);

  // Return grouped array (Backdrop select supports optgroups)
  $options = $grouped;

  if (empty($options)) {
    backdrop_set_message(t('No embedding engines available. Please enable an embedding engine module.'), 'warning');
  }
  return $options;
}

/**
 * AJAX callback to update the engine configuration (dimension).
 *
 * Compatible with both legacy and current embedding model formats:
 * - Provider/model format
 * - Bare model ID format
 * Both include `model_id` in their engine definition, so the shared dimension
 * helper works for both.
 *
 * Uses triggering_element to find the selected value and navigate to the
 * sibling form element regardless of form nesting depth.
 */
function ai_search_update_embedding_configuration_form($form, &$form_state) {
  // Use triggering element value — works regardless of form nesting depth.
  $selected = isset($form_state['triggering_element']['#value'])
    ? $form_state['triggering_element']['#value']
    : NULL;

  // Navigate to embeddings_engine_configuration using the triggering element's
  // array_parents (swap 'embeddings_engine' for 'embeddings_engine_configuration').
  $element = _ai_search_find_sibling_element($form, $form_state, 'embeddings_engine_configuration');

  if (empty($element) || !is_array($element)) {
    $element = [
      '#type' => 'fieldset',
      '#title' => t('Embeddings Engine Configuration'),
      '#prefix' => '<div id="embedding-configuration-wrapper">',
      '#suffix' => '</div>',
    ];
  }

  if (empty($element['dimension']) || !is_array($element['dimension'])) {
    $element['dimension'] = [
      '#type' => 'number',
      '#title' => t('Dimensions'),
      '#description' => t('Fixed by the selected embedding model.'),
      '#default_value' => 1536,
      '#required' => TRUE,
      '#attributes' => ['readonly' => 'readonly'],
    ];
  }

  if (!$selected) {
    return $element;
  }

  $engines = ai_search_get_embedding_engine_definitions(NULL, TRUE);
  if (empty($engines[$selected])) {
    return $element;
  }

  $def = $engines[$selected];
  if (empty($def['class']) || empty($def['file']) || empty($def['module'])) {
    return $element;
  }

  module_load_include('inc', $def['module'], $def['file']);
  if (!class_exists($def['class'])) {
    return $element;
  }

  $dimension_value = 1536; // sane fallback
  try {
      // For per-model engines, use the embedding-dimension helper.
      // It tries dynamic detection first, then falls back to known dimensions.
      if (!empty($def['model_id']) && function_exists('ai_get_embedding_dimension')) {
        // Extract provider from the selected engine ID when prefixed.
        // For unprefixed models, default to the first enabled provider.
        $provider_id = ai_default_provider_id();
        if (strpos($selected, '/') !== FALSE) {
          list($provider_id, ) = explode('/', $selected, 2);
        }
        $dimension_value = ai_get_embedding_dimension($def['model_id'], $provider_id);
      }
      // Fallback: try instantiating the engine class.
      // This handles any legacy engines or engines without model_id.
      else {
        ai_search_load_engine_include($def['module'], $def['file']);
        if (class_exists($def['class'])) {
          $config = [];
          if (!empty($def['model_dimension'])) {
            $config['dimension'] = $def['model_dimension'];
          }
          if (!empty($def['model_id'])) {
            $config['model_id'] = $def['model_id'];
          }
          if (!empty($def['model_name'])) {
            $config['model_name'] = $def['model_name'];
          }

          $engine = new $def['class']($config, NULL);
          if (method_exists($engine, 'getDimension')) {
            $dim = $engine->getDimension();
            if ($dim > 0) {
              $dimension_value = (int) $dim;
            }
          }
        }
      }
    }
  catch (\Exception $e) {
    watchdog('ai_search', 'Failed to detect dimension for @id: @m', ['@id' => $selected, '@m' => $e->getMessage()], WATCHDOG_ERROR);
  }

  $element['dimension']['#value'] = $dimension_value;

  return $element;
}

/**
 * Navigate the form tree to find a sibling of the triggering element.
 *
 * Uses #array_parents from the triggering element, replaces the last key
 * with $sibling_key, and walks $form to that position.
 *
 * @param array $form
 *   The full form array.
 * @param array $form_state
 *   The form state array.
 * @param string $sibling_key
 *   The key of the sibling element to find.
 *
 * @return array|null
 *   The sibling form element, or NULL if not found.
 */
function _ai_search_find_sibling_element($form, &$form_state, $sibling_key) {
  if (empty($form_state['triggering_element']['#array_parents'])) {
    return isset($form[$sibling_key]) ? $form[$sibling_key] : NULL;
  }

  $parents = $form_state['triggering_element']['#array_parents'];
  array_pop($parents);
  $parents[] = $sibling_key;

  $element = $form;
  foreach ($parents as $key) {
    if (!isset($element[$key]) || !is_array($element[$key])) {
      return isset($form[$sibling_key]) ? $form[$sibling_key] : NULL;
    }
    $element = $element[$key];
  }

  return $element;
}

/* --------------------------------------------------------------------------
 * VECTOR NORMALIZER (works for Milvus & Pinecone)
 * -------------------------------------------------------------------------- */

/**
 * Normalize global $ai_search_vectors into a uniform set of rows
 * so both Milvus and Pinecone can ingest either shape:
 *
 * A] FLAT rows (current alter hook):
 *    [
 *      ['entity_type'=>'node','entity_id'=>123,'field_name'=>'search_api_viewed',
 *       'chunk_index'=>0,'vector'=>[...], 'content'=>'...', 'title'=>'...',
 *       'search_api_url'=>'...', 'url'=>'...'],
 *      ...
 *    ]
 *
 * B] LEGACY nested:
 *    $ai_search_vectors["node/123"]['vectors'][$field_name] = [
 *      'vectors' => [...], 'content' => '...', 'llm_vector_embedding' => TRUE
 *    ]
 *
 * @param string $entity_type
 * @param int    $entity_id
 * @param int    $expected_dimension
 * @param string $index_id
 *
 * @return array of rows:
 *   [
 *     [
 *       'entity_type' => 'node',
 *       'entity_id'   => 123,
 *       'field_name'  => 'search_api_viewed',
 *       'chunk_index' => 0,
 *       'values'      => [floats...],
 *       'metadata'    => ['content'=>'...', 'title'=>'...', 'search_api_url'=>'...', 'url'=>'...'],
 *       'llm_vector_embedding' => TRUE,
 *     ],
 *     ...
 *   ]
 */
function ai_search_normalize_vectors($entity_type, $entity_id, $expected_dimension, $index_id = NULL) {
  global $ai_search_vectors;
  $out = [];
  if (empty($entity_type) || empty($entity_id)) {
    return $out;
  }

  // -------- FLAT format --------
  if (is_array($ai_search_vectors) && isset($ai_search_vectors[0]) && is_array($ai_search_vectors[0])) {
    foreach ($ai_search_vectors as $row) {
      if (!is_array($row)) {
        continue;
      }
      if ($index_id !== NULL && isset($row['index_id']) && $row['index_id'] !== $index_id) {
        continue;
      }
      if (($row['entity_type'] ?? NULL) !== $entity_type) {
        continue;
      }
      if ((int) ($row['entity_id'] ?? 0) !== (int) $entity_id) {
        continue;
      }

      $vec = $row['vector'] ?? NULL;
      if (!is_array($vec) || count($vec) !== (int)$expected_dimension) {
        continue;
      }

      $out[] = [
        'entity_type' => $entity_type,
        'entity_id'   => (int)$entity_id,
        'field_name'  => $row['field_name'] ?? 'unknown',
        'chunk_index' => isset($row['chunk_index']) ? (int)$row['chunk_index'] : 0,
        'values'      => array_values(array_map('floatval', $vec)),
        'metadata'    => (function() use ($entity_type, $entity_id, $row) {
          // Include ALL fields from $row except vector/control metadata
          $m = $row;
          unset($m['vector']);
          unset($m['vectors']);
          unset($m['llm_vector_embedding']);
          unset($m['index_id']);
          
          // Ensure required fields are present
          if (!isset($m['entity_type'])) {
            $m['entity_type'] = $entity_type;
          }
          if (!isset($m['entity_id'])) {
            $m['entity_id'] = (int)$entity_id;
          }
          if ($entity_type === 'node' && !isset($m['nid'])) {
            $m['nid'] = (int)$entity_id;
          }
          
          return array_filter($m, function($v) { return $v !== NULL; });
        })(),
        'llm_vector_embedding' => $row['llm_vector_embedding'] ?? TRUE,
      ];
    }
  }

  // -------- LEGACY nested format --------
  $key = "$entity_type/$entity_id";
  if (isset($ai_search_vectors[$key]['vectors']) && is_array($ai_search_vectors[$key]['vectors'])) {
    foreach ($ai_search_vectors[$key]['vectors'] as $field_name => $entry) {
      if (!is_array($entry) || !isset($entry['vectors'])) {
        continue;
      }
      $vec = $entry['vectors'];
      if (!is_array($vec) || count($vec) !== (int) $expected_dimension) {
        continue;
      }

      $out[] = [
        'entity_type' => $entity_type,
        'entity_id'   => (int)$entity_id,
        'field_name'  => $field_name,
        'chunk_index' => 0,
        'values'      => array_values(array_map('floatval', $vec)),
        'metadata'    => (function() use ($entity_type, $entity_id, $entry) {
          $m = [
            'entity_type' => $entity_type,
            'entity_id'   => (int)$entity_id,
            'content'     => isset($entry['content']) ? (string)$entry['content'] : '',
          ];
          if ($entity_type === 'node') {
            $m['nid'] = (int)$entity_id;
          }
          return array_filter($m, function($v) { return $v !== NULL; });
        })(),
        'llm_vector_embedding' => $entry['llm_vector_embedding'] ?? TRUE,
      ];
    }
  }

  return $out;
}

/**
 * Recursively flatten a Search API keys structure into a flat term list.
 *
 * getKeys() can return nested groups, e.g.
 *   ['#conjunction' => 'OR', 0 => 'foo', 1 => ['#conjunction' => 'AND', 0 => 'bar']]
 * Control entries are identified by array KEY (e.g. '#conjunction',
 * '#negation'), not by value, since a conjunction's value ('AND'/'OR') is a
 * legitimate string that would otherwise leak into the embedding text.
 *
 * @param mixed $keys
 *   A getKeys() return value: a string, or a (possibly nested) keys array.
 *
 * @return string[]
 *   Flat list of search term strings.
 */
function ai_search_flatten_query_keys($keys) {
  if (is_string($keys)) {
    return [$keys];
  }
  if (!is_array($keys)) {
    return [];
  }

  $terms = [];
  foreach ($keys as $key => $value) {
    if (is_string($key) && $key[0] === '#') {
      continue;
    }
    if (is_array($value)) {
      $terms = array_merge($terms, ai_search_flatten_query_keys($value));
    }
    elseif (is_string($value)) {
      $terms[] = $value;
    }
  }
  return $terms;
}

/**
 * Resolve a Search API query into a query embedding, for backend search().
 *
 * Every vector backend's search() needs to: pick the configured embedding
 * model, extract search text from the query keys, resolve an AI API client,
 * apply the query-side embedding task prefix, and request the embedding.
 * That block was duplicated near-verbatim across all four backends (and one
 * of them — Pinecone — drifted and forgot the prefix step, causing a
 * document/query mismatch for asymmetric embedding models). Call this from
 * each backend's search() instead of re-implementing the block.
 *
 * @param SearchApiQueryInterface $query
 *   The incoming Search API query.
 * @param array $options
 *   The backend's $this->options; must contain 'embeddings_engine' or the
 *   legacy 'embedding_model' key.
 * @param string $watchdog_channel
 *   Module name to log under, e.g. 'ai_search_pinecone'.
 *
 * @return array|null
 *   NULL if there's no configured model or no search text (caller should
 *   return an empty result set in that case). Otherwise:
 *   [
 *     'text'  => string   // extracted/trimmed search text
 *     'vector' => float[] // the query embedding
 *     'model' => string   // resolved model id actually used for embedding()
 *   ]
 *
 * @throws Exception
 *   If an API client can't be resolved for the model, or the embedding
 *   request fails/returns empty.
 */
function ai_search_resolve_query_embedding(SearchApiQueryInterface $query, array $options, $watchdog_channel) {
  $model = $options['embeddings_engine'] ?? $options['embedding_model'] ?? NULL;
  if (empty($model)) {
    watchdog($watchdog_channel, 'No model configured!', [], WATCHDOG_ERROR);
    return NULL;
  }

  $keys = $query->getKeys();
  $text = trim(implode(' ', ai_search_flatten_query_keys($keys)));
  if ($text === '') {
    return NULL;
  }

  $stripped_model = NULL;
  $api = ai_get_api_for_model($model, $stripped_model);
  $model_to_use = !empty($stripped_model) ? $stripped_model : $model;

  if (!$api) {
    watchdog($watchdog_channel, 'Failed to resolve API client for model: @model', ['@model' => $model], WATCHDOG_ERROR);
    throw new Exception('Failed to get API client for model: ' . $model);
  }

  $start_time = microtime(TRUE);
  $vector = NULL;
  $error_message = NULL;
  try {
    $vector = $api->embedding(ai_search_apply_embedding_task_prefix($text, $model_to_use, 'query'), $model_to_use);
  }
  catch (\Exception $e) {
    $error_message = $e->getMessage();
    throw $e;
  }
  finally {
    if (method_exists($api, 'recordLog')) {
      $duration = microtime(TRUE) - $start_time;
      $success = !empty($vector);
      $api->recordLog('search_embedding', $model, ['text_len' => strlen($text)], $success ? $vector : NULL, $success, $duration, $error_message);
    }
  }

  if (empty($vector)) {
    throw new Exception('Failed to retrieve embedding.');
  }

  return [
    'text' => $text,
    'vector' => $vector,
    'model' => $model_to_use,
  ];
}

/**
 * Adds metadata re-ranking form elements to a backend configuration form.
 *
 * Call from configurationForm() in each backend service class.
 */
function ai_search_reranking_configuration_form(array $form, array &$form_state, array $options) {
  $rerank_options = !empty($options['metadata_reranking']) ? $options['metadata_reranking'] : [];
  $form['metadata_reranking'] = [
    '#type' => 'fieldset',
    '#title' => t('Metadata Re-ranking'),
    '#description' => t('After vector search, boost results whose stored metadata contains query terms. Combines vector similarity with a metadata match score.'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($rerank_options['enabled']),
  ];
  $form['metadata_reranking']['enabled'] = [
    '#type' => 'checkbox',
    '#title' => t('Enable metadata re-ranking'),
    '#default_value' => !empty($rerank_options['enabled']),
  ];
  $form['metadata_reranking']['metadata_weight'] = [
    '#type' => 'number',
    '#title' => t('Metadata score weight'),
    '#description' => t('Weight (0.0–1.0) given to the metadata match score. Vector score receives the remaining weight.'),
    '#default_value' => isset($rerank_options['metadata_weight']) ? $rerank_options['metadata_weight'] : 0.3,
    '#min' => 0,
    '#max' => 1,
    '#step' => 0.05,
  ];
  return $form;
}

/**
 * Extracts meaningful terms from a search query, stripping common stopwords.
 *
 * @param string $query_text
 * @return string[]
 */
function ai_search_extract_query_terms($query_text) {
  static $stopwords = [
    'a', 'an', 'the', 'is', 'are', 'was', 'were', 'be', 'been', 'being',
    'who', 'what', 'where', 'when', 'how', 'why', 'which', 'that', 'this',
    'these', 'those', 'and', 'or', 'but', 'in', 'on', 'at', 'to', 'for',
    'of', 'with', 'by', 'from', 'do', 'does', 'did', 'has', 'have', 'had',
    'can', 'could', 'will', 'would', 'should', 'may', 'might', 'shall',
    'me', 'my', 'our', 'your', 'their', 'its', 'i', 'we', 'you', 'they',
    'he', 'she', 'it', 'us', 'tell', 'about', 'get', 'give', 'find', 'show',
  ];
  $words = preg_split('/\W+/', strtolower(trim($query_text)), -1, PREG_SPLIT_NO_EMPTY);
  return array_values(array_filter($words, function ($w) use ($stopwords) {
    return strlen($w) > 2 && !in_array($w, $stopwords, TRUE);
  }));
}

/**
 * Applies metadata re-ranking to a normalized set of search matches.
 *
 * Each entry in $normalized_matches must have:
 *   - 'score'    float  The original vector similarity score.
 *   - 'metadata' array  Flat key→value metadata stored alongside the vector.
 *
 * On return, 'score' is replaced with the combined score and the array is
 * re-sorted highest-first.  '_vector_score' and '_metadata_score' are added
 * for diagnostics/logging.
 *
 * @param array  $normalized_matches  Modified in place.
 * @param string $query_text          Raw search query string.
 * @param array  $settings            Keys: 'enabled' (bool), 'metadata_weight' (float 0–1).
 */
function ai_search_apply_metadata_reranking(array &$normalized_matches, $query_text, array $settings) {
  if (empty($settings['enabled']) || empty($normalized_matches)) {
    return;
  }

  $terms = ai_search_extract_query_terms($query_text);
  if (empty($terms)) {
    return;
  }

  $metadata_weight = isset($settings['metadata_weight']) ? (float) $settings['metadata_weight'] : 0.3;
  $vector_weight   = 1.0 - $metadata_weight;
  $term_count      = count($terms);

  foreach ($normalized_matches as &$match) {
    $metadata = isset($match['metadata']) ? $match['metadata'] : [];

    // Flatten all string metadata values for term matching.
    $flat_values = [];
    foreach ($metadata as $value) {
      if (is_string($value) && $value !== '') {
        $flat_values[] = strtolower($value);
      }
      elseif (is_array($value)) {
        foreach ($value as $v) {
          if (is_string($v) && $v !== '') {
            $flat_values[] = strtolower($v);
          }
        }
      }
    }

    // Count how many query terms appear in at least one metadata value.
    $matched = 0;
    foreach ($terms as $term) {
      foreach ($flat_values as $flat) {
        if (strpos($flat, $term) !== FALSE) {
          $matched++;
          break;
        }
      }
    }

    $metadata_score = $term_count > 0 ? ($matched / $term_count) : 0;
    $match['_vector_score']   = $match['score'];
    $match['_metadata_score'] = $metadata_score;
    $match['score']           = ($match['score'] * $vector_weight) + ($metadata_score * $metadata_weight);
  }
  unset($match);

  usort($normalized_matches, function ($a, $b) {
    return $b['score'] <=> $a['score'];
  });
}
