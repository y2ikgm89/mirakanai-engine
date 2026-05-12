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

# Rendering Change

## Scope

Use this skill for renderer, RHI, shaders, GPU resources, frame graph, materials, textures, meshes, and graphics backends.

## Context Budget Rules

- Start with targeted file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` whenever possible.
- Do not load `references/full-guidance.md` by default. Load it only when the current task needs exact API names, validation counters, retained ids, package lanes, or backend/editor details not present here.
- Keep implementation slices small, clean-break, and evidence-backed. Do not add compatibility shims, stale aliases, broad ready claims, or unsupported host assumptions.
- Prefer focused build/test/static loops while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice gate.

## Required Discipline

- Keep renderer interfaces backend-neutral and native handles behind backend/PIMPL or explicit interop designs.
- Read `references/full-guidance.md` only when detailed backend lanes, shader artifact rules, smoke counters, or historical rendering needles are needed.
- Use focused renderer/RHI/shader checks while iterating, then `tools/validate.ps1` at the slice gate.
- Use official SDK/tool docs for backend/toolchain behavior and keep D3D12/Vulkan/Metal readiness claims evidence-backed.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and historical validation evidence. Load only the sections needed for the current task.
