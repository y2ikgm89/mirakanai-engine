---
name: gameengine-game-development
description: Scaffolds and maintains C++ games, game.agent.json, and desktop or mobile validation lanes. Use when editing games/, new-game scripts, or game manifests.
paths:
  - "games/**"
  - "tools/new-game.ps1"
  - "tools/new-game-helpers.ps1"
  - "docs/ai-game-development.md"
---

# GameEngine Game Development

## Scope

Use this skill for C++ games, game.agent.json, new-game scaffolding, desktop runtime packages, and mobile validation lanes.

## Context Budget Rules

- Start with targeted file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` whenever possible.
- Do not load `references/full-guidance.md` by default. Load it only when the current task needs exact API names, validation counters, retained ids, package lanes, or backend/editor details not present here.
- Keep implementation slices small, clean-break, and evidence-backed. Do not add compatibility shims, stale aliases, broad ready claims, or unsupported host assumptions.
- Prefer focused build/test/static loops while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice gate.

## Required Discipline

- Use `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` before engine-facing game/API decisions.
- Read `references/full-guidance.md` only for detailed public API lists, package-lane counters, generated-game manifests, or mobile/desktop runtime recipes.
- Keep `game_name` and `new-game -Name` values matching `^[a-z][a-z0-9_]*$` and keep source-tree paths lowercase snake_case.
- Runtime package payloads are byte-hashed. When adding a text cooked/runtime extension or `runtimePackageFiles` entry, update the game/scaffold `runtime/.gitattributes` with `text eol=lf`, keep scaffold/static checks aligned, and run the narrowest package smoke before the slice gate.
- Validate with the smallest relevant package/game lane first, then `tools/validate.ps1` at the slice gate.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
