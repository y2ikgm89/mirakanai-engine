# 2026-05-31 First-Party Editor Shell v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the optional Dear ImGui-backed `MK_editor` shell with a first-party retained editor UI and Windows-native shell, then remove active Dear ImGui, Qt, Slint, RmlUi, and UI-middleware dependency paths from the supported editor/runtime UI story without compatibility shims.

**Architecture:** Keep `MK_editor_core` as the GUI-independent source of editor state, authoring models, services, diagnostics, and AI-operable rows. Build the visible shell from first-party `mirakana::ui` documents, `MK_ui_renderer` renderer submissions, private Win32 services, and private D3D12 presentation plumbing. Keep low-level platform text, font, IME, accessibility, image decoding, and platform integration behind explicit adapter seams selected from official SDKs or audited dependencies after policy review; do not leak native handles or third-party UI object models into public engine, editor-core, gameplay, or generated-game APIs.

**Tech Stack:** C++23, PowerShell 7 repository wrappers, CMake presets, Windows SDK Win32/DXGI/D3D12, existing `MK_platform_win32`, `MK_editor_core`, `MK_ui`, `MK_ui_renderer`, `MK_renderer`, `MK_rhi_d3d12`, `MK_runtime_host_win32_presentation` design patterns, static contract checks, agent manifest fragments, and documentation/legal dependency policy. Dear ImGui, Qt, Slint, RmlUi, NoesisGUI, and other UI middleware are not implementation dependencies for this plan.

---

## Plan ID

**Plan ID:** `first-party-editor-shell-v1`

`first-party-editor-shell-v1`

## Status

**Status:** Active.

Execution Phase 0 selects this plan in the manifest/registry before code changes. The repository truth for current implementation work is now this clean-break replacement for the completed Native Win32 Editor Shell v1.

## Current Baseline

The repository already has the right foundation for a first-party editor shell:

- `MK_ui` owns a retained `UiDocument`, semantic roles, layout solving, focus/navigation, renderer-submission payloads, adapter contracts, clipboard/text-edit rows, IME publication rows, accessibility publication rows, font rasterization request rows, text shaping request rows, and image decode request rows.
- `MK_ui_renderer` converts `mirakana::ui` renderer submissions into `MK_renderer` sprite submissions with theme, glyph atlas, and image palette evidence.
- `MK_editor_core` and `editor/core` own GUI-independent editor documents, workspace state, project settings, scene/prefab/prefab-variant authoring models, material preview rows, resource panel rows, AI command rows, profiler rows, timeline rows, input rebinding models, and retained UI model helpers.
- `MK_platform_win32` owns first-party Win32 window/event/clipboard/file-dialog/process service adapters behind engine interfaces.
- `MK_runtime_host_win32_presentation` already contains validated private Win32/DXGI/D3D12 presentation and native UI overlay evidence that can guide editor-private presentation work.

The current visible shell is intentionally temporary for this new direction:

- `MK_editor` is active only through the optional `desktop-gui` lane.
- `editor/src/main.cpp` instantiates `Win32ImguiD3d12Host`.
- `editor/src/native_editor_panels.cpp` renders the panel set directly with `ImGui::*`.
- `editor/src/win32_imgui_d3d12_host.*`, `win32_imgui_message_bridge.*`, and `win32_imgui_descriptor_allocator.*` own the private Dear ImGui backend.
- `native_editor_launch.*` exposes `NativeEditorImguiUserConfigPolicy`.
- `vcpkg.json`, `CMakePresets.json`, `tools/build-gui.ps1`, `tools/bootstrap-deps.ps1`, dependency/legal docs, static checks, and manifest fragments currently describe `desktop-gui` as the Dear ImGui editor lane.

This plan keeps the editor state and AI-operable models, but replaces the immediate-mode rendering and dependency lane with first-party retained UI.

## External Engine Pattern

The migration follows the common production-engine split rather than attempting to hand-roll every UI subsystem:

- Unity 6 documents separate runtime/editor recommendations and recommends UI Toolkit for complex Editor UI, with IMGUI as the alternative for legacy or quick extensibility cases. The lesson for this engine is to keep the editor UI model first-party and retained, while not treating immediate-mode tooling as the long-term editor foundation.
- Unreal Engine exposes Slate as its custom UI programming framework and uses higher-level UI authoring layers for game-facing UI. The lesson is to own widget identity, docking state, command routing, and editor automation surfaces inside the engine instead of delegating them to a third-party object model.
- Godot builds runtime UI and editor UI around first-party `Control` / `Container` nodes, while `TextServer` owns low-level font/text rendering services. The lesson is to expose stable engine-owned nodes and semantic rows, but keep shaping/font complexity behind a specialized service boundary.
- Engines and tools that use Qt-class editor frameworks get mature docking/text/accessibility quickly, but inherit framework object models, licensing, distribution, and automation boundaries that this clean-break plan intentionally avoids for the core editor shell.

The adopted rule is: own the UI/editor contract and the AI-operable semantic model, not every low-level platform implementation detail.

## Official-Recommendation Clean-Break Policy

This plan treats official platform guidance and established engine architecture as constraints, not optional reading material:

- Prefer official Windows SDK surfaces for platform duties: Win32 for windows/messages, DXGI/D3D12 for private presentation, DirectWrite/Direct2D for Windows text/rendering adapters when selected, Text Services Framework for IME/text service adapters, and UI Automation for Windows accessibility publication.
- Prefer engine-owned retained contracts for editor/game UI duties: stable element ids, semantic roles, dock graph, command catalog, focus/navigation, rich-text model, validation diagnostics, and serialization.
- Prefer audited optional adapters for non-Windows or cross-platform low-level services only after dependency/legal/static-check updates. Do not import a UI framework to recover convenience lost by removing Dear ImGui.
- Remove obsolete names and lanes instead of forwarding them. The clean closeout deletes `desktop-gui`, `build-gui.ps1`, `MK_ENABLE_DESKTOP_GUI`, ImGui source files, and active Dear ImGui dependency records.
- Make every operation machine-readable before making it visually rich. AI and tests must be able to inspect state, enumerate commands, dry-run mutations, apply reviewed commands, and read results without native UI automation.
- Treat old behavior breakage as expected when it protects the new contract. If a stale script, CMake option, manifest recipe, smoke counter, or API name still works through compatibility forwarding, the phase is not clean-break complete.
- Use official built-in platform controls only where the selected host UI stack actually owns those controls. Because this editor renders a first-party retained tree rather than WinUI/WPF controls, text scaling, reflow, focus, accessibility semantics, and UI Automation publication must be explicitly planned and validated instead of assumed.

The result should resemble the major-engine pattern at the architecture level: first-party UI/editor model like Unreal Slate/Godot Control/UI Toolkit-style retained trees, plus official/audited low-level adapters for text, IME, accessibility, font, image, and platform services.

## Adopted Recommendation Snapshot

Official-source refresh on 2026-05-31 leads to these implementation decisions:

| Source pattern | Adopted decision |
| --- | --- |
| Unity 6 recommends UI Toolkit for complex Editor UI and keeps runtime/editor UI choices explicit. | `MK_editor` moves to a retained first-party editor document model; generated/runtime UI stays on public `mirakana::ui`, not editor-private APIs. |
| Unreal keeps a custom engine UI framework and separate higher-level authoring/game UI layers. | `MK_ui`, `MK_editor_core`, shell-common document composition, dock graph rows, and command catalogs are engine-owned contracts. |
| Godot exposes UI through first-party `Control` nodes and routes font/text complexity through text-server infrastructure. | Docking, rich-text intent, semantic roles, focus, command ids, and AI rows are first-party; shaping, bidi, complex line breaking, font fallback, and raster quality remain adapter work. |
| Microsoft Windows guidance gives the best text/accessibility experience through built-in controls, and requires custom providers for custom controls. | The first-party renderer must not claim Windows text scaling or accessibility for free; UI Automation provider publication, text reflow, and high-quality font adapters need focused official-SDK phases. |

This means the plan is not "write Qt/Slate/UI Toolkit from scratch." It is "own the durable engine contract, then use official SDKs or audited low-level adapters behind that contract where the platform already has hard problems solved."

## Ownership Boundary Matrix

| Area | First-party ownership | Adapter ownership |
| --- | --- | --- |
| Docking | Dock graph, split/tab stacks, stable panel ids, layout persistence, focus, keyboard navigation, command routing, smoke counters | Native multi-window behavior, OS drag affordances, monitor-specific placement if later selected |
| Rich text | Document model, paragraph/span ids, style tokens, selection model, editor commands, serialization, AI-operable diagnostics | Shaping, bidi, complex line breaking, glyph fallback, font loading/rasterization, text scaling quality |
| AI operation | Snapshot rows, command catalog, command request values, dry-run result, apply result, confirmation policy, diagnostics, state revision | External LLM/tool orchestration; no editor-private or native automation dependency |
| IME/text input | `PlatformTextInputSession`, composition rows, committed text rows, caret rect, surrounding text contract, focus ownership | Windows TSF or platform text-service implementation, candidate UI integration, virtual keyboard behavior |
| Accessibility | Semantic tree, roles, names, states, actions, bounds, stable ids, validation before publication | UI Automation, NSAccessibility, AT-SPI, Android/iOS accessibility bridge publication |
| Font quality | Font family/style tokens, fallback policy rows, atlas handoff contracts, deterministic test glyph fixtures | DirectWrite/CoreText/FreeType-class rasterization, hinting, subpixel quality, OpenType shaping |
| Image decode | Source asset identity, decoded-image request/result contracts, atlas metadata, renderer handoff | PNG/KTX/SVG/vector/source codec implementations selected after dependency and license review |

Implementation phases may add first-party contracts in the left column. They must not add broad adapter implementations from the right column unless the phase also includes official-source refresh, dependency/legal records, static checks, and focused validation evidence.

## Official Source Refresh Gate

Before each implementation phase, re-check the current official documentation for the API being touched and record the checked source in that phase evidence.

- Win32 windows and messages: <https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window> and <https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues>
- Direct3D 12 private presentation and synchronization: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide>
- DXGI swap chains and display behavior: <https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/d3d10-graphics-programming-guide-dxgi>
- Direct2D for official Windows 2D/UI rendering guidance when a Windows SDK 2D adapter is selected: <https://learn.microsoft.com/en-us/windows/win32/direct2d/direct2d-portal>
- DirectWrite for official Windows text layout/rendering adapters: <https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal>
- Windows text scaling and custom-control UX guidance when claiming text scale/reflow behavior: <https://learn.microsoft.com/en-us/windows/apps/develop/input/text-scaling>
- Text Services Framework for IME/text-service adapter design: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>
- UI Automation providers for accessibility bridge publication: <https://learn.microsoft.com/en-us/windows/win32/winauto/uiauto-providersoverview>
- CMake and vcpkg documentation through Context7 or official docs for target/preset/dependency-policy changes.
- Unity UI system comparison and UI Toolkit guidance when updating rationale docs: <https://docs.unity3d.com/6000.0/Documentation/Manual/UI-system-compare.html>
- Unreal Slate and UMG UI architecture when updating rationale docs: <https://dev.epicgames.com/documentation/en-us/unreal-engine/slate-user-interface-programming-framework-for-unreal-engine> and <https://dev.epicgames.com/documentation/en-us/unreal-engine/creating-user-interfaces-with-umg-and-slate-in-unreal-engine>
- Godot Control UI and RichTextLabel/TextServer docs when updating rationale docs: <https://docs.godotengine.org/en/stable/tutorials/ui/index.html>, <https://docs.godotengine.org/en/stable/classes/class_richtextlabel.html>, and <https://docs.godotengine.org/en/stable/classes/class_textserver.html>

If an official source recommends native handles or framework object exposure in sample code, keep those handles private to backend implementation files and preserve this repository's public-boundary policy.

## Non-Negotiable Constraints

- No Dear ImGui, Qt, Slint, RmlUi, NoesisGUI, SDL3, or other UI middleware implementation dependency in the replacement shell.
- No dual editor shell, compatibility wrapper, deprecated alias, `desktop-gui` shim, `build-gui.ps1` shim, or `MK_ENABLE_DESKTOP_GUI` bridge after closeout.
- No public `HWND`, `HINSTANCE`, `HANDLE`, `IDXGI*`, `ID3D12*`, `D3D12_*`, `DXGI_*`, COM pointer, Dear ImGui, Qt, Slint, RmlUi, or middleware type exposure through engine public headers, `editor/core/include`, `editor/include`, generated-game APIs, runtime UI APIs, manifests, or docs.
- Do not move Win32, D3D12, DXGI, file dialogs, process execution, renderer/RHI ownership, or platform-native handles into `MK_editor_core`.
- Do not claim full docking, multi-window, cross-platform editor shell parity, production text shaping, production font loading/rasterization, native IME candidate UI, OS accessibility publication, or full viewport/material GPU texture display until focused phases prove them.
- Do not hand-roll production IME engines, OS accessibility bridges, OpenType shaping, bidi reordering, glyph hinting, broad font fallback, or complex Unicode line breaking inside editor panels or shell host code.
- Do not claim platform text scaling or accessibility parity merely because semantic rows exist; custom first-party controls require explicit reflow and provider publication evidence.
- Do not weaken public-boundary, dependency-policy, JSON contract, or AI-integration checks to make the migration easier.
- Keep generated games on `mirakana::ui` and public engine APIs; generated games must not use `mirakana_editor`, editor-private APIs, native handles, Dear ImGui, or UI middleware.

## Target Architecture

```text
MK_editor_core
  Owns editor documents, workspace state, authoring operations, services, diagnostics,
  retained editor UI models, AI operation snapshots, command catalogs, dry-run rows,
  apply-result rows, and AI-readable diagnostics.

MK_editor_shell_common
  Owns launch parsing, NativeEditorApp, first-party editor document composition,
  service binding, editor dock graph, rich-text document rows, deterministic smoke
  counters, and testable shell planning helpers.

MK_editor_shell_win32
  Owns private Win32 window/event loop, private D3D12/DXGI presentation plumbing,
  MK_ui_renderer submission, Win32 clipboard/file-dialog/process adapters, and
  platform text/accessibility/font adapter dispatch boundaries.

Future adapter modules
  Own DirectWrite/TSF/UI Automation or other official-SDK implementations behind
  narrow interfaces after license/dependency/static-check updates. They are not
  required for the first visible first-party shell slice.

MK_editor
  Thin executable entrypoint that parses launch options, constructs NativeEditorApp,
  runs the first-party shell, prints deterministic smoke counters, and exposes no SDK surface.
```

AI operation improves because the visible shell is driven by stable `UiDocument` ids, semantic roles, command ids, adapter diagnostics, and editor-core rows instead of transient immediate-mode call order. Agents inspect `EditorAiOperationSnapshot`, enumerate `EditorAiCommandCatalog`, submit reviewed `EditorAiCommandRequest` values, dry-run them before mutation, and read `EditorAiCommandApplyResult` rows afterward. No AI path depends on screen coordinates, Win32 handles, D3D12 handles, Dear ImGui state, or UI middleware automation.

## Clean Break Naming

The implementation closes on the new names below. Transitional local edits may exist inside a phase, but no compatibility names remain at phase closeout.

| Old active name | New active name |
| --- | --- |
| `desktop-gui` validation lane | `desktop-editor` validation lane |
| `tools/build-gui.ps1` | `tools/build-editor.ps1` |
| `MK_ENABLE_DESKTOP_GUI` | `MK_ENABLE_DESKTOP_EDITOR` |
| `cpp23-desktop-gui-eval` | `cpp23-desktop-editor-eval` |
| `tools/evaluate-cpp23.ps1 -Gui` | `tools/evaluate-cpp23.ps1 -Editor` |
| `Win32ImguiD3d12Host` | `Win32FirstPartyEditorHost` |
| `NativeEditorImguiUserConfigPolicy` | `NativeEditorUserConfigPolicy` |
| `win32_imgui_*` files | first-party shell host files |

## Proposed File Structure

Create:

- `editor/src/first_party_editor_document.hpp`
- `editor/src/first_party_editor_document.cpp`
- `editor/src/first_party_editor_docking.hpp`
- `editor/src/first_party_editor_docking.cpp`
- `editor/src/first_party_editor_rich_text.hpp`
- `editor/src/first_party_editor_rich_text.cpp`
- `editor/src/first_party_editor_adapter_boundaries.hpp`
- `editor/src/first_party_editor_theme.hpp`
- `editor/src/first_party_editor_theme.cpp`
- `editor/src/win32_first_party_editor_host.hpp`
- `editor/src/win32_first_party_editor_host.cpp`
- `editor/core/include/mirakana/editor/ai_operation_surface.hpp`
- `editor/core/src/ai_operation_surface.cpp`
- `tools/build-editor.ps1`

Modify:

- `CMakeLists.txt`
- `CMakePresets.json`
- `vcpkg.json`
- `README.md`
- `editor/CMakeLists.txt`
- `editor/src/main.cpp`
- `editor/src/native_editor_app.hpp`
- `editor/src/native_editor_app.cpp`
- `editor/src/native_editor_launch.hpp`
- `editor/src/native_editor_launch.cpp`
- `tests/unit/editor_native_shell_tests.cpp`
- `tests/unit/ui_renderer_tests.cpp` only when renderer submission guarantees need expansion
- `tests/unit/editor_core_tests.cpp` only if rich-text or dock graph contracts move into `editor/core`
- `tools/bootstrap-deps.ps1`
- `tools/evaluate-cpp23.ps1`
- `tools/run-validation-recipe-plans.ps1`
- `tools/check-dependency-policy.ps1`
- `tools/check-json-contracts*.ps1`
- `tools/check-ai-integration*.ps1`
- `tools/check-validation-recipe-runner.ps1`
- `tools/check-public-api-boundaries.ps1` only to preserve or strengthen forbidden public UI-middleware checks
- `docs/ui.md`
- `docs/editor.md`
- `docs/dependencies.md`
- `docs/legal-and-licensing.md`
- `docs/testing.md`
- `docs/workflows.md`
- `docs/current-capabilities.md`
- `docs/roadmap.md`
- `docs/superpowers/plans/README.md`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- `engine/agent/manifest.fragments/009-validationRecipes.json` if recipe ids are manifest-backed there
- `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- `engine/agent/manifest.json` through `tools/compose-agent-manifest.ps1 -Write`

Delete after replacement is green:

- `editor/src/native_editor_panels.cpp`
- `editor/src/win32_imgui_d3d12_host.hpp`
- `editor/src/win32_imgui_d3d12_host.cpp`
- `editor/src/win32_imgui_message_bridge.hpp`
- `editor/src/win32_imgui_message_bridge.cpp`
- `editor/src/win32_imgui_descriptor_allocator.hpp`
- `editor/src/win32_imgui_descriptor_allocator.cpp`
- `tools/build-gui.ps1`
- `desktop-gui` vcpkg feature and all active docs/manifests/static-check references to it
- Dear ImGui row in `THIRD_PARTY_NOTICES.md` if no other active dependency keeps it

## Phase 0 - Selection, Drift Audit, And Source Refresh

**Goal:** Make this plan the selected active slice and prove the migration surface is fully inventoried before code changes.

**Context:** The manifest currently points to `next-production-gap-selection`; Native Win32 Editor Shell v1 is completed historical evidence.

**Constraints:** Do not change behavior yet. Do not hand-edit `engine/agent/manifest.json`.

**Done When:** Manifest fragments, composed manifest, registry, and baseline inventory all agree that `first-party-editor-shell-v1` is the selected active plan.

Files:

- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Audit: `CMakeLists.txt`
- Audit: `CMakePresets.json`
- Audit: `vcpkg.json`
- Audit: `editor/CMakeLists.txt`
- Audit: `editor/src`
- Audit: `tests/unit/editor_native_shell_tests.cpp`
- Audit: `docs`
- Audit: `tools`
- Audit: `engine/agent/manifest.fragments`

Steps:

- [x] Read `engine/agent/manifest.json.aiOperableProductionLoop`, `docs/superpowers/plans/README.md`, `docs/ui.md`, `docs/editor.md`, `docs/dependencies.md`, and `editor/CMakeLists.txt`.
- [x] Run the active UI-middleware surface inventory:

```powershell
rg -n "desktop-gui|build-gui|MK_ENABLE_DESKTOP_GUI|cpp23-desktop-gui-eval|Dear ImGui|imgui|ImGui|win32_imgui|Qt|Slint|RmlUi|NoesisGUI" CMakeLists.txt CMakePresets.json vcpkg.json README.md docs engine editor tests tools THIRD_PARTY_NOTICES.md
```

Expected: all current active build, test, docs, manifest, and static-check references that must be updated or intentionally retained as forbidden-term guards or historical archive evidence.

- [x] Record the official source refresh links used for Win32, D3D12/DXGI, DirectWrite, TSF, and UI Automation in the phase evidence section before implementation begins.
- [x] Record the Unity/Unreal/Godot comparison outcome in docs as the accepted design pattern: first-party UI/editor state plus official-SDK or audited adapter implementations for low-level text/platform services.
- [x] Record the clean-break decision that old lane names and Dear ImGui implementation files are deleted at closeout, with no compatibility forwarding scripts or aliases.
- [x] Select `first-party-editor-shell-v1` in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` and explain that it is a clean-break replacement for the completed Dear ImGui shell, not a compatibility extension.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: composed manifest updated, JSON/static contract checks pass after the selection text is aligned.

### Phase 0 Evidence

- Worktree: `G:/workspace/development/GameEngine/.worktrees/first-party-editor-shell-v1` on branch `codex/first-party-editor-shell-v1`; `tools/prepare-worktree.ps1` reported `linked-worktree=true`, `external-vcpkg=linked`, `vcpkg-installed=linked`, and `ok`.
- Official source refresh: Microsoft Win32 windows/messages, Direct3D 12, DXGI, Direct2D, DirectWrite, Windows text scaling/custom-control guidance, Text Services Framework, and UI Automation provider documentation were rechecked for the Windows SDK boundaries; Context7 was used for current CMake target-scope and vcpkg manifest/feature behavior.
- Engine comparison refresh: Unity UI Toolkit/editor UI guidance, Unreal Slate/UMG, and Godot Control/RichTextLabel/TextServer patterns support the selected architecture: first-party retained UI/editor contracts plus official-SDK or audited adapters for low-level text, IME, accessibility, font, image, and platform services.
- Clean-break decision: `desktop-gui`, `build-gui.ps1`, `MK_ENABLE_DESKTOP_GUI`, Dear ImGui implementation files, and active UI-middleware dependency claims are deletion targets at closeout, not compatibility aliases.
- Inventory command: `rg -n "desktop-gui|build-gui|MK_ENABLE_DESKTOP_GUI|cpp23-desktop-gui-eval|Dear ImGui|imgui|ImGui|win32_imgui|Qt|Slint|RmlUi|NoesisGUI" CMakeLists.txt CMakePresets.json vcpkg.json README.md docs engine editor tests tools THIRD_PARTY_NOTICES.md` found the active build, dependency, editor source, test, docs, and static-check surfaces that later phases must update or remove.
- Validation: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-text-format.ps1`, and `tools/check-agents.ps1` passed for the Phase 0 docs/manifest/static-check selection slice.

## Phase 1 - Test-First First-Party Shell Contract

**Goal:** Replace the visible shell contract tests before replacing the shell implementation.

**Context:** Current `editor_native_shell_tests.cpp` covers launch options, ImGui persistence, panel count, Win32 services, viewport/material diagnostic plans, and ImGui descriptor allocation.

**Constraints:** Tests must describe first-party shell behavior without mentioning Dear ImGui as an implementation detail.

**Done When:** Focused tests fail for the old ImGui implementation and specify the new first-party contract.

Files:

- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `editor/src/native_editor_launch.hpp`
- Modify: `editor/src/native_editor_launch.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`

Steps:

- [x] Rename the user-config policy test from `imgui persistence` to first-party shell persistence:

```cpp
MK_TEST("editor native shell no-user-config disables shell persistence")
```

Expected: it calls `make_native_editor_user_config_policy`, not `make_native_editor_imgui_user_config_policy`.

- [x] Delete ImGui descriptor allocator tests and replace them with first-party document/renderer-planning tests:
  - `first party editor document includes visible panel roots`
  - `first party editor document keeps stable semantic element ids`
  - `first party editor document produces renderer submission without native handles`
  - `first party editor shell smoke counters report imgui disabled`
- [x] Add expected smoke key coverage:

```text
editor_shell_ui=first_party
editor_shell_imgui=0
editor_shell_backend=d3d12
editor_shell_panels=11
editor_shell_sdl3=0
editor_shell_viewport_native_handles_exposed=0
editor_shell_material_preview_native_handles_exposed=0
```

- [x] Keep service routing tests for file dialog, clipboard, and reviewed process execution unchanged except recipe ids renamed from `desktop-gui` to `desktop-editor`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
```

Expected at this phase start: tests fail or do not compile until later phases replace the implementation. At phase close: they pass.

### Phase 1 Evidence

- RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests` failed because `first_party_editor_document.hpp` did not exist after the test replacement.
- GREEN: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests` passed after the first-party shell contract implementation.

## Phase 2 - First-Party Editor Document Composition

**Goal:** Build a retained `mirakana::ui::UiDocument` for the full editor panel set from `NativeEditorApp` and `editor/core` models.

**Context:** `editor/core/src/ui_model.cpp` already creates retained UI documents for inspector, assets, command palette, diagnostics, and timeline. `NativeEditorApp` already exposes the panel data and service status needed by the shell.

**Constraints:** No Win32, D3D12, Dear ImGui, Qt, Slint, RmlUi, or middleware includes in document composition files.

**Done When:** Unit tests can serialize/traverse the first-party editor document and verify stable ids, roles, visibility, panel count, and renderer payload availability.

Files:

- Create: `editor/src/first_party_editor_document.hpp`
- Create: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_native_shell_tests.cpp`

Steps:

- [x] Add a private shell document API:

```cpp
namespace mirakana::editor {

struct FirstPartyEditorDocument {
    mirakana::ui::UiDocument document;
    mirakana::ui::LayoutResult layout;
    mirakana::ui::RendererSubmission renderer_submission;
    std::uint32_t panel_root_count{0};
    bool native_handles_exposed{false};
};

struct FirstPartyEditorShellSmokeCounters {
    std::string ui{"first_party"};
    std::string backend{"d3d12"};
    std::uint32_t panel_count{0};
    bool imgui_enabled{false};
    bool sdl3_enabled{false};
    bool viewport_native_handles_exposed{false};
    bool material_preview_native_handles_exposed{false};
};

[[nodiscard]] FirstPartyEditorDocument make_first_party_editor_document(const NativeEditorApp& app);

} // namespace mirakana::editor
```

- [x] Compose top-level panel roots for Main Menu, Scene, Inspector, Assets, Console, Viewport, Resources, AI Commands, Profiler, Timeline, and Project Settings.
- [x] Reuse existing `MK_ui` document composition patterns; deeper per-panel core UI model embedding remains for later richer panel phases.
- [x] Assign stable element ids using `editor.panel.<panel>` prefixes such as `editor.panel.viewport.status`, `editor.panel.resources.status`, and `editor.panel.project_settings.status`.
- [x] Confirm no `first_party_editor_theme.*` files are needed for this candidate; deterministic style tokens live in the private document composition helper.
- [x] Ensure all renderer payloads come from `mirakana::ui` submission helpers; do not draw through native UI middleware.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests MK_ui_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests|MK_ui_renderer_tests"
```

Expected: first-party document and renderer submission tests pass.

### Phase 2 Evidence

- Added private `editor/src/first_party_editor_document.hpp` and `.cpp` to `MK_editor_shell_common`.
- `make_first_party_editor_document` now builds deterministic retained panel roots, layout, renderer submission, accessibility rows, and native-handle non-exposure evidence.
- `make_first_party_editor_shell_smoke_counters` reports `ui=first_party`, `imgui_enabled=false`, `sdl3_enabled=false`, panel count, backend, and native handle exposure flags.
- Focused validation passed: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`; `tools/cmake.ps1 --build --preset dev --target MK_ui_renderer_tests`; `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests|MK_ui_renderer_tests"`.
- Static drift validation passed: `tools/check-format.ps1`; `tools/check-tidy.ps1 -Files editor/src/first_party_editor_document.cpp,editor/src/native_editor_launch.cpp,tests/unit/editor_native_shell_tests.cpp -ReuseExistingFileApiReply`; `tools/check-public-api-boundaries.ps1`; `tools/check-native-desktop-contracts.ps1`; `tools/check-json-contracts.ps1`; `tools/check-ai-integration.ps1`; `tools/check-agents.ps1`; `git diff --check`.
- Full slice validation passed: `tools/validate.ps1`.

## Phase 3 - First-Party Docking, Rich Text, And Adapter Boundary Contracts

**Goal:** Add the first durable editor UI contracts that make the shell AI-operable without taking on low-level text/IME/accessibility/font implementation debt.

**Context:** Unity, Unreal, and Godot all keep engine-owned UI/editor abstractions above lower-level text/platform services. This phase mirrors that split: dock graph and rich-text document state are first-party; shaping, IME, accessibility bridge publication, and high-quality font rasterization stay behind explicit adapter contracts.

**Constraints:** Do not implement TSF, DirectWrite, UI Automation, FreeType, HarfBuzz, ICU, Qt, Slint, RmlUi, or middleware bindings in this phase. Add contracts and fail-closed diagnostics only.

**Done When:** Unit tests prove deterministic dock graph rows, rich-text rows, adapter boundary rows, and first-party editor document integration without native handles or UI middleware.

Files:

- Create: `editor/src/first_party_editor_docking.hpp`
- Create: `editor/src/first_party_editor_docking.cpp`
- Create: `editor/src/first_party_editor_rich_text.hpp`
- Create: `editor/src/first_party_editor_rich_text.cpp`
- Create: `editor/src/first_party_editor_adapter_boundaries.hpp`
- Modify: `editor/src/first_party_editor_document.hpp`
- Modify: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_native_shell_tests.cpp`

Steps:

- [x] Add a private dock graph API:

```cpp
namespace mirakana::editor {

enum class FirstPartyEditorDockNodeKind : std::uint8_t {
    split,
    tab_stack,
    panel,
};

enum class FirstPartyEditorDockSplitAxis : std::uint8_t {
    horizontal,
    vertical,
};

struct FirstPartyEditorDockNode {
    std::string id;
    FirstPartyEditorDockNodeKind kind{FirstPartyEditorDockNodeKind::panel};
    FirstPartyEditorDockSplitAxis axis{FirstPartyEditorDockSplitAxis::horizontal};
    float split_ratio{0.5F};
    std::vector<std::string> children;
    std::vector<std::string> tabs;
    std::string active_tab;
};

struct FirstPartyEditorDockGraph {
    std::string root_id;
    std::vector<FirstPartyEditorDockNode> nodes;
};

struct FirstPartyEditorDockValidation {
    bool valid{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] FirstPartyEditorDockGraph make_default_first_party_editor_dock_graph();
[[nodiscard]] FirstPartyEditorDockValidation
validate_first_party_editor_dock_graph(const FirstPartyEditorDockGraph& graph);

} // namespace mirakana::editor
```

- [x] Add tests:
  - `first party editor dock graph validates default split and tab stacks`
  - `first party editor dock graph rejects duplicate node ids`
  - `first party editor dock graph rejects missing active tab`
  - `first party editor document orders panels through dock graph`
- [x] Add a private rich-text row API:

```cpp
namespace mirakana::editor {

struct FirstPartyEditorRichTextSpan {
    std::string id;
    std::string style_token;
    std::string text;
};

struct FirstPartyEditorRichTextParagraph {
    std::string id;
    std::vector<FirstPartyEditorRichTextSpan> spans;
};

struct FirstPartyEditorRichTextDocument {
    std::string id;
    std::vector<FirstPartyEditorRichTextParagraph> paragraphs;
};

struct FirstPartyEditorRichTextValidation {
    bool valid{false};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] FirstPartyEditorRichTextValidation
validate_first_party_editor_rich_text_document(const FirstPartyEditorRichTextDocument& document);
[[nodiscard]] mirakana::ui::UiDocument
make_first_party_editor_rich_text_ui_model(const FirstPartyEditorRichTextDocument& document);

} // namespace mirakana::editor
```

- [x] Add tests:
  - `first party rich text validates paragraph and span ids`
  - `first party rich text preserves style tokens without shaping claims`
  - `first party rich text produces semantic ui labels`
- [x] Add adapter boundary declarations that are values only:

```cpp
namespace mirakana::editor {

enum class FirstPartyEditorAdapterBoundary : std::uint8_t {
    text_shaping,
    font_rasterization,
    ime_text_services,
    accessibility_bridge,
    image_decoding,
};

struct FirstPartyEditorAdapterBoundaryRow {
    FirstPartyEditorAdapterBoundary boundary{FirstPartyEditorAdapterBoundary::text_shaping};
    std::string id;
    std::string official_source_family;
    bool implemented{false};
    bool native_handles_public{false};
};

[[nodiscard]] std::vector<FirstPartyEditorAdapterBoundaryRow>
first_party_editor_required_adapter_boundaries();

} // namespace mirakana::editor
```

- [x] Add tests proving required rows exist for `text_shaping`, `font_rasterization`, `ime_text_services`, and `accessibility_bridge`, and that every row has `implemented == false` until a focused adapter phase lands.
- [x] Integrate the default dock graph into `make_first_party_editor_document` so the shell document is structured by dock `Split` and `TabStack` rows instead of hard-coded panel ordering.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
```

Expected: dock graph, rich-text, and adapter-boundary tests pass with no native SDK or middleware implementation dependency.

### Phase 3 Evidence

- RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests` failed because `first_party_editor_adapter_boundaries.hpp` did not exist after the Phase 3 tests were added.
- GREEN: `tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests` passed after adding dock graph, rich-text, adapter-boundary contracts, and dock-graph-backed document composition.
- Focused static validation passed: `tools/check-format.ps1`; `tools/check-tidy.ps1 -Files editor/src/first_party_editor_docking.cpp,editor/src/first_party_editor_document.cpp,editor/src/first_party_editor_rich_text.cpp,tests/unit/editor_native_shell_tests.cpp -ReuseExistingFileApiReply`.

## Phase 4 - AI Operation Surface Contract

**Goal:** Make the first-party editor shell understandable and safely operable by agents through `MK_editor_core` values, not through visual scraping or native UI automation.

**Context:** The visible shell will be first-party and retained, but AI operation should not depend on the visible shell being present. The operation surface belongs in `editor/core` so tests, manifests, generated workflows, and future shells can use the same contract.

**Constraints:** Do not execute arbitrary shell commands, mutate manifests, call validation recipes, expose native handles, or route through Win32/D3D12/ImGui/Qt/Slint/RmlUi automation. Every mutating command needs dry-run evidence first.

**Done When:** Focused editor-core tests can build an operation snapshot, enumerate commands, dry-run a command, reject unsafe requests, and apply a reviewed in-memory command with deterministic diagnostics and revision changes.

Files:

- Create: `editor/core/include/mirakana/editor/ai_operation_surface.hpp`
- Create: `editor/core/src/ai_operation_surface.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `docs/editor.md`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1` only if the public header/source literals need an owning static guard before Phase 8 closeout
- Modify: `editor/src/first_party_editor_document.cpp` only to surface command ids into UI element metadata when that metadata exists
- Modify: `tests/unit/editor_native_shell_tests.cpp` only for shell-facing command catalog checks

Steps:

- [x] Add the editor-core operation surface API:

```cpp
namespace mirakana::editor {

struct EditorAiOperationElementRow {
    std::string id;
    std::string role;
    std::string label;
    bool visible{true};
    bool enabled{true};
};

struct EditorAiOperationDiagnostic {
    std::string code;
    std::string message;
};

struct EditorAiOperationSnapshot {
    std::uint64_t revision{0};
    std::vector<EditorAiOperationElementRow> elements;
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

struct EditorAiCommandRow {
    std::string id;
    std::string label;
    std::string target_element_id;
    bool enabled{true};
    bool mutates_state{false};
    bool requires_confirmation{false};
};

struct EditorAiCommandCatalog {
    std::uint64_t revision{0};
    std::vector<EditorAiCommandRow> commands;
};

struct EditorAiCommandParameter {
    std::string key;
    std::string value;
};

struct EditorAiCommandRequest {
    std::string command_id;
    std::string target_element_id;
    std::vector<EditorAiCommandParameter> parameters;
    bool user_confirmed{false};
};

struct EditorAiCommandDryRunResult {
    bool accepted{false};
    bool would_mutate{false};
    bool requires_confirmation{false};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

struct EditorAiCommandApplyResult {
    bool applied{false};
    std::uint64_t before_revision{0};
    std::uint64_t after_revision{0};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

[[nodiscard]] EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace);
[[nodiscard]] EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace);
[[nodiscard]] EditorAiCommandDryRunResult
dry_run_editor_ai_command(const Workspace& workspace, const EditorAiCommandCatalog& catalog,
                          const EditorAiCommandRequest& request);
[[nodiscard]] EditorAiCommandApplyResult
apply_editor_ai_command(Workspace& workspace, const EditorAiCommandCatalog& catalog,
                        const EditorAiCommandRequest& request);

} // namespace mirakana::editor
```

- [x] Add tests:
  - `editor ai operation snapshot exposes visible panel rows`
  - `editor ai command catalog exposes stable panel visibility commands`
  - `editor ai command dry run rejects unknown command`
  - `editor ai command dry run rejects target mismatch`
  - `editor ai command apply toggles panel visibility after accepted dry run`
  - `editor ai command apply requires confirmation for mutating commands marked confirmable`
- [x] Keep the first command set deliberately narrow:

```text
editor.panel.resources.show
editor.panel.resources.hide
editor.panel.ai_commands.show
editor.panel.ai_commands.hide
editor.panel.profiler.show
editor.panel.profiler.hide
```

Expected: no command launches processes, runs validation, edits project files, mutates manifests, or touches renderer/native handles.

- [x] Require command ids and target ids to match catalog rows exactly; return diagnostics instead of guessing.
- [x] Require `dry_run_editor_ai_command` to be valid before `apply_editor_ai_command` applies a mutation.
- [x] Return deterministic snapshot/catalog revision values that change after an applied in-memory workspace mutation.
- [x] Update `docs/editor.md`, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/014-gameCodeGuidance.json`, and the owning AI-integration static check so agents can discover `EditorAiOperationSnapshot`, command catalogs, dry-run, apply-result rows, confirmation policy, and the narrow panel command set before the broader Phase 8 closeout.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
```

Expected: AI operation surface tests pass without building the visible editor shell.

### Phase 4 Evidence

- RED: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` failed because `mirakana/editor/ai_operation_surface.hpp` did not exist after the Phase 4 tests were added.
- GREEN: `tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests` passed after adding the operation snapshot, command catalog, dry-run, and apply-result implementation.
- Focused static validation passed before the plan-update docs edit: `tools/check-format.ps1`; `tools/check-tidy.ps1 -Files editor/core/src/ai_operation_surface.cpp,tests/unit/editor_core_tests.cpp -ReuseExistingFileApiReply`.
- Agent-surface sync passed after the official-recommendation plan update: `tools/compose-agent-manifest.ps1 -Write`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-format.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`, and `git diff --check`.
- Full validation passed for the candidate before publication: `tools/validate.ps1` completed with `validate: ok` and 85/85 CTest tests passing; Metal/iOS checks remained diagnostic host-gated on this Windows host as expected.

## Phase 5 - First-Party Win32 Shell Host

**Goal:** Replace `Win32ImguiD3d12Host` with a private Win32/D3D12 host that renders `MK_ui_renderer` submissions and routes Windows services without UI middleware.

**Context:** The current ImGui host already owns Win32 window creation, D3D12 host readiness, resize smoke, service binding, and diagnostic viewport/material preview counters. `MK_runtime_host_win32_presentation` provides project-local D3D12 presentation planning patterns. Phases 3 and 4 supply first-party dock/rich-text/adapter-boundary and AI-operation contracts that this host consumes without implementing low-level text or accessibility services.

**Constraints:** Keep native handles private. Do not expose renderer/RHI handles through UI documents, editor core, smoke output, or public APIs.

**Done When:** `MK_editor --smoke-frames 2 --smoke-resize --no-user-config` reports first-party UI counters and exits cleanly without loading Dear ImGui.

Files:

- Create: `editor/src/win32_first_party_editor_host.hpp`
- Create: `editor/src/win32_first_party_editor_host.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/src/native_editor_launch.hpp`
- Modify: `editor/src/native_editor_launch.cpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_native_shell_tests.cpp`

Steps:

- [ ] Add private host result types:

```cpp
namespace mirakana::editor {

enum class Win32FirstPartyEditorAdapterKind : std::uint8_t {
    hardware,
    warp,
    null_renderer,
    none,
};

struct Win32FirstPartyEditorHostDesc {
    NativeEditorLaunchOptions launch;
};

struct Win32FirstPartyEditorRunResult {
    bool succeeded{false};
    int exit_code{1};
    std::uint32_t frames_rendered{0};
    std::uint32_t resize_count{0};
    Win32FirstPartyEditorAdapterKind adapter_kind{Win32FirstPartyEditorAdapterKind::none};
    std::uint32_t renderer_boxes_submitted{0};
    std::uint32_t renderer_text_runs_available{0};
    std::string diagnostic;
};

class Win32FirstPartyEditorHost final {
  public:
    explicit Win32FirstPartyEditorHost(Win32FirstPartyEditorHostDesc desc);
    [[nodiscard]] Win32FirstPartyEditorRunResult run(NativeEditorApp& app);
};

} // namespace mirakana::editor
```

- [ ] Bind Win32 file dialog, clipboard, and reviewed process services through existing platform interfaces.
- [ ] Build one `FirstPartyEditorDocumentResult` per frame and submit it through `submit_ui_renderer_submission`.
- [ ] Start with a renderer path that is deterministic on hosts without a ready D3D12 device. Use `NullRenderer` fallback only as an explicit fallback report; keep `editor_shell_backend=d3d12` only when the private D3D12 path is actually selected.
- [ ] Preserve resize smoke and frame counters.
- [ ] Record resource, viewport, and material-preview host readiness through `NativeEditorApp` without exposing native texture handles.
- [ ] Update `editor/src/main.cpp` smoke output to include `editor_shell_ui=first_party` and `editor_shell_imgui=0`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests MK_editor
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests|MK_editor_smoke"
```

Expected: focused native shell tests and smoke pass on a Windows host with the dev preset configured.

## Phase 6 - Clean CMake, Preset, And Validation Lane Rename

**Goal:** Replace the active `desktop-gui` dependency lane with a dependency-free `desktop-editor` lane.

**Context:** `desktop-gui` currently exists because Dear ImGui is supplied by vcpkg. The first-party replacement should not require package-manager UI dependencies.

**Constraints:** No `desktop-gui`, `build-gui.ps1`, `MK_ENABLE_DESKTOP_GUI`, or `-Gui` compatibility entrypoints remain after phase closeout.

**Done When:** Operators use `tools/build-editor.ps1`, `desktop-editor`, `MK_ENABLE_DESKTOP_EDITOR`, and `-Editor`; old names fail by absence rather than compatibility forwarding.

Files:

- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Modify: `editor/CMakeLists.txt`
- Modify: `vcpkg.json`
- Modify: `tools/bootstrap-deps.ps1`
- Create: `tools/build-editor.ps1`
- Delete: `tools/build-gui.ps1`
- Modify: `tools/evaluate-cpp23.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `tools/check-validation-recipe-runner.ps1`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: `tools/check-ai-integration*.ps1`

Steps:

- [ ] Rename the root CMake option to `MK_ENABLE_DESKTOP_EDITOR`.
- [ ] Rename presets:
  - `desktop-gui` to `desktop-editor`
  - `cpp23-desktop-gui-eval` to `cpp23-desktop-editor-eval`
- [ ] Remove `find_package(imgui CONFIG REQUIRED)` and `imgui::imgui` from `editor/CMakeLists.txt`.
- [ ] Link the shell target only against first-party engine targets and Windows SDK libraries required by private implementation files.
- [ ] Remove the `desktop-gui` vcpkg feature from `vcpkg.json`.
- [ ] Remove `--x-feature=desktop-gui` from `tools/bootstrap-deps.ps1`.
- [ ] Add `tools/build-editor.ps1` to configure, build, and test the `desktop-editor` preset.
- [ ] Rename `tools/evaluate-cpp23.ps1 -Gui` to `-Editor` and align static checks.
- [ ] Rename the validation recipe from `desktop-gui` to `desktop-editor` and update host gates from `desktop-gui-vcpkg` / `windows-msvc-desktop-gui` to dependency-free Windows editor gates such as `windows-msvc-desktop-editor`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

Expected: dependency policy no longer requires Dear ImGui; first-party editor lane configures, builds, and tests through the new entrypoint.

## Phase 7 - Delete Dear ImGui Implementation And Dependency Records

**Goal:** Remove active Dear ImGui implementation files, dependency records, and documentation claims after the first-party shell is green.

**Context:** Leaving old ImGui files or docs after the new shell lands would create a second accidental UI path and weaken the clean-break decision.

**Constraints:** Preserve historical completed-plan evidence where it is explicitly historical, but remove active docs/manifests/static checks that claim Dear ImGui is supported.

**Done When:** The active source, build, dependency, docs, manifest, and validation surfaces no longer depend on Dear ImGui.

Files:

- Delete: `editor/src/native_editor_panels.cpp`
- Delete: `editor/src/win32_imgui_d3d12_host.hpp`
- Delete: `editor/src/win32_imgui_d3d12_host.cpp`
- Delete: `editor/src/win32_imgui_message_bridge.hpp`
- Delete: `editor/src/win32_imgui_message_bridge.cpp`
- Delete: `editor/src/win32_imgui_descriptor_allocator.hpp`
- Delete: `editor/src/win32_imgui_descriptor_allocator.cpp`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `docs/ui.md`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `README.md`
- Modify: `AGENTS.md` and matching Claude/Cursor guidance only if active policy text changes there

Steps:

- [ ] Delete old Dear ImGui implementation files once no target references them.
- [ ] Remove the Dear ImGui row from `THIRD_PARTY_NOTICES.md` if no active dependency keeps it.
- [ ] Update dependency/legal docs to say the first-party editor shell has no UI-middleware dependency, while future low-level text/font/accessibility/image dependencies still require official-source refresh, license audit, dependency records, and notices before introduction.
- [ ] Update UI/editor docs to describe `MK_editor` as first-party retained UI over `MK_editor_core`, `MK_ui`, and `MK_ui_renderer`, with dock graph and rich-text model ownership inside the shell/core contract.
- [ ] Update UI/editor docs to state that IME, OS accessibility bridge publication, shaping, bidi, line breaking, font fallback, and high-quality rasterization are adapter-owned responsibilities, not panel code responsibilities.
- [ ] Update roadmap/current capabilities to remove active `desktop-gui` and Dear ImGui claims.
- [ ] Keep forbidden-term checks for public APIs so future Dear ImGui or middleware symbols still fail if exposed through public surfaces.
- [ ] Run the closeout inventory:

```powershell
rg -n "desktop-gui|build-gui|MK_ENABLE_DESKTOP_GUI|cpp23-desktop-gui-eval|Dear ImGui|imgui|ImGui|win32_imgui" CMakeLists.txt CMakePresets.json vcpkg.json README.md docs engine editor tests tools THIRD_PARTY_NOTICES.md
```

Expected: results are limited to historical completed-plan/archive evidence and explicit forbidden-term/public-boundary guards. Any active build, dependency, manifest, validation, or current-capability reference is a closeout blocker.

## Phase 8 - Agent Surface And AI-Operable Contract Closeout

**Goal:** Keep Codex/Claude/Cursor, manifest fragments, schemas, docs, and static checks aligned with the new first-party editor shell contract.

**Context:** Current static checks intentionally assert the Dear ImGui-backed shell. Those checks must change to assert the first-party shell and reject old middleware dependencies.

**Constraints:** Update only owning surfaces. Do not broad-load unrelated agent surfaces. Do not hand-edit the composed manifest.

**Done When:** Agent-facing docs and machine-readable contracts describe the same first-party editor shell and validation recipes that the code implements.

Files:

- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json` if applicable
- Generate: `engine/agent/manifest.json`
- Modify: `schemas/engine-agent.schema.json` only if schema shape changes
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: matching `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.cursor/skills/gameengine-editor/SKILL.md` if present
- Modify: `tools/check-ai-integration*.ps1`
- Modify: `tools/check-json-contracts*.ps1`
- Modify: `tools/check-public-api-boundaries.ps1` if forbidden terms need strengthening

Steps:

- [ ] Update `gameCodeGuidance.currentEditor` to state that `MK_editor` is a first-party retained shell, not a Dear ImGui shell.
- [ ] Update `gameCodeGuidance.currentEditor` to list `EditorAiOperationSnapshot`, `EditorAiCommandCatalog`, `EditorAiCommandRequest`, `EditorAiCommandDryRunResult`, `EditorAiCommandApplyResult`, dock graph, rich-text rows, semantic roles, command ids, and adapter-boundary diagnostics as AI-operable editor surfaces.
- [ ] Update unsupported claims in `gameCodeGuidance.currentEditor` so production IME engines, OS accessibility publication, broad text shaping, font raster quality, and cross-platform shell parity remain unsupported until focused adapter phases validate them.
- [ ] Update smoke counters in manifest guidance to include `editor_shell_ui=first_party` and `editor_shell_imgui=0`.
- [ ] Update validation recipes and command surfaces from `desktop-gui` to `desktop-editor`.
- [ ] Update editor-change skill guidance to prefer first-party editor documents and shell adapters rather than adapting to ImGui.
- [ ] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Expected: all agent/static checks assert the first-party editor shell and no active Dear ImGui lane.

## Phase 9 - Full Validation And Closeout

**Goal:** Prove the clean-break replacement end to end and return the production loop to a truthful next-plan state.

**Context:** This phase closes the migration. It should not leave dangling old names, dependency records, stale docs, or unsupported readiness claims.

**Constraints:** Do not mark the plan complete if host-gated `desktop-editor` validation cannot run; record the blocker precisely instead.

**Done When:** Focused editor checks, dependency/static checks, full validation, docs, manifest, registry, and closeout evidence all agree.

Files:

- Modify: `docs/superpowers/plans/2026-05-31-first-party-editor-shell-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

Steps:

- [ ] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests MK_ui_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests|MK_ui_renderer_tests|MK_editor_smoke"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

- [ ] Run static and contract checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

- [ ] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Update this plan's status and phase evidence with exact commands, outcomes, and any host-gated blockers.
- [ ] Return `currentActivePlan` to the production-completion selection gate or select the next explicit plan in `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, then compose the manifest.
- [ ] Run a final text/agent/static sweep after closeout edits:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: the branch is ready for a validated publication checkpoint.

## Closeout Search Expectations

The final implementation must satisfy these repository searches:

```powershell
rg -n "find_package\\(imgui|imgui::imgui|#include <imgui|ImGui::|ImGui_Impl|win32_imgui" editor engine tests CMakeLists.txt
```

Expected: no active source or CMake hits.

```powershell
rg -n "desktop-gui|build-gui|MK_ENABLE_DESKTOP_GUI|cpp23-desktop-gui-eval" CMakeLists.txt CMakePresets.json README.md docs engine editor tests tools vcpkg.json
```

Expected: no active hits outside historical completed plans or archive-only evidence.

```powershell
rg -n "Dear ImGui|Qt|Slint|RmlUi|NoesisGUI" engine/include engine/ui/include editor/include editor/core/include games
```

Expected: no public API or generated-game hits except forbidden-term policy tests when those tests intentionally scan strings.

## Acceptance Criteria

- `MK_editor` visible shell builds and smokes through `desktop-editor`.
- The shell renders first-party retained UI documents through `MK_ui` / `MK_ui_renderer`; it does not include or link Dear ImGui or UI middleware.
- Dock graph, tab stacks, rich-text rows, semantic roles, command ids, and adapter-boundary diagnostics are first-party, deterministic, and covered by focused tests.
- AI operation is first-party and GUI-independent through `EditorAiOperationSnapshot`, `EditorAiCommandCatalog`, `EditorAiCommandRequest`, `EditorAiCommandDryRunResult`, and `EditorAiCommandApplyResult`.
- AI command execution requires catalog match, target match, dry-run acceptance, and confirmation for commands that require it; it never uses screen coordinates, native handles, arbitrary shell commands, or UI middleware automation.
- IME, accessibility bridge publication, text shaping, bidi, line breaking, font fallback, and high-quality glyph rasterization remain behind official-SDK or audited-dependency adapter contracts and are not hand-rolled inside panel or shell host code.
- `MK_editor_core` stays GUI-independent and free of native handles, renderer/RHI ownership, Windows SDK types, Dear ImGui, and UI middleware.
- Public engine/game/runtime UI APIs expose no native handles or UI-middleware types.
- `vcpkg.json` has no `desktop-gui` / Dear ImGui feature.
- `THIRD_PARTY_NOTICES.md`, `docs/dependencies.md`, and `docs/legal-and-licensing.md` match active dependencies.
- Agent manifest, docs, skills, validation recipes, and static checks use `desktop-editor` and first-party editor-shell language.
- Full `tools/validate.ps1` passes, or a concrete host/toolchain blocker is recorded with exact failing command.

## Self-Review Checklist

- [ ] Does the implementation remove active Dear ImGui dependency records instead of only hiding includes?
- [ ] Are old lane names deleted rather than forwarded?
- [ ] Are all first-party UI elements identified by stable ids and semantic roles that an agent can inspect?
- [ ] Are dock graph and rich-text rows first-party values rather than immediate-mode call order or middleware objects?
- [ ] Can an agent inspect a snapshot, enumerate commands, dry-run a request, apply a reviewed command, and read deterministic diagnostics without native UI automation?
- [ ] Do mutating AI commands reject unknown command ids, target mismatches, missing confirmation, and arbitrary execution attempts?
- [ ] Are IME, accessibility, shaping, bidi, line breaking, and font rasterization kept behind adapter contracts with unsupported claims preserved until validated?
- [ ] Does smoke output prove `editor_shell_ui=first_party` and `editor_shell_imgui=0`?
- [ ] Are official Windows SDK responsibilities behind adapters rather than panel code?
- [ ] Did dependency/legal notices change exactly with dependency reality?
- [ ] Did agent-surface drift checks update only the owning surfaces?
- [ ] Did validation evidence include the first-party editor lane and final full validation?
