# 2026-06-12 First-Party UI Runtime And Editor Production Roadmap v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan phase-by-phase. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `MK_ui` the production runtime/editor UI foundation by adding first-party widgets, binding, authoring, renderer execution, text/font, IME, accessibility, and editor authoring layers while keeping mature low-level platform work behind official-SDK or audited-dependency adapters.

**Architecture:** Own UI documents, widgets, layout, focus, command routing, authoring documents, semantic accessibility rows, package counters, and editor models as first-party C++23 contracts. Keep text shaping, font rasterization, IME, OS accessibility publication, image decoding, renderer uploads, and optional middleware behind narrow adapters that never leak native handles, middleware object models, or platform SDK types into public game/runtime/editor-core APIs.

**Tech Stack:** C++23, `MK_ui`, `MK_ui_renderer`, `MK_editor_core`, `MK_editor`, `MK_tools`, `MK_runtime`, PowerShell 7 repository wrappers, CMake/CTest, Windows DirectWrite/TSF/UIA adapters first where host evidence exists. HarfBuzz/FreeType/ICU/CoreText/NSAccessibility/AT-SPI/Android/iOS adapters remain unimplemented until a separate selected plan completes dependency, legal, and host gates.

---

**Plan ID:** `first-party-ui-runtime-editor-production-roadmap-v1`

**Status:** Candidate implementation milestone. This plan does not change `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` until an operator explicitly selects it.

**Date:** 2026-06-12

## Context7 Status

Context7 was invoked on 2026-06-12 for Unity, Unreal Engine, and HarfBuzz documentation. The tool returned `Invalid or expired OAuth token. Please re-authenticate to obtain a new token.` No Context7 result is used as evidence in this plan. Official vendor documentation and repository-local current-truth docs are the evidence sources for this plan.

## Official Practice Sources

- Unity UI systems: Unity documents `UI Toolkit`, `uGUI`, and legacy `IMGUI`; `UI Toolkit` supports Editor extensions and runtime UI, uses UXML/USS, UI Builder, data binding, retained-mode visual trees, and a renderer integrated with Unity graphics. Source: <https://docs.unity3d.com/6000.4/Documentation/Manual/UIToolkits.html> and <https://docs.unity3d.com/6000.4/Documentation/Manual/UIElements.html>.
- Unreal UI systems: Unreal documents Slate, UMG, and Common UI. Slate is the custom cross-platform UI framework; UMG supplies Widget Blueprints and designer-facing runtime UI; Common UI adds multilayer, input-routing, controller-icon, and gamepad-navigation support. Source: <https://dev.epicgames.com/documentation/unreal-engine/creating-user-interfaces-with-umg-and-slate-in-unreal-engine>, <https://dev.epicgames.com/documentation/unreal-engine/slate-overview-for-unreal-engine>, and <https://dev.epicgames.com/documentation/unreal-engine/common-ui-plugin-for-advanced-user-interfaces-in-unreal-engine>.
- HarfBuzz: HarfBuzz is the text shaping library route for converting Unicode input into positioned glyph output and preserving shaping clusters. Source: <https://harfbuzz.github.io/> and <https://harfbuzz.github.io/clusters.html>.
- FreeType: FreeType documents glyph metrics, pen positions, horizontal/vertical metrics, and glyph retrieval/rasterization concepts. Source: <https://freetype.org/freetype2/docs/glyphs/glyphs-3.html> and <https://freetype.org/freetype2/docs/reference/ft2-glyph_retrieval.html>.
- Microsoft DirectWrite: DirectWrite is the Windows API family for high-quality text layout and glyph rendering. Source: <https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal>.
- Microsoft TSF: TSF is the Windows framework for advanced text input and language technologies. Source: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>.
- Microsoft UI Automation: UIA is the Windows accessibility framework and provider model for exposing UI elements to assistive technology and automation. Source: <https://learn.microsoft.com/en-us/windows/win32/winauto/entry-uiauto-win32> and <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview>.
- Apple Core Text, NSAccessibility, and UITextInput: Core Text covers low-level text layout/font operations; NSAccessibilityProtocol and UITextInput cover AppKit accessibility and UIKit text input. Source: <https://developer.apple.com/documentation/coretext/>, <https://developer.apple.com/documentation/appkit/nsaccessibilityprotocol>, and <https://developer.apple.com/documentation/uikit/uitextinput>.
- Android and Linux accessibility: Android custom views must provide accessibility events and platform-compatible behavior; AT-SPI is the Linux accessibility stack used by GUI toolkits and screen readers. Source: <https://developer.android.com/develop/ui/views/layout/custom-views/create-view>, <https://developer.android.com/guide/topics/ui/accessibility/composables>, and <https://gnome.pages.gitlab.gnome.org/at-spi2-core/devel-docs/architecture.html>.

## Repository Baseline

- `MK_ui` is `implemented-production-runtime-ui-workbench`, but current docs define it as headless and contract-first; it owns deterministic document/layout/focus/binding/adapter rows but not production text engines, OS accessibility publication, broad image codecs, or platform SDK execution.
- `MK_ui_renderer` is `implemented-runtime-ui-font-image-adapter`; it converts `MK_ui` renderer submissions to renderer sprite commands and consumes cooked image/glyph atlas metadata, while texture upload and runtime residency stay outside `MK_ui`.
- `MK_editor_core` is implemented and GUI-independent. It owns project/workspace/authoring/history/dock/rich-text/AI-operation models.
- `MK_editor` is `optional-native-win32-shell-active`; it renders eleven Windows-native first-party panels through private Win32/D3D12/DirectWrite/TSF/UIA implementation code. It is not a runtime game dependency.
- UI middleware remains an optional adapter candidate only. Qt, NoesisGUI, Slint, RmlUi, Dear ImGui, SDL3 UI, and native handles must not become public `mirakana::ui`, generated-game, or `MK_editor_core` APIs.

## Non-Negotiable Decisions

1. `mirakana::ui` is the public runtime game UI contract.
2. `mirakana::editor` editor models remain GUI-independent in `MK_editor_core`.
3. `MK_editor` is an optional native developer shell and never a runtime dependency.
4. Low-level text/font/IME/accessibility/image/platform work uses official SDK or audited dependency adapters; it is not hand-rolled inside widgets or editor panels.
5. Middleware adapters are outside the default product path until a specific feature plan selects one and updates `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md`.
6. No public API may expose `HWND`, `HANDLE`, COM pointers, DirectWrite/TSF/UIA interfaces, D3D12/DXGI/Vulkan/Metal handles, SDL3, Dear ImGui, Qt, NoesisGUI, Slint, RmlUi, or third-party UI object models.

## Target Architecture

```text
MK_ui
  Owns public runtime UI documents, widgets, style/theme tokens,
  layout, focus, navigation, data binding, commands, semantic rows,
  first-party authoring documents, validation, and deterministic tests.

MK_ui_renderer
  Converts MK_ui renderer submissions into renderer sprite commands,
  owns first-party batching/clipping/layer review rows, and consumes
  cooked atlas metadata plus host-owned upload/residency evidence.

MK_platform_* / audited dependency adapters
  Own text shaping, font rasterization, IME, accessibility publication,
  clipboard, image decoding, virtual keyboard, and OS-specific bridges.

MK_editor_core
  Owns editor UI models, inspector/tree/editor authoring documents,
  UI Builder-style state, command rows, AI snapshots, persistence,
  and validation without native handles or platform SDK types.

MK_editor
  Renders editor-core models through private native shell adapters.
```

## Capability Gap Matrix

| Gap | Required first-party work | Adapter work allowed | Completion evidence |
| --- | --- | --- | --- |
| Runtime widgets | Button, toggle, slider, text field, menu stack, modal, list/tree, HUD prompt rows, state and command contracts | None for logic | `MK_runtime_ui_widgets_tests`, package counters, docs/manifest rows |
| Binding and input routing | Typed binding rows, command enablement, focus scopes, gamepad navigation, pointer capture | Platform input translation only | `MK_runtime_ui_binding_tests`, package smoke with keyboard/gamepad navigation counters |
| Authoring format | `GameEngine.UiDocument.v1`, `GameEngine.UiTheme.v1`, deterministic text IO, validation diagnostics | Optional import/export tools after legal gate | `MK_runtime_ui_authoring_tests`, sample package authored UI files |
| Renderer execution | Layers, clipping, scissor/mask review, batching, z-order, modal composition, atlas handoff | Host-owned upload/residency and backend execution | `MK_ui_renderer_tests`, selected D3D12 package smoke, no native handle exposure |
| Text/font production | First-party request/result rows, fallback policy, atlas policy, replay hashes | DirectWrite first; HarfBuzz/FreeType/ICU/CoreText after gates | Focused adapter tests and package counters that distinguish host/dependency gates |
| IME/text editing | Text edit state, selection, surrounding text, composition rows, candidate intent rows | TSF first; Cocoa/IBus/Fcitx/Android/iOS are excluded until separate selected adapter plans pass gates | TSF host tests, package counters, no broad parity claim |
| Accessibility | Semantic tree, role/name/state/focus/action/relationship rows, reading order validation | UIA first; NSAccessibility/AT-SPI/Android/iOS are excluded until separate selected adapter plans pass gates | UIA provider tests, accessibility smoke counters, no cross-platform inference |
| Editor UI Builder | GUI-independent visual authoring state, hierarchy, inspector, style editor, preview model | Native shell rendering only | `MK_editor_core_tests`, `MK_editor_native_shell_tests`, `tools/build-editor.ps1` |
| Optional middleware | Adapter selection record and legal/dependency gate | NoesisGUI/RmlUi/Qt/Slint only behind adapters | Separate plan, dependency notices, validation recipe |

## Files

- Create: `engine/ui/include/mirakana/ui/runtime_ui_widgets.hpp`
- Create: `engine/ui/src/runtime_ui_widgets.cpp`
- Create: `engine/ui/include/mirakana/ui/runtime_ui_binding.hpp`
- Create: `engine/ui/src/runtime_ui_binding.cpp`
- Create: `engine/ui/include/mirakana/ui/runtime_ui_authoring.hpp`
- Create: `engine/ui/src/runtime_ui_authoring.cpp`
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Modify: `engine/ui/CMakeLists.txt`
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`
- Modify: `engine/ui_renderer/CMakeLists.txt`
- Create: `tests/unit/runtime_ui_widgets_tests.cpp`
- Create: `tests/unit/runtime_ui_binding_tests.cpp`
- Create: `tests/unit/runtime_ui_authoring_tests.cpp`
- Modify: `tests/unit/runtime_ui_workbench_tests.cpp`
- Modify: `tests/unit/runtime_ui_production_stack_tests.cpp`
- Modify: `tests/unit/ui_renderer_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Create: `games/sample_2d_desktop_runtime_package/runtime/ui/main_menu.uidoc`
- Create: `games/sample_2d_desktop_runtime_package/runtime/ui/default.uitheme`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `editor/core/include/mirakana/editor/ui_model.hpp`
- Modify: `editor/core/src/ui_model.cpp`
- Create: `editor/core/include/mirakana/editor/ui_authoring.hpp`
- Create: `editor/core/src/ui_authoring.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `docs/ui.md`
- Modify: `docs/editor.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/006-runtimeBackendReadiness.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Generate: `engine/agent/manifest.json`

## Phase 0 - Select The Plan And Preserve Current Claims

**Goal:** Make this plan discoverable without claiming new runtime/editor behavior.

**Steps:**

- [ ] Add this plan to `docs/superpowers/plans/README.md` as a candidate UI production milestone.
- [ ] Do not change `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` in Phase 0.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
git diff --check
```

**Expected:** All checks pass. If an active-plan pointer check fails because this plan is not selected, remove the registry pointer rather than changing `currentActivePlan`.

## Phase 1 - Runtime Widget Vocabulary v1

**Goal:** Add first-party widgets that game UI can use without middleware.

**Public contract:** `RuntimeUiWidgetKind`, `RuntimeUiWidgetRow`, `RuntimeUiWidgetStateRow`, `RuntimeUiWidgetCommandRow`, `RuntimeUiWidgetPlan`, and `plan_runtime_ui_widgets`.

**Required widgets:** `button`, `toggle`, `slider`, `text_field`, `menu_stack`, `modal_layer`, `list`, `tree`, `hud_prompt`, `controller_glyph`.

**Steps:**

- [ ] Add RED tests in `tests/unit/runtime_ui_widgets_tests.cpp` for duplicate ids, missing labels, invalid slider ranges, command rows that target missing widgets, native/middleware token rejection, and controller glyph rows without an input-source id.
- [ ] Add the public types in `engine/ui/include/mirakana/ui/runtime_ui_widgets.hpp`.
- [ ] Implement validation in `engine/ui/src/runtime_ui_widgets.cpp`.
- [ ] Register the new source/test target in `engine/ui/CMakeLists.txt`.
- [ ] Add selected package counters in `games/sample_2d_desktop_runtime_package/main.cpp`: `runtime_ui_widgets_status`, `runtime_ui_widgets_ready`, `runtime_ui_widget_rows`, `runtime_ui_widget_command_rows`, `runtime_ui_widget_focusable_rows`, `runtime_ui_widget_diagnostics`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_widgets_tests sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_widgets_tests"
```

**Expected:** Tests pass and invalid requests fail closed with deterministic diagnostics.

## Phase 2 - Binding, Focus, And Input Routing v1

**Goal:** Make widgets useful in real game menus and HUDs by adding typed binding and input routing.

**Public contract:** `RuntimeUiBindingDocument`, `RuntimeUiBindingRow`, `RuntimeUiCommandAvailabilityRow`, `RuntimeUiFocusScope`, `RuntimeUiNavigationEdge`, `RuntimeUiInputRoutingPlan`, `plan_runtime_ui_binding`, and `plan_runtime_ui_input_routing`.

**Steps:**

- [ ] Add RED tests in `tests/unit/runtime_ui_binding_tests.cpp` for missing binding keys, type mismatches, disabled command invocation, navigation cycles, modal focus escape, unknown controller glyph refs, and pointer capture conflicts.
- [ ] Add `runtime_ui_binding.hpp` / `runtime_ui_binding.cpp`.
- [ ] Connect binding output to `UiDocument::set_text`, enabled state, visibility, command availability, and focus rows without executing gameplay commands.
- [ ] Add package counters: `runtime_ui_binding_rows`, `runtime_ui_command_rows`, `runtime_ui_focus_scopes`, `runtime_ui_navigation_edges`, `runtime_ui_input_routing_ready`.
- [ ] Run focused tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_binding_tests sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_binding_tests"
```

## Phase 3 - First-Party UI Authoring Documents v1

**Goal:** Add deterministic authoring files equivalent in role to Unity UXML/USS, without adopting web/CSS or middleware semantics.

**File formats:**

- `GameEngine.UiDocument.v1`: line-based UI hierarchy, widget rows, binding ids, localization keys, semantic roles, and style-token refs.
- `GameEngine.UiTheme.v1`: line-based color, spacing, typography-token, border, opacity, transition, and controller-glyph-token rows.

**Steps:**

- [ ] Add RED tests in `tests/unit/runtime_ui_authoring_tests.cpp` for valid parse/save round-trip, duplicate element ids, unknown parent ids, invalid theme tokens, unsafe path/native/middleware tokens, missing localization keys, missing accessibility names, and unknown widget ids.
- [ ] Add `runtime_ui_authoring.hpp` / `runtime_ui_authoring.cpp`.
- [ ] Add sample files `runtime/ui/main_menu.uidoc` and `runtime/ui/default.uitheme`.
- [ ] Add package manifest entries in `games/sample_2d_desktop_runtime_package/game.agent.json` for the two runtime UI files.
- [ ] Add docs in `docs/ui.md` naming the exact format ids and non-goals.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_authoring_tests sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_authoring_tests"
```

## Phase 4 - UI Renderer Execution v1

**Goal:** Promote `MK_ui_renderer` from adapter/handoff proof toward production game UI rendering primitives.

**Renderer contract:** Layer order, modal order, clipping rects, scissor rows, mask review rows, batch grouping rows, atlas residency refs, and unresolved-resource diagnostics.

**Steps:**

- [ ] Add RED tests in `tests/unit/ui_renderer_tests.cpp` for layer sorting, modal top-most ordering, clipping bounds, unresolved glyph/image resources, invalid atlas residency refs, native handle rejection, and deterministic batch grouping.
- [ ] Extend `UiRendererSubmission` and `submit_ui_renderer_submission` in `engine/ui_renderer`.
- [ ] Add package smoke counters: `ui_renderer_layers`, `ui_renderer_batches`, `ui_renderer_clip_rects`, `ui_renderer_unresolved_resources`, `ui_renderer_native_handles_exposed=0`.
- [ ] Keep texture upload host-owned and outside `mirakana::ui`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_ui_renderer_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

## Phase 5 - Windows Text And Font Adapter v1

**Goal:** Use official Windows text/font technology behind first-party adapter contracts.

**Selected platform:** Windows DirectWrite only.

**Out of scope for this phase:** HarfBuzz, FreeType, ICU, CoreText, Linux, Android, iOS, broad font fallback parity, and runtime UI OS accessibility publication.

**Steps:**

- [ ] Add RED tests for a private Windows DirectWrite adapter test target that returns glyph ids, clusters, advances, offsets, fallback rows, glyph bitmap metrics, and atlas allocation evidence without exposing DirectWrite interfaces.
- [ ] Implement the private adapter under `engine/platform/win32/src/`.
- [ ] Keep public headers first-party; platform SDK includes stay in `.cpp` files or private implementation headers.
- [ ] Add manifest/docs rows that say this is Windows host evidence only.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_adapter_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_ui_text_adapter_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

## Phase 6 - Runtime Text Editing And IME v1

**Goal:** Make runtime text fields validate committed text, selection, composition, caret rect, surrounding text, and candidate intent rows.

**Selected platform:** Windows TSF host evidence first.

**Steps:**

- [ ] Add tests for `RuntimeUiTextEditDocument`, selection ranges, grapheme-safe cursor movement rows, committed-text validation, composition update rows, candidate intent rows, and fail-closed unsupported native candidate UI claims.
- [ ] Route Windows TSF evidence through private adapter rows without exposing TSF interfaces.
- [ ] Add package counters: `runtime_ui_text_edit_ready`, `runtime_ui_ime_composition_rows`, `runtime_ui_ime_candidate_intent_rows`, `runtime_ui_ime_native_candidate_ui_ready=0`, `runtime_ui_ime_native_handles_exposed=0`.
- [ ] Run focused tests and `tools/build-editor.ps1` if editor shell text input is touched.

## Phase 7 - Runtime Accessibility Publication v1

**Goal:** Publish runtime UI accessibility through first-party semantic rows and selected Windows UIA provider evidence.

**Selected platform:** Windows UI Automation provider proof only.

**Steps:**

- [ ] Add tests for role/name/state/focus/action/relationship/live-region/reading-order rows.
- [ ] Add Windows UIA provider host evidence behind private adapter code.
- [ ] Add package counters: `runtime_ui_accessibility_nodes`, `runtime_ui_accessibility_role_rows`, `runtime_ui_accessibility_name_rows`, `runtime_ui_accessibility_action_rows`, `runtime_ui_accessibility_uia_provider_ready`, `runtime_ui_accessibility_cross_platform_ready=0`.
- [ ] Run focused UIA provider tests, public API boundary check, and package smoke.

## Phase 8 - Editor UI Builder Foundation v1

**Goal:** Add a GUI-independent UI authoring model in `MK_editor_core` that can inspect and edit `GameEngine.UiDocument.v1` and `GameEngine.UiTheme.v1`.

**Editor-core contract:** `EditorUiAuthoringDocument`, `EditorUiHierarchyModel`, `EditorUiInspectorModel`, `EditorUiStylePanelModel`, `EditorUiPreviewModel`, and reviewed command rows for add/remove/reorder/select/style-edit operations.

**Steps:**

- [ ] Add RED tests in `tests/unit/editor_core_tests.cpp` for hierarchy rows, inspector rows, selected element state, style token edits, undoable document edits, invalid authoring file rejection, and no native handle exposure.
- [ ] Add `editor/core/include/mirakana/editor/ui_authoring.hpp` and `editor/core/src/ui_authoring.cpp`.
- [ ] Extend `editor/core/include/mirakana/editor/ui_model.hpp` and `editor/core/src/ui_model.cpp` only for shared retained model helpers.
- [ ] Add visible shell rows in `tests/unit/editor_native_shell_tests.cpp` only after editor-core rows are green.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

## Phase 9 - Cross-Platform Adapter Gate Records v1

**Goal:** Record exact unselected adapter gates without implementing unsupported platform claims.

**Gate rows:**

- `runtime_ui.adapter.windows.directwrite`
- `runtime_ui.adapter.windows.tsf`
- `runtime_ui.adapter.windows.uia`
- `runtime_ui.adapter.cross_platform.harfbuzz`
- `runtime_ui.adapter.cross_platform.freetype`
- `runtime_ui.adapter.cross_platform.icu`
- `runtime_ui.adapter.macos.core_text`
- `runtime_ui.adapter.macos.nsaccessibility`
- `runtime_ui.adapter.linux.at_spi`
- `runtime_ui.adapter.android.input_method_service`
- `runtime_ui.adapter.android.accessibility`
- `runtime_ui.adapter.ios.uitextinput`
- `runtime_ui.adapter.ios.uiaccessibility`

**Steps:**

- [ ] Add rows to manifest fragments and docs as host-gated/dependency-gated.
- [ ] Do not add vcpkg dependencies in this phase.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

## Phase 10 - Optional Middleware Adapter Decision Gate v1

**Goal:** Keep middleware out of the core while defining the exact process required before any middleware can be adopted.

**Rule:** No middleware implementation occurs in this roadmap unless a separate dated plan selects one adapter and passes legal/dependency gates.

**Allowed adapter IDs for separate selected plans:**

- `runtime_ui.middleware.noesisgui_adapter`
- `runtime_ui.middleware.rmlui_adapter`
- `editor_tooling.middleware.qt_tool_adapter`
- `editor_tooling.middleware.slint_tool_adapter`

**Steps:**

- [ ] Add docs that these ids are not implemented and are not public API.
- [ ] Add static checks rejecting middleware tokens in `engine/ui/include`, `editor/core/include`, and generated game code.
- [ ] If a middleware adapter is selected by a separate dated implementation plan, run `license-audit`, update dependency docs and notices, add a vcpkg feature or external SDK host gate, and keep the adapter outside public `mirakana::ui`, generated-game, and `MK_editor_core` APIs.

## Phase 11 - Closeout And Validation

**Goal:** Close the selected milestone only after implementation, docs, manifest, validation recipes, and non-claims match evidence.

**Required checks for C++/runtime/editor behavior phases:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

**Required additional checks when optional dependencies are added:**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

**Done When:**

- Runtime sample package validates first-party widgets, authoring documents, binding, focus/navigation, renderer layers/clipping/batches, and selected text/IME/accessibility counters.
- Editor shell validates UI authoring models through `tools/build-editor.ps1`.
- Docs and manifest fragments name every ready, host-gated, dependency-gated, and unsupported UI capability honestly.
- Public API boundary checks prove no native or middleware types leak.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker for explicitly host-gated work.

## Explicit Non-Claims

- This plan does not claim broad production text shaping until a selected adapter returns glyph ids, clusters, advances, offsets, fallback, bidi, line-break, and package evidence.
- This plan does not claim broad font production until selected adapters provide real font loading, glyph metrics, bitmap rows, atlas placement, budget, eviction, and upload handoff evidence.
- This plan does not claim native IME parity from TSF alone.
- This plan does not claim runtime OS accessibility publication from semantic rows alone.
- This plan does not claim macOS, Linux, Android, or iOS UI parity from Windows evidence.
- This plan does not claim UI middleware readiness.
- This plan does not make `MK_editor` a runtime dependency.
