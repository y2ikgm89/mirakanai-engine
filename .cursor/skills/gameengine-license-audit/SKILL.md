---
name: gameengine-license-audit
description: Audits third-party code, dependencies, and distributable assets for license compliance. Use when changing vcpkg.json, legal notices, or shipping external material.
paths:
  - "vcpkg.json"
  - "docs/legal-and-licensing.md"
  - "docs/dependencies.md"
  - "THIRD_PARTY_NOTICES.md"
  - "tools/check-dependency-policy.ps1"
---

# GameEngine license audit (Cursor)

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-license-audit/SKILL.md` |
| Codex | `.agents/skills/license-audit/SKILL.md` |
| Baseline | `AGENTS.md` |

Dependency policy: `docs/legal-and-licensing.md`, `docs/dependencies.md`, `vcpkg.json`, `THIRD_PARTY_NOTICES.md`, and `tools/check-dependency-policy.ps1`.
