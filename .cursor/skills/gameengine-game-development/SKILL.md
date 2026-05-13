---
name: gameengine-game-development
description: Scaffolds and maintains C++ games, game.agent.json, and desktop or mobile validation lanes. Use when editing games/, new-game scripts, or game manifests.
paths:
  - "games/**"
  - "tools/new-game.ps1"
  - "tools/new-game-helpers.ps1"
  - "docs/ai-game-development.md"
---

# GameEngine game development (Cursor)

Full workflow lives in shared skills. Start with the short `SKILL.md` routers, then load the shared detailed references listed below only when exact game API names, manifest fields, package lanes, or mobile/desktop validation recipes are needed.

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-game-development/SKILL.md` |
| Claude Code detailed reference | `.claude/skills/gameengine-game-development/references/full-guidance.md` |
| Codex | `.agents/skills/gameengine-game-development/SKILL.md` |
| Codex detailed reference | `.agents/skills/gameengine-game-development/references/full-guidance.md` |
| Baseline | `AGENTS.md` |

Games live under `games/<game_name>/` with `game.agent.json`; see `AGENTS.md` **AI-Driven Game Development**.

Runtime package payloads are byte-hashed. New text cooked/runtime extensions or `runtimePackageFiles` entries need matching `runtime/.gitattributes` `text eol=lf`, scaffold/static-check parity, and the narrowest package smoke before slice completion.
