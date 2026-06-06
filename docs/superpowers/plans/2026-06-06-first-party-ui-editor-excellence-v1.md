# First-Party UI Editor Excellence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `first-party-ui-editor-excellence-v1`

**Status:** Active. Phase 0 selected this milestone in `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` while preserving `unsupportedProductionGaps = []` and leaving every implementation claim unready until its exact phase gate passes.

**Goal:** Promote the first-party retained `MK_editor` / `mirakana::ui` stack into a production-grade, AI-operable, high-performance editor and UI platform covering broad optimization, multi-window docking, complete editable rich text, Vulkan/Metal editor texture parity, cross-platform editor shells, and broad text/font/IME/accessibility parity.

**Architecture:** Keep public game/runtime UI on first-party `mirakana::ui` contracts and keep durable editor behavior in GUI-independent `MK_editor_core` models before visible shell wiring. Keep Win32/D3D12/Vulkan/Metal/Core Text/TSF/UIA/AppKit/Linux/Android/iOS details in private platform or backend adapters, never in public engine/game APIs. Promote each claim only through exact tests, package or editor smoke counters, host-gated validation recipes, docs, manifest fragments, and static checks.

**Tech Stack:** C++23, PowerShell 7 repository tools, CMake presets, `MK_ui`, `MK_ui_renderer`, `MK_editor_core`, `MK_editor_shell_common`, `MK_editor`, private Win32/D3D12/DirectWrite/TSF/UIA adapters, private Vulkan 1.3 synchronization2 editor texture display, Apple-host-gated Metal/AppKit/Core Text/InputMethodKit/NSAccessibility adapters, Linux AT-SPI2/IBus/Fcitx host-gated adapters, Android/iOS text/accessibility host-gated adapters, audited HarfBuzz/FreeType/ICU-class dependencies only after license and dependency records, no Dear ImGui, no SDL3, no UI middleware.

---

## Current Audit Verdict

Audit sources used on 2026-06-06:

- `docs/current-capabilities.md`
- `docs/editor.md`
- `docs/ui.md`
- `docs/superpowers/plans/README.md`
- `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- `engine/agent/manifest.fragments/005-applications.json`
- `engine/agent/manifest.fragments/004-modules.json`
- Targeted `rg` scans over `editor`, `engine/ui`, `engine/ui_renderer`, `engine/platform`, `engine/rhi`, `engine/agent`, `tools`, `CMakeLists.txt`, and `CMakePresets.json`
- Current manifest summary: `unsupportedProductionGaps = 0`, `validationRecipes = 69`, `modules = 32`, `currentActivePlan = docs/superpowers/plans/2026-06-06-first-party-ui-editor-excellence-v1.md`, `recommendedNextPlan.id = first-party-ui-editor-excellence-v1`

Current proven state:

- `MK_editor` is a dependency-free first-party Windows editor shell through the `desktop-editor` lane.
- The active visible shell reports `editor_shell_ui=first_party`, `editor_shell_imgui=0`, `editor_shell_sdl3=0`, `editor_shell_backend=d3d12`, and eleven native panels.
- The shell renders single-window dock tabs/gutters, active/focused panel state, hidden-tab disabled commands, keyboard focus traversal, Console diagnostics, AI Commands rows, Inspector rows, private D3D12 Viewport texture display, and private D3D12 Material Preview display.
- `MK_editor_core` owns `EditorDockLayout`, `EditorDockCommandPlan`, workspace v2 dock persistence, `EditorRichText*` rows, AI operation snapshot/catalog/dry-run/apply surfaces, and reviewed dock show/hide/activate/move/split/reset commands.
- Windows DirectWrite, TSF, and UIA evidence exists as private editor-shell adapter evidence; it does not promote cross-platform text/font/IME/accessibility parity.
- `mirakana::ui` owns deterministic document, style, layout, focus/navigation, text/image/accessibility payload, text edit, clipboard, shaping request, font rasterization request, image decode request, and renderer submission contracts.
- `MK_ui_renderer` converts first-party UI renderer payloads into renderer sprite submissions and supports deterministic glyph/image atlas palette rows.
- AI operation is value-first: `EditorAiOperationSnapshot.status_rows` includes dock, rich text, text input, adapter, IME, accessibility, viewport, and material preview status rows; the command catalog rejects native handles, shell/process execution, validation recipe execution, screen coordinates, stale revisions, unsupported parameters, disabled commands, and unknown command ids.

Unclaimed by design:

- Broad UI/editor optimization and measured low-latency readiness.
- Multi-window drag, tear-off, docking across windows, monitor/DPI-aware workspace persistence, and window lifecycle restoration.
- Complete editable rich text beyond focused Project Settings plain-text editing and read-only Console/AI Commands/Inspector rich text.
- Production text shaping, bidi, line breaking, font fallback, real font loading/rasterization, glyph atlas upload, and broad runtime/editor text quality.
- Custom native IME candidate UI rendering, non-Windows IME execution, rich clipboard, full platform shortcut localization, and full widget-level text editing.
- Full UIA control pattern/event parity, cross-platform accessibility parity, and runtime UI OS accessibility publication.
- Vulkan editor texture display parity and Metal editor texture display parity.
- Cross-platform visible editor shells for macOS, Linux, Android, or iOS.
- Public native handle exposure, UI middleware, Dear ImGui, SDL3, Qt, Slint, RmlUi, or compatibility shims.

## Official Source Baseline

Re-open these official sources before executing the phase that uses them. Use them as design constraints, not copied implementation code.

Windows text, IME, and accessibility:

- Microsoft DirectWrite and Direct2D text rendering: <https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-and-directwrite>
- Microsoft DirectWrite portal: <https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal>
- Microsoft Text Services Framework: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>
- Microsoft TSF Thread Manager: <https://learn.microsoft.com/en-us/windows/win32/tsf/thread-manager>
- Microsoft TSF API reference: <https://learn.microsoft.com/en-us/windows/win32/api/_tsf/>
- Microsoft UI Automation overview: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-uiautomationoverview>
- Microsoft UI Automation fundamentals: <https://learn.microsoft.com/en-us/windows/win32/winauto/entry-uiautocore-overview>
- Microsoft UI Automation control patterns overview: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controlpatternsoverview>
- Microsoft UI Automation control pattern identifiers: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-controlpattern-ids>
- Microsoft UI Automation properties overview: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-propertiesoverview>
- Microsoft UI Automation events overview: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-eventsoverview>
- Microsoft UI Automation for automated testing: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-usefortesting>

Vulkan and Metal editor texture display:

- Khronos Vulkan synchronization: <https://docs.vulkan.org/spec/latest/chapters/synchronization.html>
- Khronos Vulkan synchronization2 guide: <https://docs.vulkan.org/guide/latest/extensions/VK_KHR_synchronization2.html>
- Khronos Vulkan dynamic rendering tutorial: <https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/02_Graphics_pipeline_basics/03_Render_passes.html>
- Khronos Vulkan validation layers: <https://docs.vulkan.org/guide/latest/layers.html>
- Apple Metal render passes: <https://developer.apple.com/documentation/metal/render-passes>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple Metal resource objects, buffers, and textures: <https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Mem-Obj/Mem-Obj.html>
- Apple Metal Feature Set Tables: <https://developer.apple.com/metal/capabilities/>

Cross-platform text, font, IME, and accessibility:

- Apple Core Text: <https://developer.apple.com/documentation/coretext>
- Apple `NSTextInputClient`: <https://developer.apple.com/documentation/appkit/nstextinputclient>
- Apple InputMethodKit: <https://developer.apple.com/documentation/inputmethodkit>
- Apple Accessibility for AppKit: <https://developer.apple.com/documentation/appkit/accessibility-for-appkit>
- Apple `NSAccessibility`: <https://developer.apple.com/documentation/appkit/accessibility_for_appkit/nsaccessibility>
- GNOME AT-SPI2 core developer docs: <https://gnome.pages.gitlab.gnome.org/at-spi2-core/devel-docs/>
- WAI-ARIA Authoring Practices Guide: <https://www.w3.org/WAI/ARIA/apg/>
- freedesktop AT-SPI2: <https://www.freedesktop.org/wiki/Accessibility/AT-SPI2/>
- Fcitx 5: <https://fcitx-im.org/wiki/Fcitx_5/en>
- ICU Boundary Analysis: <https://unicode-org.github.io/icu/userguide/boundaryanalysis/>
- ICU Break Rules: <https://unicode-org.github.io/icu/userguide/boundaryanalysis/break-rules.html>
- HarfBuzz documentation: <https://harfbuzz.github.io/>
- FreeType documentation: <https://freetype.org/freetype2/docs/documentation.html>

Build and validation:

- CMake custom commands and generated files: <https://cmake.org/cmake/help/latest/guide/tutorial/Custom%20Commands%20and%20Generated%20Files.html>
- CMake `add_custom_command`: <https://cmake.org/cmake/help/latest/command/add_custom_command.html>

Context7 checks performed on 2026-06-06:

- `/khronosgroup/vulkan-docs` confirmed Vulkan synchronization2 barrier patterns for color/depth attachment writes to shader-read layouts, transfer writes to shader-read layouts, queue ownership rules, and command submission ordering evidence.
- `/harfbuzz/harfbuzz` confirmed the shaping model: UTF-8 buffer input, direction/script/language segment properties, `hb_shape`, glyph ids, clusters, advances, and offsets.
- `/freetype/freetype` confirmed the font model: library/face creation, face properties, charmap glyph index lookup, glyph metrics, glyph rasterization, bitmap pixel formats, and bitmap conversion.

Source implications for this repository:

- Official SDK behavior determines adapter boundaries; public game/editor/runtime UI APIs expose first-party rows and never native handles.
- Windows DirectWrite/TSF/UIA evidence must not be generalized to macOS, Linux, Android, iOS, HarfBuzz, FreeType, or ICU readiness.
- Vulkan editor texture parity requires strict Vulkan runtime/toolchain readiness, SPIR-V artifacts, validation layers, synchronization2 layout transitions, descriptor updates, render pass or dynamic rendering evidence, and readback or visible compositor evidence.
- Metal editor texture parity requires Apple-host evidence over command queues, render passes, texture sampling, metallib/pipeline readiness, synchronization, feature availability, and visible presentation; Windows rows cannot promote Metal.
- HarfBuzz, FreeType, ICU, Fcitx/IBus-class, AT-SPI2, font packages, or other third-party dependencies require `license-audit`, `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` updates before use.
- Multi-window and cross-platform shells must be private adapter work over the same `MK_editor_core` and `mirakana::ui` contracts, not new editor-private public APIs.

## Resolved Planning Decisions

- This is one milestone plan with PR-sized phases. It is intentionally broader than one PR because the phases share editor/UI architecture, validation, and agent-surface contracts.
- Phase 0 selects this plan. Selection changes only the active milestone pointer and does not implement or promote broad editor/UI excellence claims.
- `MK_editor_core` is the owner for persistent behavior, AI-operable rows, text document state, dock/window layout state, and reviewed commands.
- `MK_editor_shell_common` and `editor/src` own visible-shell composition and private platform/backend adapters.
- Runtime game UI stays on public `mirakana::ui`; generated games must not depend on editor-private APIs.
- Broad optimization starts with measured editor/UI budgets and counters, then promotes targeted optimizations only when before/after evidence exists.
- Multi-window docking requires a new clean-break workspace format version; do not mutate `GameEngine.Workspace.v2` semantics.
- Complete editable rich text starts with editor-core document/selection/command semantics, then adapter text shaping/font/IME/accessibility proof, then visible shell controls.
- Vulkan and Metal editor texture display parity are backend-local claims; D3D12 proof does not promote them.
- Cross-platform shell readiness is per-host and per-adapter. One platform cannot promote another.
- AI operation must remain dry-run-before-apply, row-based, revision-checked, and fail-closed. It must not use screen scraping, coordinates, arbitrary shell, raw manifest evaluation, native handles, or validation execution from editor core.
- Dear ImGui, SDL3, UI middleware, public native handles, compatibility aliases, and migration shims are forbidden.

## Target Claim Matrix

| Claim | Current state | Promotion target |
| --- | --- | --- |
| `editor_ui_performance_budget_status` | Unclaimed | Ready editor/UI budget rows for layout, text, render submission, texture compositor, memory, frame time, and no broad optimization overclaim. |
| Retained UI diff/caching | Partial first-party retained model | Dirty-region, layout-cache, text-cache, glyph-cache, renderer-submission cache, and deterministic invalidation counters. |
| Multi-window docking | Single-window ready | `GameEngine.Workspace.v3` with window ids, monitor/DPI rows, tear-off/dock/merge commands, persistence, and AI rows. |
| Complete editable rich text | Partial | Structured mutable rich text document, selection, editing commands, undo/redo, clipboard, IME, accessibility, AI commands, and visible shell editing. |
| Broad text shaping/font fallback | Adapter boundary only | HarfBuzz/FreeType/ICU or official SDK-backed rows with script/language/direction, glyph clusters, fallback runs, atlas handoff, and licensing gates. |
| Windows IME parity | Selected TSF status | Full TSF session, composition, candidate, reconversion, `ITextStoreACP` callback, caret/surrounding text, and fail-closed diagnostics. |
| Windows accessibility parity | UIA provider ready | UIA patterns/events for editor controls, text patterns, stable AutomationId/RuntimeId strategy, and Inspect/tool evidence. |
| Vulkan editor texture parity | Unclaimed | Private Vulkan texture display adapter/compositor with synchronization2, descriptors, SPIR-V, validation layers, visible composites, and no native handle exposure. |
| Metal editor texture parity | Host-gated | Apple-host private Metal texture display adapter/compositor with render pass, texture sampling, metallib/pipeline, visible presentation, and no native handle exposure. |
| Cross-platform editor shell | Windows only | Host-gated macOS and Linux visible shell adapters over the same `MK_editor_core` / `mirakana::ui` contracts; Android/iOS editor shells remain explicit non-goals unless separately selected. |
| AI editor operation excellence | Narrow rows/commands | AI-operable window/dock/text/backend/readiness commands with structured dry-run/apply, evidence rows, and no screen automation. |
| Broad `first_party_editor_excellence` | Unclaimed | Aggregate only after selected exact rows pass; backend parity and cross-platform parity remain false unless proven. |

## File Responsibility Map

Expected files for implementation phases:

| Area | Files |
| --- | --- |
| Editor core dock/window/text/AI models | `editor/core/include/mirakana/editor/editor_dock_layout.hpp`, `editor/core/include/mirakana/editor/editor_rich_text.hpp`, `editor/core/include/mirakana/editor/ai_operation_surface.hpp`, `editor/core/include/mirakana/editor/workspace.hpp`, matching `editor/core/src/*.cpp`, `tests/unit/editor_core_tests.cpp` |
| First-party shell document | `editor/src/first_party_editor_document.hpp`, `editor/src/first_party_editor_document.cpp`, `editor/src/native_editor_app.hpp`, `editor/src/native_editor_app.cpp`, `tests/unit/editor_native_shell_tests.cpp` |
| Native Windows adapters | `editor/src/native_editor_text_font_adapters.*`, `editor/src/native_editor_text_atlas_handoff.*`, `editor/src/native_editor_tsf_text_input.*`, `editor/src/native_editor_uia_provider.*`, `editor/src/native_texture_display_adapter.*`, `editor/src/native_editor_visible_texture_compositor.*` |
| Vulkan/Metal editor texture adapters | `editor/src/native_texture_display_adapter.*`, new private `editor/src/vulkan_editor_texture_display.*` only if the existing adapter cannot stay backend-neutral, new private `editor/src/metal_editor_texture_display.*` or `editor/src/metal_editor_texture_display.mm` only when Apple-host work is selected |
| Runtime UI contracts and renderer adapter | `engine/ui/include/mirakana/ui/*.hpp`, `engine/ui/src/*.cpp`, `engine/ui_renderer/include/mirakana/ui_renderer/*.hpp`, `engine/ui_renderer/src/*.cpp`, `tests/unit/ui_*_tests.cpp`, `tests/unit/ui_renderer_tests.cpp` |
| Platform adapters | `engine/platform/win32/**`, future host-gated `engine/platform/macos/**`, `engine/platform/linux/**`, `engine/platform/android/**`, `engine/platform/ios/**` only after phase selection and dependency/legal gates |
| RHI/backend evidence | `engine/rhi/include/mirakana/rhi/rhi.hpp`, `engine/rhi/vulkan/**`, `engine/rhi/metal/**`, `tests/unit/backend_scaffold_tests.cpp`, `tests/unit/d3d12_rhi_tests.cpp` |
| Tooling and validation recipes | `tools/build-editor.ps1`, `tools/evaluate-cpp23.ps1`, `tools/run-validation-recipe-plans.ps1`, `tools/validation-recipe-core.ps1`, `tools/check-ai-integration-060-editor-workflows.ps1`, `tools/check-json-contracts-040-agent-surfaces.ps1` |
| Docs, manifest, static checks | `docs/current-capabilities.md`, `docs/editor.md`, `docs/ui.md`, `docs/roadmap.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.fragments/*.json`, composed `engine/agent/manifest.json` |
| Dependency and legal gates | `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `tools/check-dependency-policy.ps1` |

## Execution Strategy For Maximum Speed And Quality

- Use one isolated worktree per PR-sized phase. Run `tools/prepare-worktree.ps1` before CMake configure in linked worktrees.
- Keep Phase 0 selection docs-only. Do not mix selection with implementation.
- Start every implementation phase with focused RED tests, implement the smallest working slice, then batch docs/manifest/static-check sync after behavior is green.
- Use subagents for independent read-only audits and backend-specific investigations. Do not run parallel writers against the same docs, manifest fragments, or static-check files.
- Prefer D3D12/Windows focused loops for shared UI/editor behavior, then backend-local Vulkan/Metal host loops for parity.
- Use Context7 and official docs refresh at the start of each phase that touches SDK/library behavior.
- Run `tools/build-editor.ps1` for editor-shell phases and `tools/validate.ps1` once at each C++/runtime/build/packaging/public-contract phase close.
- Use `tools/check-publication-preflight.ps1` before staging, push, PR creation, ready conversion, auto-merge, or cleanup.
- Keep every phase mergeable on its own; do not leave partial broad claims for a later phase.

## Highest-Level Development Environment Configuration

Use this environment contract before implementing Phase 1 or later. It keeps the work fast by making host/toolchain blockers explicit instead of letting broad claims drift.

- Windows is the primary local proof lane for shared editor/UI behavior: PowerShell 7 repository tools, CMake `dev` preset, MSVC, Windows SDK, Direct3D 12 debug tooling where available, DirectWrite, TSF, and UIA provider tests.
- Vulkan is a separate backend-local proof lane: require Vulkan runtime, Vulkan SDK/toolchain discovery, Vulkan 1.3 synchronization2 support, validation layers, SPIR-V shader artifacts, and backend-local editor texture display counters before any Vulkan editor texture ready claim.
- Metal is an Apple-host proof lane: require Apple host evidence, Metal feature availability, metallib/pipeline execution, render pass evidence, texture sampling evidence, and visible compositor/presentation counters before any Metal editor texture ready claim.
- Cross-platform shells are per-host lanes: macOS AppKit/Core Text/`NSTextInputClient`/`NSAccessibilityProtocol` and Linux X11 or Wayland/AT-SPI2/IBus/Fcitx evidence must remain `host_gated` until executed on the matching host.
- Text/font dependencies are dependency-gated lanes: HarfBuzz, FreeType, ICU, font packages, and any additional shaping/rasterization/font data dependency require license audit, manifest feature gates, dependency docs, legal notices, and validation before integration.
- Official documentation and Context7 refresh is mandatory at phase start for every SDK/library touched. Record the source family, date, and relevant adapter implication in phase evidence; do not rely on memory for Vulkan, Metal, DirectWrite, TSF, UIA, HarfBuzz, FreeType, ICU, CMake, IBus/Fcitx, or AT-SPI behavior.
- Performance work must be evidence-first: add budget rows, capture before/after counters, preserve deterministic tests, and keep `*_broad_optimization_claimed=0` until the exact aggregate gate defines and proves the claim.
- AI operation must be row-first and revision-checked: every mutating command requires `expected_revision`, dry-run evidence, apply evidence, deterministic rejection rows, and no access to native handles, screen coordinates, arbitrary shell, package scripts, or validation recipe execution.
- Missing SDKs, host tools, runtime support, validation layers, Apple hosts, Linux desktop services, or accessibility/IME test tools are recorded as exact `host_gated` blockers with the failing command. They are never converted into inferred ready claims.
- Full repository validation is reserved for implementation slice closeout; use focused tests and `tools/build-editor.ps1` during iteration, then run `tools/validate.ps1` once after code, docs, manifest, static checks, and agent-surface drift are stable.

## Phase 0: Select The Milestone Without Implementing Claims

**Goal:** Make this plan the selected editor/UI excellence milestone while preserving `unsupportedProductionGaps = []`.

**Files:**

- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate `engine/agent/manifest.json`
- Modify `docs/superpowers/plans/README.md`
- Modify `docs/current-capabilities.md` and `docs/roadmap.md` only for selected-plan context
- Modify static checks only if new selected-plan literals must be enforced

- [x] Confirm current truth:

```powershell
git status --short --branch
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
```

Expected: clean task-owned tree, `currentActivePlan` points to the production-completion master selection gate, and `recommendedNextPlan.id = next-production-gap-selection`.

- [x] Select this plan in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`.
- [x] Preserve `unsupportedProductionGaps = []`.
- [x] State explicitly that broad optimization, visible OS-level multi-window drag/drop shell restoration, Vulkan/Metal texture parity, cross-platform shell parity, broad text/font/IME/accessibility parity, and broad `first_party_editor_excellence` remain unclaimed until exact phase gates pass.
- [x] Compose and check:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: all checks pass and no C++ runtime behavior changes.

Phase 0 validation evidence:

- `tools/agent-context.ps1 -ContextProfile Minimal` confirms `currentActivePlan = docs/superpowers/plans/2026-06-06-first-party-ui-editor-excellence-v1.md`, `recommendedNextPlan.id = first-party-ui-editor-excellence-v1`, and `unsupportedProductionGaps = []`.
- `tools/check-toolchain.ps1`: ok.
- `tools/check-json-contracts.ps1`: ok.
- `tools/check-ai-integration.ps1`: ok.
- `tools/check-agents.ps1`: ok.
- `tools/check-text-format.ps1`: ok.
- `tools/validate.ps1`: ok; 99/99 tests passed. Metal/Apple lanes remain host-gated diagnostics on this Windows host.

## Phase 1: Editor/UI Performance Budgets And Baseline Telemetry

**Goal:** Establish exact budgets and before/after measurement rows for editor/UI optimization without claiming broad optimization.

**Files:** `editor/src/first_party_editor_document.*`, `editor/src/native_editor_app.*`, `editor/core/include/mirakana/editor/editor_ui_performance.hpp`, `editor/core/src/editor_ui_performance.cpp`, `tests/unit/editor_core_tests.cpp`, `tests/unit/editor_native_shell_tests.cpp`, docs/manifest/static checks.

- [x] Add RED editor-core tests for `EditorUiPerformanceBudget`, `EditorUiPerformanceSample`, and `summarize_editor_ui_performance`.
- [x] Add RED native-shell tests for budget rows copied into `FirstPartyEditorShellSmokeCounters`.
- [x] Track at least these counters:

```text
editor_ui_performance_budget_status=ready
editor_ui_performance_layout_us_p95
editor_ui_performance_document_build_us_p95
editor_ui_performance_renderer_submission_us_p95
editor_ui_performance_text_runs
editor_ui_performance_renderer_boxes
editor_ui_performance_visible_texture_composites
editor_ui_performance_memory_high_water_bytes
editor_ui_performance_budget_violations=0
editor_ui_performance_diagnostics=0
editor_ui_performance_broad_optimization_claimed=0
```

- [x] Implement value-only budget summarization in `MK_editor_core`.
- [x] Capture measured first-party shell document-build, layout, and renderer-submission timing samples before reporting p95 rows.
- [x] Populate smoke counters from the visible shell without exposing native handles.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

Expected: budget rows are ready, broad optimization remains explicitly unclaimed, and editor smoke still reports `editor_shell_imgui=0` and `editor_shell_sdl3=0`.

Phase 1 validation evidence:

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests` failed before implementation because `mirakana/editor/editor_ui_performance.hpp` was missing.
- GREEN focused loop: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`, and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"` passed; 2/2 focused editor tests passed.
- Editor lane: `tools/build-editor.ps1` passed after the repository mutex wait; `desktop-editor` built, `MK_editor_smoke` ran, and 100/100 tests passed.
- Agent surface sync: docs, manifest fragments, composed manifest, CTest smoke regex, `check-ai-integration`, and `check-json-contracts` now require `editor_ui_performance_budget_status=ready`, positive measured p95/row counters, `editor_ui_performance_budget_violations=0`, `editor_ui_performance_diagnostics=0`, and `editor_ui_performance_broad_optimization_claimed=0`.
- Source refresh: Phase 1 did not touch SDK/library adapter behavior; the official-source and Context7 baseline recorded in this plan remains the active constraint for later SDK/library phases.

## Phase 2: Retained UI Diff, Cache, And Submission Optimization

**Goal:** Add deterministic dirty-region and cache evidence for first-party retained UI layout/text/submission paths.

**Files:** `engine/ui/**`, `engine/ui_renderer/**`, `editor/src/first_party_editor_document.*`, `tests/unit/ui_*_tests.cpp`, `tests/unit/ui_renderer_tests.cpp`, `tests/unit/editor_native_shell_tests.cpp`.

- [x] Add RED tests for stable element-id dirty tracking, invalidation on text/style/layout changes, and no invalidation on row ordering alone.
- [x] Add RED tests for layout cache hits/misses, text row cache hits/misses, glyph atlas binding reuse, image binding reuse, and deterministic renderer submission ordering.
- [x] Add counters:

```text
ui_retained_diff_status=ready
ui_retained_dirty_rows
ui_retained_layout_cache_hits
ui_retained_layout_cache_misses
ui_retained_text_cache_hits
ui_retained_text_cache_misses
ui_retained_submission_reused_rows
ui_retained_submission_rebuilt_rows
ui_retained_cache_native_handle_access=0
```

- [x] Implement cache keys over first-party rows only; do not cache native handles or backend objects.
- [x] Run focused UI/editor tests.
- [x] Run `tools/build-editor.ps1` and full slice validation.

Expected: optimization has deterministic evidence and does not alter public UI semantics.

Phase 2 validation evidence:

- RED: after preparing the linked worktree with `tools/prepare-worktree.ps1` and configuring `tools/cmake.ps1 --preset dev`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests MK_editor_native_shell_tests` failed before implementation because the retained UI diff API (`RetainedUiSnapshot`, `RetainedUiDiffRequest`, `RetainedUiDiffSummary`, `diff_retained_ui_snapshots`) and renderer reuse/order counters were missing.
- GREEN focused loop: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests MK_editor_native_shell_tests` passed after implementation, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests|MK_editor_native_shell_tests"` passed 2/2 focused tests.
- Implemented evidence: `MK_ui` now exposes `RetainedUiSnapshot`, `RetainedUiDiffRequest`, `RetainedUiDiffSummary`, `make_retained_ui_snapshot`, `diff_retained_ui_snapshots`, and `retained_ui_diff_status_id`; `MK_ui_renderer` reports `renderer_submission_order_key`, `glyph_atlas_binding_reuse_rows`, `image_binding_reuse_rows`, and `cache_native_handle_access=false`; the native editor shell records frame-to-frame retained diff summaries and surfaces the `ui_retained_*` smoke counters.
- Review hardening: the retained UI row submission key now mixes actual `RendererSubmission` element/layout/box/text/image/accessibility payload rows by stable element id so stale submission payloads cannot be reported as reused rows; regression tests also cover image invalidation, removed rows, duplicate snapshot diagnostics, and missing current snapshot diagnostics.
- Required closeout lane: `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, focused build for `MK_editor_core_tests MK_editor_native_shell_tests MK_ui_renderer_tests`, focused CTest for those three suites, `tools/build-editor.ps1`, `tools/check-format.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-native-desktop-contracts.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1` all passed on the Phase 2 worktree; `tools/build-editor.ps1` passed 100/100 tests including `MK_editor_smoke`.
- Full slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; static checks were clean, `check-ai-integration` kept `unsupportedProductionGaps=0`, diagnostic-only Metal/Apple host gates remained host-gated on Windows, and the dev CTest lane passed 99/99 tests.
- Source refresh: Phase 2 did not touch SDK/library adapter behavior; the official-source and Context7 baseline recorded in this plan remains the active constraint for later SDK/library phases.

## Phase 3: Multi-Window Docking And Workspace v3

**Goal:** Promote single-window docking to multi-window dock/tear-off/merge with deterministic workspace persistence and AI-operable commands.

**Files:** `editor/core/include/mirakana/editor/editor_dock_layout.hpp`, `editor/core/include/mirakana/editor/workspace.hpp`, matching `editor/core/src/*.cpp`, `editor/src/first_party_editor_document.*`, `editor/src/native_editor_app.*`, `tests/unit/editor_core_tests.cpp`, `tests/unit/editor_native_shell_tests.cpp`, docs/manifest/static checks.

- [x] Add RED tests for `GameEngine.Workspace.v3` read/write with stable window ids, monitor ids, DPI scale rows, bounds rows, dock roots per window, focused window, and active panel rows.
- [x] Add RED tests for create-window, tear-off, move-to-window, merge-window, close-window, and reset-all-windows command planning.
- [x] Add RED tests rejecting duplicate panel residency, orphan windows, cycles, invalid monitor/DPI values, empty window roots, native-handle tokens, and UI middleware tokens.
- [x] Add AI command ids:

```text
editor.dock.window.create
editor.dock.window.close
editor.dock.panel.tear_off
editor.dock.panel.move_to_window
editor.dock.window.merge
editor.dock.window.reset_all
```

- [x] Add smoke counters:

```text
editor_shell_multi_window_docking_status=ready
editor_shell_dock_windows
editor_shell_dock_tear_off_commands
editor_shell_dock_window_merge_commands
editor_shell_workspace_v3_status=ready
editor_shell_multi_window_native_handles_exposed=0
```

- [x] Implement editor-core model first; wire visible Win32 windows only after model tests pass.
- [x] Run editor-core/native-shell tests and `tools/build-editor.ps1`.

Expected: core-owned multi-window docking is usable, persistent, AI-operable, and does not require Dear ImGui or SDL3; visible OS-level multi-window drag/drop shell restoration remains a later private shell adapter phase.

Evidence:

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed before implementation because `make_default_editor_dock_multi_window_layout`, `EditorDockWindowCommandRequest`, `EditorDockWindowBounds`, `serialize_workspace_v3`, `deserialize_workspace_v3`, and related workspace v3 APIs were missing.
- Implemented evidence: `MK_editor_core` now exposes `EditorDockMultiWindowLayout`, `EditorDockWindowRow`, `EditorDockWindowBounds`, `EditorDockWindowCommandKind`, `EditorDockWindowCommandRequest`, `EditorDockWindowCommandPlan`, `make_default_editor_dock_multi_window_layout`, `validate_editor_dock_multi_window_layout`, `plan_editor_dock_window_command`, `apply_editor_dock_window_command`, `Workspace::multi_window_dock_layout`, `serialize_workspace_v3`, and `deserialize_workspace_v3`.
- AI-operable evidence: `EditorAiCommandCatalog` now exposes `editor.dock.window.create`, `editor.dock.window.close`, `editor.dock.panel.tear_off`, `editor.dock.panel.move_to_window`, `editor.dock.window.merge`, and `editor.dock.window.reset_all`, with dry-run/apply overloads over `EditorDockMultiWindowLayout` and no native handle exposure.
- Smoke evidence: `MK_editor_smoke` now requires `editor_shell_multi_window_docking_status=ready`, `editor_shell_dock_windows=1`, `editor_shell_dock_tear_off_commands=1`, `editor_shell_dock_window_merge_commands=1`, `editor_shell_workspace_v3_status=ready`, and `editor_shell_multi_window_native_handles_exposed=0`.
- Boundary evidence: Phase 3 proves the value/core model, workspace v3 persistence, AI command surface, and smoke planning counters; it does not claim visible OS-level multi-window drag/drop shell restoration.
- Validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"` passed; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1` passed the `desktop-editor` lane with 100/100 tests including `MK_editor_smoke`.

## Phase 4: Complete Editable Rich Text Core

**Goal:** Turn read-only rich text rows into a complete editor-core editable rich text contract with visible shell editing.

**Files:** `editor/core/include/mirakana/editor/editor_rich_text.hpp`, `editor/core/src/editor_rich_text.cpp`, `editor/core/src/ai_operation_surface.cpp`, `editor/src/first_party_editor_document.*`, `editor/src/native_editor_app.*`, tests.

- [x] Add RED tests for paragraph/span insertion, deletion, replacement, style toggles, inline object preservation, selection normalization, scalar-boundary cursor movement, undo/redo, copy, cut, paste, and plain/rich clipboard separation.
- [x] Add RED tests for rejecting invalid UTF-8, unsupported inline markup, native handle tokens, shell execution tokens, and stale document revisions.
- [x] Add command ids:

```text
<rich_text_document_id>.insert_text
<rich_text_document_id>.delete_selection
<rich_text_document_id>.replace_selection
<rich_text_document_id>.toggle_bold
<rich_text_document_id>.toggle_italic
<rich_text_document_id>.copy_plain_text
<rich_text_document_id>.copy_selection_plain_text
<rich_text_document_id>.copy_rich_text
<rich_text_document_id>.cut_selection
<rich_text_document_id>.paste_plain_text
<rich_text_document_id>.paste_rich_text
```

- [x] Add visible shell edit controls for selected editable documents only; Console, AI Commands, and diagnostic Inspector sections remain read-only unless the model marks a row editable.
- [x] Add counters:

```text
editor_rich_text_edit_status=ready
editor_rich_text_editable_documents
editor_rich_text_command_rows
editor_rich_text_clipboard_plain_ready=1
editor_rich_text_clipboard_rich_ready=1
editor_rich_text_native_handles_exposed=0
```

Expected: rich text editing is complete at the first-party document level before broad shaping/font/IME/accessibility parity is claimed.

**Phase 4 Evidence:** Candidate `codex/first-party-ui-editor-excellence-phase4` adds `EditorRichTextDocument.editable`, `EditorRichTextDocumentState`, undo/redo history stacks, `EditorRichTextEditCommandKind`, `EditorRichTextClipboardPayload`, `EditorRichTextEditRequest`, `EditorRichTextEditResult`, public `editor_rich_text_revision`, `normalize_editor_rich_text_selection`, and `apply_editor_rich_text_edit_command` in `MK_editor_core`. The editable contract supports paragraph/newline insertion, deletion, replacement, bold/italic style toggles, inline-object preservation, scalar-boundary cursor movement, copy/cut/paste, plain/rich clipboard payload separation, undo/redo, strict UTF-8 validation, unsupported markup rejection, native-handle/shell-token rejection, and stale document revision rejection. `EditorAiCommandCatalog` now exposes editable rich-text commands only for editable documents: `<rich_text_document_id>.insert_text`, `.delete_selection`, `.replace_selection`, `.toggle_bold`, `.toggle_italic`, `.copy_plain_text`, `.copy_selection_plain_text`, `.copy_rich_text`, `.cut_selection`, `.paste_plain_text`, and `.paste_rich_text`; read-only rich-text documents keep copy rows only. The first-party shell renders visible edit command buttons only for editable rich-text documents and keeps Console/AI Commands read-only, while `MK_editor_smoke` reports `editor_rich_text_edit_status=ready`, positive `editor_rich_text_editable_documents`, positive `editor_rich_text_command_rows`, `editor_rich_text_clipboard_plain_ready=1`, `editor_rich_text_clipboard_rich_ready=1`, and `editor_rich_text_native_handles_exposed=0`. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; focused validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`. This completes editable rich text at the first-party document/AI command/shell-control level only; broad shaping/font fallback/glyph atlas, full IME parity, full accessibility parity, and cross-platform text/editor parity remain later phases.

## Phase 5: Broad Text Shaping, Font Fallback, Glyph Atlas, And Licensing Gates

**Goal:** Add broad text/font evidence through official SDK or audited HarfBuzz/FreeType/ICU-class adapters while keeping `mirakana::ui` contract-first.

**Files:** `engine/ui/**`, `engine/ui_renderer/**`, optional `engine/tools/**` adapter files, `vcpkg.json`, dependency/legal docs when third-party adapters are selected, editor native text/font files, tests.

- [x] Run `license-audit` before adding HarfBuzz, FreeType, ICU, font files, or new codec/font dependencies.
- [x] Add dependency records before integration when third-party adapters are selected.
- [x] Add RED tests for shaping evidence rows: direction, script, language, glyph ids, clusters, advances, offsets, fallback run boundaries, bidi boundaries, and line-break boundaries.
- [x] Add RED tests for font evidence rows: face family/style, glyph index lookup, glyph metrics, pixel size, bitmap format, atlas allocation, fallback face id, and licensing/provenance row.
- [x] Add RED tests for ICU or official SDK boundary rows: grapheme, word, and line-break segmentation over Latin, Japanese, Arabic, emoji, combining marks, and mixed-direction text.
- [x] Add counters:

```text
editor_text_shaping_status=ready
editor_text_font_fallback_status=ready
editor_text_glyph_atlas_status=ready
editor_text_bidi_status=ready
editor_text_line_break_status=ready
editor_text_dependency_license_records=ready
editor_text_native_handles_exposed=0
```

- [x] Keep DirectWrite adapter ready on Windows; keep HarfBuzz/FreeType/ICU rows dependency-gated until dependency/legal records and tests pass.

Expected: broad text/font quality is evidence-backed and dependency-compliant.

**Phase 5 Evidence:** Candidate `codex/first-party-ui-editor-excellence-phase5` extends the first-party Windows DirectWrite text/font evidence surface without adding third-party dependencies. `NativeEditorTextAtlasHandoffEvidence` and `FirstPartyEditorShellSmokeCounters` now expose `editor_text_shaping_status=ready`, `editor_text_font_fallback_status=ready`, `editor_text_glyph_atlas_status=ready`, `editor_text_bidi_status=ready`, `editor_text_line_break_status=ready`, `editor_text_dependency_license_records=ready`, positive shaping segment, glyph cluster, advance/offset, bidi, word, line-break, font face, glyph metric, bitmap format, atlas allocation, and font-license/provenance rows, `editor_text_dependency_gated_rows=3`, and `editor_text_native_handles_exposed=0`. `EditorAiOperationSnapshot.status_rows` now also includes `editor.ai.text.shaping`, `editor.ai.text.font_fallback`, `editor.ai.text.glyph_atlas`, `editor.ai.text.bidi`, `editor.ai.text.line_break`, and `editor.ai.text_dependency.licenses` value rows. HarfBuzz, FreeType, and ICU remain explicit `dependency_gated` rows (`editor_text_harfbuzz_dependency_status=dependency_gated`, `editor_text_freetype_dependency_status=dependency_gated`, `editor_text_icu_dependency_status=dependency_gated`), so `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` are intentionally unchanged in this slice. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; focused validation passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`. This completes Phase 5 at the first-party evidence/counter/AI-row level only; production HarfBuzz/FreeType/ICU integration, real cross-platform shaping/font fallback parity, full accessibility parity, Vulkan/Metal editor texture parity, and broad `first_party_editor_excellence` remain later phases, while Phase 6 covers selected Windows TSF/text-store IME evidence separately.

## Phase 6: Full IME And Text Input Parity

**Goal:** Promote text input from selected Windows TSF status into explicit per-platform IME/text-input evidence.

**Files:** `engine/ui/**`, `engine/platform/win32/**`, `editor/src/native_editor_tsf_text_input.*`, future selected platform adapter files, tests, docs/manifest.

- [x] Add RED tests for first-party text edit state over grapheme-aware cursor movement, selection, composition range, committed text, candidate selection, reconversion request, surrounding text, and caret rectangle rows.
- [x] Add Windows RED tests for TSF `ITextStoreACP` callback coverage, document locks, selection updates, text replacement, candidate UI host ownership, and reconversion diagnostics.
- [x] Add macOS host-gated rows for `NSTextInputClient` and InputMethodKit.
- [x] Add Linux host-gated rows for IBus and Fcitx integration.
- [x] Add Android/iOS host-gated rows for selected platform text input and accessibility services only when mobile editor/runtime UI work is explicitly selected.
- [x] Add counters:

```text
editor_ime_parity_status=ready
editor_ime_windows_tsf_status=ready
editor_ime_macos_status=host_gated
editor_ime_linux_ibus_status=host_gated
editor_ime_linux_fcitx_status=host_gated
editor_ime_android_status=host_gated
editor_ime_ios_status=host_gated
editor_ime_native_handles_exposed=0
```

Expected: IME parity is per-platform and fail-closed; Windows readiness does not promote other hosts.

**Phase 6 Evidence:** Candidate `codex/first-party-ui-editor-excellence-phase6` adds dependency-free first-party IME/text-input parity evidence. `mirakana::ui` now exposes `TextInputParityEvidenceRequest`, `TextInputParityEvidenceSummary`, `TextInputCandidateSelection`, `TextInputReconversionRequest`, and `plan_text_input_parity_evidence` so grapheme boundary, cursor, selection, composition, candidate, reconversion, surrounding text, caret rectangle, and native-handle rejection rows are value-checked before editor shell promotion. The Windows editor shell keeps TSF private in `editor/src/native_editor_tsf_text_input.*` and records `NativeEditorTsfTextStoreEvidence` over an app-owned private `ITextStoreACP` adapter, including sink advisory, document lock/request lock, selection, text read/replace/insert, candidate UI host ownership, reconversion diagnostics, and zero public native handles. `MK_editor_smoke` reports `editor_ime_parity_status=ready`, `editor_ime_windows_tsf_status=ready`, `editor_ime_macos_status=host_gated`, `editor_ime_linux_ibus_status=host_gated`, `editor_ime_linux_fcitx_status=host_gated`, `editor_ime_android_status=host_gated`, `editor_ime_ios_status=host_gated`, positive grapheme/composition/candidate/reconversion evidence counters, and `editor_ime_native_handles_exposed=0`. `EditorAiOperationSnapshot.status_rows` now includes `editor.ai.ime.parity`, `editor.ai.ime.candidate_selection`, `editor.ai.ime.reconversion`, and `editor.ai.ime.platform_host_gates`. Microsoft TSF, Apple `NSTextInputClient` / InputMethodKit, IBus, Fcitx, AT-SPI2-adjacent Linux host context, and ICU boundary-analysis documentation were refreshed on 2026-06-06; no HarfBuzz, FreeType, ICU, IBus, Fcitx, AT-SPI2, or font package dependency was added. Focused validation passed for `MK_ui_renderer_tests`, `MK_editor_core_tests`, and `MK_editor_native_shell_tests`. This completes Phase 6 for selected Windows TSF/text-store evidence and cross-platform host-gated IME parity rows only; accessibility parity, Vulkan/Metal editor texture parity, cross-platform visible editor shells, visible OS-level multi-window drag/drop shell restoration, and broad `first_party_editor_excellence` remain later phases.

## Phase 7: Accessibility Provider Parity

**Goal:** Promote accessibility from selected Windows UIA provider readiness into exact control pattern, event, and cross-platform host-gated evidence.

**Files:** `engine/ui/**`, `editor/src/native_editor_uia_provider.*`, future platform accessibility adapters, tests, docs/manifest/static checks.

- [x] Add RED tests for first-party semantic role/name/state/focus/action/relationship/live-region rows.
- [x] Add Windows UIA RED tests for AutomationId, RuntimeId opacity, Name, ControlType, Invoke, Value, Selection, Text, TextEdit, Scroll, Window, and Toggle patterns where the first-party row supports them.
- [x] Add RED tests for UIA events: focus change, property change, text edit, selection change, structure change, and window opened/closed rows.
- [x] Add macOS host-gated rows for `NSAccessibilityProtocol`.
- [x] Add Linux host-gated rows for AT-SPI2 tree, role, action, text, selection, and event exposure.
- [x] Add counters:

```text
editor_accessibility_parity_status=ready
editor_accessibility_windows_uia_patterns_ready=1
editor_accessibility_windows_uia_events_ready=1
editor_accessibility_macos_status=host_gated
editor_accessibility_linux_at_spi_status=host_gated
editor_accessibility_android_status=host_gated
editor_accessibility_ios_status=host_gated
editor_accessibility_native_handles_exposed=0
```

- [x] Validate Windows rows with UIA provider tests and Inspect-compatible evidence rows where host tools are available.

Expected: accessibility parity becomes explicit evidence, not a generic provider-ready claim.

**Phase 7 Evidence:** Candidate `codex/first-party-ui-editor-excellence-phase7` adds dependency-free first-party accessibility parity evidence. `mirakana::ui` now propagates `accessibility_live_region` through retained element descriptions, elements, accessibility nodes, and retained UI hashes so live-region changes participate in deterministic snapshot/diff evidence. The Windows editor shell keeps UIA private in `editor/src/native_editor_uia_provider.*` and records `NativeEditorUiaProviderState` rows for AutomationId, opaque child `UiaAppendRuntimeId` runtime ids, Name, ControlType, live regions, Invoke, Value, Selection, Text, TextEdit, Scroll, Window, Toggle, focus, property-change, text-edit, selection-change, structure-change, window, and live-region evidence without exposing native handles. `MK_editor_smoke` reports `editor_accessibility_parity_status=ready`, `editor_accessibility_windows_uia_patterns_ready=1`, `editor_accessibility_windows_uia_events_ready=1`, `editor_accessibility_macos_status=host_gated`, `editor_accessibility_linux_at_spi_status=host_gated`, `editor_accessibility_android_status=host_gated`, `editor_accessibility_ios_status=host_gated`, positive `editor_accessibility_live_region_rows`, positive `editor_accessibility_windows_uia_pattern_rows`, positive `editor_accessibility_windows_uia_event_rows`, and `editor_accessibility_native_handles_exposed=0`. `EditorAiOperationSnapshot.status_rows` now includes `editor.ai.accessibility.parity`. Microsoft UI Automation pattern/property/event documentation, Apple AppKit accessibility documentation, GNOME AT-SPI2 documentation, and WAI-ARIA/APG role/state/live-region guidance via Context7 were refreshed on 2026-06-06; no accessibility middleware or third-party dependency was added. Focused validation passed for `MK_editor_core_tests` and `MK_editor_native_shell_tests`. This completes Phase 7 for selected first-party Windows UIA pattern/event evidence and cross-platform host-gated accessibility rows only; external Inspect/tool execution, macOS/Linux/Android/iOS accessibility execution, Vulkan/Metal editor texture parity, cross-platform visible editor shells, visible OS-level multi-window drag/drop shell restoration, and broad `first_party_editor_excellence` remain later phases.

## Phase 8: Vulkan Editor Texture Display Parity

**Goal:** Add private Vulkan visible texture display for Viewport and Material Preview with strict validation.

**Files:** `editor/src/native_texture_display_adapter.*`, optional private Vulkan adapter files, `engine/rhi/vulkan/**`, `tests/unit/backend_scaffold_tests.cpp`, `tests/unit/editor_native_shell_tests.cpp`, shader fixture tools and docs.

- [x] Add RED tests for Vulkan offscreen color texture creation, descriptor image updates, sampler rows, shader-read layout transition, visible compositor draw, presentation, fence wait, resize-safe teardown, and no native handle exposure.
- [x] Require DXC SPIR-V artifacts, `spirv-val`, Vulkan runtime, Vulkan 1.3 synchronization2, and validation layers before ready.
- [x] Use synchronization2 barriers for color attachment write to shader read and transfer write to shader read.
- [x] Add counters:

```text
editor_shell_viewport_vulkan_status=vulkan_texture_ready
editor_shell_viewport_vulkan_visible_texture_composites
editor_shell_material_preview_vulkan_status=vulkan_texture_ready
editor_shell_material_preview_vulkan_visible_texture_composites
editor_shell_vulkan_validation_layer_ready=1
editor_shell_vulkan_native_handles_exposed=0
```

- [x] Keep D3D12 and Metal claims separate.

Expected: Vulkan editor texture display is backend-local and host/toolchain-gated.

**Phase 8 Evidence:** Candidate `codex/first-party-ui-editor-excellence-phase8` extends the private editor texture-display contract with backend-local Vulkan readiness rows while keeping the default visible Windows editor shell on D3D12. `NativeViewportDisplayPlan`, `NativeMaterialPreviewDisplayPlan`, `NativeTextureDisplayAdapter`, and `NativeEditorVisibleTextureCompositor` now carry `vulkan_host_available`, `vulkan_validation_layer_ready`, `vulkan_spirv_artifacts_available`, and `vulkan_synchronization2_ready` gates before reporting `vulkan_texture_ready`; missing Vulkan validation layer, SPIR-V artifact, or synchronization2 evidence remains fail-closed. `FirstPartyEditorShellSmokeCounters` and `MK_editor_smoke` expose `editor_shell_viewport_vulkan_status`, `editor_shell_viewport_vulkan_visible_texture_composites`, `editor_shell_material_preview_vulkan_status`, `editor_shell_material_preview_vulkan_visible_texture_composites`, `editor_shell_vulkan_validation_layer_ready`, and `editor_shell_vulkan_native_handles_exposed=0`. `MK_backend_scaffold_tests` locks Vulkan color-attachment-write to shader-read and transfer-write to shader-read synchronization2 barrier mapping, and `MK_editor_native_shell_tests` covers private Vulkan viewport/material readiness and visible compositor sampling without native handle exposure. Context7 `/khronosgroup/vulkan-docs` was refreshed on 2026-06-06 for synchronization2 image barriers, shader-read layout transitions, descriptor sampling, and validation-layer implications. This completes Phase 8 for host/toolchain-gated Vulkan counter and private adapter evidence only; Metal editor texture-display parity, default visible-shell Vulkan backend selection, broader material-preview GPU parity, visible OS-level multi-window drag/drop shell restoration, cross-platform visible editor shells, and broad `first_party_editor_excellence` remain later phases.

## Phase 9: Metal Editor Texture Display Parity

**Goal:** Add Apple-host private Metal visible texture display for Viewport and Material Preview.

**Files:** Apple-host Metal backend files, private editor Metal texture display files, `tests/unit/backend_scaffold_tests.cpp`, editor native tests, hosted macOS workflow/recipe files if needed.

- [ ] Add host-gated RED tests for `MTLCommandQueue`, non-empty metallib, render pipeline, texture render target, shader-read texture sampling, sampler state, render pass, presentable drawable, command buffer completion, and no native handle exposure.
- [ ] Add counters:

```text
editor_shell_viewport_metal_status=metal_texture_ready
editor_shell_viewport_metal_visible_texture_composites
editor_shell_material_preview_metal_status=metal_texture_ready
editor_shell_material_preview_metal_visible_texture_composites
editor_shell_metal_feature_set_ready=1
editor_shell_metal_native_handles_exposed=0
```

- [ ] Run on Apple host:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-apple-host-evidence.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe renderer-metal-apple-host-evidence
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

Expected: Metal editor texture parity is promoted only by Apple-host execution evidence.

## Phase 10: Cross-Platform Editor Shell Adapters

**Goal:** Define and implement host-gated macOS and Linux visible editor shell adapters over the same editor-core and UI contracts.

**Files:** future selected `editor/src/*macos*`, `editor/src/*linux*`, `engine/platform/macos/**`, `engine/platform/linux/**`, CMake presets, validation recipes, docs/manifest/static checks.

- [ ] Add core shell abstraction tests proving window lifecycle, DPI scale, monitor rows, pointer/keyboard event rows, clipboard rows, file dialog rows, text input rows, accessibility rows, and renderer presentation rows are first-party and host-gated.
- [ ] Add macOS adapter rows for AppKit windowing, Core Text, `NSTextInputClient`, `NSAccessibilityProtocol`, Metal presentation, and private native handle boundaries.
- [ ] Add Linux adapter rows for X11/Wayland selection, AT-SPI2, IBus, Fcitx, Vulkan presentation, and private native handle boundaries.
- [ ] Add counters:

```text
editor_shell_cross_platform_status=host_gated
editor_shell_macos_status=host_gated
editor_shell_linux_status=host_gated
editor_shell_android_status=unsupported
editor_shell_ios_status=unsupported
editor_shell_cross_platform_native_handles_exposed=0
```

- [ ] Keep Android/iOS visible editor shells unsupported unless a separate selected product requirement accepts them.

Expected: cross-platform shell work is exact, host-gated, and does not dilute Windows evidence.

## Phase 11: AI-Operable Editor Command Expansion

**Goal:** Make the new editor features controllable by AI through structured rows and reviewed commands.

**Files:** `editor/core/include/mirakana/editor/ai_operation_surface.hpp`, `editor/core/src/ai_operation_surface.cpp`, `editor/src/first_party_editor_document.*`, tests, manifest/docs.

- [ ] Add AI snapshot rows:

```text
editor.ai.window.layout
editor.ai.dock.multi_window
editor.ai.rich_text.editable_documents
editor.ai.text.shaping
editor.ai.text.font_fallback
editor.ai.ime.parity
editor.ai.accessibility.parity
editor.ai.viewport.backend_parity
editor.ai.material_preview.backend_parity
editor.ai.performance.budgets
```

- [ ] Add reviewed command catalog entries for multi-window dock commands, editable rich text commands, text/font diagnostics copy, accessibility diagnostics copy, and backend readiness refresh requests.
- [ ] Require `expected_revision` for every mutating command.
- [ ] Reject native handles, shell/process execution, validation recipe execution, package script execution, screen coordinates, arbitrary file mutation, stale revisions, disabled commands, and unsupported parameters.
- [ ] Add counters:

```text
editor_ai_operation_excellence_status=ready
editor_ai_operation_snapshot_rows
editor_ai_operation_command_rows
editor_ai_operation_mutating_commands_revision_checked=1
editor_ai_operation_native_handles_exposed=0
```

Expected: AI can operate the advanced editor through first-party structures, not screen automation.

## Phase 12: Aggregate Excellence Gate And Closeout

**Goal:** Add a single exact aggregate only when selected rows have landed, then close or return to the selection gate.

**Files:** package/editor smoke validation, manifest fragments, docs, static checks, registry.

- [ ] Add an aggregate validator requiring the selected exact rows from this plan.
- [ ] Add counters:

```text
first_party_editor_excellence_status=ready
first_party_editor_excellence=1
first_party_editor_excellence_windows_d3d12=1
first_party_editor_excellence_vulkan=<0|1>
first_party_editor_excellence_metal=<0|1>
first_party_editor_excellence_cross_platform=<0|1>
first_party_editor_excellence_text_parity=<0|1>
first_party_editor_excellence_accessibility_parity=<0|1>
first_party_editor_excellence_broad_optimization_claimed=0
first_party_editor_excellence_native_handles_exposed=0
```

- [ ] If Vulkan, Metal, cross-platform, text parity, or accessibility parity is not selected for the current host, the aggregate must leave the matching value `0` and must not claim parity.
- [ ] Update static checks so docs cannot claim broad editor excellence without the aggregate definition and evidence.
- [ ] Return `currentActivePlan` to the production-completion master selection gate or select the next active plan with evidence.

Expected: the broad claim has one exact meaning and remains fail-closed.

## Required Validation At Phase Close

Every C++/editor/build/public-contract phase must run the relevant focused loop plus a final slice gate:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests MK_ui_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests|MK_ui_renderer_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

For dependency/legal phases:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Before staging, push, PR creation, ready conversion, auto-merge, or cleanup:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Docs-only planning edits may use narrower checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

## Non-Goals

- No Dear ImGui, SDL3, Qt, Slint, RmlUi, NoesisGUI, or UI middleware.
- No public D3D12, Vulkan, Metal, Win32, AppKit, X11, Wayland, TSF, UIA, Core Text, InputMethodKit, AT-SPI2, IBus, Fcitx, RHI, texture, descriptor, command queue, swapchain, fence, file dialog, clipboard, process, or native handle exposure.
- No compatibility parser or shim for workspace v2 multi-window semantics; use clean-break workspace v3.
- No cross-platform readiness inferred from Windows evidence.
- No Vulkan readiness inferred from D3D12 evidence.
- No Metal readiness inferred from Windows or Vulkan evidence.
- No dependency introduction without license audit, dependency docs, legal notices, manifest feature gates, and validation.
- No screen scraping, coordinate-based AI automation, arbitrary shell, raw manifest command evaluation, package script execution from editor core, or validation recipe execution from editor core.
- No broad optimization claim without before/after budget evidence and exact counters.

## Completion Definition

The milestone is complete only when:

- Phase 0 has selected the plan or the operator explicitly keeps it as proposed.
- Every selected implementation phase has focused tests, docs, manifest fragments, static checks, and full `tools/validate.ps1` evidence.
- `tools/build-editor.ps1` passes after every visible-shell phase.
- Host-gated Vulkan and Metal phases include actual host/toolchain evidence before ready claims.
- Dependency and legal records are complete before third-party text/font/IME/accessibility dependencies are used.
- `currentActivePlan`, `recommendedNextPlan`, registry, roadmap, current capabilities, manifest fragments, composed manifest, skills, and static checks describe the same ready/non-ready truth.
- Broad `first_party_editor_excellence` is either still unclaimed with explicit counters or claimed only through Phase 12 aggregate evidence.
- No untracked temporary files, stale generated outputs, Dear ImGui/SDL3 dependencies, UI middleware, native handle leaks, unsupported broad claims, or validation weakening remain.

## Plan Self-Review

- This plan creation does not implement runtime/editor behavior.
- Phase 0 changes the active manifest pointer and does not implement runtime/editor behavior.
- No broad ready claim is added by this selection slice.
- Every requested gap has a concrete phase: broad optimization, multi-window docking, complete editable rich text, Vulkan/Metal editor texture parity, cross-platform editor shell, broad text/font/IME/accessibility parity, and AI operation.
- Host-gated work has explicit host requirements and counters.
- Official documentation and Context7 checks are recorded for refresh during implementation.
