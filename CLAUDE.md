# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Search API AI is a Backdrop CMS module that integrates AI-powered search capabilities using embeddings and vector storage. The module provides a flexible, extensible architecture for creating semantic search experiences by leveraging multiple AI providers and vector databases (Pinecone, Milvus, PGVector).

## Architecture

### Core Module Structure

The base `ai_search` module provides foundational classes and interfaces:

- **AiSearchEmbeddingsBase** (`includes/AiSearchEmbeddingsBase.inc`) - Abstract base class for embedding engine implementations
- **AiSearchVectorClientBase** (`includes/AiSearchVectorClientBase.inc`) - Abstract base class for vector storage clients
- **AiSearchBackendInterface** (`includes/AiSearchBackendInterface.inc`) - Interface for backend plugin implementations
- **AiSearchBackendPluginBase** (`includes/AiSearchBackendPluginBase.inc`) - Base plugin for Search API integration
- **AiSearchBackend** (`includes/AiSearchBackend.inc`) - Helper functions for engine configuration and form building
- **TextChunker** (`includes/TextChunker.inc`) - Text chunking utilities for processing large content
- **DatabaseBoostByAiSearch** (`includes/DatabaseBoostByAiSearch.inc`) - Hybrid search implementation boosting database results with AI similarity scores

### Module Categories

**Embedding Providers** (submodules that register embedding engines):
- `ai_search_openai` - OpenAI embedding models (text-embedding-3-small, text-embedding-3-large, text-embedding-ada-002)
- `ai_search_ollama` - Ollama embedding models with auto-dimension detection
- `ai_search_openrouter` - OpenRouter embedding models

**Vector Storage Backends** (Search API service implementations):
- `ai_search_pinecone` - Pinecone vector database backend
- `ai_search_milvus` - Milvus vector database backend
- `ai_search_pgvector` - PostgreSQL with pgvector extension backend

**User-Facing Features**:
- `ai_search_explorer` - Admin UI for conversational search exploration (Admin → Configuration → AI → Search API AI Explorer)
- `ai_search_simple_chatbot` - Block-based chatbot with streaming responses and session-based chat history
- `ai_search_search_block` - AI-enhanced search block with optional logging submodule

### Key Design Patterns

1. **Provider Pattern**: Embedding providers register engines via `hook_embedding_engine_info()` that return class definitions with model metadata
2. **Service Pattern**: Vector backends extend `SearchApiAbstractService` and use client classes extending `AiSearchVectorClientBase`
3. **Compatibility Layer**: Uses `ai_get_api_for_model()` from core `ai` module for provider-aware client resolution.
4. **Configuration Storage**: API keys and sensitive data stored via Key module integration

## AI Module Compatibility

Current code should use the provider-aware helpers from the `ai` module:
- Use `ai_get_api_for_model($model, $stripped_model)` to get a compatible client
- Use `$client->embedding($input, $model)` for provider-agnostic embedding calls

## Development Environment

### DDEV Configuration

The project includes DDEV setup with Milvus support:
- Configuration: `resources/.ddev/docker-compose.milvus.yaml`
- To enable Milvus locally, symlink or copy this file to `.ddev/docker-compose.milvus.yaml`

### File Locations

- **Module root**: Core module files (`ai_search.module`, `.info`, `.install`)
- **includes/**: Core classes and interfaces (autoloaded via `hook_autoload_info()`)
- **modules/**: Submodules providing specific functionality
- **resources/**: Development resources (DDEV configs)

### Autoloading

Classes are registered in `ai_search_autoload_info()`. When adding new base classes to `includes/`, register them in this hook.

## Creating New Embedding Providers

1. Create a submodule `ai_search_[provider]`
2. Extend `AiSearchEmbeddingsBase` for each model
3. Implement `hook_embedding_engine_info()` to register engines:
   ```php
   function mymodule_embedding_engine_info() {
     return [
       'mymodel' => [
         'label' => t('My Model'),
         'class' => 'MyEmbeddingsEngine',
         'file' => 'includes/MyEmbeddingsEngine.inc',
         'module' => 'ai_search_mymodule',
       ],
     ];
   }
   ```
4. Set `$modelDimension` and `$modelName` in your class
5. Implement `generateEmbeddings()` to call your API

## Creating New Vector Backends

1. Create a submodule `ai_search_[backend]`
2. Create `includes/service.inc` extending `SearchApiAbstractService`
3. Create a client class extending `AiSearchVectorClientBase` with `query()`, `upsert()`, and `stats()` methods
4. Implement `hook_search_api_service_info()` to register the backend
5. Add embeddings engine configuration via `ai_search_update_embedding_configuration_form()`
6. Store credentials using Key module with `#type => 'key_select'`

## Data Flow

1. **Indexing**: Content → Text chunking → Embedding generation → Vector storage upsert
2. **Search**: Query → Embedding generation → Vector similarity search → Result ranking → Response generation
3. **Hybrid Search**: Traditional database search results boosted by AI similarity scores (see `DatabaseBoostByAiSearch`)

## Branch Structure

- **Main branch**: `1.x-1.x` (for Backdrop CMS 1.x)
- Commits should target `1.x-1.x` for pull requests
- Recent work includes provider-aware AI module support and null value handling

## Configuration Paths

- Search API indexes: Admin → Structure → Search → [Index] → Settings
- AI Explorer: Admin → Configuration → Open AI → Search API AI Explorer
- Block configuration: Structure → Blocks → [AI Search Block]
- Logging: Automatically stored when `ai_search_search_block_log` is enabled
