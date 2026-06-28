---
name: gameengine-codebase-memory
description: Uses codebase-memory-mcp as a developer-local code intelligence helper for GameEngine indexing, graph exploration, impact analysis, and sanitized runtime trace ingestion.
paths:
  - "AGENTS.md"
  - "docs/ai-integration.md"
  - "docs/specs/2026-06-28-codebase-memory-local-ai-helper-v1.md"
  - "engine/agent/manifest.fragments/012-documentationPolicy.json"
  - "tools/check-ai-integration-145-codebase-memory-local-helper.ps1"
  - "tools/install-codebase-memory-wsl-deps.ps1"
  - ".agents/skills/gameengine-codebase-memory/**"
  - ".claude/skills/gameengine-codebase-memory/**"
  - ".cursor/skills/gameengine-codebase-memory/**"
---

# GameEngine Codebase Memory

## Scope

Use this skill when a task mentions `codebase-memory-mcp`, codebase memory, MCP graph indexing, `ingest_traces`, `RuntimeTrace`, `RuntimeSpan`, `OBSERVED_EXECUTION`, or when broad architecture, symbol, dependency, caller/callee, or change-impact exploration would benefit from the indexed graph.

## Official Source

- Use Context7 with `/deusdata/codebase-memory-mcp` for current upstream behavior before changing install, update, indexing, tool, or trace-ingestion guidance.
- Official setup is developer-local: install/configure the MCP server for coding agents, restart the agent, then index the repository with an absolute path.

## Operating Contract

- Treat `codebase-memory-mcp` as a local AI helper only. It is not an engine, editor, runtime, package, CMake, vcpkg, CI, or public API dependency.
- Confirm the active project with `list_projects` and `index_status` before relying on graph output. For this repository the canonical project is `mnt-g-workspace-development-GameEngine` when indexed through WSL.
- If the project is absent or stale, run `index_repository` with an absolute repo path, `mode=full`, and `persistence=false`.
- Use `persistence=true` only when the operator explicitly wants a local ignored graph artifact. `.codebase-memory/graph.db.zst` and all `.codebase-memory/` contents stay uncommitted.
- Use `trace_path`, `search_graph`, `get_code_snippet`, `get_graph_schema`, `detect_changes`, and `index_status` as discovery aids. Verify exact claims with `rg`, direct file reads, `git status`, manifest data, tests, and supported validation scripts.
- Do not use `manage_adr`; MIRAIKANAI decisions belong in `docs/adr/`, `docs/specs/`, `docs/superpowers/plans/`, and `engine/agent/manifest.fragments/`.
- Do not commit `.mcp.json`, `.codebase-memory/`, cache databases, trace payloads, raw logs, credentials, machine-local paths, crash dumps, PIX/ETW traces, binary payloads, or MCP server state.

## Runtime Traces

- Use `ingest_traces` only with a patched local or upstream build that has passed smoke verification for `RuntimeTrace`, `RuntimeSpan`, `CONTAINS_SPAN`, and `OBSERVED_EXECUTION`.
- Send sanitized OpenTelemetry-compatible OTLP trace JSON shaped as `resourceSpans[] -> scopeSpans[] -> spans[]`.
- Prefer `code.function.name` plus repository-relative `code.file.path` for `OBSERVED_EXECUTION` resolution. Treat missing or ambiguous code attributes as diagnostics, not proof.
- Allowed project-specific attributes include `validation.recipe.id`, `test.target`, `game.id`, `package.target`, `ge.status`, and `ge.diagnostic_code`.
- Never ingest raw stdout/stderr, raw logs, environment variables, user names, home directories, absolute workspace paths in committed artifacts, command-line secrets, signing material, crash dump bodies, PIX/ETW trace bodies, or credentials.
- `ingest_traces` output is local evidence for graph exploration only; it cannot by itself prove runtime coverage, validation execution, or production readiness.

## WSL Validation

- For external `codebase-memory-mcp` C validation on WSL, use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/install-codebase-memory-wsl-deps.ps1 [-DistroName Ubuntu-24.04] [-CheckOnly]`.
- Required WSL packages for the external validation lane are `zlib1g-dev`, `clang-format`, and `pkg-config`.
- If sudo needs a password, run the command from an interactive terminal. Do not move these packages into GameEngine CMake, vcpkg, or CI.

## Completion Checks

- After policy or agent-surface edits, run `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-text-format.ps1`.
- Full C++ validation is not required for codebase-memory policy-only changes unless the task also changes engine, runtime, editor, build, package, or public C++ surfaces.
