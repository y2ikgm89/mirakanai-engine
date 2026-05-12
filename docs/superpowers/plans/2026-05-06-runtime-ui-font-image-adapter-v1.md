# Runtime UI Font Image Adapter v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free runtime UI renderer adapter path that can turn deterministic text adapter glyph rows into atlas-backed sprite submissions, while keeping font rasterization, shaping, IME, accessibility publication, and image decoding behind future host/audited adapters.

**Status:** Completed

**Architecture:** Keep `mirakana_ui` as the headless contract owner and extend its glyph placeholder rows with stable glyph identifiers derived by the existing deterministic monospace layout path. Keep `mirakana_ui_renderer` as the renderer-intent adapter by adding a first-party glyph atlas palette that maps `(font_family, glyph)` rows to already-cooked atlas pages and UVs, mirroring the existing image palette model. This slice must not add fonts, third-party text/image libraries, OS APIs, SDL3, Dear ImGui, native handles, real shaping, font loading, glyph rasterization, runtime image decoding, or production atlas packing.

**Tech Stack:** C++23, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_renderer` `SpriteCommand`, focused `mirakana_ui_renderer_tests`, docs/manifest/skills/static AI guidance sync.

---

## Context

- `mirakana_ui` already exposes retained UI documents, renderer submission payloads, text/image/accessibility adapter payload rows, and a dependency-free `MonospaceTextLayoutPolicy`.
- `mirakana_ui_renderer` already submits boxes and resolved image placeholders through `UiRendererTheme` and `UiRendererImagePalette`.
- The master plan still lists production-minimum runtime UI text/image/font adapter path work before a 1.0 AI-operable production claim.
- Generated games need a first-party way to render simple HUD/menu text with already-cooked glyph atlas resources without depending on Dear ImGui, SDL3 UI APIs, source image decoding, or a bundled font/text library.

## Constraints

- Do not add third-party dependencies, fonts, UI art, or source image decoding.
- Do not implement complex text shaping, bidirectional reordering, IME, OS accessibility publication, or font rasterization in this slice.
- Do not expose renderer/RHI/native handles through `mirakana_ui` or generated game APIs.
- Keep `mirakana_ui` independent from `mirakana_renderer`, `mirakana_rhi`, `mirakana_runtime`, SDL3, Dear ImGui, and editor code.
- Keep glyph atlas resolution deterministic and value-type only.
- Existing image atlas palette behavior remains the image path; this slice may document it as part of the combined text/image adapter boundary but should not broaden atlas packing claims.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Glyph Atlas Renderer Tests

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`

- [x] Add a test named `ui renderer submits monospace text glyphs through glyph atlas palette`.
- [x] Build a `RendererSubmission` with a text run using `font_family = "ui/body"` and monospace policy `{8, 4, 10}`.
- [x] Add glyph atlas bindings for `P`, `l`, `a`, and `y` that point at one cooked atlas page and distinct UV rects.
- [x] Assert the submit path reports four glyphs resolved/submitted and `NullRenderer` receives four texture-enabled sprite commands.
- [x] Add a test named `ui renderer reports missing glyph atlas bindings without fake sprites`.
- [x] Assert a missing glyph increments missing glyph counters and submits no text sprites.
- [x] Run `cmake --build --preset dev --target mirakana_ui_renderer_tests` and confirm RED because the glyph atlas types/functions do not exist yet.

### Task 2: mirakana_ui Glyph Identity Rows

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`

- [x] Add a `std::uint32_t glyph` field to `TextAdapterGlyphPlaceholder`.
- [x] Populate `glyph` in the existing monospace text layout path using the decoded scalar for valid UTF-8 and a single-byte fallback for malformed bytes.
- [x] Keep the non-monospace placeholder path diagnostic-only by leaving its aggregate placeholder glyph as `0`.
- [x] Preserve existing text adapter row ordering, bounds, and diagnostics.

### Task 3: mirakana_ui_renderer Glyph Atlas Palette

**Files:**
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`

- [x] Add `UiRendererGlyphAtlasBinding` and `UiRendererGlyphAtlasPalette` with duplicate rejection by `(font_family, glyph)`.
- [x] Add glyph atlas fields to `UiRenderSubmitDesc` and glyph counters to `UiRenderSubmitResult`.
- [x] Add `resolve_ui_text_glyph_binding` and `make_ui_text_glyph_sprite_command`.
- [x] Extend `submit_ui_renderer_submission` to build text adapter payload rows with the caller-selected monospace policy when a glyph atlas palette is present, submit resolved glyph sprites, and report missing glyphs without drawing fallback sprites.
- [x] Keep image palette submission behavior unchanged.

### Task 4: GREEN Focused Tests

**Files:**
- Modify: files from Tasks 1-3

- [x] Run `cmake --build --preset dev --target mirakana_ui_renderer_tests` until the new tests pass.
- [x] Run `ctest --preset dev -R "mirakana_ui_renderer_tests" --output-on-failure`.
- [x] Refactor only after focused tests are green.

### Task 5: Docs, Manifest, Skills, And Static Guidance

**Files:**
- Modify: `docs/ui.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-05-06-runtime-ui-font-image-adapter-v1.md`

- [x] Mark this plan as the active slice in the plan registry and manifest before implementation.
- [x] After implementation, document the glyph atlas palette, glyph sprite submission counters, and combined text/image adapter boundary.
- [x] Keep ready claims narrow: no real font loading/rasterization, shaping, bidi, IME, OS accessibility publication, runtime image decoding, production atlas packing, native handles, SDL3, Dear ImGui, UI middleware, or editor-private runtime APIs.
- [x] Update static AI checks so Codex and Claude guidance stay synchronized.
- [x] Return `currentActivePlan` to the master plan and `recommendedNextPlan` to `next-production-gap-selection` after validation passes.

### Task 6: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run focused `mirakana_ui_renderer_tests` build/CTest commands.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and move this plan to Recent Completed in the registry.

## Done When

- `TextAdapterGlyphPlaceholder` rows carry deterministic glyph ids for the dependency-free monospace text layout path.
- `UiRendererGlyphAtlasPalette` resolves glyphs by `font_family` and glyph id without dependencies or native handles.
- `submit_ui_renderer_submission` can submit atlas-backed text glyph sprites and report resolved/missing glyph counters.
- Existing image palette submission remains intact and documented as the current cooked-image adapter path.
- Docs, manifest, Codex/Claude skills, and static checks state the new ready boundary and non-goals honestly.
- Focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete local-tool blocker is recorded.

## Validation Results

- RED confirmed: `cmake --build --preset dev --target mirakana_ui_renderer_tests` failed before implementation because the glyph atlas API and counters did not exist.
- Focused GREEN confirmed: sanitized dev-preset build of `mirakana_ui_renderer_tests` passed after implementation with `text_glyph_sprites_submitted` coverage.
- Focused CTest confirmed: `ctest --preset dev -R "mirakana_ui_renderer_tests" --output-on-failure` passed after implementation.
- Ready claim is narrow and without claiming font loading/rasterization, glyph atlas generation, shaping, bidi reordering, IME, OS accessibility publication, runtime image decoding, production atlas packing, native handles, SDL3, Dear ImGui, UI middleware, or editor-private runtime APIs.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed after restoring `persistent blackboard services` guidance in `docs/roadmap.md`.
- Direct `cmake --build --preset dev --target mirakana_ui_renderer_tests` is blocked in this shell by the host `Path` / `PATH` duplicate MSBuild environment issue; the same build passed through a sanitized environment with a single `Path`.
- `ctest --preset dev -R "mirakana_ui_renderer_tests" --output-on-failure`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Diagnostic-only host gates remain unchanged: Metal shader tools and Apple packaging are unavailable on this Windows host.
