Read and follow Agent.md in this project's root directory.

# AI Search Architecture & Development Guide

## Project Overview

`ai_search` is a Backdrop CMS module providing a vector search and embeddings framework for AI-powered content discovery and RAG (retrieval-augmented generation). It indexes content as embeddings via Search API, stores vectors in a pluggable backend, and exposes RAG search as both a `hook_ai_tools()` tool (for chatbots/agents) and user-facing blocks (search box, chat, explorer UI).

Embedding generation and chat completions are delegated to the core `ai` module's provider system (`ai_get_api_for_model()`), not to per-provider code inside `ai_search` itself.

## Architecture

### Core Module (`ai_search`)

Root files: `ai_search.module`, `ai_search.info`, `ai_search.install` — only dependency is `ai`.

`includes/`:
- **AiSearchBackend.inc** – embedding engine discovery/config (`ai_search_get_embedding_engine_definitions()`, engine config forms, vector normalization, metadata re-ranking)
- **AiSearchBackendInterface.inc** – interface for vector backend plugins
- **AiSearchBackendPluginBase.inc** – base plugin for Search API service integration
- **AiSearchEmbeddingsBase.inc** / **AiSearchEmbeddingsEngine.inc** – embedding engine base classes
- **AiSearchVectorClientBase.inc** – abstract base for vector storage clients (`query()`, `upsert()`, `stats()`)
- **AITextChunker.inc** – text chunking for indexing
- **AISearchDatabaseBoost.inc** – hybrid search: boosts DB search results with AI similarity scores
- **rag.tools.inc** – implements `hook_ai_tools()` (`ai_search_rag_tools()`), i.e. `list_rag_indexes` / `rag_search` tools callable by AI agents/chatbots

Classes are registered in `ai_search_autoload_info()` — register new `includes/` classes there.

### Vector Storage Backends (siblings, NOT submodules)

Vector backends live as their own top-level contrib modules, decoupled from `ai_search` core (see git history: "decouple vector backends: remove Milvus/Pinecone specifics from core"):

- `modules/contrib/ai_search_provider_mariadb` (`ai_search_mariadb.module`) – MariaDB native `VECTOR` type (requires MariaDB 11.7+, 11.8 LTS recommended)
- `modules/contrib/ai_search_provider_milvus` (`ai_search_milvus.module`) – Milvus
- `modules/contrib/ai_search_provider_pgvector` (`ai_search_pgvector.module`) – PostgreSQL + pgvector
- `modules/contrib/ai_search_provider_pinecone` (`ai_search_pinecone.module`) – Pinecone

Each depends only on `ai_search` and provides a `includes/*Service.inc` extending `SearchApiAbstractService`, registered via `hook_search_api_service_info()`. Each is also its own separate git repo (`backdrop-contrib/ai_search_provider_*`) with its own `CLAUDE.md` and `README.md` — read those for backend-specific details rather than duplicating them here.

### Embedding Providers

**Embedding models are discovered directly from the `ai` module's provider system** (e.g. `ai_provider_openai`, `ai_provider_ollama`, `ai_provider_openrouter`) — there are no dedicated embedding-provider submodules for current use.

Legacy compatibility shims exist in `modules/custom/`:
- `ai_search_provider_openai`, `ai_search_provider_ollama`, `ai_search_provider_openrouter`

These are **deprecated** per their own `.info` descriptions and `CLAUDE.md` files — keep enabled only to avoid breaking existing Search API index configs that reference their legacy class names. Do not build new embedding integrations here; new providers are picked up automatically once they register with the core `ai` module. Note: their `README.md` files are stale (pre-deprecation) — trust the `.info`/`CLAUDE.md` for current behavior.

### User-Facing Submodules (`ai_search/modules/`)

- **ai_search_explorer** – Admin UI for conversational search exploration (Admin → Configuration → AI → Search API AI Explorer)
- **ai_search_simple_chatbot** – Block-based chatbot with streaming responses and session-based chat history
- **ai_search_search_block** – AI-enhanced search block (form → animated response → optional DB results). Has its own submodule `modules/ai_search_search_block_log` for logging queries.
- **ai_search_header** – Header search form that redirects to the AI Search page with `?q=` and auto-runs the AI search block (depends on `ai_search_search_block`, not `ai_search` directly)

### Key Design Patterns

1. **Provider Pattern**: Embedding engines resolved dynamically via `ai_search_get_embedding_engine_definitions()` from whatever `ai_provider_*` modules are enabled; `ai_search_build_embedding_engine_definition($provider_id, $model_id, $model_name)` builds an engine definition on the fly.
2. **Service Pattern**: Vector backends extend `SearchApiAbstractService` and use client classes extending `AiSearchVectorClientBase`.
3. **Compatibility Layer**: `ai_get_api_for_model($model, $stripped_model)` from core `ai` for provider-aware client resolution; `$client->embedding($input, $model)` for embeddings.
4. **Tool Pattern**: RAG is exposed to AI agents as tools via `hook_ai_tools()` (`includes/rag.tools.inc`), not just as a UI feature.
5. **Configuration Storage**: API keys/secrets via Key module integration.

## Development Environment

### DDEV Configuration

- Milvus DDEV config: `resources/.ddev/docker-compose.milvus.yaml` — symlink/copy into `.ddev/docker-compose.milvus.yaml` to enable locally.

### File Locations

- **Module root**: `ai_search.module`, `.info`, `.install`
- **includes/**: Core classes (autoloaded via `hook_autoload_info()`)
- **modules/**: User-facing submodules (explorer, chatbot, search block, header)
- **resources/**: Dev resources (DDEV configs)
- Vector backends: sibling modules `modules/contrib/ai_search_provider_{mariadb,milvus,pgvector,pinecone}`
- Legacy embedding shims: `modules/custom/ai_search_provider_{openai,ollama,openrouter}`

## Creating a New Vector Backend

1. Create a top-level module `ai_search_provider_[backend]` (sibling to `ai_search`, depending on it) — do NOT nest it under `ai_search/modules/`.
2. Create `includes/[Name]Service.inc` extending `SearchApiAbstractService`.
3. Create a client class extending `AiSearchVectorClientBase` with `query()`, `upsert()`, and `stats()` methods.
4. Implement `hook_search_api_service_info()` to register the backend.
5. Store credentials using Key module with `#type => 'key_select'`.

## Data Flow

1. **Indexing**: Content → `AITextChunker` chunking → embedding generation (via `ai` module provider) → vector storage upsert.
2. **Search**: Query → embedding generation → vector similarity search → result ranking → response generation.
3. **Hybrid Search**: Traditional DB search results boosted by AI similarity scores (`AISearchDatabaseBoost.inc`).
4. **RAG-as-tool**: Agent/chatbot calls `rag_search`/`list_rag_indexes` tools (`includes/rag.tools.inc`) → same query/vector/ranking pipeline → sources returned to the calling agent.

## Known Gotchas

- **`ai_search_search_block` status region**: `js/ai_search_search_block.js` maintains a screen-reader-only live region (`#search-api-ai-search-block-status`) for status messages like "AI response ready." It applies inline hiding styles rather than relying solely on a `.visually-hidden`/`.sr-only` theme utility class — several active themes (e.g. `updraft_foundation`) don't define `.visually-hidden`, which previously caused status text to render visibly on the page. Don't remove the inline styles in favor of a class-only approach unless every consuming theme is verified to define that class.
- **Legacy provider shims**: `modules/custom/ai_search_provider_{openai,ollama,openrouter}` are deprecated but must stay enabled on sites with existing Search API indexes referencing their class names. Don't delete them without a migration path for those index configs.

## Branch Structure

- **Main branch**: `1.x-1.x` (Backdrop CMS 1.x)
- Commits should target `1.x-1.x` for pull requests

## Configuration Paths

- Search API indexes: Admin → Structure → Search → [Index] → Settings
- AI Explorer: Admin → Configuration → AI → Search API AI Explorer (`admin/config/ai/explorer/search`)
- Search block log settings: `admin/config/ai/search/block-log`
- Block configuration: Structure → Blocks → [AI Search Block / Simple Chatbot / Header]

**Last Updated:** 2026-07-07 by Claude
