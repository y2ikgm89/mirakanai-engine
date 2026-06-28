# Codebase Memory Local AI Helper v1

## Status

Retained policy design record.

## Decision

`codebase-memory-mcp` is accepted for MIRAIKANAI Engine as a developer-local AI assistance tool only. It is not an engine, editor, runtime, package, CMake, vcpkg, or public API dependency.

The project uses the B+C approach:

- B: GameEngine documents and validates local helper operation, storage boundaries, and forbidden uses.
- C: runtime edge creation belongs in an external `codebase-memory-mcp` fork or upstream contribution, not in this repository. The local developer setup may point Codex at a patched WSL build while the engine repository keeps no runtime dependency.

No compatibility layer is required. If external runtime edge creation lands, this project should adopt one current, explicit trace-to-graph contract instead of supporting legacy or ambiguous trace shapes.

## Official And Local Evidence

- Context7 resolved `/deusdata/codebase-memory-mcp` as the relevant MCP and documented repository indexing, graph search, snippet lookup, graph schema, architecture/search utilities, ADR management, and trace ingestion tool surfaces.
- Context7 resolved `/open-telemetry/opentelemetry-specification` and `/websites/opentelemetry_io` for trace data. OpenTelemetry spans carry trace/span identifiers, parent relationships, names, start/end timestamps, attributes, events, links, status, and resource attributes such as `service.name`.
- OpenTelemetry semantic conventions expose stable source-code attributes such as `code.function.name` and `code.file.path`; older `code.function` / `code.namespace` forms must not be the primary GameEngine contract.
- A patched external `codebase-memory-mcp` local build on 2026-06-28 passed `make -f Makefile.cbm test`, `make -f Makefile.cbm lint-format`, production build, MCP stdio `tools/list`, Windows-path WSL translation, and runtime trace smoke verification.
- The runtime trace smoke indexed a Windows-path temporary repository through the WSL wrapper, translated it to `/mnt/...`, then `ingest_traces` returned `status=accepted`, `nodes_created=2`, and `edges_created=2`; follow-up graph queries returned one `RuntimeTrace` and one `OBSERVED_EXECUTION` edge.
- A local full index of this repository reached ready status as project `mnt-g-workspace-development-GameEngine` with 63502 nodes and 183330 edges using `persistence=false`. `mode=fast` is insufficient here because this repository's AI-operable workflow depends on docs and tools.
- After Codex restart on 2026-06-28, `mcp__codebase_memory_mcp` tools were exposed in-session. `list_projects`, `index_status`, `get_graph_schema`, `trace_path`, `detect_changes`, and `ingest_traces` all executed against `mnt-g-workspace-development-GameEngine`; after the agent-surface update and full `persistence=false` re-index, the active local cache reported ready status with 63518 nodes and 185686 edges after one sanitized smoke trace created `RuntimeTrace`, `RuntimeSpan`, `CONTAINS_SPAN`, and `OBSERVED_EXECUTION`.
- The cross-tool agent surface was updated with the dedicated `gameengine-codebase-memory` skill for Codex, Claude Code, and Cursor, plus matching rule and subagent guidance.

Official references:

- `codebase-memory-mcp`: https://github.com/DeusData/codebase-memory-mcp
- OpenTelemetry traces: https://opentelemetry.io/docs/concepts/signals/traces/
- OpenTelemetry source code attributes: https://opentelemetry.io/docs/specs/semconv/general/attributes/
- OpenTelemetry specification: https://github.com/open-telemetry/opentelemetry-specification

## GameEngine Local Operation Policy

Allowed local uses:

- Use the `gameengine-codebase-memory` skill when a task mentions `codebase-memory-mcp`, codebase memory, MCP graph indexing, `ingest_traces`, runtime trace graph edges, or broad graph-backed code exploration.
- Use `index_repository` with `mode=full` and `persistence=false` by default for this repository.
- Use `persistence=true` only when an operator explicitly wants a local repo artifact. `.codebase-memory/graph.db.zst` and all other `.codebase-memory/` contents are ignored repository-local artifacts and must not be committed.
- Use `list_projects`, `index_status`, `get_graph_schema`, `search_graph`, `get_code_snippet`, and `trace_path` when exposed by the active MCP as exploration aids.
- Use `detect_changes` only as impact-analysis input; it is not a replacement for `git status`, especially for untracked files.
- Use semantic/vector search only as a supplement. Exact claims still need verification through repository files, manifest data, tests, and supported scripts.
- Use `ingest_traces` only with a patched local or upstream `codebase-memory-mcp` build that has passed runtime-edge smoke verification for `RuntimeTrace`, `RuntimeSpan`, `CONTAINS_SPAN`, and `OBSERVED_EXECUTION`.
- For external `codebase-memory-mcp` validation on WSL, use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/install-codebase-memory-wsl-deps.ps1 [-DistroName Ubuntu-24.04]` to install `zlib1g-dev`, `clang-format`, and `pkg-config` inside WSL. Use `-CheckOnly` for status, and run from an interactive terminal when sudo needs a password.

Forbidden project uses:

- Do not add `codebase-memory-mcp` to CMake, vcpkg, engine source, editor source, game package output, runtime package output, or CI requirements.
- Do not commit `.mcp.json`, `.codebase-memory/`, graph artifacts, trace payloads, MCP server state, credentials, or machine-local paths.
- Do not use `manage_adr` for project decisions. MIRAIKANAI decisions belong in `docs/adr/`, `docs/specs/`, `docs/superpowers/plans/`, and `engine/agent/manifest.fragments/` as appropriate.
- Do not treat `search_graph` or `semantic_query` as proof. They can locate candidates; they cannot replace `rg`, direct file reads, manifests, or validation commands.
- Do not ingest unsanitized runtime traces, raw logs, raw stdout/stderr, credentials, crash dumps, PIX/ETW traces, binary payloads, or machine-local absolute paths.
- Do not use `ingest_traces` output alone to claim runtime coverage, validation execution, or production readiness.

## External Runtime Edge Creation Contract

C is an external implementation target for `codebase-memory-mcp`; GameEngine only consumes the resulting local MCP behavior.

The external MCP should ingest OpenTelemetry-compatible OTLP trace JSON shaped as `resourceSpans[] -> scopeSpans[] -> spans[]`. It should normalize resource, scope, span, event, link, and status data before graph writes.

Minimum graph nodes:

- `RuntimeTrace`: one trace identity with service/resource summary.
- `RuntimeSpan`: one span identity with name, kind, status, start/end time, duration, resource attributes, scope attributes, and selected span attributes.

Minimum graph edges:

- `CONTAINS_SPAN`: `RuntimeTrace` to `RuntimeSpan`.
- `PARENT_SPAN`: parent span to child span when `parentSpanId` resolves.
- `SPAN_LINK`: span to linked span when link identifiers resolve.
- `OBSERVED_EXECUTION`: runtime span to existing Function/Method nodes when code attributes resolve unambiguously.
- `OBSERVED_HTTP_ROUTE` or `OBSERVED_HTTP_CALL`: runtime span to Route or endpoint-like nodes when standardized HTTP attributes resolve.

Symbol resolution order for `OBSERVED_EXECUTION`:

1. `code.function.name` plus repository-relative `code.file.path`.
2. `code.function.name` plus a stable namespace/module attribute when available.
3. Span name only when the graph has exactly one high-confidence candidate; otherwise emit a diagnostic and skip the edge.

Required ingest result fields:

- `traces_received`
- `spans_processed`
- `nodes_created`
- `edges_created`
- `skipped_spans`
- `diagnostics`

Ambiguous, missing, or privacy-redacted code attributes must produce diagnostics, not guessed edges.

## Future GameEngine Trace Attribute Contract

If this repository later produces sanitized trace payloads for local MCP ingestion, use OpenTelemetry-compatible fields plus project-specific attributes:

- `service.name`: `mirakanai-local-validation`, `mirakanai-editor`, or another reviewed service name.
- `code.function.name`: source function name for symbol resolution.
- `code.file.path`: repository-relative source path only.
- `validation.recipe.id`: manifest validation recipe id when a recipe is being observed.
- `test.target`: CMake/CTest target or script-level test target.
- `game.id`: `game.agent.json` id when a game-owned flow is observed.
- `package.target`: package target id when package validation is observed.
- `ge.status`: `ok`, `failed`, `skipped`, or `blocked`.
- `ge.diagnostic_code`: stable diagnostic code for skipped/blocked/failure cases.
- `ge.counter.<name>`: numeric counters with reviewed names.

The trace producer must not include raw stdout/stderr, raw logs, environment variables, user names, home directories, absolute workspace paths in committed artifacts, command-line secrets, crash dump bodies, PIX/ETW trace bodies, binary payloads, signing material, or credentials.

## Validation

GameEngine-side policy changes are validated with:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`

Full C++ validation is not required for this policy because it intentionally changes no engine, runtime, editor, package, build, or public C++ surface.
