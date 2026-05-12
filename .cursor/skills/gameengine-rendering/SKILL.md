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

Full workflow lives in shared skills. Read these canonical files (ASCII paths):

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-rendering/SKILL.md` |
| Codex | `.agents/skills/rendering-change/SKILL.md` |
| Baseline | `AGENTS.md` |

Shader and GPU toolchain: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` when relevant.
