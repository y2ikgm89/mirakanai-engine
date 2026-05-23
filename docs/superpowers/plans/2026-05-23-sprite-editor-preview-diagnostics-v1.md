# Sprite Editor Preview Diagnostics v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add retained editor and game-facing diagnostics for selected sprites, atlas pages, animation frames, sorting metadata, hitboxes, and package dependencies so operators can review 2D sprite readiness without renderer-native handle exposure.

**Plan ID:** `sprite-editor-preview-diagnostics-v1`

**Status:** Validated; PR pending.

**Gap:** `sprite-editor-preview-diagnostics-v1`

**Architecture:** Planned next child of Candidate Backlog Burn-down v1. Keep diagnostics value-oriented and reviewable, prefer existing editor-core retained models and runtime diagnostics contracts, and keep visible preview execution host-gated.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_runtime`, `MK_scene`, `MK_scene_renderer` only through public rows as needed, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official Docs Review

- Use project editor retained-model conventions and official target-scoped CMake patterns for new tests.
- Use first-party diagnostics rows and package manifests as the source of truth; do not scrape renderer-native objects or mutate game packages from preview code.

## Non-Goals

- Production sprite editor UX polish, source image decoding/import breadth, GPU/RHI preview handles, live package mutation, arbitrary shell execution, Vulkan/Metal preview parity, subjective visual-quality claims, or broad renderer readiness.

## Done When

- Unit tests prove retained diagnostics for selected sprite/package inputs and fail-closed malformed references.
- Visible/editor or package-facing smoke evidence is host-gated and reports selected diagnostic rows without native handle exposure.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.

## Implementation Summary

- Added retained `MK_editor_core` sprite preview diagnostics in `playtest_package_review`:
  `EditorSpritePreviewDiagnosticsDesc`, `EditorSpritePreviewDiagnosticsModel`,
  `EditorSpritePreviewExecutionSnapshot`, `make_editor_sprite_preview_diagnostics_model`,
  `apply_editor_sprite_preview_execution_snapshot`, and
  `make_editor_sprite_preview_diagnostics_ui_model`.
- Diagnostics are value-only and package-facing: selected sprite texture/material readiness, atlas page size
  and usage counts, sprite animation frame/dependency rows, sorting labels, collision hitbox/hurtbox/hit
  counters, runtime package diagnostics, and retained UI row ids.
- Visible preview execution stays host-owned. The model defaults to `Host Required`; snapshots can report
  host-rendered evidence, while execution/native-handle claims are rejected and never flip editor-core
  `executes` or native-handle flags.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_editor_core_tests` | pass | Focused editor-core build after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests"` | pass | Covers happy path, selected-sprite animation frame counting, animation dependency-edge propagation, fail-closed malformed sprite refs, missing selected frame/entity poses, sanitized selection-id collisions, collision diagnostics, retained UI rows, and unsafe host snapshot rejection. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -Files "editor/core/src/playtest_package_review.cpp,tests/unit/editor_core_tests.cpp"` | pass | Focused clang-tidy lane for the editor-core implementation and tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` | pass | Re-run after final review fixes on 2026-05-24 JST; completed with `validate: ok`; `test.ps1 -SkipBuild` reported 76/76 tests passed. |
