---
name: gameengine-plan-registry
description: >-
  Places new GameEngine production work into dated capability, gap-cluster, or milestone plans under docs/superpowers/plans/
  per docs/superpowers/plans/README.md lifecycle rules. Use when creating or extending roadmap
  items, implementation plans, milestones, or specs tied to engine/editor/runtime packaging work,
  or when unsure whether to append to historical MVP closure docs.
paths:
  - "docs/superpowers/plans/**"
  - "docs/superpowers/plans/README.md"
  - "docs/roadmap.md"
  - "docs/current-capabilities.md"
disable-model-invocation: true
---

# GameEngine plan registry

## Before writing or extending plans

1. Read **`docs/superpowers/plans/README.md`** for active roadmap vs active phase/child vs completed vs host-gated rules.
2. Read **`docs/current-capabilities.md`** and **`docs/roadmap.md`** for current truth; plans are evidence records, not replacements for those pages.
3. **Do not append unrelated work** to **`docs/superpowers/plans/2026-05-01-core-first-mvp-closure.md`** or historical MVP bodies — start a dated capability/gap-cluster/milestone plan and use phases for behavior/API/validation boundary decisions.
4. Keep plan width broader than PR width: one plan may hold multiple phase checkpoints, while each phase/PR stays one reviewable purpose with validation evidence.
5. **Pick the `YYYY-MM-DD` prefix** from the authoritative authoring date: use the Cursor/chat **`Today's date`** when present; otherwise confirm with the operator or read the local calendar (`Get-Date -Format yyyy-MM-dd`). Keep the same date in the filename and the plan heading. Do not use model-default or UTC-inferred “tomorrow” when the operator’s date is still “today.”

## After a slice lands

Update **`docs/superpowers/plans/README.md`**, **`docs/roadmap.md`**, relevant subsystem docs (including **`docs/architecture.md`** when game manifest or cross-layer contracts change), and the **engine agent manifest** (`engine/agent/manifest.fragments/*.json` + `tools/compose-agent-manifest.ps1 -Write`) when capabilities or agent contracts change (per registry instructions).

`pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` validates `.cursor/skills/*/SKILL.md` frontmatter together with Codex and Claude skills, enforces `claudeToCodexSkillMap` twins for `gameengine-*` topics, and verifies Cursor thin-pointer folder names (see `docs/workflows.md`).

## Docs navigation entry

**`docs/README.md`** — canonical reading order for contributors and agents.
