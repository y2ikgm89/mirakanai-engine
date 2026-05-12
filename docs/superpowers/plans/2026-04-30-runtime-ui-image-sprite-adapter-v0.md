# Runtime UI Image Sprite Adapter v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote `mirakana_ui` image placeholder payloads into a concrete dependency-free `mirakana_ui_renderer` image sprite submission path backed by first-party palette bindings.

**Architecture:** Keep image decoding, texture upload, glyph/font work, SDL3, Dear ImGui, RHI devices, and native handles outside the public game UI path. `mirakana_ui` remains renderer-free and continues to emit stable image placeholder rows; `mirakana_ui_renderer` owns an adapter palette that maps a UI image `resource_id` or `asset_uri` to a renderer color/sprite placeholder so generated games can validate image UI composition through `IRenderer` without pretending production texture rendering exists.

**Tech Stack:** C++23, `mirakana_ui`, `mirakana_ui_renderer`, `mirakana_renderer::IRenderer`, `NullRenderer`, focused unit tests, docs/manifest/skills sync.

---

## Context

- `mirakana_ui` already exposes `ImageContent`, `RendererImagePlaceholder`, and `build_image_adapter_payload`.
- `mirakana_ui_renderer` currently submits styled `RendererBox` payloads as `SpriteCommand` rectangles and only reports image placeholder availability.
- The missing host-feasible production step is a concrete first-party image adapter boundary that can submit resolved image placeholders through the renderer without adding a decoder, font stack, RHI handle, SDL3, Dear ImGui, or middleware dependency.

## Constraints

- Do not add third-party dependencies.
- Keep `mirakana_ui` independent from renderer/RHI/platform/editor code.
- Keep public APIs under `mirakana::` and avoid native/GPU handles.
- Do not claim PNG/JPEG decode, texture upload, atlas allocation, nine-slice scaling, masking, filtering, batching, or accessibility OS bridge support.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End the slice with focused tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: Failing UI Renderer Image Tests

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`

- [x] Add a focused test named `ui renderer submits resolved image placeholders through image sprite palette`.
- [x] Build a `UiDocument` with a root, one styled panel box, one image element with `resource_id="ui/portrait"`, `asset_uri="assets/ui/portrait.texture"`, and `tint_token="image.tint"`.
- [x] Use a new `UiRendererImagePalette` in the test with a binding for `resource_id="ui/portrait"` and an expected color.
- [x] Submit through `NullRenderer` and assert `result.image_placeholders_available == 1`, `result.image_sprites_submitted == 1`, `result.image_resources_resolved == 1`, `result.image_resources_missing == 0`, and `renderer.stats().sprites_submitted == 2`.
- [x] Add a second test named `ui renderer reports unresolved image placeholders without submitting fake sprites` with one image placeholder and no palette binding; assert the placeholder is counted, missing count is `1`, submitted image sprites remain `0`, and renderer sprite stats stay at `0`.
- [x] Run `cmake --build --preset dev --target mirakana_ui_renderer_tests` and confirm the new tests fail because the image palette/result fields do not exist.

### Task 2: Image Sprite Palette API And Implementation

**Files:**
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`

- [x] Add `UiRendererImageBinding` with `std::string resource_id`, `std::string asset_uri`, and `Color color`.
- [x] Add `UiRendererImagePalette` with `try_add`, `add`, `find_by_resource_id`, `find_by_asset_uri`, and `count`, mirroring the current `UiRendererTheme` duplicate-rejection style.
- [x] Extend `UiRenderSubmitDesc` with `const UiRendererImagePalette* image_palette{nullptr}`.
- [x] Extend `UiRenderSubmitResult` with `image_sprites_submitted`, `image_resources_resolved`, and `image_resources_missing`.
- [x] Add `resolve_ui_image_color(const ui::ImageAdapterRow&, const UiRenderSubmitDesc&)` returning `const Color*` for an exact `resource_id` match first, then exact `asset_uri`.
- [x] Add `make_ui_image_sprite_command(const ui::ImageAdapterRow&, Color)` using the row bounds center/size, matching `make_ui_box_sprite_command`.
- [x] Update `submit_ui_renderer_submission` to build the existing image payload, submit only resolved image rows as sprites, count unresolved rows as missing, and keep invalid image rows diagnostic-only through the existing payload diagnostics.
- [x] Run focused `mirakana_ui_renderer_tests` until all pass.

### Task 3: Docs, Manifest, Skills, And Plan Registry

**Files:**
- Modify: `docs/ui.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-04-30-runtime-ui-image-sprite-adapter-v0.md`

- [x] Document that image placeholders can now be resolved by an engine-owned sprite palette through `mirakana_ui_renderer`.
- [x] State clearly that this is a colored sprite placeholder adapter, not image decoding, texture upload, atlas management, or production image rendering.
- [x] Update manifest and game-development skills so generated games use `UiRendererImagePalette`, `UiRendererImageBinding`, and `submit_ui_renderer_submission` for deterministic UI image smoke tests only.
- [x] Add AI integration checks for `UiRendererImagePalette`, `image_sprites_submitted`, and the no-texture-upload caveat across docs/manifest/skills.

### Task 4: Focused And Full Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `cmake --build --preset dev --target mirakana_ui_renderer_tests`.
- [x] Run `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_ui_renderer_tests"`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Request cpp-reviewer after implementation.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Move this plan to Completed in `docs/superpowers/plans/README.md` after evidence is recorded.

## Validation Results

- RED check: `cmake --build --preset dev --target mirakana_ui_renderer_tests` failed before implementation because `UiRendererImagePalette`, `UiRendererImageBinding`, image submit desc/result fields, and helper APIs did not exist.
- Focused build: `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake @('--build','--preset','dev','--target','mirakana_ui_renderer_tests')` passed after implementation and after reviewer follow-up tests.
- Focused CTest: `ctest --test-dir out\build\dev -C Debug --output-on-failure -R "mirakana_ui_renderer_tests"` passed, 1/1 test target.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config passed; strict analysis remains diagnostic-only because the Visual Studio generator did not emit `out/build/dev/compile_commands.json` for the current host.
- `cpp-reviewer`: found missing precedence/command-shape tests and missing precedence docs; follow-up added `resource_id`-first / `asset_uri`-fallback tests, `make_ui_image_sprite_command` transform/color tests, public header comments, and docs/manifest/skills/subagent guidance.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Known diagnostic-only host gates remain Metal `metal`/`metallib` missing, Apple packaging/Xcode unavailable, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database unavailable.

## Remaining Follow-Up

- Real image decoding, texture upload, atlas allocation, UVs, filtering, nine-slice, masking/clipping, batching, renderer material integration, OS accessibility bridge, and production font/text shaping.

## Done When

- `mirakana_ui_renderer` can submit resolved UI image placeholders through `IRenderer` using only first-party `mirakana::` APIs.
- Missing image bindings are diagnosed by result counters without submitting fake sprites.
- Docs, manifest, skills, and AI checks distinguish this adapter from production image texture rendering.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or concrete host blockers are recorded.
