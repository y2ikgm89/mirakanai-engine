# 2026-05-31 First-Party UI Editor Production Stack v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan phase-by-phase. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Productize the first-party UI/editor stack after the Dear ImGui clean break by moving the remaining editor UI gaps to engine-owned retained contracts, AI-operable editor rows, and official-SDK or audited-dependency adapters without preserving backward compatibility for removed UI middleware, old lane names, or private host APIs.

**Architecture:** Own the durable UI/editor contract: dock graph, rich text documents, semantic roles, focus, command ids, AI snapshots, dry-run/apply rows, validation diagnostics, and persistence. Keep low-level text shaping, font rasterization, IME, accessibility bridge publication, image decoding, and platform integration behind narrow private adapters backed by official SDKs or audited dependencies. Runtime game UI stays on public `mirakana::ui`; editor productization stays behind `MK_editor_core`, `MK_editor_shell_common`, and private native shell adapters.

**Tech Stack:** C++23, PowerShell 7 repository wrappers, CMake presets, `MK_editor_core`, `MK_editor_shell_common`, dependency-free `desktop-editor`, `MK_ui`, `MK_ui_renderer`, private Win32/DXGI/D3D12 editor host code, Windows SDK DirectWrite/Direct2D where selected for Windows text/font adapter work, Windows Text Services Framework for IME/text services, Microsoft UI Automation providers for accessibility publication, existing runtime UI adapter contracts, manifest fragments, static contract checks, and focused CTest/build-editor/full-validation gates.

---

## Plan ID

**Plan ID:** `first-party-ui-editor-production-stack-v1`

## Status

**Status:** Active.

Selected on 2026-05-31 after a repository-wide UI/editor/agent-contract audit. The previous active state was the production-completion selection gate after [First-Party Editor Shell v1](2026-05-31-first-party-editor-shell-v1.md) completed the clean-break Dear ImGui replacement and returned `recommendedNextPlan.id = next-production-gap-selection`.

## Current Audit

The repository already has the right foundation:

- `MK_ui` owns retained `UiDocument` values, layout solving, renderer submissions, semantic roles, accessibility payload rows, text shaping/rasterization request plans, IME composition publication plans, platform text-input session plans, clipboard/text-edit rows, image decode request plans, and adapter interfaces without depending on editor, renderer backend, platform SDK, Dear ImGui, or UI middleware.
- `MK_ui_renderer` converts `MK_ui` renderer submissions into renderer sprite submissions and already has cooked image/glyph atlas metadata handoff evidence, while leaving live text shaping, font loading/rasterization, image decoding, OS accessibility publication, and renderer texture upload execution behind separate boundaries.
- `MK_editor_core` is GUI-independent and already owns project IO, workspace state, authoring models, material/viewport diagnostic rows, playtest/package review rows, resource/profiler/input/prefab/game-module-driver models, AI-operable command surfaces, the core-backed `EditorDockLayout` / `editor_dock_panel_catalog` foundation for deterministic dock graph and panel identity rows, and the core-backed `EditorRichTextDocument` foundation for deterministic paragraph/span/inline-object/selection/copy rows.
- `MK_editor` now builds through the dependency-free `desktop-editor` lane with private Win32 first-party retained UI, D3D12 host probing, eleven core-backed panels, Win32 file-dialog/clipboard/reviewed process-runner services, and deterministic smoke counters.
- `first_party_editor_adapter_boundaries.hpp` remains a value-only shell contract for adapter-boundary rows. `editor/src/first_party_editor_rich_text.*` has been removed; rich-text document, selection, plain-copy, inline semantic object, unsupported-markup, and low-level text/font/IME/accessibility gate rows now live in `MK_editor_core` through `EditorRichText*`. Read-only rich-text shell surfaces, selected Windows DirectWrite text/font validation, selected text atlas handoff evidence, focused Windows TSF session selection, and Windows UIA provider publication are implemented; help rich text, broad editable rich text, broad shaping/bidi/fallback, Direct2D GPU text rendering/upload, native IME candidate UI, full `ITextStoreACP` callback coverage, full UIA pattern/event parity, and cross-platform parity remain later phases.
- `editor/src/native_viewport_surface.*`, `native_material_preview_cache.*`, `native_texture_display_adapter.*`, and `native_editor_visible_texture_compositor.*` now promote the Windows visible shell from `diagnostic_only` to `d3d12_texture_ready` only after requested host-private texture display, private D3D12 texture adapter preparation, offscreen targets, descriptor leases, resource barriers, fences, viewport resize-safe lifecycle during recreation, visible-compositor consumption, swapchain render-pass recording, present, submit, and wait evidence are all present. Native texture handles remain private.
- The active manifest and docs keep full docking productization, native IME candidate UI, runtime UI OS accessibility publication, full UIA pattern/event parity, broad production font fallback, Vulkan/Metal editor texture-display parity, broader material-preview GPU parity, cross-platform editor shells, public native handles, SDL3, and UI middleware automation unsupported.

The plan therefore starts from a useful first-party shell, not from zero. The next work is productization of gaps that are already explicitly named in the current contract.

## Official Practice Check

Official documentation and current engine patterns were rechecked for this plan selection:

- Unity 6.4 documents separate runtime/editor UI recommendations: runtime recommends uGUI with UI Toolkit as an alternative, while Editor UI recommends UI Toolkit and treats IMGUI as an alternative for legacy or quick extensibility work. The adopted lesson is to prefer retained, reusable editor UI contracts for complex editor tools and avoid returning to immediate-mode tooling as the foundation: <https://docs.unity3d.com/Manual/UI-system-compare.html>.
- Unreal Engine exposes Slate as its custom UI framework and UMG as a higher-level user-interface layer, with separate documentation for text, fonts, accessibility, testing/debugging, and widgets. The adopted lesson is to own stable widget identity, command routing, diagnostics, and editor tooling inside the engine rather than inheriting a third-party object model: <https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-user-interface-programming-framework-for-unreal-engine> and <https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-user-interfaces-with-umg-and-slate-in-unreal-engine>.
- Godot exposes UI through first-party `Control` nodes and rich text through `RichTextLabel`, while low-level text/font work is centralized behind `TextServer`. The adopted lesson is to own visible UI nodes and rich-text intent, but keep shaping, bidi, font fallback, metrics, and glyph operations in a specialized text service boundary: <https://docs.godotengine.org/en/stable/tutorials/ui/index.html>, <https://docs.godotengine.org/en/stable/classes/class_richtextlabel.html>, and <https://docs.godotengine.org/en/stable/classes/class_textserver.html>.
- Microsoft DirectWrite is the Windows text layout/rendering API family; Windows App SDK DWriteCore is a related route for wider Windows-version and cross-platform deployment decisions. This plan uses DirectWrite-class work only behind private adapters: <https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal>.
- Microsoft Text Services Framework is the official Windows text service/IME framework. This plan treats TSF as adapter work, not panel code: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>.
- Microsoft UI Automation provider guidance requires custom providers to expose custom-rendered UI as an element tree with control/content views and fragment roots. Because this editor renders first-party controls, accessibility publication must be explicitly implemented rather than assumed from the HWND: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview>.

## Adopted Design Decision

Do not add Dear ImGui, Qt, Slint, RmlUi, NoesisGUI, or another UI middleware framework. Do not preserve old `desktop-gui`, `build-gui`, `MK_ENABLE_DESKTOP_GUI`, or ImGui source/API names. The first-party route is appropriate for this engine because:

- AI operation needs stable semantic ids, state rows, command catalogs, dry-run/apply results, and diagnostics. Those are easier to make reliable in first-party retained models than in screen-coordinate automation or middleware object models.
- The codebase already has the right first-party value contracts in `MK_ui`, `MK_editor_core`, `MK_ui_renderer`, and the current `desktop-editor` shell.
- Mature low-level platform problems are not worth hand-rolling inside editor panels. Windows text/font/IME/accessibility should use DirectWrite/Direct2D, TSF, and UIA behind private adapters; non-Windows equivalents remain host-gated future adapter phases.
- Clean break is cheaper now than after public editor/plugin APIs exist. Removed middleware paths should fail by absence, not by deprecation forwarding.

## Ownership Boundary Matrix

| Area | First-party ownership | Adapter ownership |
| --- | --- | --- |
| Docking | Dock graph, split/tab stacks, panel ids, layout persistence, focus, keyboard commands, AI command rows, smoke counters | OS multi-window affordances, monitor placement, native drag/tear-off behavior if selected later |
| Rich text | Document/paragraph/span ids, style tokens, inline semantic rows, selection/copy model, command rows, serialization, diagnostics | Text shaping, bidi, complex line breaking, font fallback, glyph metrics, raster quality |
| Font quality | Font policy rows, fallback intent, atlas handoff, editor theme tokens, deterministic tests | DirectWrite/Direct2D, CoreText, FreeType/HarfBuzz/ICU-class adapters after dependency/legal review |
| IME/text input | Text target ownership, caret rect, surrounding text, composition rows, commit rows, AI-visible diagnostics | TSF, platform text services, candidate windows, virtual keyboard behavior |
| Accessibility | Semantic tree, stable ids, role/name/state/action/focus/bounds rows, validation before publication | UI Automation, NSAccessibility, AT-SPI, Android/iOS bridge publication |
| Viewport/material preview | Editor model state, viewport command rows, material preview descriptors, renderer handoff requirements, smoke counters | Private D3D12 offscreen targets, barriers, descriptor leases, viewport resize-safe teardown, backend-specific texture display |
| AI operation | Snapshot, catalog, dry-run, apply, confirmation, revision, diagnostics, unsupported claims | External LLM/tool orchestration only; no native UI automation dependency |

## Non-Negotiable Constraints

- No Dear ImGui, Qt, Slint, RmlUi, NoesisGUI, SDL3, or UI middleware dependency in `MK_editor`, runtime game UI, public engine APIs, generated games, or active docs.
- No backward-compatibility shims for removed lane names, CMake options, script names, source filenames, smoke counters, API aliases, or dependency features.
- No public `HWND`, `HINSTANCE`, `HANDLE`, `IDXGI*`, `ID3D12*`, `D3D12_*`, `DXGI_*`, COM pointer, DirectWrite/Direct2D/TSF/UIA interface, UI middleware type, renderer/RHI native handle, or platform SDK object in public engine, game, runtime UI, or `editor/core` headers.
- `MK_editor_core` remains GUI-independent. Native shell and platform adapters stay in private editor/platform implementation code.
- Do not claim full docking, cross-platform shell parity, production text shaping, production font fallback, native IME candidate UI, full UIA control pattern/event parity, cross-platform accessibility parity, runtime UI OS accessibility publication, Vulkan/Metal editor texture-display parity, broader material-preview GPU parity, or broad UI renderer quality until focused phases prove them.
- Do not hand-roll OpenType shaping, bidi reordering, glyph hinting, broad font fallback, OS accessibility bridges, or IME engines inside panel code.
- Each implementation phase must update tests, docs, manifest fragments, static checks, and validation recipes only to the scope actually proven.

## Target Shape

```text
MK_editor_core
  Owns durable editor state, dock layout records, rich text documents,
  focus/command/catalog rows, AI snapshot/dry-run/apply contracts,
  persistence reviews, and GUI-independent diagnostics.

MK_editor_shell_common
  Converts editor-core state into first-party UiDocument trees,
  renderer submissions, accessibility payloads, smoke counters,
  and shell-planning rows without native handles.

MK_editor_shell_win32
  Owns the visible HWND, private D3D12/DXGI presentation, Win32 services,
  DirectWrite/Direct2D text/font adapter dispatch, TSF text-service adapter,
  UIA provider fragment root, and private viewport/material texture adapters.

MK_ui / MK_ui_renderer
  Remain the runtime/game-facing first-party UI contracts and renderer bridge.
  They do not depend on editor code or platform SDK objects.
```

## Phase 0 - Plan Selection, Audit, And Guard Sync

**Goal:** Select this plan as the active milestone and make the current audit machine-readable.

**Context:** The repository is at the `next-production-gap-selection` gate after First-Party Editor Shell v1. This phase is docs/manifest/static-check work only; it must not claim new runtime/editor behavior.

**Done When:** The plan exists, registry/roadmap/current-capabilities/master-plan pointers agree, manifest fragments compose, and static checks recognize `first-party-ui-editor-production-stack-v1` as the active plan while preserving `unsupportedProductionGaps = []`.

Files:

- Create: `docs/superpowers/plans/2026-05-31-first-party-ui-editor-production-stack-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/editor.md` only if current editor wording needs a pointer update
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/01-one-dot-zero-readiness-ledger.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify: active-plan/static-check needles under `tools/check-ai-integration*.ps1` and `tools/check-json-contracts*.ps1`

Steps:

- [x] Record the audit summary in this plan without broad source-tree prose.
- [x] Switch `currentActivePlan` and `recommendedNextPlan` to this plan in `010-aiOperableProductionLoop.json`; keep `unsupportedProductionGaps = []`.
- [x] Update registry, current-capabilities, roadmap, and production-completion pointers so agents can find this plan without bulk-reading historical plans.
- [x] Add static-check branches for `first-party-ui-editor-production-stack-v1`.
- [x] Compose the manifest and run focused docs/agent checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
git diff --check
```

## Phase 1 - Promote Durable Editor UI State Into `MK_editor_core`

**Goal:** Move productized dock layout and rich-text state from private shell-only values into GUI-independent editor-core contracts that AI and tests can inspect without launching the shell.

**Context:** Current dock/rich-text files in `editor/src` are value-only shell contracts. They are useful, but persistent editor behavior and AI operation should live in `MK_editor_core` or a clearly named editor-core UI model layer.

**Done When:** Editor-core exposes durable dock layout and rich text document models with validation, serialization or workspace integration where appropriate, unit tests, and no native/UI-middleware dependencies.

Files:

- Modify or create under `editor/core/include/mirakana/editor/`
- Modify or create under `editor/core/src/`
- Keep `editor/src/first_party_editor_*` as shell composition helpers only if they remain private and thin, or delete/replace them cleanly with core-backed equivalents
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`

Steps:

- [x] Add RED tests for duplicate dock ids, invalid splits, missing tabs, stale active tab ids, native-shell focus rejection, and unsafe/native/middleware tokens.
- [x] Add core-level dock layout records that can represent split nodes, tab stacks, active panels, focus target, layout revision, workspace persistence intent, and native-shell panel identity.
- [x] Add core-level rich text records for paragraph/span ids, style tokens, inline semantic rows, selection/copy metadata, and unsupported markup diagnostics.
- [x] Make shell document composition consume the core-level dock model instead of hard-coded private docking defaults.
- [x] Keep low-level shaping/font/IME/accessibility unsupported flags explicit in the core dock model.
- [x] Run focused editor-core/native-shell tests for the dock foundation slice.

**Phase 1a Evidence:** Candidate `codex/first-party-ui-editor-core-state-candidate1` promotes the shell-only dock graph into GUI-independent `MK_editor_core` as `EditorDockLayout`, adds `editor_dock_panel_catalog()` as the canonical panel identity source, deletes `editor/src/first_party_editor_docking.*`, and switches `MK_editor_shell_common` document composition plus `NativeEditorApp` panel counting to consume the core layout/catalog. The catalog explicitly classifies `main_menu` as shell chrome / native shell, and `input_rebinding` as a workspace panel that is not in the current native shell panel set. Focused RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; rich text promotion is recorded separately as Phase 1b. Validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`; `tools/check-tidy.ps1 -Files editor/core/src/editor_dock_layout.cpp,editor/src/first_party_editor_document.cpp,editor/src/native_editor_app.cpp,tests/unit/editor_core_tests.cpp,tests/unit/editor_native_shell_tests.cpp -ReuseExistingFileApiReply`; `tools/check-format.ps1`; `tools/check-public-api-boundaries.ps1`; `tools/check-native-desktop-contracts.ps1`; `tools/check-json-contracts.ps1`; `tools/check-ai-integration.ps1`; `tools/check-agents.ps1`; `tools/build-editor.ps1` (86/86); and full `tools/validate.ps1` (85/85).

**Phase 1b Evidence:** Candidate `codex/first-party-ui-editor-rich-text-candidate2` promotes the shell-only rich text values into GUI-independent `MK_editor_core` as `EditorRichTextDocument`, `EditorRichTextParagraph`, `EditorRichTextSpan`, `EditorRichTextInlineObject`, `EditorRichTextSelection`, and `EditorRichTextUnsupportedCapability`; adds validation for duplicate ids, unsafe middleware/native tokens, unsupported inline markup, selection order/offsets, and unsupported low-level text/font/IME/accessibility capability claims; adds plain-text copy extraction; emits retained `mirakana::ui` label/button/image rows with stable ids; and deletes `editor/src/first_party_editor_rich_text.*` without compatibility aliases. This is value/state productization only: rich text display/editing UX, virtualization, AI snapshot integration, rich clipboard formats, production shaping, bidi, font fallback, glyph quality, native IME candidate UI, and OS accessibility publication remain later phases. Validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`; `tools/check-tidy.ps1 -Files editor/core/src/editor_rich_text.cpp,tests/unit/editor_core_tests.cpp,tests/unit/editor_native_shell_tests.cpp -ReuseExistingFileApiReply`; `tools/check-format.ps1`; `tools/check-public-api-boundaries.ps1`; `tools/check-native-desktop-contracts.ps1`; `tools/check-json-contracts.ps1`; `tools/check-ai-integration.ps1`; `tools/check-agents.ps1`; `tools/build-editor.ps1` (86/86); and full `tools/validate.ps1` (85/85).

## Phase 2 - Docking Productization v1

**Goal:** Turn dock graph rows into a usable single-window docking system for the current `desktop-editor` shell.

**Context:** Full Unity/Unreal-style editor docking includes multi-window drag/tear-off behavior. This phase intentionally starts smaller: single-window split/tab layout, persistence, keyboard/focus commands, and AI-operable mutations.

**Done When:** Docking supports validated split/tab layouts, active-tab changes, panel show/hide, layout reset, workspace persistence, keyboard focus traversal, AI snapshot/catalog/dry-run/apply rows, and smoke counters without native handles or middleware.

Steps:

- [x] Add RED tests for the Phase 2a dock command subset: show panel, hide panel, activate tab, move panel to existing stack, reset layout, reject unknown panels, reject shell chrome hide, reject non-native shell panels, reject empty-stack mutations, and require confirmation for destructive reset.
- [x] Implement value-only dock mutation planners in `MK_editor_core` through `EditorDockCommandKind`, `EditorDockCommandRequest`, `EditorDockCommandPlan`, `plan_editor_dock_command`, and `apply_editor_dock_command`.
- [x] Add RED tests for the remaining dock command subset: split stack, reject invalid cycles, and workspace persistence edge cases.
- [x] Add shell rendering for tab headers, split gutters, active/focused panels, disabled commands, and deterministic style tokens.
- [x] Add persisted workspace layout save/load through reviewed editor-core IO; do not write from shell host code directly.
- [x] Extend `EditorAiOperationSnapshot` / command catalog / dry-run / apply to include dock operations.
- [x] Add keyboard focus traversal over shell dock tab controls without native handles or middleware.
- [x] Update smoke output only for proven counters, for example `editor_shell_docking_status=single_window_ready`.

**Phase 2a Evidence:** Candidate `codex/first-party-ui-editor-dock-commands-candidate3` adds GUI-independent dock mutation planning to `MK_editor_core`. The new value-only API covers show/hide/activate/move/reset over existing tab stacks, returns before/after revision rows, preserves plan-before-apply behavior, validates result layouts, rejects unknown panels, shell chrome hide, non-native shell panels, and mutations that would leave a tab stack empty, and requires explicit confirmation for reset. This does not yet implement shell-rendered tab headers/gutters, workspace persistence, split-stack authoring, invalid-cycle mutation tests, AI operation catalog integration, or docking smoke counters. RED/GREEN evidence covered `MK_editor_core_tests`; validation passed: `tools/check-toolchain.ps1`; `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`; `tools/check-tidy.ps1 -Files editor/core/src/editor_dock_layout.cpp,tests/unit/editor_core_tests.cpp -ReuseExistingFileApiReply`; `tools/check-format.ps1`; `tools/check-public-api-boundaries.ps1`; `tools/check-native-desktop-contracts.ps1`; `tools/check-json-contracts.ps1`; `tools/check-ai-integration.ps1`; `tools/check-agents.ps1`; `tools/build-editor.ps1` (86/86); and full `tools/validate.ps1` (85/85).

**Phase 2b Evidence:** Candidate `codex/first-party-ui-editor-ai-dock-ops-candidate4` extends the GUI-independent AI operation surface with `EditorDockLayout` overloads for `make_editor_ai_operation_snapshot`, `make_editor_ai_command_catalog`, `dry_run_editor_ai_command`, and `apply_editor_ai_command`. AI snapshots now expose `editor.dock.layout`, `editor.dock.stack.*`, and `editor.dock.panel.*` rows; the catalog exposes reviewed dock show/hide/activate/move commands plus confirmation-gated `editor.dock.layout.reset`; dry-run/apply routes dock operations through the Phase 2a planner, requires `target_stack_id` for move/show when needed, preserves exact target matching and confirmation policy, and keeps process execution, validation recipes, manifest/file mutation, native handles, UI middleware automation, and visual scraping out of editor core. This still does not implement shell-rendered tab headers/gutters, workspace persistence, split-stack authoring, invalid-cycle mutation tests, or docking smoke counters. RED/GREEN evidence covered `MK_editor_core_tests`; focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests"`; `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`; `tools/check-tidy.ps1 -Files editor/core/src/ai_operation_surface.cpp,tests/unit/editor_core_tests.cpp -ReuseExistingFileApiReply`; and `tools/check-format.ps1`.

**Phase 2c Evidence:** Candidate `codex/first-party-ui-editor-dock-split-persistence-candidate5` adds the remaining GUI-independent dock command subset and persistence foundation. `EditorDockCommandKind::split_panel_to_stack` moves a panel from an existing tab stack into a new adjacent tab stack under a generated split node, validates source stack identity, split ratio, new node ids, shell chrome, empty source-stack risk, result layout validity, and revision rows, and remains value-only. `validate_editor_dock_layout` now rejects reachable dock graph cycles, unreachable nodes, duplicate split children, duplicate tabs across stacks, comma/newline/`=` unsafe ids, native-handle tokens, and UI middleware tokens. `Workspace` now owns an `EditorDockLayout`, `serialize_workspace` writes clean-break `GameEngine.Workspace.v2` panel and dock graph rows, `deserialize_workspace` rejects older workspace formats instead of migrating them, and project bundle IO continues to route persistence through reviewed `MK_editor_core` text-store helpers. The AI operation surface exposes reviewed `editor.dock.panel.<id>.split` command rows with `source_stack_id`, `new_stack_id`, `split_axis`, and `split_ratio` parameters through dry-run-before-apply. `MK_editor_shell_common` consumes `Workspace::dock_layout()` rather than constructing an unpersisted default layout. This still does not implement shell-rendered tab headers/gutters, keyboard focus traversal UX, docking smoke counters, or multi-window drag/tear-off behavior. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; focused validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests"`; `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`.

**Phase 2d Evidence:** Candidate `codex/first-party-ui-editor-shell-tabs-focus-candidate6` adds the first visible single-window dock UX in `MK_editor_shell_common`: stack tab bars with deterministic `editor.dock.<stack>.tab.<panel>` ids, split gutters with orientation style tokens, active/focused panel style tokens, hidden workspace tab disabled state, focusable dock tab buttons through `mirakana::ui::InteractionState`, inactive panel roots excluded from renderer submission, and `FirstPartyEditorShellSmokeCounters` / `MK_editor` smoke output rows for `editor_shell_docking_status=single_window_ready`, `editor_shell_dock_tab_headers=11`, `editor_shell_dock_split_gutters=3`, `editor_shell_dock_active_panels=4`, and `editor_shell_dock_focusable_controls=11`. This remains single-window retained UI only: native drag/tear-off, multi-window docking, OS accessibility publication, and viewport/material texture display remain future phases. RED/GREEN evidence covered `MK_editor_native_shell_tests`; focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"`.

## Phase 3 - Rich Text Display And Editing v1

**Goal:** Productize first-party rich text for editor logs, AI command output, diagnostics, and future inspector/help text without adopting HTML/BBCode/Markdown as an execution surface.

**Context:** Godot-style rich text shows that document/formatting intent should be first-party, while shaping and glyph quality remain text-service work. This phase owns editor rich text semantics, not the low-level text engine.

**Done When:** Rich text supports paragraphs, spans, style tokens, inline icons/images as placeholder resource ids, links/actions as command ids, selection/copy, virtualization for log-like content, deterministic accessibility labels, and AI-visible diagnostics. It must not claim production shaping, bidi, font fallback, or rich clipboard formats.

Steps:

- [x] Add RED tests for style token validation, inline object ids, link/action ids, selection ranges, copy extraction, unsupported markup rejection, console diagnostics virtualization, and AI-visible rows.
- [x] Implement a first-party rich text model in `MK_editor_core` with stable row ids, view-model metadata, read-only console diagnostic document creation, and no markup execution surface.
- [x] Render read-only Console diagnostics through `MK_ui` labels/spans and `MK_ui_renderer` text runs until Phase 4 provides real font quality.
- [x] Add AI snapshot rows for rich text diagnostics, selected/copyable text, inline command ids, inline resource ids, and unsupported low-level capability diagnostics.
- [x] Keep clipboard output plain UTF-8 text unless a separate rich clipboard plan is accepted.
- [x] Extend the same read-only rich text composition to AI Commands status, command, evidence, and diagnostic output.
- [x] Extend the same read-only rich text composition to Inspector property rows.
- [ ] Add a first-party help rich-text surface only when the native shell has a focused help panel or help output model that needs formatted output.
- [ ] Add editable rich-text target state, caret/focus ownership, and reviewed text-edit command routing after the TSF/clipboard boundary is selected.

**Phase 3a Evidence:** Candidate `codex/first-party-ui-editor-rich-text-display-candidate7` adds `EditorRichTextViewport`, `EditorRichTextUiModel`, `EditorRichTextAiRow`, `EditorRichTextAiSnapshot`, `make_editor_console_rich_text_document`, `make_editor_rich_text_view_model`, and `make_editor_rich_text_ai_snapshot` in `MK_editor_core`; extends `EditorAiOperationSnapshot` with `rich_text_rows`; virtualizes long diagnostic documents by paragraph window; exposes copyable and selected plain UTF-8 text; emits AI rows for document, paragraph, span, inline command, and inline resource objects; and keeps shaping, bidi, font fallback, IME, accessibility publication, and rich clipboard as explicit adapter-owned diagnostics. `MK_editor_shell_common` now composes the Console panel through the core rich-text view model and submits the resulting spans through the first-party `mirakana::ui` / `MK_ui_renderer` path. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; focused validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`.

**Phase 3b Evidence:** Candidate `codex/first-party-ui-editor-rich-text-panels-candidate8` narrows the remaining formatted-panel work to AI Commands first: `MK_editor_core` adds `make_editor_ai_command_panel_rich_text_document` for read-only status, operator handoff, evidence, command, host-gate/blocker, and diagnostic paragraphs with stable `editor.rich_text.ai_commands.*` ids; the native shell composes the AI Commands panel through `editor.panel.ai_commands.rich_text`; and help rich text plus editable rich text remained later phases after that slice. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; focused validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`.

**Phase 3c Evidence:** Candidate `codex/first-party-ui-editor-inspector-rich-text-candidate9` extends the read-only rich-text panel path to Inspector property rows: `MK_editor_core` adds `make_editor_inspector_rich_text_document` for stable `editor.rich_text.inspector.paragraph.property.*` label/value/state spans, and the native shell composes Inspector through `editor.panel.inspector.rich_text`. This keeps property editing, caret/focus ownership, TSF text services, rich clipboard, and any future help rich-text surface outside this slice. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests`; focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests`; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`.

## Phase 4 - Windows DirectWrite/Direct2D Font And Text Adapter v1

**Goal:** Implement the first concrete Windows text/font quality adapter behind existing `MK_ui` and editor-shell boundaries.

**Context:** Current runtime/editor UI text evidence has value-only shaping and font-rasterization gates. Windows should use DirectWrite-class APIs behind a private adapter rather than a hand-rolled text engine.

**Done When:** Windows editor text rendering can produce real glyph metrics/bitmap or atlas rows for selected fonts, DPI scale, fallback diagnostics, zero-ink glyph handling, and deterministic adapter evidence without exposing DirectWrite/Direct2D or COM types outside implementation files.

Steps:

- [x] Recheck current Microsoft DirectWrite/Direct2D documentation before implementation and record links in phase evidence.
- [x] Add RED tests around adapter result validation using existing runtime fake-adapter coverage plus a private editor adapter RED.
- [x] Add private Win32 editor text/font adapter types under `editor/src` or a narrowly scoped platform adapter target.
- [x] Route adapter output through existing `ITextShapingAdapter` / `IFontRasterizerAdapter` result validation.
- [x] Add atlas handoff evidence that distinguishes adapter invoked, glyphs ready, fallback used, host-gated, and unsupported rows.
- [x] Update dependency/legal records only if a non-Windows-SDK dependency is selected. Windows SDK usage alone must remain a host SDK boundary, not a vcpkg UI dependency.

**Phase 4a Evidence:** Candidate `codex/first-party-ui-editor-directwrite-text-font-candidate10` adds a private Windows SDK DirectWrite text/font adapter under `editor/src/native_editor_text_font_adapters.*`, links only the Windows SDK `dwrite` library from `MK_editor_shell_common`, and keeps COM/DirectWrite types out of public engine/game/runtime UI and `editor/core` headers. The implementation follows current Microsoft guidance rechecked during this phase: DirectWrite provides high-quality Unicode text layout, glyph measurement, and low-level glyph rendering APIs ([DirectWrite portal](https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal)); Direct2D and DirectWrite are separate cooperating APIs where `IDWriteTextLayout`, `IDWriteTextRenderer::DrawGlyphRun`, and Direct2D/DirectWrite glyph rendering stay behind the renderer boundary ([Text Rendering with Direct2D and DirectWrite](https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-and-directwrite)); and `IDWriteFactory::CreateGlyphRunAnalysis` creates glyph-run analysis used for selected glyph bitmap evidence ([CreateGlyphRunAnalysis](https://learn.microsoft.com/en-us/windows/win32/api/dwrite/nf-dwrite-idwritefactory-createglyphrunanalysis)). RED/GREEN evidence: `MK_editor_native_shell_tests` first failed because `native_editor_text_font_adapters.hpp` did not exist, then passed after `make_native_editor_directwrite_text_shaping_adapter`, `make_native_editor_directwrite_font_rasterizer_adapter`, and `native_editor_directwrite_text_font_adapters_expose_native_handles` fed `shape_text_run` / `rasterize_font_glyph` validation with selected `Segoe UI` text/glyph evidence and `native_handles_public=false`. Focused validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"`. Slice-gate validation also passed: `tools/build-editor.ps1` built the `desktop-editor` lane and ran 86/86 tests, `tools/validate.ps1` returned `validate: ok`, and `git diff --check` reported no whitespace errors. This is not a broad runtime UI production text readiness claim: atlas handoff counters, Direct2D GPU text rendering, full bidi/line-breaking, production fallback, editable rich text, IME, UIA publication, and cross-platform adapters remain later phases.

**Phase 4b Evidence:** Candidate `codex/first-party-ui-editor-text-atlas-handoff-candidate11` adds private editor text atlas handoff evidence under `editor/src/native_editor_text_atlas_handoff.*`. The evidence is side-effect-free and reports row/counter separation for `adapter_invoked`, `glyphs_ready`, `fallback_used`, `host_evidence_required`, `unsupported`, and `native_handles_public=false` while routing selected Windows DirectWrite output through the existing `ITextShapingAdapter` / `IFontRasterizerAdapter` validation path. Current Microsoft guidance was rechecked through Context7 from official Microsoft Learn sources before implementation: DirectWrite `IDWriteTextLayout::Draw` uses `IDWriteTextRenderer::DrawGlyphRun` callback boundaries for glyph-run collection, `IDWriteGlyphRunAnalysis` is low-level glyph-run render evidence, and Direct2D `DrawGlyphRun` is the rendering side rather than an atlas/package upload claim. RED/GREEN evidence: `MK_editor_native_shell_tests` first failed because `native_editor_text_atlas_handoff.hpp` did not exist, then passed after `NativeEditorTextAtlasHandoffEvidence`, `make_native_editor_directwrite_text_atlas_handoff_evidence`, `record_native_text_atlas_handoff_evidence`, and smoke counter fields reported `editor_shell_text_atlas_handoff_status=glyphs_ready_atlas_handoff_host_gated`, adapter invocation, ready glyphs, fallback not used for selected Segoe UI text, one host-gated Direct2D/atlas row, one unsupported GPU-upload row, and no native handle exposure. Validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"`; `tools/check-json-contracts.ps1`; `tools/check-ai-integration.ps1`; `tools/check-agents.ps1`; `tools/check-format.ps1`; `tools/cmake.ps1 --build --preset desktop-editor --target MK_editor`; `tools/ctest.ps1 --preset desktop-editor --output-on-failure -R "MK_editor_smoke"`; `tools/build-editor.ps1` (86/86 before the fallback-key cleanup); full `tools/validate.ps1` (85/85); and `git diff --check`. This completes the selected Windows editor atlas-handoff evidence slice but still does not claim Direct2D GPU text rendering, renderer texture upload, broad shaping/bidi/line-breaking, production font fallback, editable rich text, IME, UIA publication, or cross-platform adapters.

## Phase 5 - Windows TSF IME Adapter v1

**Goal:** Replace minimal committed-text-only editor behavior with a reviewed Windows TSF-backed IME/text services adapter for the focused editor text targets.

**Context:** `MK_ui` already has text-input session and IME composition publication contracts. The missing piece is the native Windows adapter and focused text-target integration.

**Done When:** The editor can start/end text input for focused rich/text fields, publish composition text, candidate/session diagnostics, caret rect/surrounding text rows, and committed text through first-party text-edit state. Candidate UI remains host-owned; broad cross-platform IME parity remains unclaimed.

Steps:

- [x] Recheck current TSF documentation before implementation and record links in phase evidence.
- [x] Add fake-adapter tests for session lifecycle, focus changes, caret rect, surrounding text, composition update, commit, cancel, and invalid target rejection.
- [x] Add private Windows TSF adapter code without leaking COM/TSF types to public headers.
- [x] Wire editor text targets through `PlatformTextInputSession` and `ImeComposition` rows.
- [x] Add smoke counters that distinguish the selected private Windows TSF session adapter from cross-platform parity and candidate UI/reconversion support.

**Phase 5a Evidence:** Candidate `codex/first-party-ui-editor-tsf-ime-candidate12` adds a first-party value-only text-input/IME session controller under `editor/src/native_editor_text_input.*` and wires it into `NativeEditorApp`, `MK_editor_shell_common`, retained `mirakana::ui` documents, and deterministic smoke counters. Current Microsoft TSF guidance was rechecked before implementation: TSF architecture puts the TSF manager between applications and text services ([Architecture](https://learn.microsoft.com/en-us/windows/win32/tsf/architecture)); the thread manager owns activation, document-manager creation, and input-focus relationships ([Thread Manager](https://learn.microsoft.com/en-us/windows/win32/tsf/thread-manager)); `ITextStoreACP` is the application-owned text store manipulated by TSF ([ITextStoreACP](https://learn.microsoft.com/en-us/windows/win32/api/textstor/nn-textstor-itextstoreacp)); and compositions are temporary input states whose start/update/end must be reflected by the application ([Compositions](https://learn.microsoft.com/en-us/windows/win32/tsf/compositions)). Based on that official shape, this slice deliberately does not instantiate COM/TSF yet: it first gives the native shell a focused editable Project Settings text field, caret rect, surrounding text, begin/end session routing through `ui::PlatformTextInputRequest`, composition publication through `ui::ImeComposition`, commit through `ui::apply_committed_text_input`, cancel, invalid-target fail-closed behavior, service counters, and smoke rows such as `editor_shell_ime_status=value_text_input_controller_ready`, `editor_shell_ime_caret_rect_rows=1`, `editor_shell_ime_surrounding_text_rows=1`, `editor_shell_ime_candidate_ui_host_owned=1`, and `editor_shell_ime_native_handles_exposed=0`. RED/GREEN evidence covered `MK_editor_native_shell_tests` for focus changes, previous-session end, composition update, commit, cancel, read-only/invalid-caret rejection before adapter dispatch, retained text-field composition, and smoke counters. This is not the private Windows TSF COM adapter, native candidate UI, reconversion, rich-text editing, cross-platform IME parity, production shaping/bidi/fallback, UIA publication, or public native handle exposure; `win32_tsf_selected` remains reserved for the next TSF adapter slice.

**Phase 5b Evidence:** Candidate `codex/first-party-ui-editor-tsf-ime-candidate13` adds private `editor/src/native_editor_tsf_text_input.*` Windows TSF session selection and binds it through `NativeEditorWin32Services` as both the platform text-input and IME adapter with service id `win32_tsf`. Current Microsoft TSF guidance was rechecked before implementation: applications implement text access behind TSF while the TSF manager mediates services ([Architecture](https://learn.microsoft.com/en-us/windows/win32/tsf/architecture)); the thread manager activates TSF, creates document managers, and maintains focus relationships ([Thread Manager](https://learn.microsoft.com/en-us/windows/win32/tsf/thread-manager)); `ITfThreadMgr::Activate`, `ITfThreadMgr::CreateDocumentMgr`, `ITfDocumentMgr::CreateContext`, `ITfDocumentMgr::Push`, and `ITfThreadMgr::SetFocus` are the selected private session setup calls; and `ITextStoreACP` remains the application-owned text-store interface for a later callback-capable text service slice ([ITextStoreACP](https://learn.microsoft.com/en-us/windows/win32/api/textstor/nn-textstor-itextstoreacp)). This slice keeps all COM/TSF types in implementation files, exposes only first-party request rows, extends `ui::PlatformTextInputRequest` with strict-UTF-8 surrounding text plus cursor/selection byte offsets, validates scalar-boundary cursor and selection spans before adapter dispatch, creates/deactivates a TSF thread manager/document manager/context on Windows, sets TSF focus for the active document manager, reports no native handle exposure, and updates smoke rows to `editor_shell_text_input_service=win32_tsf`, `editor_shell_ime_service=win32_tsf`, and `editor_shell_ime_status=win32_tsf_selected`. RED/GREEN evidence covered `MK_ui_renderer_tests` for surrounding-text/cursor/selection validation and `MK_editor_native_shell_tests` for private TSF adapter request retention, thread/document-manager readiness, no native handles, and app-level TSF service selection. Focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests MK_editor_native_shell_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests|MK_editor_native_shell_tests"`. This still does not claim full app-owned `ITextStoreACP` callback coverage, native candidate UI, reconversion, rich-text editing, cross-platform IME parity, production shaping/bidi/fallback, UIA publication, or public native handle exposure.

## Phase 6 - Windows UI Automation Provider v1

**Goal:** Publish the custom-rendered first-party editor UI as a UI Automation provider tree on Windows.

**Context:** The current shell emits accessibility payload rows, but a custom-rendered HWND remains opaque to UI Automation until a provider maps semantic nodes to UIA elements.

**Done When:** The Windows shell exposes a provider fragment root with stable element ids, role/control type mapping, names, states, focus, screen-space bounds, hosted-root null runtime ids, child `UiaAppendRuntimeId` rows, actions, and tree navigation for the supported panel/control set. Tests prove mapping with fakes; host smoke records provider availability without claiming full accessibility parity across platforms.

Steps:

- [x] Recheck current Microsoft UI Automation provider documentation before implementation and record links in phase evidence.
- [x] Add model tests for semantic node to UIA role/name/state/action mapping.
- [x] Add private UIA provider implementation under the Win32 editor shell.
- [x] Keep provider node ids tied to `UiDocument` ids and editor-core model ids.
- [x] Add accessibility smoke counters and diagnostics for missing names, missing roles, invalid bounds, hidden nodes, and unsupported patterns.

**Phase 6 Evidence:** Candidate `codex/first-party-ui-editor-uia-candidate14` adds private `editor/src/native_editor_uia_provider.*` UI Automation publication for the Windows editor shell. Current Microsoft UI Automation guidance was rechecked before implementation: custom controls need server-side providers when they are otherwise opaque to accessibility clients ([UI Automation Providers Overview](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview)); complex custom controls should expose `IRawElementProviderSimple`, `IRawElementProviderFragment`, and `IRawElementProviderFragmentRoot` with provider tree navigation, properties, focus, bounds, and fragment roots ([Implement a Server-Side UI Automation Provider](https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-serversideprovider)); `IRawElementProviderFragment::Navigate` defines parent/child/sibling tree relationships ([Navigate](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcore/nf-uiautomationcore-irawelementproviderfragment-navigate)); and Win32 controls return providers from `WM_GETOBJECT` through `UiaReturnRawElementProvider` ([UiaReturnRawElementProvider](https://learn.microsoft.com/en-us/windows/win32/api/uiautomationcoreapi/nf-uiautomationcoreapi-uiareturnrawelementprovider)). This slice keeps all Win32/UIA/COM types in implementation files, binds a private `win32_uia` accessibility adapter through `NativeEditorWin32Services`, subclasses only the private editor HWND to answer `WM_GETOBJECT`, maps retained `AccessibilityPayload` rows to stable `UiDocument` element ids, UIA control type ids, names, enabled/focusable/focused states, screen-space bounds from `ClientToScreen`, focus actions, a hosted-root null runtime id, child `UiaAppendRuntimeId` partial ids, parent/child/sibling navigation rows, and fail-closed diagnostics for missing names, missing roles, and invalid bounds. RED/GREEN evidence covered `MK_editor_native_shell_tests`: the tests first failed because `native_editor_uia_provider.hpp` did not exist, then passed after semantic role/name/state/action mapping, diagnostics, adapter-boundary readiness, app publication, screen-space bounds, hosted-root runtime id behavior, and no-native-handle checks were implemented. Desktop smoke reports `editor_shell_accessibility_service=win32_uia`, `editor_shell_accessibility_status=uia_provider_ready`, positive node/role/name/state/action/relationship/navigation counters, one focus row, zero diagnostics, `editor_shell_accessibility_hidden_nodes=0`, `editor_shell_accessibility_unsupported_pattern_diagnostics=0`, and `editor_shell_accessibility_native_handles_exposed=0`. This completes the selected Windows editor UIA provider publication slice without claiming cross-platform accessibility parity, runtime UI OS accessibility publication, full invoke/value/toggle/range control pattern execution, live-region parity, UIA event parity, or public native handle exposure.

## Phase 7 - Viewport And Material Preview Texture Display v1

**Goal:** Replace `diagnostic_only` viewport/material-preview display with private D3D12 texture display for the current Windows editor lane.

**Context:** The current editor shell already probes D3D12 host readiness and records diagnostic-only viewport/material-preview counters. Productization now needs private offscreen targets, descriptor leases, barriers, viewport resize-safe teardown, material upload gates, and smoke evidence.

**Done When:** `MK_editor` can display a real viewport texture and material preview texture through private D3D12 paths, with no public native handles and with failure modes that fall back to explicit diagnostics.

Steps:

- [x] Add RED tests around `NativeViewportDisplayPlan` and `NativeMaterialPreviewDisplayPlan` for ready/invalid/resize/host-unavailable paths.
- [x] Implement private texture display adapter ownership under `editor/src` using existing renderer/RHI patterns without moving D3D12 types into editor core.
- [x] Add resize-safe teardown and descriptor/fence lifecycle tests or smoke diagnostics.
- [x] Require visible compositor consumption evidence before `d3d12_texture_ready`; keep prepared adapter frames in explicit pending states until the visible pass samples the private texture.
- [x] Change smoke counters only after real texture display is validated, for example from `diagnostic_only` to `d3d12_texture_ready`.
- [x] Keep Vulkan/Metal editor texture display parity host-gated.

**Phase 7a Evidence:** Candidate `codex/first-party-ui-editor-viewport-texture-candidate15` adds value-only private D3D12 texture-display lifecycle gates to `NativeViewportDisplayDesc` / `NativeViewportDisplayPlan` and `NativeMaterialPreviewDisplayDesc` / `NativeMaterialPreviewDisplayPlan` without exposing native handles or moving D3D12 types into `editor/core`. The new plan rows distinguish `texture_adapter_unavailable`, `offscreen_target_unavailable`, `descriptor_lease_unavailable`, `barrier_lifecycle_missing`, `fence_lifecycle_missing`, `resize_recreate_required`, and `d3d12_texture_ready` from the existing visible-shell `diagnostic_only` path. RED/GREEN evidence covered `MK_editor_native_shell_tests`: zero-extent rejection before readiness, viewport resize-safe teardown gating, viewport private texture readiness, material-preview descriptor/fence gating, material-preview private texture readiness, host-unavailable and shader/payload failure paths, no native handle exposure, and a ready material execution snapshot with `executes=false`, `frames_rendered=0`, and `display_path_label=host-private-native`. This slice follows the Microsoft D3D12 guidance rechecked for descriptor heaps, resource barriers, and fence/command-list lifecycles, but it intentionally does not promote `MK_editor` smoke counters yet because the shell has not bound real offscreen targets into the UI presentation path. Focused validation passed: `tools/cmake.ps1 --preset dev`; `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"`.

**Phase 7b Evidence:** Candidate `codex/first-party-ui-editor-viewport-texture-candidate16` adds private `editor/src/native_texture_display_adapter.*` as the first RHI-backed texture-display adapter evidence under `MK_editor_shell_common`. It uses the existing `RhiViewportSurface` offscreen target path, backend-neutral sampled-texture descriptor set updates, resource transitions, and fence waits, keeps D3D12/native handles out of `editor/core` and public headers, and lets material-preview execution snapshots report real host-private frames rendered only when the adapter prepared a frame. `MK_editor_native_shell_tests` cover viewport descriptor/resource-barrier/fence evidence, no texture-buffer copy/readback fallback, resize wait-before-replacement evidence, and material-preview `host-private-native` execution evidence. The visible shell still reports `editor_shell_viewport_status=diagnostic_only` and `editor_shell_material_preview_status=diagnostic_only`; smoke promotion remains blocked until the UI presentation path consumes the real texture. Validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"`; `tools/check-tidy.ps1 -Files editor/src/native_texture_display_adapter.cpp,editor/src/native_material_preview_cache.cpp,tests/unit/editor_native_shell_tests.cpp -ReuseExistingFileApiReply`; `tools/check-format.ps1`; `tools/check-json-contracts.ps1`; `tools/check-ai-integration.ps1`; `tools/check-agents.ps1`; full `tools/validate.ps1` (85/85); and `tools/build-editor.ps1` (86/86).

**Phase 7c Evidence:** Candidate `codex/first-party-ui-editor-visible-texture-candidate17` tightens the private texture-display contract so RHI offscreen preparation, descriptor updates, resource barriers, and fence waits are no longer enough to report `d3d12_texture_ready`. `NativeViewportDisplayDesc` / `NativeViewportDisplayPlan` and `NativeMaterialPreviewDisplayDesc` / `NativeMaterialPreviewDisplayPlan` now carry `visible_panel_available`, `visible_texture_composite_recorded`, and `visible_texture_composites` rows. Viewport plans remain `visible_composite_pending` until a visible compositor pass samples the private texture; material-preview plans remain `visible_panel_unavailable` while the current visible shell lacks a dedicated material preview panel. `native_texture_display_adapter.*` remains a producer of private RHI preparation evidence and does not expose GPU/native handles or promote visible smoke counters. RED/GREEN evidence covered `MK_editor_native_shell_tests`: prepared viewport adapter frames staying pending until visible composite evidence exists, material-preview readiness blocked by missing visible panel, and ready states requiring explicit visible composite rows. Focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"`.

**Phase 7d Evidence:** Candidate `codex/first-party-ui-editor-visible-compositor-candidate18` adds private `editor/src/native_editor_visible_texture_compositor.*` and the editor HLSL shader pair for the current Windows D3D12 visible path. The Win32 shell now creates one private D3D12 RHI device for the offscreen viewport/material adapters, visible compositor, and editor swapchain, compiles DXIL through the `desktop-editor` lane, samples the private texture through cached SRV/sampler/pipeline resources, records a full-screen triangle into the swapchain render pass, presents, submits, and waits before promoting `NativeViewportDisplayPlan` / `NativeMaterialPreviewDisplayPlan` to `d3d12_texture_ready`. Smoke counters now include `editor_shell_viewport_status=d3d12_texture_ready`, `editor_shell_viewport_visible_texture_composites`, `editor_shell_material_preview_status=d3d12_texture_ready`, and `editor_shell_material_preview_visible_texture_composites`, while native texture handles remain private and public editor/game/runtime UI APIs still expose no Win32/DXGI/D3D12 handles. The RED step failed as expected on the missing `native_editor_visible_texture_compositor.hpp`; GREEN evidence covered `MK_editor_native_shell_tests` for viewport and material-preview visible compositor readiness, no-native-handle policy, descriptor bind/render-pass/draw/present/fence counters, and smoke-counter propagation. Focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"`; and `tools/build-editor.ps1` with 86/86 passing, including `MK_editor_smoke`. This slice uses the Microsoft D3D12 sequence rechecked for descriptor heaps, sampler/SRV binding, resource barriers, command recording, presentation, and fence waits, and it does not claim Vulkan/Metal editor texture-display parity, public native handles, material-preview GPU parity beyond the current host-private D3D12 preview display, cross-platform editor shells, or broad renderer quality.

## Phase 8 - AI-Operable Editor UX Expansion

**Goal:** Ensure every productized UI feature is inspectable and operable by AI through first-party contracts, not screen automation.

**Context:** Current AI command rows are narrow panel visibility commands. Docking, rich text, text targets, Windows UIA accessibility state, and viewport/material diagnostics need the same inspect/dry-run/apply discipline.

**Done When:** AI can inspect dock layout, rich text state, focused text target, IME/accessibility/font adapter status, viewport/material display readiness, command availability, dry-run results, apply results, and diagnostics without native handles, screen coordinates, or shell command execution.

Steps:

- [x] Extend `EditorAiOperationSnapshot` to include selected dock, rich text, text input, adapter status, Windows UIA accessibility state, and viewport/material display rows.
- [x] Extend command catalog/dry-run/apply for reviewed dock and rich-text operations only.
- [x] Add rejection tests for arbitrary command ids, stale revisions, target mismatches, missing confirmation, native handles, shell execution, and validation-recipe execution from editor core.
- [x] Update manifest guidance and static checks with exact AI-operable row names.

**Phase 8 Evidence:** Candidate `codex/first-party-ui-editor-ai-ux-candidate19` adds GUI-independent `EditorAiOperationStatusRow` / `EditorAiOperationUxStatusDesc` rows plus `make_editor_ai_operation_ux_status_rows`, extends `EditorAiOperationSnapshot` with `status_rows`, and wires the first-party shell through `make_first_party_editor_ai_operation_snapshot`. The exact AI-visible UX rows are `editor.ai.dock.selected_panel`, `editor.ai.rich_text.documents`, `editor.ai.text_input.focused_target`, `editor.ai.adapter.text_font`, `editor.ai.ime.session`, `editor.ai.accessibility.uia_provider`, `editor.ai.viewport.display`, and `editor.ai.material_preview.display`; native handle exposure on any row emits `native_handle_unsupported`. The reviewed rich-text command ids are `<rich_text_document_id>.copy_plain_text` and `<rich_text_document_id>.copy_selection_plain_text`, returning `text/plain;charset=utf-8` output through dry-run/apply without mutating state. `EditorAiCommandRequest::expected_revision` rejects stale catalogs, and core command validation now fails closed for native handles, shell/process execution, validation-recipe execution, screen coordinates, target mismatches, missing confirmation, unsupported parameters, disabled commands, and unknown command ids. RED/GREEN evidence covered `MK_editor_core_tests` and `MK_editor_native_shell_tests` for UX status rows, shell-projected native readiness rows, rich-text copy commands, stale revision rejection, native handle rejection, shell execution rejection, and validation recipe rejection without exposing Win32/D3D12/DirectWrite/TSF/UIA handles.

## Phase 9 - Cross-Platform Adapter Boundary Review

**Goal:** Keep future macOS/Linux/mobile editor or runtime UI adapters honest without implementing broad cross-platform parity in this Windows-focused milestone.

**Context:** The current engine is Windows-primary for visible editor validation. Cross-platform UI needs official SDK choices and host gates, not inferred readiness from Windows.

**Done When:** Docs/manifest/static checks list future adapter families and host gates clearly, and the Windows implementation does not accidentally claim cross-platform text/font/IME/accessibility/editor parity.

Steps:

- [x] Add or update host-gated rows for CoreText/NSAccessibility, AT-SPI/ibus/Fcitx, Android, iOS, and optional HarfBuzz/FreeType/ICU-class dependency adapters only as future gates.
- [x] Verify generated games and runtime UI guidance still do not depend on editor-private APIs.
- [x] Verify dependency/legal docs still require `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md` before third-party text/font/image dependencies are introduced.

**Phase 9 Evidence:** Candidate `codex/first-party-ui-editor-adapter-gates-candidate20` records the cross-platform adapter boundary as docs/manifest/static-check truth only; it does not add macOS, Linux, Android, iOS, HarfBuzz, FreeType, or ICU implementation code and does not promote Windows DirectWrite/TSF/UIA evidence into cross-platform readiness. Official adapter guidance was rechecked before this gate update: Apple Core Text remains the low-level text layout/font boundary (<https://developer.apple.com/documentation/coretext>), Apple InputMethodKit remains the macOS input-method boundary (<https://developer.apple.com/documentation/inputmethodkit>), Apple NSAccessibility/UIAccessibility remain accessibility bridge boundaries (<https://developer.apple.com/documentation/appkit/nsaccessibilityprotocol> and <https://developer.apple.com/documentation/uikit/uiaccessibility>), Apple UITextInput is the iOS text-input boundary (<https://developer.apple.com/documentation/uikit/uitextinput>), Android InputMethodService and Android accessibility docs define Android IME/accessibility host boundaries (<https://developer.android.com/reference/android/inputmethodservice/InputMethodService> and <https://developer.android.com/guide/topics/ui/accessibility>), GNOME AT-SPI is the Linux accessibility bridge boundary (<https://gnome.pages.gitlab.gnome.org/at-spi2-core/libatspi/index.html>), IBus/Fcitx are Linux input-method framework boundaries (<https://ibus.github.io/docs/ibus-1.5/index.html> and <https://fcitx-im.org/wiki/Fcitx_5/en>), HarfBuzz is a shaping dependency candidate (<https://harfbuzz.github.io/shaping-concepts.html>), FreeType is a glyph/font rasterization dependency candidate (<https://freetype.org/freetype2/docs/index.html>), and ICU is a Unicode boundary/line-break/text-processing dependency candidate (<https://unicode-org.github.io/icu/userguide/boundaryanalysis/>). The durable future-gate row names are `editor.cross_platform.adapter.macos.core_text`, `editor.cross_platform.adapter.macos.input_method_kit`, `editor.cross_platform.adapter.macos.nsaccessibility`, `editor.cross_platform.adapter.linux.at_spi`, `editor.cross_platform.adapter.linux.ibus`, `editor.cross_platform.adapter.linux.fcitx`, `editor.cross_platform.adapter.android.input_method_service`, `editor.cross_platform.adapter.android.accessibility`, `editor.cross_platform.adapter.ios.uitextinput`, `editor.cross_platform.adapter.ios.uiaccessibility`, `editor.cross_platform.adapter.dependency.harfbuzz`, `editor.cross_platform.adapter.dependency.freetype`, and `editor.cross_platform.adapter.dependency.icu`; every one is future host-gated or dependency-gated. Generated games and runtime UI remain on public `mirakana::ui` contracts and must not depend on editor-private APIs. Dependency/legal policy was verified: third-party text/font/image dependencies still require `license-audit`, `vcpkg.json`, `docs/dependencies.md`, and `THIRD_PARTY_NOTICES.md` updates before use.

## Phase 10 - Full Validation And Closeout

**Goal:** Close the milestone only when implemented behavior, docs, manifest, static checks, and validation evidence agree.

**Done When:** Focused tests, `desktop-editor`, static checks, and full validation pass; unsupported claims remain explicit; the manifest either selects the next plan or returns to `next-production-gap-selection`.

Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests MK_ui_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests|MK_ui_renderer_tests|MK_editor_smoke"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-native-desktop-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

## Acceptance Criteria

- `MK_editor_core` owns durable docking/rich-text/text-target/AI operation state; native shell code only adapts and renders it.
- `MK_editor` remains dependency-free of UI middleware and continues through `desktop-editor`.
- Docking v1 is usable, persisted, keyboard/focus-aware, AI-operable, and single-window ready without claiming full multi-window parity.
- Rich text v1 is first-party, semantic, selectable/copyable, AI-readable, and safe against arbitrary markup execution.
- Windows text/font quality goes through private DirectWrite/Direct2D-class adapters; no platform SDK types leak to public headers.
- Windows IME goes through private TSF-class adapter rows; committed text and composition state flow through `MK_ui` text contracts.
- Windows accessibility publication goes through a private UIA provider tree mapped from first-party semantic rows.
- Viewport/material preview status changes from `diagnostic_only` to `d3d12_texture_ready` only after real private D3D12 texture display is validated.
- AI can inspect and operate the supported editor UI through snapshots/catalog/dry-run/apply results without native UI automation, screen coordinates, arbitrary shell, or UI middleware automation.
- Manifest, docs, skills, schemas/static checks, dependency/legal records, smoke counters, and validation recipes match the implemented scope.

## Self-Review Checklist

- [ ] Did this phase remove or replace stale private contracts cleanly instead of adding aliases?
- [ ] Are all new editor UI states inspectable without launching the native shell?
- [ ] Are AI command ids stable, reviewed, dry-runnable, and revision-checked?
- [ ] Are native handles and platform SDK types kept out of public headers and editor core?
- [ ] Are DirectWrite/TSF/UIA responsibilities private adapters, not panel logic?
- [ ] Do docs and manifest avoid broad cross-platform or broad production UI claims?
- [ ] Did dependency/legal records change exactly with any selected third-party dependency?
- [ ] Did focused validation run before full validation?
