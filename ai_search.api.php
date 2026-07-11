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

/**
 * Declare per-model embedding quirks that ai_search should apply.
 *
 * Some embedding models require a task-specific prefix on their input to
 * perform well -- nomic-embed-text is the first known case, trained
 * expecting "search_query: " / "search_document: " prefixes. Rather than
 * every module that generates embeddings hardcoding a check for a specific
 * model, the provider that owns a model declares its quirks here; ai_search
 * (and any other embedding consumer) looks this registry up by matching the
 * model id against each returned pattern.
 *
 * @return array
 *   Keyed by a substring pattern matched (case-insensitively) against the
 *   model id, e.g. "nomic-embed-text". Each entry may contain:
 *   - query_prefix: prepended to text embedded as a search query.
 *   - document_prefix: prepended to text embedded as indexed content.
 */
function hook_ai_embedding_model_quirks() {
  return [
    'my-embedding-model' => [
      'query_prefix'    => 'query: ',
      'document_prefix' => 'passage: ',
    ],
  ];
}
