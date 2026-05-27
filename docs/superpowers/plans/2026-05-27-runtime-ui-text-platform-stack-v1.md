# Runtime UI Text Platform Stack v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Move runtime UI from selected value-only evidence toward real production text shaping, font rasterization, IME, accessibility, and platform UI parity lanes behind first-party contracts and audited adapters.

**Architecture:** Keep `MK_ui` and gameplay-facing APIs first-party, value-based, and independent from SDL3, Dear ImGui, platform SDKs, HarfBuzz, FreeType, ICU, or renderer backend handles. Optional dependencies and platform SDK calls live behind reviewed adapters with host/dependency gates, package-visible counters, fail-closed diagnostics, legal records, and no compatibility shims.

**Tech Stack:** C++23, `MK_ui`, `MK_ui_renderer`, `MK_platform_sdl3`, `MK_runtime_host_sdl3`, `MK_tools`, optional vcpkg features for HarfBuzz/FreeType/ICU-class services where selected, SDL3 text input/clipboard, HarfBuzz shaping, FreeType glyph rasterization, ICU boundary analysis, WAI-ARIA/APG semantics, Microsoft UI Automation, platform accessibility SDKs, CMake/CTest, and PowerShell validation wrappers.

---

**Plan ID:** `runtime-ui-text-platform-stack-v1`

**Status:** Active.

Selected child plan of `clean-break-broad-production-readiness-master-plan-v1` after `renderer-production-quality-backend-parity-v1` completed through PRs #261, #262, and #263.

**Date:** 2026-05-27

## Context

Current runtime UI evidence is intentionally narrow:

- `production-runtime-ui-workbench-v1` proves dense retained UI intent rows and package counters without renderer, text shaping, font rasterization, IME, accessibility bridge, image decoding, or native platform adapter execution.
- Runtime UI Production Stack Evidence v1 proves first-party text/font/IME/accessibility contract rows and package counters but keeps real adapters uninvoked.
- Existing SDL3 text input, IME composition, committed-text, clipboard, text shaping request, font rasterization request, image decode request, accessibility publish, and platform text-input plans are useful foundations, but broad runtime UI parity remains unclaimed.

This child plan strengthens those foundations without reopening Engine 1.0. It must not claim production platform UI parity until text shaping, real font rasterization, native IME, accessibility publication, renderer atlas/upload handoff, package counters, host gates, docs, manifest, and validation evidence are implemented.

## Official Practice Check

Official documentation rechecked for this plan selection:

- SDL3 text input and IME events: <https://wiki.libsdl.org/SDL3/SDL_StartTextInput>, <https://wiki.libsdl.org/SDL3/SDL_TextInputEvent>, <https://wiki.libsdl.org/SDL3/README-migration>, and <https://wiki.libsdl.org/SDL3/CategoryKeyboard>.
- HarfBuzz shaping: <https://harfbuzz.github.io/harfbuzz-hb-shape.html> and Context7 `/harfbuzz/harfbuzz` evidence for `hb_shape`, buffer segment properties, glyph infos, glyph positions, and clusters.
- FreeType glyph rasterization and metrics: <https://freetype.org/freetype2/docs/glyphs/index.html>, <https://freetype.org/freetype2/docs/glyphs/glyphs-7.html>, and <https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html>.
- ICU boundary analysis: <https://unicode-org.github.io/icu/userguide/boundaryanalysis/>.
- WAI-ARIA/APG accessibility semantics and keyboard practices: <https://www.w3.org/WAI/ARIA/apg/> and <https://www.w3.org/WAI/standards-guidelines/aria/>.
- Microsoft UI Automation provider guidance: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview> and <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview>.

## Constraints

- `MK_ui` must not depend on SDL3, Dear ImGui, renderer/RHI backends, HarfBuzz, FreeType, ICU, or platform accessibility SDK types.
- Native handles, platform objects, middleware types, and UI toolkit objects stay private to adapters.
- Optional dependencies require vcpkg manifest features, `tools/bootstrap-deps.ps1`, dependency docs, legal records, notices, validation wrappers, and manifest feature rows before adapter code is enabled.
- Text shaping and font rasterization readiness require deterministic glyph id, cluster, advance/offset, bitmap/metrics, atlas placement, and fallback diagnostics; value-only request rows are not enough.
- IME readiness requires start/update/end/candidate/text-area/committed-text evidence and platform-host adapter dispatch. SDL3 proof cannot imply Win32/TSF, Cocoa, Linux IME, Android, or iOS parity.
- Accessibility readiness requires role/name/state/focus/action/relationship/live-region rows plus platform publication evidence. WAI-ARIA/APG semantics and Microsoft UI Automation evidence do not imply other platform bridges.
- Package counters must distinguish ready, host-gated, dependency-gated, adapter-invoked, skipped, and unsupported rows.

## Candidate Evidence

| Candidate | Scope | Current evidence | Remaining gap |
| --- | --- | --- | --- |
| `runtime-ui-text-stack-audit-v1` | Audit existing UI text/font/IME/accessibility plans, tests, samples, package counters, and non-claims. | Runtime UI Workbench and Runtime UI Production Stack Evidence v1 provide value-only rows and selected package counters. | Need a concise evidence map before adapter implementation so future claims stay fail-closed. |
| `runtime-ui-shaping-raster-adapter-contracts-v1` | Clean-break shaping/raster adapter contracts plus deterministic package counters. | `plan_text_shaping_request`, `shape_text_run`, `plan_font_rasterization_request`, and `rasterize_font_glyph` validate request/result rows around caller-owned adapters. | Need audited dependency-gated HarfBuzz/FreeType-class adapters or explicit host/dependency-gated rows, glyph cluster/bitmap/atlas evidence, legal records, and package counters. |
| `runtime-ui-ime-platform-parity-contracts-v1` | SDL3 and platform text input/IME publication lanes. | Existing SDL3 text input/session/event translation rows and committed text application keep SDL3 behind platform adapters. | Need package-visible IME host evidence, text input area/candidate evidence, per-platform non-inference diagnostics, and platform parity rows. |
| `runtime-ui-accessibility-publication-v1` | Accessibility payload publication and platform bridge evidence. | `plan_accessibility_publish` and `publish_accessibility_payload` validate first-party payloads before adapter dispatch. | Need OS bridge host evidence, role/name/state/focus/action/live-region counters, keyboard/focus pattern evidence, and per-platform host gates. |
| `runtime-ui-renderer-atlas-handoff-v1` | Glyph atlas/image atlas/renderer upload handoff. | UI atlas metadata/tooling and native UI atlas package metadata exist; runtime UI production stack has renderer handoff evidence rows only. | Need package-visible glyph atlas placement, budget/eviction, texture upload handoff, renderer-owned submission counters, and unsupported broad text/image rendering claims. |

## Baseline Evidence Map - `runtime-ui-text-stack-audit-v1`

| Category | Supported baseline | Evidence | Must remain unclaimed |
| --- | --- | --- | --- |
| Value-only retained UI workbench | Dense menu, inventory, equipment, shop, simulation dashboard, text-input intent, focus, localization identity, and accessibility identity rows with zero renderer/text/font/IME/accessibility/image/native adapter invocation counters. | `RuntimeUiWorkbenchDocument` / `plan_runtime_ui_workbench`; `sample_2d_desktop_runtime_package --require-runtime-ui-workbench`; `docs/current-capabilities.md` and `docs/ai-game-development.md` runtime UI workbench rows. | Rendering, shaping, rasterization, IME sessions, OS accessibility publication, image decoding, native platform work, UI middleware adoption, or broad production UI readiness. |
| Production stack evidence rows | Six first-party evidence rows for text shaping, font rasterization, glyph atlas, renderer submission, IME, and accessibility. The selected sample reports `host_evidence_required`, four ready rows, two host-gated rows, selected package counters, zero adapter/native/renderer-upload invocation counters, and replay hash evidence. | `RuntimeUiProductionEvidenceRow`, `RuntimeUiProductionStackRequest`, `RuntimeUiProductionStackPlan`, `plan_runtime_ui_production_stack`, `MK_runtime_ui_production_stack_tests`, and `sample_2d_desktop_runtime_package --require-runtime-ui-production-stack`. | Production text shaping, real font loading/rasterization, native IME parity, OS accessibility publication, renderer texture upload execution, UI middleware, or broad platform UI parity. |
| First-party adapter contracts | `MK_ui` exposes value-based `ITextShapingAdapter`, `IFontRasterizerAdapter`, `IImageDecodingAdapter`, `IImeAdapter`, `IAccessibilityAdapter`, `IPlatformIntegrationAdapter`, and `IClipboardTextAdapter` plus request/result planners that validate before adapter dispatch. | `engine/ui/include/mirakana/ui/ui.hpp`, `engine/ui/src/ui.cpp`, and focused coverage in `MK_ui_renderer_tests`. | Dependency types, OS/native handles, renderer/RHI handles, Dear ImGui/SDL3 types, UI middleware APIs, and game-local bypass systems in gameplay-facing UI APIs. |
| SDL3 platform adapter proof | SDL3 text input begin/end, text input area, text editing to first-party IME composition, committed text rows, and clipboard command bridges are behind `MK_platform_sdl3` adapters and tests. | `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`, `engine/platform/sdl3/src/sdl_clipboard.cpp`, and `MK_platform_sdl3_tests` coverage in `tests/unit/sdl3_platform_tests.cpp`. | Win32/TSF, Cocoa, ibus/Fcitx, Android, iOS, candidate UI parity, virtual keyboard behavior, or cross-platform IME parity inferred from SDL3 proof. |
| Renderer atlas metadata bridge | `UiRendererImagePalette` and `UiRendererGlyphAtlasPalette` can resolve already-cooked `GameEngine.UiAtlas.v1` image/glyph metadata into sprite commands and report missing/resolved glyph counters. | `build_ui_renderer_image_palette_from_runtime_ui_atlas`, `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas`, `make_ui_text_glyph_sprite_command`, `submit_ui_renderer_submission`, and `MK_ui_renderer_tests`. | Runtime font rasterization, live glyph atlas generation, runtime source image decoding, renderer texture upload as a gameplay API, broad atlas allocation/eviction, Metal overlay readiness, or production UI renderer quality. |
| Dependency-gated tool adapters | Optional host/tool/editor PNG decode and already-rasterized glyph/image packing paths exist for reviewed `asset-importers`/tooling lanes before cooked package consumption. | `docs/ai-game-development.md` runtime UI guidance and `MK_tools` UI atlas package helpers. | Arbitrary importers, broad codec/source import, SVG/vector parsing, runtime source parsing, live shader generation, or generated-game runtime dependency loading. |
| Durable non-claim surfaces | Docs, manifest fragments, and package manifests already describe runtime UI production stack evidence as selected package evidence only. | `docs/ui.md`, `engine/agent/manifest.fragments/006-runtimeBackendReadiness.json`, `engine/agent/manifest.fragments/009-validationRecipes.json`, and `games/sample_2d_desktop_runtime_package/game.agent.json`. | Treating value-only rows, SDL3 proof, cooked glyph metadata, or generated-package smokes as broad production text/font/IME/accessibility/platform UI parity. |

## Files

- Modify: `engine/ui/include/mirakana/ui/*.hpp`
- Modify: `engine/ui/src/*.cpp`
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/*.hpp`
- Modify: `engine/ui_renderer/src/*.cpp`
- Modify: `engine/platform/sdl3/**`
- Modify: `engine/runtime_host/sdl3/**`
- Modify: `engine/tools/**`
- Modify: `tests/unit/ui*_tests.cpp`
- Modify: `tests/unit/runtime_ui_*_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify: `tests/unit/runtime_host_sdl3_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `tools/bootstrap-deps.ps1`
- Modify: `vcpkg.json`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `tools/check-ai-integration*.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/*.json`
- Generate: `engine/agent/manifest.json`

## Task 1 - Baseline Runtime UI Evidence Audit

- [x] Read current `MK_ui`, `MK_ui_renderer`, SDL3 host, package sample, and runtime UI tests related to text shaping, font rasterization, IME, accessibility, image decoding, atlas metadata, and platform integration.
- [x] Add a short evidence table to this plan with supported rows, host-gated rows, dependency-gated rows, and unsupported broad UI parity claims.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_workbench_tests MK_runtime_ui_production_stack_tests MK_ui_renderer_tests MK_runtime_host_sdl3_tests sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_workbench_tests|MK_runtime_ui_production_stack_tests|MK_ui_renderer_tests|MK_runtime_host_sdl3_tests"
```

Expected: current baseline passes or records exact pre-existing tool/host blockers before implementation.

Validation evidence:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Passed; linked worktree, vcpkg junction, CMake 3.31.6, MSVC BuildTools, and clang-format 19.1.5 ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_workbench_tests MK_runtime_ui_production_stack_tests MK_ui_renderer_tests MK_runtime_host_sdl3_tests sample_2d_desktop_runtime_package` | Initial attempt failed because `out/build/dev` was absent in the fresh worktree; the plan now includes `tools/cmake.ps1 --preset dev` before build. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed; `out/build/dev` generated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_workbench_tests MK_runtime_ui_production_stack_tests MK_ui_renderer_tests MK_runtime_host_sdl3_tests sample_2d_desktop_runtime_package` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_workbench_tests\|MK_runtime_ui_production_stack_tests\|MK_ui_renderer_tests\|MK_runtime_host_sdl3_tests"` | Passed; 4/4 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `git diff --check` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | Passed with `validate: static ok`; Windows host keeps Metal/Apple gates diagnostic or host-gated. |

## Task 2 - RED Tests For Runtime UI Production Gate Expansion

- [ ] Add tests that fail until runtime UI production rows distinguish ready, host-gated, dependency-gated, adapter-invoked, skipped, and unsupported statuses.
- [ ] Add tests that reject broad text shaping readiness without glyph ids, clusters, advances/offsets, direction/script/language, fallback, bidi/line-break, and package evidence.
- [ ] Add tests that reject font rasterization readiness without glyph bitmap, metrics, pixel format, atlas placement, budget/eviction, renderer upload handoff, and package counters.
- [ ] Add tests that reject native IME or accessibility parity when only SDL3, WAI-ARIA, or Microsoft UI Automation evidence is present.
- [ ] Add tests that reject public native-handle leakage and middleware/platform token strings in gameplay-facing UI rows.
- [ ] Run the focused tests and record expected RED failures in this plan.

## Task 3 - Text Shaping And Font Rasterization Adapter Gates

- [ ] Extend first-party UI value rows with explicit evidence categories for shaping input segmentation, glyph output, cluster mapping, positioning, fallback, boundary analysis, raster bitmap, metrics, atlas placement, upload handoff, dependency gate, host gate, and unsupported claim.
- [ ] Implement fail-closed diagnostics for missing categories, duplicate row ids, invalid glyph/cluster mappings, invalid bitmap metrics, unsupported broad shaping/raster claims, backend inference, and native handle leakage.
- [ ] If HarfBuzz/FreeType/ICU-class adapters are selected, add optional dependency features, bootstrap path, legal/dependency notices, and adapter-private implementation without exposing dependency types in public UI APIs.
- [ ] Run focused UI, renderer, dependency/static, and package tests and record GREEN evidence.

## Task 4 - IME And Platform Text Input Publication Gates

- [ ] Extend first-party UI/host rows for text input session begin/end, composition update, candidate rows, committed text, text input area/cursor placement, clipboard, and platform dispatch evidence.
- [ ] Keep SDL3 as one adapter proof only; do not infer Win32/TSF, Cocoa, ibus/Fcitx, Android, or iOS readiness from SDL3 evidence.
- [ ] Add package-visible counters and diagnostics for selected SDL3 host proof and per-platform host gates.
- [ ] Run focused SDL3 runtime host and package tests and record GREEN evidence.

## Task 5 - Accessibility Publication And Keyboard/Focus Pattern Gates

- [ ] Extend first-party accessibility rows for role, accessible name, description, state, focus, action, relationship, live region, keyboard pattern, and publication status.
- [ ] Add host-gated platform bridge rows for Microsoft UI Automation and future Apple/Linux/mobile bridges without exposing native objects.
- [ ] Add package-visible counters and diagnostics for accessibility publication readiness and unsupported platform parity.
- [ ] Run focused UI/accessibility tests and package checks and record GREEN evidence.

## Task 6 - Renderer Atlas Handoff, Docs, Manifest, Static Checks, And Closeout

- [ ] Connect implemented glyph/image atlas evidence to renderer-owned upload/submission handoff counters without moving renderer/RHI handles into `MK_ui`.
- [ ] Update generated-game guidance, current capabilities, roadmap, backlog/projection chapters, manifest fragments, schemas/static checks, dependency/legal records, and plan registry.
- [ ] Compose the manifest.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

Expected: all checks pass, or exact host/dependency blockers are recorded.

## Done When

- Runtime UI production claims are backed by real first-party value rows, adapter/private dependency gates, package counters, and validation evidence for text shaping, font rasterization, IME, accessibility, and renderer atlas handoff.
- Broad platform UI parity remains unclaimed for missing platform, SDK, host, dependency, or renderer lanes.
- No public UI/gameplay API exposes SDL3, Dear ImGui, HarfBuzz, FreeType, ICU, platform accessibility, renderer/RHI, OS, or middleware handles.
- Docs, manifest fragments, schemas/static checks, generated-game guidance, dependency/legal records, and validation recipes match the implemented scope.
- A validated commit and reviewable PR exist for each independent candidate before moving to the next child plan.
