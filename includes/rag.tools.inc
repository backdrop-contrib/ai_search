<?php


/**
 * AI tools for Search API retrieval.
 */

/**
 * Returns AI tool definitions for Search API retrieval.
 */
function ai_search_rag_tools() {
  return [
    'ai_search_list_rag_indexes' => [
      'definition' => [
        'type' => 'function',
        'function' => [
          'name' => 'ai_search_list_rag_indexes',
          'description' => 'List all available Search API AI vector indexes with their labels, entity types, and backend. Call this FIRST before searching so you can choose the most relevant index for the query. Each index covers a different type of content — use the label and entity_type to select the best match.',
          'parameters' => [
            'type' => 'object',
            'properties' => new stdClass(),
            'required' => [],
          ],
        ],
      ],
      'execute' => 'ai_search_execute_list_rag_indexes',
      'module' => 'ai_search',
      'category' => 'rag',
      'operation' => 'read',
      'permission' => 'administer site configuration',
      'requirements' => 'ai_search_rag_tools_requirements',
    ],
    'ai_search_rag_search' => [
      'definition' => [
        'type' => 'function',
        'function' => [
          'name' => 'ai_search_rag_search',
          'description' => 'Search Search API AI vector indexes and return ranked source snippets with citations. Always call ai_search_list_rag_indexes first to identify which index_ids are relevant to the query, then pass those index_ids here for targeted, accurate results. Searching all indexes at once reduces relevance.',
          'parameters' => [
            'type' => 'object',
            'properties' => [
              'query' => [
                'type' => 'string',
                'description' => 'The user question or search text.',
              ],
              'index_ids' => [
                'type' => 'array',
                'items' => [
                  'type' => 'string',
                ],
                'description' => 'List of Search API index machine names to search. Use ai_search_list_rag_indexes to discover available index IDs and their content, then choose the most relevant ones. Omit only if no index can be determined.',
              ],
              'top_k' => [
                'type' => 'integer',
                'description' => 'Maximum number of sources to return across all searched indexes.',
              ],
              'score_threshold' => [
                'type' => 'number',
                'description' => 'Minimum score or similarity threshold for a source to be included.',
              ],
              'entity_types' => [
                'type' => 'array',
                'items' => [
                  'type' => 'string',
                ],
                'description' => 'Optional entity types to filter by. Accepts either the Backdrop entity type (e.g. node, taxonomy_term) or a bundle/content type name (e.g. person, faq).',
              ],
            ],
            'required' => ['query'],
          ],
        ],
      ],
      'execute' => 'ai_search_execute_rag_search',
      'module' => 'ai_search',
      'category' => 'rag',
      'operation' => 'read',
      'requirements' => 'ai_search_rag_tools_requirements',
    ],
  ];
}

/**
 * Requirements callback for RAG tools.
 */
function ai_search_rag_tools_requirements() {
  return module_exists('ai_search') && module_exists('ai_tools');
}

/**
 * Execute the index-list tool.
 */
function ai_search_execute_list_rag_indexes(array $args) {
  return ai_search_list_rag_indexes();
}

/**
 * Execute the RAG search tool.
 */
function ai_search_execute_rag_search(array $args) {
  $query = trim(isset($args['query']) ? (string) $args['query'] : '');
  if ($query === '') {
    return [
      'error' => 'query is required.',
    ];
  }

  $top_k = !empty($args['top_k']) ? max(1, (int) $args['top_k']) : 5;
  $score_threshold = isset($args['score_threshold']) ? (float) $args['score_threshold'] : 0.5;
  $allowed_entity_types = [];
  if (!empty($args['entity_types']) && is_array($args['entity_types'])) {
    $allowed_entity_types = array_values(array_filter(array_map('strval', $args['entity_types'])));
  }

  $index_ids = [];
  if (!empty($args['index_ids']) && is_array($args['index_ids'])) {
    $index_ids = array_values(array_filter(array_map('strval', $args['index_ids'])));
  }

  $indexes = ai_search_rag_load_indexes($index_ids);
  if (empty($indexes)) {
    $available_indexes = ai_search_list_rag_indexes();
    $supported_indexes = !empty($available_indexes['indexes']) && is_array($available_indexes['indexes']) ? $available_indexes['indexes'] : [];

    if (!empty($index_ids)) {
      return [
        'error' => 'No supported Search API AI indexes matched the requested index_ids.',
        'query' => $query,
        'requested_index_ids' => $index_ids,
        'count' => 0,
        'sources' => [],
        'indexes' => [],
        'available_indexes' => $supported_indexes,
      ];
    }

    return [
      'error' => 'No supported Search API AI indexes are available.',
      'query' => $query,
      'count' => 0,
      'sources' => [],
      'indexes' => [],
    ];
  }

  $all_sources = [];
  $index_results = [];
  $errors = [];

  foreach ($indexes as $index) {
    $result = ai_search_rag_search_index($index, $query, [
      'top_k' => $top_k,
      'score_threshold' => $score_threshold,
      'allowed_entity_types' => $allowed_entity_types,
    ]);

    if (!empty($result['error'])) {
      $errors[] = $result['error'];
      continue;
    }

    $index_results[] = [
      'index_id' => $result['index_id'],
      'label' => $result['index_label'],
      'backend' => $result['backend'],
      'count' => $result['count'],
    ];
    foreach ($result['sources'] as $source) {
      $all_sources[] = $source;
    }
  }

  usort($all_sources, 'ai_search_rag_sort_sources');
  if (count($all_sources) > $top_k) {
    $all_sources = array_slice($all_sources, 0, $top_k);
  }

  return [
    'query' => $query,
    'top_k' => $top_k,
    'score_threshold' => $score_threshold,
    'count' => count($all_sources),
    'indexes' => $index_results,
    'sources' => $all_sources,
    'errors' => $errors,
  ];
}

/**
 * Load supported indexes, optionally filtered by machine name.
 */
function ai_search_rag_load_indexes(array $index_ids = []) {
  return ai_search_load_rag_indexes($index_ids);
}

/**
 * Search one index and normalize results.
 */
function ai_search_rag_search_index($index, $query, array $options = []) {
  if (empty($index) || empty($index->machine_name)) {
    return [
      'error' => 'Invalid index.',
    ];
  }

  $server = !empty($index->server) ? search_api_server_load($index->server) : NULL;
  if (!ai_search_index_supports_rag($index, $server)) {
    return [
      'error' => 'Unsupported backend for index ' . $index->machine_name . '.',
    ];
  }

  $embedding = ai_search_rag_build_embedding_vector($index, $query);
  if (!empty($embedding['error'])) {
    return [
      'error' => $embedding['error'],
    ];
  }

  $top_k = !empty($options['top_k']) ? max(1, (int) $options['top_k']) : 5;
  $score_threshold = isset($options['score_threshold']) ? (float) $options['score_threshold'] : 0.5;
  $allowed_entity_types = !empty($options['allowed_entity_types']) && is_array($options['allowed_entity_types']) ? $options['allowed_entity_types'] : [];

  $vector_results = ai_search_rag_query_backend($server, $index, $embedding['vector'], $embedding['dimension'], $top_k);
  if (!empty($vector_results['error'])) {
    return [
      'error' => $vector_results['error'],
    ];
  }

  $sources = ai_search_rag_extract_sources($server, $index, $vector_results, $score_threshold, $allowed_entity_types);

  return [
    'index_id' => $index->machine_name,
    'index_label' => !empty($index->name) ? $index->name : $index->machine_name,
    'backend' => $server->class,
    'query' => $query,
    'top_k' => $top_k,
    'score_threshold' => $score_threshold,
    'count' => count($sources),
    'sources' => $sources,
  ];
}

/**
 * Build an embedding vector for one search query.
 */
function ai_search_rag_build_embedding_vector($index, $query) {
  $model = ai_search_rag_index_embedding_model($index);
  $dimension = ai_search_rag_index_embedding_dimension($index);

  if ($model === '') {
    return [
      'error' => 'No embedding model is configured for index ' . $index->machine_name . '.',
    ];
  }

  $embedding_configuration = [
    'embeddings_engine' => $model,
    'embeddings_engine_configuration' => [
      'dimension' => $dimension,
    ],
  ];

  $embedding_engine = ai_search_load_embeddings_engine($embedding_configuration);
  if (!$embedding_engine && strpos($model, '/') === FALSE) {
    foreach (['openai', 'ollama', 'openrouter'] as $provider_id) {
      $try_conf = $embedding_configuration;
      $try_conf['embeddings_engine'] = $provider_id . '/' . $model;
      $embedding_engine = ai_search_load_embeddings_engine($try_conf);
      if ($embedding_engine) {
        break;
      }
    }
  }

  if (!$embedding_engine || !method_exists($embedding_engine, 'generateEmbeddings')) {
    return [
      'error' => 'Failed to load an embeddings engine for index ' . $index->machine_name . '.',
    ];
  }

  try {
    $vector = $embedding_engine->generateEmbeddings(ai_search_apply_embedding_task_prefix($query, $model, 'query'));
  }
  catch (\Exception $e) {
    return [
      'error' => $e->getMessage(),
    ];
  }

  if (empty($vector) || !is_array($vector)) {
    return [
      'error' => 'Embedding generation returned no vector for index ' . $index->machine_name . '.',
    ];
  }

  return [
    'vector' => $vector,
    'dimension' => $dimension,
    'model' => $model,
  ];
}

/**
 * Query the configured backend for one index.
 */
function ai_search_rag_query_backend($server, $index, array $vector, $dimension, $top_k) {
  try {
    return ai_search_query_backend($server, $index, $vector, $dimension, $top_k);
  }
  catch (\Exception $e) {
    return [
      'error' => $e->getMessage(),
    ];
  }
}

/**
 * Extract and normalize source snippets from a vector result payload.
 */
function ai_search_rag_extract_sources($server, $index, $vector_results, $score_threshold, array $allowed_entity_types = []) {
  $backend_class = $server->class;
  $matches = ai_search_backend_result_matches($server, $vector_results, $index);

  // Normalize once so LLM-supplied casing/whitespace (e.g. "Person ") still
  // matches a lowercase bundle/entity type like "person".
  $allowed_entity_types_lower = array_map('strtolower', array_map('trim', $allowed_entity_types));

  $sources = [];
  foreach ($matches as $match) {
    if (!is_array($match)) {
      continue;
    }

    $score = ai_search_match_score($match);
    if ($score === NULL || $score < $score_threshold) {
      continue;
    }

    $resolved = ai_search_match_entity($match);
    if (!$resolved) {
      continue;
    }
    $metadata = $resolved['metadata'];
    $entity_type = $resolved['entity_type'];
    $entity_id = $resolved['entity_id'];
    if (!empty($allowed_entity_types)) {
      // The tool's entity_types param is documented as the raw Backdrop
      // entity type ("node", "taxonomy_term"), but models routinely pass
      // content/bundle names instead (e.g. "person", "faq") since that's
      // the more intuitive reading of "type of content." Matching only the
      // raw entity type meant any bundle-style filter silently zeroed out
      // every result. Accept either.
      $bundle = !empty($metadata['type']) ? (string) $metadata['type'] : '';
      // Some indexes don't store the bundle in metadata; fall back to
      // deriving it from the already-loaded entity so bundle-style filters
      // still work in that case.
      if ($bundle === '' && !empty($resolved['entity'])) {
        list(, , $loaded_bundle) = entity_extract_ids($entity_type, $resolved['entity']);
        $bundle = !empty($loaded_bundle) ? (string) $loaded_bundle : '';
      }
      if (!ai_search_rag_entity_type_allowed($entity_type, $bundle, $allowed_entity_types_lower)) {
        continue;
      }
    }

    $entity = $resolved['entity'];
    if (empty($entity)) {
      continue;
    }
    if (function_exists('entity_access') && !entity_access('view', $entity_type, $entity)) {
      continue;
    }

    $title = ai_search_rag_entity_title($entity_type, $entity, $metadata);
    $url = ai_search_rag_entity_url($entity_type, $entity, $metadata);
    $snippet = ai_search_rag_entity_snippet($match, $metadata);

    $sources[] = [
      'source_id' => ai_search_rag_source_id($index->machine_name, $entity_type, $entity_id, $metadata),
      'index_id' => $index->machine_name,
      'backend' => $backend_class,
      'entity_type' => $entity_type,
      'entity_id' => $entity_id,
      'field_name' => !empty($metadata['field_name']) ? (string) $metadata['field_name'] : '',
      'chunk_index' => isset($metadata['chunk_index']) ? (int) $metadata['chunk_index'] : 0,
      'title' => $title,
      'url' => $url,
      'snippet' => $snippet,
      'score' => $score,
    ];
  }

  usort($sources, 'ai_search_rag_sort_sources');
  return $sources;
}

/**
 * Decide whether an entity_type/bundle pair passes an entity_types filter.
 *
 * Matches case-insensitively against either the raw Backdrop entity type
 * or the bundle/content-type name, since the tool's entity_types param
 * accepts either form.
 *
 * @param string $entity_type
 *   The raw Backdrop entity type (e.g. "node").
 * @param string $bundle
 *   The bundle/content-type name (e.g. "person"), or '' if unknown.
 * @param array $allowed_entity_types_lower
 *   Allowed entity_types values, already lowercased.
 *
 * @return bool
 *   TRUE if the pair passes the filter (or no filter is configured).
 */
function ai_search_rag_entity_type_allowed($entity_type, $bundle, array $allowed_entity_types_lower) {
  if (empty($allowed_entity_types_lower)) {
    return TRUE;
  }
  if (in_array(strtolower((string) $entity_type), $allowed_entity_types_lower, TRUE)) {
    return TRUE;
  }
  $bundle = (string) $bundle;
  return $bundle !== '' && in_array(strtolower($bundle), $allowed_entity_types_lower, TRUE);
}

/**
 * Build a stable source id for citations.
 */
function ai_search_rag_source_id($index_id, $entity_type, $entity_id, array $metadata = []) {
  $field_name = !empty($metadata['field_name']) ? $metadata['field_name'] : '';
  $chunk_index = isset($metadata['chunk_index']) ? (int) $metadata['chunk_index'] : 0;
  return $index_id . ':' . $entity_type . '/' . $entity_id . ':' . $field_name . ':' . $chunk_index;
}

/**
 * Return a readable title for one source entity.
 */
function ai_search_rag_entity_title($entity_type, $entity, array $metadata = []) {
  return ai_search_match_title($entity_type, $entity, $metadata);
}

/**
 * Build a relative URL for one source entity.
 */
function ai_search_rag_entity_url($entity_type, $entity, array $metadata = []) {
  return ai_search_match_relative_url($entity_type, $entity, $metadata);
}

/**
 * Build a short text snippet for one source.
 */
function ai_search_rag_entity_snippet(array $match, array $metadata = []) {
  return ai_search_match_snippet($match, $metadata);
}

/**
 * Sort sources by score descending.
 */
function ai_search_rag_sort_sources(array $left, array $right) {
  $left_score = isset($left['score']) ? (float) $left['score'] : 0;
  $right_score = isset($right['score']) ? (float) $right['score'] : 0;

  if ($left_score === $right_score) {
    return 0;
  }

  return ($left_score < $right_score) ? 1 : -1;
}
