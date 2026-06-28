---
name: gameengine-codebase-memory
description: Use codebase-memory-mcp as a developer-local GameEngine code intelligence helper.
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

# GameEngine codebase memory (Cursor)

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-codebase-memory/SKILL.md` |
| Codex | `.agents/skills/gameengine-codebase-memory/SKILL.md` |
| Policy | `docs/specs/2026-06-28-codebase-memory-local-ai-helper-v1.md` |

Read the Claude skill for the canonical workflow. Use `codebase-memory-mcp` only as a developer-local helper: `index_repository` with `mode=full` and `persistence=false` by default, verify graph output with files/tests/manifests, never use `manage_adr`, and keep `.mcp.json`, `.codebase-memory/graph.db.zst`, graph artifacts, and traces uncommitted.

Use `ingest_traces` only for sanitized OTLP-shaped spans after `RuntimeTrace`, `RuntimeSpan`, `CONTAINS_SPAN`, and `OBSERVED_EXECUTION` smoke verification.
Trace payloads use `code.function.name`, repository-relative `code.file.path`, `validation.recipe.id`, and reviewed attributes only; WSL validation uses `tools/install-codebase-memory-wsl-deps.ps1`.
