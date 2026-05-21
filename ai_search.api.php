<?php

/**
 * @file
 * Hooks for the ai_search module.
 */

/**
 * Declare vector backends that support ai_search RAG retrieval.
 *
 * This lets provider modules expose RAG-queryable Search API backends without
 * hardcoding backend class names inside ai_search itself.
 *
 * @return array
 *   An associative array of backend definitions keyed by any unique id.
 *   Each definition may contain:
 *   - server_classes: array of Search API server class ids this backend
 *     supports, such as ['milvus'].
 *   - query_callback: callable that executes the vector query:
 *     callback($server, $index, array $vector, $dimension, $top_k)
 *   - supports_callback: optional callable used for finer-grained runtime
 *     checks. Signature: callback($server, $index)
 */
function hook_ai_search_rag_backend_info() {
  return [
    'my_backend' => [
      'server_classes' => ['my_backend'],
      'query_callback' => 'my_module_ai_search_rag_query',
      'supports_callback' => 'my_module_ai_search_rag_supports',
    ],
  ];
}
