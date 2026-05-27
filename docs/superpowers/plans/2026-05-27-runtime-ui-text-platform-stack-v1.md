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
| `runtime-ui-shaping-raster-adapter-contracts-v1` | Clean-break shaping/raster adapter contracts plus deterministic package counters. | `plan_text_shaping_request`, `shape_text_run`, `plan_font_rasterization_request`, and `rasterize_font_glyph` now validate request/result rows with `TextShapingSegmentEvidence`, `TextShapedGlyph`, `TextBoundaryEvidence`, `TextFontFallbackEvidence`, `GlyphRasterBitmap`, `GlyphRasterMetrics`, and `FontRasterizationPixelFormat`. `plan_runtime_ui_production_stack` requires shaping direction/script/language, glyph ids/clusters/advances/offsets/fallback/bidi/line-break rows, glyph bitmap/metrics/pixel-format rows, atlas placement/budget/eviction, renderer upload handoff, dependency/host gates, unsupported-claim rows, and ready/host-gated/dependency-gated/skipped/adapter-invoked/unsupported counters before any adapter implementation is promoted. | Audited HarfBuzz/FreeType/ICU-class adapters were not selected in this candidate; real production shaping/rasterization execution, legal records, dependency bootstrap, and host/dependency validation remain gated before production shaping or rasterization readiness can be claimed. |
| `runtime-ui-ime-platform-parity-contracts-v1` | SDL3 and platform text input/IME publication lanes. | Runtime UI production rows now split IME evidence into session begin/end, composition update, candidate, text-area/cursor, committed-text, clipboard, selected SDL3 adapter proof, per-platform host-gate, and dispatch-boundary rows. The selected package reports these as explicit counters and keeps platform parity false. Existing SDL3 text input/session/event translation rows and committed text application stay behind platform adapters. | Win32/TSF, Cocoa, ibus/Fcitx, Android, and iOS host evidence remain gated; SDL3 proof is one adapter proof only and does not imply broad platform UI parity. |
| `runtime-ui-accessibility-publication-v1` | Accessibility payload publication and platform bridge evidence. | `plan_accessibility_publish` and `publish_accessibility_payload` validate first-party payloads before adapter dispatch. Runtime UI production rows now split accessibility evidence into role, accessible name, description, state, focus, action, relationship, live-region, keyboard-pattern, publication-status, Microsoft UI Automation host-gate, and per-platform host-gate rows. The selected package reports these as explicit counters and keeps platform parity false. | Concrete Microsoft UI Automation, NSAccessibility, AT-SPI, Android, and iOS host evidence remain gated; UIA proof is one host gate only and does not imply broad platform UI parity. |
| `runtime-ui-renderer-atlas-handoff-v1` | Glyph atlas/image atlas/renderer upload handoff. | `review_ui_renderer_atlas_handoff` now reviews cooked image/glyph atlas metadata, atlas placement/budget/eviction rows, texture handoff review rows, renderer submission counters, unresolved resource counters, unsupported broad-claim rows, side-effect flags, and replay evidence. `sample_2d_desktop_runtime_package --require-runtime-ui-renderer-atlas-handoff` ships `runtime/assets/2d/hud.uiatlas` and proves one image plus one glyph submission through `MK_ui_renderer`; package lane and full validation passed in the runtime UI text/platform stack worktree. | Commit, push, and PR evidence are blocked by local Git metadata write denial; production text shaping, font rasterization, source image decoding, live glyph atlas generation, renderer upload execution, native handles, and broad UI renderer quality remain unclaimed. |

## Baseline Evidence Map - `runtime-ui-text-stack-audit-v1`

| Category | Supported baseline | Evidence | Must remain unclaimed |
| --- | --- | --- | --- |
| Value-only retained UI workbench | Dense menu, inventory, equipment, shop, simulation dashboard, text-input intent, focus, localization identity, and accessibility identity rows with zero renderer/text/font/IME/accessibility/image/native adapter invocation counters. | `RuntimeUiWorkbenchDocument` / `plan_runtime_ui_workbench`; `sample_2d_desktop_runtime_package --require-runtime-ui-workbench`; `docs/current-capabilities.md` and `docs/ai-game-development.md` runtime UI workbench rows. | Rendering, shaping, rasterization, IME sessions, OS accessibility publication, image decoding, native platform work, UI middleware adoption, or broad production UI readiness. |
| Production stack evidence rows | Six first-party evidence rows for text shaping, font rasterization, glyph atlas, renderer submission, IME, and accessibility. The selected sample reports `host_evidence_required`, four ready rows, two host-gated rows, selected package counters, explicit IME SDL3 proof and per-platform host-gate counters, zero adapter/native/renderer-upload invocation counters, and replay hash evidence. | `RuntimeUiProductionEvidenceRow`, `RuntimeUiProductionStackRequest`, `RuntimeUiProductionStackPlan`, `plan_runtime_ui_production_stack`, `MK_runtime_ui_production_stack_tests`, `MK_sdl3_platform_tests`, `MK_runtime_host_sdl3_tests`, and `sample_2d_desktop_runtime_package --require-runtime-ui-production-stack`. | Production text shaping, real font loading/rasterization, native IME parity, OS accessibility publication, renderer texture upload execution, UI middleware, or broad platform UI parity. |
| First-party adapter contracts | `MK_ui` exposes value-based `ITextShapingAdapter`, `IFontRasterizerAdapter`, `IImageDecodingAdapter`, `IImeAdapter`, `IAccessibilityAdapter`, `IPlatformIntegrationAdapter`, and `IClipboardTextAdapter` plus request/result planners that validate before adapter dispatch. Text shaping results now require segment direction/script/language, glyph id/cluster/advance/offset, fallback, grapheme/line/bidi boundary evidence; font rasterization results now require glyph bitmap, metrics, pixel format, and atlas allocation evidence. | `engine/ui/include/mirakana/ui/ui.hpp`, `engine/ui/src/ui.cpp`, and focused coverage in `MK_ui_renderer_tests`. | Dependency types, OS/native handles, renderer/RHI handles, Dear ImGui/SDL3 types, UI middleware APIs, and game-local bypass systems in gameplay-facing UI APIs. |
| SDL3 platform adapter proof | SDL3 text input begin/end, text input area, text editing to first-party IME composition, committed text rows, and clipboard command bridges are behind `MK_platform_sdl3` adapters and tests. | `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`, `engine/platform/sdl3/src/sdl_clipboard.cpp`, and `MK_platform_sdl3_tests` coverage in `tests/unit/sdl3_platform_tests.cpp`. | Win32/TSF, Cocoa, ibus/Fcitx, Android, iOS, candidate UI parity, virtual keyboard behavior, or cross-platform IME parity inferred from SDL3 proof. |
| Renderer atlas metadata bridge | `UiRendererImagePalette` and `UiRendererGlyphAtlasPalette` can resolve already-cooked `GameEngine.UiAtlas.v1` image/glyph metadata into sprite commands, and `review_ui_renderer_atlas_handoff` can fail closed over cooked metadata, placement/budget/eviction, texture handoff review, renderer submission counters, unsupported broad claims, and side-effect flags. | `build_ui_renderer_image_palette_from_runtime_ui_atlas`, `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas`, `make_ui_text_glyph_sprite_command`, `submit_ui_renderer_submission`, `review_ui_renderer_atlas_handoff`, `MK_ui_renderer_tests`, and `sample_2d_desktop_runtime_package --require-runtime-ui-renderer-atlas-handoff`. | Runtime font rasterization, live glyph atlas generation, runtime source image decoding, renderer texture upload as a gameplay API, broad atlas allocation/eviction execution, Metal overlay readiness, or production UI renderer quality. |
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

- [x] Add tests that fail until runtime UI production rows distinguish ready, host-gated, dependency-gated, adapter-invoked, skipped, and unsupported statuses.
- [x] Add tests that reject broad text shaping readiness without glyph ids, clusters, advances/offsets, direction/script/language, fallback, bidi/line-break, and package evidence.
- [x] Add tests that reject font rasterization readiness without glyph bitmap, metrics, pixel format, atlas placement, budget/eviction, renderer upload handoff, and package counters.
- [x] Add tests that reject native IME or accessibility parity when only SDL3, WAI-ARIA, or Microsoft UI Automation evidence is present.
- [x] Add tests that reject public native-handle leakage and middleware/platform token strings in gameplay-facing UI rows.
- [x] Run the focused tests and record expected RED failures in this plan.

Validation evidence for `runtime-ui-shaping-raster-adapter-contracts-v1`:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev; pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests` | RED failed as expected before implementation: missing `RuntimeUiProductionProofKind::skipped`, `RuntimeUiProductionStackStatus::dependency_evidence_required`, and package row counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests; pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_production_stack_tests"` | GREEN passed for runtime UI production stack row taxonomy tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests sample_2d_desktop_runtime_package` | Passed; rebuilt the production stack tests plus the package smoke executable exposing dependency-gated/skipped/adapter-invoked/unsupported row counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "(MK_runtime_ui_production_stack_tests|MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests|MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests|MK_runtime_package_hot_reload_replacement_intent_review_tests)"` | Passed 4/4; also verified compact CMake target names keep long descriptive CTest names runnable after the MSVC object-path fix. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed after shortening compile-PDB output layout and compacting five long single-source test target names; this resolves the linked-worktree MSVC C1041/C1083 path failures seen during full validation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check` | Passed after docs, manifest fragments, CMake guidance, and static guards were updated for the runtime UI counters and MSVC path hygiene. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed with `validate: ok`; 19 static checks, build, generated MSVC C++23 mode check, tidy smoke, and 93/93 CTest tests passed. Metal/Apple diagnostics remain expected host-gated rows on Windows. |

## Task 3 - Text Shaping And Font Rasterization Adapter Gates

- [x] Extend first-party UI value rows with explicit evidence categories for shaping input segmentation, glyph output, cluster mapping, positioning, fallback, boundary analysis, raster bitmap, metrics, atlas placement, upload handoff, dependency gate, host gate, and unsupported claim.
- [x] Implement fail-closed diagnostics for missing categories, duplicate row ids, invalid glyph/cluster mappings, invalid UTF-8 scalar boundaries, invalid bitmap metrics, unsupported broad shaping/raster claims, backend inference, and native handle leakage.
- [x] If HarfBuzz/FreeType/ICU-class adapters are selected, add optional dependency features, bootstrap path, legal/dependency notices, and adapter-private implementation without exposing dependency types in public UI APIs.
- [x] Run focused UI, renderer, dependency/static, and package tests and record GREEN evidence.

Validation evidence for `runtime-ui-shaping-raster-adapter-contracts-v1`:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests` followed by `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests"` | RED failed as expected before implementation: newly added shaping/raster adapter result tests rejected current broad success when glyph/boundary and bitmap/metrics evidence was missing. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests MK_runtime_ui_production_stack_tests sample_2d_desktop_runtime_package`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests\|MK_runtime_ui_production_stack_tests\|sample_2d_desktop_runtime_package"` | Passed 5/5 after adding `TextShapingSegmentEvidence`, `TextShapedGlyph`, `TextBoundaryEvidence`, `TextFontFallbackEvidence`, `GlyphRasterBitmap`, `GlyphRasterMetrics`, and production-stack direction/script/language plus pixel-format gates. No HarfBuzz/FreeType/ICU-class adapter was selected in this candidate; dependency/legal/bootstrap work remains gated before real production shaping or rasterization implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests MK_runtime_ui_production_stack_tests sample_2d_desktop_runtime_package`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests\|MK_runtime_ui_production_stack_tests\|sample_2d_desktop_runtime_package"` | Passed 5/5 after review fixes: zero-advance shaping glyphs and zero-ink raster glyphs remain valid evidence, text shaping byte offsets must land on strict UTF-8 scalar boundaries, and `ILineBreakingAdapter` now returns separate `TextLineBreakRun` rows instead of requiring shaping glyph evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`; `git diff --check` | Passed after public UI API/docs/manifest/static guard sync for `TextLineBreakRun`, scalar-boundary shaping evidence, and zero-ink raster evidence. |
| `out/build/dev/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-production-stack` | Passed; selected package evidence reported `runtime_ui_production_stack_status=host_evidence_required`, `ready=1`, six rows, four ready rows, two host-gated rows, zero adapter/native/renderer-upload invocation counters, zero diagnostics, and positive replay hash. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed with `validate: ok`; 19 static checks, build, generated MSVC C++23 mode check, tidy smoke, and 93/93 CTest tests passed. Metal/Apple diagnostics remain expected host-gated rows on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` | Passed; release package build, 3/3 release CTest smokes, installed desktop runtime validation, ZIP creation, and SHA-256 generation succeeded. |

## Task 4 - IME And Platform Text Input Publication Gates

- [x] Extend first-party UI/host rows for text input session begin/end, composition update, candidate rows, committed text, text input area/cursor placement, clipboard, and platform dispatch evidence.
- [x] Keep SDL3 as one adapter proof only; do not infer Win32/TSF, Cocoa, ibus/Fcitx, Android, or iOS readiness from SDL3 evidence.
- [x] Add package-visible counters and diagnostics for selected SDL3 host proof and per-platform host gates.
- [x] Run focused SDL3 runtime host and package tests and record GREEN evidence.

Validation evidence for `runtime-ui-ime-platform-parity-contracts-v1`:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests` | RED failed as expected before implementation: missing `ime_session_begin_end_rows`, `ime_composition_update_rows`, `ime_text_area_cursor_rows`, `ime_clipboard_rows`, `ime_sdl3_adapter_proof_rows`, `ime_platform_host_gate_rows`, and matching diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests sample_2d_desktop_runtime_package` | Passed after adding split IME evidence rows and package-visible counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_production_stack_tests\|sample_2d_desktop_runtime_package"` | Passed 4/4 for production-stack tests and package smokes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_sdl3_platform_tests MK_runtime_host_sdl3_tests`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_sdl3_platform_tests\|MK_runtime_host_sdl3_tests\|MK_runtime_ui_production_stack_tests\|sample_2d_desktop_runtime_package"` | Passed 6/6; SDL3 text input area/start/stop, text editing composition, candidate rows, committed text, text edit commands, and clipboard bridges remain selected SDL3 adapter proof only. |
| `out/build/dev/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-production-stack` | Passed; selected package evidence reported explicit IME counters including `runtime_ui_production_stack_ime_session_rows=1`, `runtime_ui_production_stack_ime_composition_rows=1`, `runtime_ui_production_stack_ime_candidate_rows=1`, `runtime_ui_production_stack_ime_text_area_cursor_rows=1`, `runtime_ui_production_stack_ime_committed_text_rows=1`, `runtime_ui_production_stack_ime_clipboard_rows=1`, `runtime_ui_production_stack_ime_sdl3_adapter_proof_rows=1`, `runtime_ui_production_stack_ime_platform_host_gate_rows=1`, `runtime_ui_production_stack_ime_platform_parity_ready=0`, zero invocation counters, zero diagnostics, and a positive replay hash. |

## Task 5 - Accessibility Publication And Keyboard/Focus Pattern Gates

- [x] Extend first-party accessibility rows for role, accessible name, description, state, focus, action, relationship, live region, keyboard pattern, and publication status.
- [x] Add host-gated platform bridge rows for Microsoft UI Automation and future Apple/Linux/mobile bridges without exposing native objects.
- [x] Add package-visible counters and diagnostics for accessibility publication readiness and unsupported platform parity.
- [x] Run focused UI/accessibility tests and package checks and record GREEN evidence.

Validation evidence for `runtime-ui-accessibility-publication-v1`:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests` | RED failed as expected before implementation: missing `accessibility_name_rows`, `accessibility_description_rows`, `accessibility_keyboard_pattern_rows`, `accessibility_publication_status_rows`, `accessibility_uia_host_gate_rows`, `accessibility_platform_host_gate_rows`, and matching diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests sample_2d_desktop_runtime_package` | Passed after adding split accessibility publication evidence rows and package-visible counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_production_stack_tests\|sample_2d_desktop_runtime_package"` | Passed 4/4 for production-stack tests and package smokes. |
| `sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-production-stack` | Passed from the built package directory; selected package evidence reported explicit accessibility counters including `runtime_ui_production_stack_accessibility_role_rows=1`, `runtime_ui_production_stack_accessibility_name_rows=1`, `runtime_ui_production_stack_accessibility_description_rows=1`, `runtime_ui_production_stack_accessibility_keyboard_pattern_rows=1`, `runtime_ui_production_stack_accessibility_publication_status_rows=1`, `runtime_ui_production_stack_accessibility_uia_host_gate_rows=1`, `runtime_ui_production_stack_accessibility_platform_host_gate_rows=1`, `runtime_ui_production_stack_accessibility_platform_parity_ready=0`, zero invocation counters, zero diagnostics, and a positive replay hash. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` | Passed; release package build, 3/3 release CTest smokes, installed desktop runtime validation, ZIP creation, and SHA-256 generation succeeded with the split accessibility counters. |

## Task 6 - Renderer Atlas Handoff, Docs, Manifest, Static Checks, And Closeout

- [x] Connect implemented glyph/image atlas evidence to renderer-owned upload/submission handoff counters without moving renderer/RHI handles into `MK_ui`.
- [x] Update generated-game guidance, current capabilities, roadmap, manifest fragments, schemas/static checks, and selected package manifest/docs for the renderer atlas handoff candidate.
- [x] Compose the manifest.
- [x] Run:

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

Validation evidence for `runtime-ui-renderer-atlas-handoff-v1`:

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests sample_2d_desktop_runtime_package` | Passed; rebuilt `MK_ui_renderer` tests and the selected 2D package smoke executable after formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests\|sample_2d_desktop_runtime_package"` | Passed 4/4: `MK_ui_renderer_tests`, package smoke, shader artifacts smoke, and Vulkan shader artifacts smoke. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; regenerated `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`; `git diff --check` | Passed after `tools/format.ps1` normalized the public UI renderer header. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed with `validate: ok`; 19 static checks, build, generated MSVC C++23 mode check, tidy smoke, and 93/93 CTest tests passed. Metal/Apple rows remain diagnostic-only or host-gated on Windows. |

Publication evidence:

| Command | Result |
| --- | --- |
| `git -c safe.directory=G:/workspace/development/GameEngine/.worktrees/runtime-ui-shaping-raster-evidence-gates-v1 add ...` | Blocked: `fatal: Unable to create 'G:/workspace/development/GameEngine/.git/worktrees/runtime-ui-shaping-raster-evidence-gates-v1/index.lock': Permission denied`. |
| `gh pr view 268 --json number,title,headRefName,headRefOid,baseRefName,state,isDraft,url,statusCheckRollup` | Blocked: GitHub CLI could not read `C:\Users\y2ikg\AppData\Roaming\GitHub CLI\config.yml` because access was denied. |

## Done When

- Runtime UI production claims are backed by real first-party value rows, adapter/private dependency gates, package counters, and validation evidence for text shaping, font rasterization, IME, accessibility, and renderer atlas handoff.
- Broad platform UI parity remains unclaimed for missing platform, SDK, host, dependency, or renderer lanes.
- No public UI/gameplay API exposes SDL3, Dear ImGui, HarfBuzz, FreeType, ICU, platform accessibility, renderer/RHI, OS, or middleware handles.
- Docs, manifest fragments, schemas/static checks, generated-game guidance, dependency/legal records, and validation recipes match the implemented scope.
- A validated commit and reviewable PR exist for each independent candidate before moving to the next child plan.
