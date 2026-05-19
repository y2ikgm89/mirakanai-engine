# Production UI Importer Platform Adapters 1.0 Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close `production-ui-importer-platform-adapters` for the Engine 1.0 Windows-default ready surface without claiming broad low-level UI, codec, or platform-service parity.

**Architecture:** Accept the existing reviewed adapter-boundary and package evidence as the supported 1.0 surface: first-party `MK_ui` contracts, SDL3 text-input/event/clipboard bridges, reviewed PNG decode adapter, decoded image atlas bridge, glyph atlas package bridge, and package-visible native UI overlay/atlas smokes. Keep production text shaping implementation, real font loading/rasterization, OS accessibility publication, native IME/text services beyond the reviewed SDL3 rows, broader source codecs, SVG/vector parsing, renderer texture-upload APIs, arbitrary importer adapters, UI middleware, and new dependency/legal records as explicit future or dependency-gated non-goals.

**Tech Stack:** C++23 contracts already implemented in `MK_ui`, `MK_platform_sdl3`, `MK_tools`, `MK_ui_renderer`, and desktop package validation; this closeout changes docs, manifest fragments, generated manifest output, validation recipe metadata, static guards, and stale `MK_tools` unsupported-gap result metadata.

---

**Plan ID:** `production-ui-importer-platform-adapters-1-0-closeout-v1`
**Status:** Completed
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Remove `production-ui-importer-platform-adapters` from `unsupportedProductionGaps` once the supported 1.0 Windows-default surface is narrowed to the reviewed runtime UI/importer/platform-adapter evidence that already exists.

## Context

- The master plan now selects `production-ui-importer-platform-adapters` as the active closeout wedge after Editor Productization closed.
- Existing implementation evidence covers host-independent accessibility/IME/text-input/text edit/text shaping/font rasterization/image decode request planning, selected SDL3 platform bridges, optional reviewed PNG decoding through `asset-importers`, decoded UI atlas package authoring, glyph atlas package authoring, and package-visible native UI overlay/atlas smokes.
- Reviewed contract evidence includes `AccessibilityPublishPlan`, `ImeCompositionPublishPlan`, `PlatformTextInputSessionPlan`, `TextShapingRequestPlan`, `FontRasterizationRequestPlan`, `ImageDecodeRequestPlan`, `PngImageDecodingAdapter`, `author_packed_ui_atlas_from_decoded_images`, and `author_packed_ui_glyph_atlas_from_rasterized_glyphs`.
- Earlier Phase 3 program docs deliberately kept the broad gap planned until the 1.0 ready boundary was selected.

## Constraints

- Do not add new dependencies, vcpkg features, legal records, or source codec/font/text libraries in this closeout.
- Do not claim production text shaping, real font loading/rasterization, OS accessibility publication, broad native IME/text services, UI middleware, broader codecs, SVG/vector parsing, arbitrary importer adapters, platform SDK parity, renderer texture-upload APIs, or broad runtime UI renderer quality.
- Keep command surfaces honest: remove this gap id from manifest `unsupportedGapIds` only for the reviewed surfaces whose remaining limitations are now explicit future/dependency-gated non-goals.
- Keep the composed manifest generated from fragments.

## Done When

- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` removes `production-ui-importer-platform-adapters` from `unsupportedProductionGaps` and manifest command-surface `unsupportedGapIds`, and `engine/agent/manifest.json` is recomposed.
- `tools/run-validation-recipe.ps1` and `tools/validation-recipe-core.ps1` stop reporting this gap as a current blocker.
- Static guards assert the closeout plan and explicit exclusions instead of requiring a planned gap row.
- Current capability docs, roadmap, registry, master plan, and stream index name the new boundary and leave only `full-repository-quality-gate` as the active unsupported gap.
- Full validation passes or a concrete host/tool blocker is recorded.

## Tasks

- [x] Update manifest fragment and compose output.
- [x] Update validation recipe outputs, tool result metadata, and static guards.
- [x] Update docs and plan registry.
- [x] Run focused agent/static checks.
- [x] Run full validation for the manifest/static/docs phase gate.
- [ ] Commit and push the validated checkpoint to PR #120.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS | Recomputed `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` | PASS | Rebuilt the focused tool test target after removing stale production UI gap ids from tool result metadata. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests` | PASS | Focused GREEN after an expected RED failure proved the new assertions caught stale `production-ui-importer-platform-adapters` output. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Static JSON/agent surface contract guard passed after the Production UI closeout needles were updated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-surface drift check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Cross-agent instruction parity and budgets passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reports `unsupported_gaps=1`; only `full-repository-quality-gate` remains. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Passed after formatting the touched C++ sources. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest passed 65/65, shader/Apple host gates remain diagnostic-only or host-gated. |
