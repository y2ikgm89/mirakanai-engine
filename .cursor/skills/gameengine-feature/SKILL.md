---
name: gameengine-feature
description: Adds engine features, public C++ APIs, sample games, and engine-facing gameplay systems. Use when editing engine/, games/, or feature specs under docs/specs/.
paths:
  - "engine/**"
  - "games/**"
  - "docs/specs/**"
---

# GameEngine feature work (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-feature/SKILL.md` |
| Codex | `.agents/skills/gameengine-feature/SKILL.md` |
| Baseline | `AGENTS.md` |

Before changing engine-facing APIs or game scaffolding: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` (optional `-ContextProfile Minimal|Standard|Full`) or targeted reads of `engine/agent/manifest.fragments/*.json` / `engine/agent/manifest.json`. To **change** the engine agent contract, edit fragments and run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
