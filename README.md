# AI Search

AI Search integrates vector embeddings and semantic search into Backdrop CMS
via the Search API framework. It provides a provider-agnostic embedding
pipeline, multiple vector storage backends, and optional UI modules for
conversational search and chatbot experiences.

## Features

- **Semantic search** — indexes content as vector embeddings and retrieves
  results by meaning rather than keyword match.
- **Multiple embedding providers** — OpenAI, Ollama, and OpenRouter embedding
  models supported out of the box.
- **Multiple vector backends** — Pinecone, Milvus, and PostgreSQL with
  pgvector all supported as Search API service backends.
- **Hybrid search** — optionally boosts traditional database search results
  with AI similarity scores.
- **Text chunking** — splits large content into chunks before embedding for
  better retrieval accuracy.
- **Extensible** — add new embedding engines or vector backends by implementing
  a small set of hooks and base classes.

## Requirements

- Backdrop CMS 1.x
- [AI module](https://github.com/backdrop-contrib/ai) with at least one
  configured provider
- [Search API](https://backdropcms.org/project/search_api)
- At least one vector backend (Pinecone, Milvus, or pgvector)

## Installation

1. Install and enable the AI module and configure an embedding-capable provider.
2. Install and enable this module and the submodules you need.
3. Create a Search API index using one of the AI Search vector backends.
4. Add your content fields to the index using the Embeddings field type.
5. Index your content.

## Included Submodules

### **ai_search_search_block**
A configurable block that provides AI-powered site search. An optional logging
submodule (`ai_search_search_block_log`) records queries and results.

### **ai_search_explorer**
Admin UI for running vector search queries and inspecting results interactively.
Found at **Admin → Configuration → AI → Search API AI Explorer**.

### **ai_search_simple_chatbot**
Block-based chatbot with streaming responses and session-based chat history,
powered by the configured AI search index. (Deprecated in favor of the more flexible and powerful AI Assistants
module which in part of [AI Agents module](https://www.drupal.org/project/ai_agents))

## Issues

Bugs and feature requests should be reported in the
[Issue Queue](https://github.com/backdrop-contrib/ai_search/issues).

## Current Maintainer

[Justin Keiser](https://github.com/keiserjb)

## Credits

- Created for Backdrop CMS by [Justin Keiser](https://github.com/keiserjb).
- Inspired by the Drupal [Search API AI module](https://www.drupal.org/project/search_api_ai) and the [AI Search
  module](https://www.drupal.org/project/ai_search).
- Developed with AI assistance.

## License

This project is GPL v2 software. See the LICENSE.txt file in this directory for
complete text.
