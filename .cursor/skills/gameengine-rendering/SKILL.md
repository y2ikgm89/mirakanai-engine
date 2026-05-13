---
name: gameengine-rendering
description: Covers renderer, RHI, shaders, GPU resources, frame graph, materials, textures, meshes, and graphics backends. Use when editing engine/renderer/, shaders/, or GPU-related sources.
paths:
  - "engine/renderer/**"
  - "shaders/**"
  - "**/*.hlsl"
  - "**/*.msl"
  - "**/*.metal"
---

# GameEngine rendering (Cursor)

Full workflow lives in shared skills. Start with the short `SKILL.md` routers, then load the shared detailed references listed below only when exact renderer/RHI API names, shader artifact rules, package lanes, or backend-specific validation recipes are needed.

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-rendering/SKILL.md` |
| Claude Code detailed reference | `.claude/skills/gameengine-rendering/references/full-guidance.md` |
| Codex | `.agents/skills/rendering-change/SKILL.md` |
| Codex detailed reference | `.agents/skills/rendering-change/references/full-guidance.md` |
| Baseline | `AGENTS.md` |

Shader and GPU toolchain: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` when relevant.
