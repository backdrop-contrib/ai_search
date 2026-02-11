# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Search API AI is a Backdrop CMS module that integrates AI-powered search capabilities using embeddings and vector storage. The module provides a flexible, extensible architecture for creating semantic search experiences by leveraging various AI providers (OpenAI, Ollama, OpenRouter) and vector databases (Pinecone, Milvus, PGVector).

## Architecture

### Core Module Structure

The base `search_api_ai` module provides foundational classes and interfaces:

- **SearchApiAiEmbeddingsBase** (`includes/SearchApiAiEmbeddingsBase.inc`) - Abstract base class for embedding engine implementations
- **SearchApiAiVectorClientBase** (`includes/SearchApiAiVectorClientBase.inc`) - Abstract base class for vector storage clients
- **SearchApiAiBackendInterface** (`includes/SearchApiBackendInterface.inc`) - Interface for backend plugin implementations
- **SearchApiAiBackendPluginBase** (`includes/SearchApiAiBackendPluginBase.inc`) - Base plugin for Search API integration
- **SearchApiAiBackend** (`includes/SearchApiAiBackend.inc`) - Helper functions for engine configuration and form building
- **TextChunker** (`includes/TextChunker.inc`) - Text chunking utilities for processing large content
- **DatabaseBoostByAiSearch** (`includes/DatabaseBoostByAiSearch.inc`) - Hybrid search implementation boosting database results with AI similarity scores

### Module Categories

**Embedding Providers** (submodules that register embedding engines):
- `search_api_ai_openai` - OpenAI embedding models (text-embedding-3-small, text-embedding-3-large, text-embedding-ada-002)
- `search_api_ai_ollama` - Ollama embedding models with auto-dimension detection
- `search_api_ai_openrouter` - OpenRouter embedding models

**Vector Storage Backends** (Search API service implementations):
- `search_api_ai_pinecone` - Pinecone vector database backend
- `search_api_ai_milvus` - Milvus vector database backend
- `search_api_ai_pgvector` - PostgreSQL with pgvector extension backend

**User-Facing Features**:
- `search_api_ai_explorer` - Admin UI for conversational search exploration (Admin → Configuration → Open AI → Search API AI Explorer)
- `search_api_ai_simple_chatbot` - Block-based chatbot with streaming responses and session-based chat history
- `search_api_ai_search_block` - AI-enhanced search block with optional logging submodule

### Key Design Patterns

1. **Provider Pattern**: Embedding providers register engines via `hook_embedding_engine_info()` that return class definitions with model metadata
2. **Service Pattern**: Vector backends extend `SearchApiAbstractService` and use client classes extending `SearchApiAiVectorClientBase`
3. **Compatibility Layer**: `search_api_ai_openai_client_for_model()` provides fallbacks for OpenAI module 1.x and 2.x API differences
4. **Configuration Storage**: API keys and sensitive data stored via Key module integration

## OpenAI Module Compatibility

The module handles three different OpenAI module API versions:
- **2.x**: Uses `openai_get_api_for_model()` with provider/model separation
- **1.x multi-provider**: Uses `openai_get_api($provider)`
- **1.x legacy**: Direct `OpenAIApi` instantiation with key from config

When adding embedding generation code:
- Use `search_api_ai_openai_client_for_model($model, $stripped_model)` to get a compatible client
- Use `search_api_ai_openai_call_embedding($client, $input, $model)` for API-agnostic embedding calls

## Development Environment

### DDEV Configuration

The project includes DDEV setup with Milvus support:
- Configuration: `resources/.ddev/docker-compose.milvus.yaml`
- To enable Milvus locally, symlink or copy this file to `.ddev/docker-compose.milvus.yaml`

### File Locations

- **Module root**: Core module files (`search_api_ai.module`, `.info`, `.install`)
- **includes/**: Core classes and interfaces (autoloaded via `hook_autoload_info()`)
- **modules/**: Submodules providing specific functionality
- **resources/**: Development resources (DDEV configs)

### Autoloading

Classes are registered in `search_api_ai_autoload_info()`. When adding new base classes to `includes/`, register them in this hook.

## Creating New Embedding Providers

1. Create a submodule `search_api_ai_[provider]`
2. Extend `SearchApiAiEmbeddingsBase` for each model
3. Implement `hook_embedding_engine_info()` to register engines:
   ```php
   function mymodule_embedding_engine_info() {
     return [
       'mymodel' => [
         'label' => t('My Model'),
         'class' => 'MyEmbeddingsEngine',
         'file' => 'includes/MyEmbeddingsEngine.inc',
         'module' => 'search_api_ai_mymodule',
       ],
     ];
   }
   ```
4. Set `$modelDimension` and `$modelName` in your class
5. Implement `generateEmbeddings()` to call your API

## Creating New Vector Backends

1. Create a submodule `search_api_ai_[backend]`
2. Create `includes/service.inc` extending `SearchApiAbstractService`
3. Create a client class extending `SearchApiAiVectorClientBase` with `query()`, `upsert()`, and `stats()` methods
4. Implement `hook_search_api_service_info()` to register the backend
5. Add embeddings engine configuration via `search_api_ai_update_embedding_configuration_form()`
6. Store credentials using Key module with `#type => 'key_select'`

## Data Flow

1. **Indexing**: Content → Text chunking → Embedding generation → Vector storage upsert
2. **Search**: Query → Embedding generation → Vector similarity search → Result ranking → Response generation
3. **Hybrid Search**: Traditional database search results boosted by AI similarity scores (see `DatabaseBoostByAiSearch`)

## Branch Structure

- **Main branch**: `1.x-1.x` (for Backdrop CMS 1.x)
- Commits should target `1.x-1.x` for pull requests
- Recent work includes OpenAI 2.x support and null value handling

## Configuration Paths

- Search API indexes: Admin → Structure → Search → [Index] → Settings
- AI Explorer: Admin → Configuration → Open AI → Search API AI Explorer
- Block configuration: Structure → Blocks → [AI Search Block]
- Logging: Automatically stored when `search_api_ai_search_block_log` is enabled
