# First-Party Runtime UI And Editor Platform Production v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close the remaining first-party runtime UI and visible UI editor production claim gaps for text shaping, real font loading/rasterization, native IME sessions, OS accessibility publication, renderer texture upload execution, and clean-room provenance without copying or claiming Unity, Unreal Engine, or Godot compatibility.

**Architecture:** Keep `mirakana::ui` as the runtime game UI contract and `mirakana::editor` / `MK_editor_core` as GUI-independent editor model contracts. Move platform and renderer side effects behind narrow first-party adapter boundaries: Windows DirectWrite, TSF, UI Automation, and D3D12 are the first selected concrete proof lanes; Core Text/InputMethodKit/NSAccessibility, HarfBuzz/FreeType/Fontconfig/AT-SPI, Android, iOS, Vulkan, and Metal stay host-gated or dependency-gated until their exact tasks pass.

**Tech Stack:** C++23, `MK_ui`, `MK_ui_renderer`, `MK_platform_win32`, `MK_runtime_rhi`, `MK_renderer`, `MK_editor_core`, `MK_editor`, PowerShell 7 repository wrappers, CMake/CTest, Windows DirectWrite, Windows TSF, Windows UI Automation, D3D12 upload/resource APIs, optional HarfBuzz/FreeType/Fontconfig/Core Text/AT-SPI/Metal/Vulkan/mobile adapters behind legal and host gates.

---

**Plan ID:** `first-party-runtime-ui-and-editor-platform-production-v1`

**Status:** Candidate implementation milestone. This plan does not change `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` until an operator explicitly selects it.

**Date:** 2026-06-24

## Scope Decision

This plan treats the following items as intentionally unclaimed production-scope gaps, not as missing design knowledge:

- Actual visible runtime UI editor/authoring product claim.
- Production text shaping beyond current value rows and selected/editor-private evidence.
- Real font loading and glyph rasterization beyond current value rows and cooked atlas evidence.
- Native IME session publication beyond current Win32/selected evidence rows.
- OS accessibility publication for runtime UI beyond semantic rows, selected Windows UIA proof rows, and selected/editor-private evidence.
- Renderer texture upload execution for UI atlas pages beyond current handoff/review rows and the selected D3D12 proof that Task 7 adds.
- Unity, Unreal Engine, or Godot compatibility, visual parity, API parity, scene/widget import parity, or marketing equivalence.

The milestone is intentionally larger than a single PR. Each task below is a reviewable phase with its own tests, docs, and validation gate. A phase may be implemented in multiple small commits, but no phase may claim readiness until its "Done When" rows are satisfied.

## Official Source Refresh

Context7 was invoked on 2026-06-24 and selected these official documentation sources:

- Unity Manual: `/websites/unity3d_6000_0_manual`. Unity documents UI Toolkit, uGUI, IMGUI, runtime UI documents/elements/styling, and editor UI authoring categories. This plan does not copy UXML, USS, C# samples, UI Builder structure, package assets, names, or visual style.
- Unreal Engine documentation: `/websites/dev_epicgames_unreal-engine`. Epic documents UMG, Slate, HUD/widgets, widget designers, input routing, and accessibility/screen-reader categories. This plan does not copy Unreal C++/Blueprint API shapes, Slate syntax, UMG designer structure, sample assets, names, or visual style.
- Godot docs: `/godotengine/godot-docs`. Godot documents Control nodes, containers, labels/buttons/text entry, focus/accessibility, and HUD overlays. This plan does not copy Godot source, scene trees, default themes, docs examples, names, or visual style.

Official SDK/legal pages rechecked on 2026-06-24:

- Unity legal terms: <https://unity.com/legal/editor-terms-of-service/software>
- Unreal Engine EULA: <https://www.unrealengine.com/eula/unreal>
- Godot license: <https://godotengine.org/license/>
- U.S. Copyright Office copyright FAQ: <https://www.copyright.gov/help/faq/faq-protect.html>
- Microsoft DirectWrite: <https://learn.microsoft.com/en-us/windows/win32/directwrite/direct-write-portal>
- Microsoft TSF: <https://learn.microsoft.com/en-us/windows/win32/tsf/text-services-framework>
- Microsoft UI Automation: <https://learn.microsoft.com/en-us/windows/win32/winauto/entry-uiauto-win32>
- Microsoft D3D12 texture upload: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/upload-and-readback-of-texture-data>
- Apple Core Text: <https://developer.apple.com/documentation/coretext/>
- Apple InputMethodKit: <https://developer.apple.com/documentation/inputmethodkit>
- Apple NSAccessibilityProtocol: <https://developer.apple.com/documentation/appkit/nsaccessibilityprotocol>
- HarfBuzz: <https://harfbuzz.github.io/what-is-harfbuzz.html>
- FreeType: <https://freetype.org/freetype2/docs/index.html>
- AT-SPI2: <https://www.freedesktop.org/wiki/Accessibility/AT-SPI2/>
- Vulkan staging/upload reference: <https://docs.vulkan.org/tutorial/latest/04_Vertex_buffers/02_Staging_buffer.html>

## Clean-Room And Legal Position

This is an engineering compliance plan, not legal advice. The implementer must preserve these gates:

- Use Unity, Unreal Engine, and Godot documentation only to identify general functional categories.
- Do not copy source code, snippets, sample files, editor layouts, blueprints, scene trees, default themes, fonts, icons, screenshots, visual styling, product names, serialized schema names, or public API shapes from Unity, Unreal Engine, Godot, UI middleware, books, blogs, Stack Overflow, or GitHub snippets.
- Do not create importers/loaders for Unity UXML/USS, Unreal UMG/Widget Blueprint/Slate syntax, or Godot scene/control trees in this milestone.
- Do not use `UXML`, `USS`, `UMG`, `Slate`, `Control`, `CanvasLayer`, `WidgetBlueprint`, or external engine class names in new public `mirakana::ui`, generated-game, or `MK_editor_core` APIs.
- Do not claim compatibility, parity, replacement equivalence, or visual/API similarity with Unity, Unreal Engine, or Godot.
- Do not add Unity, Epic, or Godot notices unless actual software/assets from those projects enter the distribution. This plan forbids that entry.
- Any optional dependency such as HarfBuzz, FreeType, Fontconfig, ICU, or a platform SDK redistributable requires `license-audit`, `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, and `THIRD_PARTY_NOTICES.md` updates before the adapter can report dependency-ready.
- Trademark usage is limited to docs/legal provenance discussion. Product-facing feature names, API identifiers, sample outputs, and marketing copy must use MIRAIKANAI names only.

## Repository Baseline

- `MK_ui` already owns value-based UI documents, layout, adapter contracts, text/font/IME/accessibility request planners, runtime UI workbench rows, runtime UI production stack rows, and standard widget rows.
- `MK_ui_renderer` already converts `MK_ui` submissions to renderer sprites and consumes cooked image/glyph atlas metadata through `UiRendererImagePalette`, `UiRendererGlyphAtlasPalette`, and `review_ui_renderer_atlas_handoff`.
- `MK_platform_win32` already owns Win32 text input planning, committed text conversion, text-edit commands, clipboard command mapping, and `Win32PlatformIntegrationAdapter`.
- `MK_runtime_rhi` already owns `upload_runtime_texture` and related D3D12/Vulkan/RHI upload execution patterns for runtime texture payloads.
- `MK_editor` already has a Windows native first-party shell path and private DirectWrite/TSF/UIA/D3D12 evidence. This plan does not treat editor-private evidence as automatic runtime UI production readiness.
- Current docs intentionally keep broad production text shaping, bundled/distributable project font asset loading, native IME parity, non-Windows runtime OS accessibility publication, full UIA/screen-reader parity, Vulkan/Metal renderer upload execution, and external engine parity unclaimed unless exact future proof rows land.

## Non-Negotiable Architecture Rules

1. `mirakana::ui` stays platform-neutral, renderer-neutral, middleware-neutral, and value-first.
2. `MK_editor_core` stays GUI-independent and native-handle-free.
3. `MK_editor` can host visible authoring UI, but it must not become a runtime game dependency.
4. Windows DirectWrite/TSF/UIA/D3D12 proof cannot imply macOS, Linux, Android, iOS, Vulkan, or Metal readiness.
5. HarfBuzz/FreeType/Fontconfig/ICU proof cannot enter the default build until optional dependency records and bootstrap gates pass.
6. Public runtime/editor-core APIs may not expose `HWND`, `HANDLE`, COM pointers, DirectWrite, TSF, UIA, D3D12, DXGI, Vulkan, Metal, HarfBuzz, FreeType, Fontconfig, ICU, Qt, Slint, NoesisGUI, RmlUi, Dear ImGui, SDL3, Unity, Unreal, or Godot types.
7. Package-visible counters must distinguish ready, host-gated, dependency-gated, unsupported, skipped, adapter-invoked, native-invoked, renderer-upload-invoked, and public-native-handle rows.
8. No "highest-level" claim is accepted without focused tests, package smoke counters, docs, manifest/static checks, and either full validation or an exact host/dependency blocker.

## Target Architecture

```text
MK_ui
  Public runtime UI documents, widgets, text-edit state, semantic tree,
  platform-neutral production readiness rows, first-party authoring format,
  validation, and deterministic package counters.

MK_platform_win32 / future platform adapters
  DirectWrite text shaping/font loading/rasterization, TSF text sessions,
  UI Automation providers, clipboard, window/input integration, private
  platform SDK objects, and host evidence rows.

MK_ui_renderer
  Renderer submission conversion, layer/clip/batch/scissor/mask rows,
  cooked atlas palette resolution, UI atlas residency review, and handoff
  into runtime_rhi upload execution without exposing native handles.

MK_runtime_rhi / renderer backends
  Actual UI atlas page texture upload, sampled descriptor binding, command
  recording, barriers, readback/hash proof, and selected backend package rows.

MK_editor_core
  Runtime UI document/theme authoring models, hierarchy/inspector/style
  panels, command review, undo/redo, preview model, AI-readable snapshots.

MK_editor
  Visible first-party UI editor panels over editor-core models, private
  Windows shell adapters, preview rendering, text/IME/accessibility evidence.
```

## Capability Matrix

| Capability | First selected proof | Host/dependency-gated proof | Unsupported until proven |
| --- | --- | --- | --- |
| Visible runtime UI editor | Windows `MK_editor` panel over `MK_editor_core` authoring models | macOS/Linux/mobile editor shells | Unity UI Builder, Unreal UMG Designer, Godot editor parity |
| Production text shaping | Windows DirectWrite adapter rows | Core Text, HarfBuzz/Fontconfig/ICU rows | Hand-rolled Unicode shaping, external API parity |
| Real font loading/rasterization | Windows DirectWrite/Direct2D or DirectWrite glyph analysis rows | Core Text/Core Graphics, FreeType rows | Shipping copied fonts or external engine default fonts |
| Native IME session | Windows TSF session evidence | InputMethodKit, IBus/Fcitx, Android/iOS input method rows | Native candidate UI parity across all OSes |
| Runtime OS accessibility publication | Windows UIA provider rows | NSAccessibility, AT-SPI2, Android/iOS accessibility rows, full UIA pattern/event parity | Screen-reader parity across all OSes from semantic rows alone |
| Renderer texture upload execution | D3D12 UI atlas upload/readback/hash rows through `MK_runtime_rhi` | Vulkan/Metal backend rows | Public renderer upload API from gameplay UI |
| Clean-room provenance | Static and package-visible clean-room rows | Legal review updates for new dependencies | Compatibility, visual parity, or API parity claims |

## Files

### Runtime UI And Renderer

- Modify: `engine/ui/include/mirakana/ui/runtime_ui_production_stack.hpp`
- Modify: `engine/ui/src/runtime_ui_production_stack.cpp`
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Create: `engine/ui/include/mirakana/ui/runtime_ui_platform_production.hpp`
- Create: `engine/ui/src/runtime_ui_platform_production.cpp`
- Modify: `engine/ui/CMakeLists.txt`
- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`
- Modify: `engine/ui_renderer/CMakeLists.txt`
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
- Modify: `engine/runtime_rhi/src/runtime_upload.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`

### Platform Adapters

- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_ui_text_font.hpp`
- Create: `engine/platform/win32/src/win32_ui_text_font.cpp`
- Modify: `engine/platform/win32/include/mirakana/platform/win32/win32_text_input.hpp`
- Modify: `engine/platform/win32/src/win32_text_input.cpp`
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_ui_accessibility.hpp`
- Create: `engine/platform/win32/src/win32_ui_accessibility.cpp`
- Modify: `engine/platform/win32/CMakeLists.txt`

### Editor

- Create: `editor/core/include/mirakana/editor/runtime_ui_authoring.hpp`
- Create: `editor/core/src/runtime_ui_authoring.cpp`
- Modify: `editor/core/include/mirakana/editor/ui_model.hpp`
- Modify: `editor/core/src/ui_model.cpp`
- Modify: `editor/core/CMakeLists.txt`
- Modify: `editor/src/first_party_editor_document.hpp`
- Modify: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/src/win32_first_party_editor_host.hpp`
- Modify: `editor/src/win32_first_party_editor_host.cpp`
- Modify: `editor/CMakeLists.txt`

### Tests And Package Smokes

- Create: `tests/unit/runtime_ui_platform_production_tests.cpp`
- Create: `tests/unit/win32_ui_text_font_tests.cpp`
- Modify: `tests/unit/win32_platform_tests.cpp`
- Modify: `tests/unit/ui_renderer_tests.cpp`
- Modify: `tests/unit/runtime_rhi_tests.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Create: `tools/validate-runtime-ui-platform-production.ps1`
- Create: `tools/check-first-party-ui-clean-room.ps1`

### Docs, Legal, And Agent Surface

- Create: `docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md`
- Modify: `docs/ui.md`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/dependencies.md` only when optional dependencies are selected.
- Modify: `docs/legal-and-licensing.md` only when optional dependencies or distributable third-party material are selected.
- Modify: `THIRD_PARTY_NOTICES.md` only when optional dependencies or distributable third-party material are selected.
- Modify: `vcpkg.json` only when optional dependencies are selected.
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/006-runtimeBackendReadiness.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only when the operator selects this milestone as active.
- Create: `tools/check-ai-integration-123-runtime-ui-platform-production.ps1`
- Generate: `engine/agent/manifest.json`

## Task 0 - Register Candidate Plan Without New Claims

**Goal:** Make the plan discoverable while preserving current runtime/editor claims.

- [x] Add this file to `docs/superpowers/plans/`.
- [x] Add a candidate pointer to `docs/superpowers/plans/README.md`.
- [x] Do not edit `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` in this task.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
git diff --check
```

**Expected:** All checks pass. If a check requires an active-plan pointer, remove the candidate registry text instead of changing the manifest pointer.

**Done When:** The repository contains the plan and registry link only; no runtime, editor, manifest-readiness, dependency, or legal notice claim has changed.

**Implementation Evidence (2026-06-24):** Task 0 landed through PR #776 and merge commit `ebc68438af2b8bfd6ccf3479e5fe29befb987158`. It added this candidate plan and the registry pointer only; the production-loop manifest active pointer stayed on the master selection gate.

## Task 1 - Clean-Room Source Ledger And Static Guard

**Goal:** Convert the legal/originality rules into enforceable repository evidence before runtime/editor code changes.

**Files:**

- Create: `docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md`
- Create: `tools/check-first-party-ui-clean-room.ps1`
- Create: `tools/check-ai-integration-123-runtime-ui-platform-production.ps1`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `docs/legal-and-licensing.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/current-capabilities.md`

- [x] Write the ledger with these exact sections: `Allowed Sources`, `Forbidden Inputs`, `Forbidden Public Names`, `Dependency Entry Gate`, `Trademark And Marketing Non-Claims`, `Review Evidence`.
- [x] Add allowed rows for official Unity/Unreal/Godot category docs only, Microsoft SDK docs, Apple SDK docs, HarfBuzz docs, FreeType docs, AT-SPI2 docs, Vulkan docs, W3C accessibility practices, and repository-owned code.
- [x] Add forbidden rows for external source, docs examples copied as implementation, sample assets, marketplace assets, default themes, fonts, icons, screenshots, UXML/USS/UMG/Slate/Godot scene/control tree imports, and visual/API parity claims.
- [x] Add `tools/check-first-party-ui-clean-room.ps1` to scan new public headers, `games/`, and `editor/core/include` for forbidden public identifiers:

```powershell
@(
  'UXML','USS','UMG','Slate','WidgetBlueprint','UnityEngine',
  'UnityEditor','Godot','CanvasLayer','ControlNode','UnrealEd',
  'BlueprintGraph','DearImGui','ImGui','RmlUi','Noesis','Slint','Qt'
)
```

- [x] The script must fail if a forbidden token appears outside docs/legal/source-ledger references and must print `first-party-ui-clean-room: ok` on success.
- [x] Register the script from `tools/check-ai-integration.ps1` through the static-contract ledger pattern used by existing chapter checks.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-first-party-ui-clean-room.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
git diff --check
```

**Expected:** The new script reports `first-party-ui-clean-room: ok`; agent checks pass.

**Done When:** Implementation cannot add public UI/editor/game API tokens that imply Unity/Unreal/Godot/middleware copying without a static failure.

**Implementation Evidence (2026-06-24):** Task 1 adds `docs/specs/2026-06-24-first-party-ui-clean-room-source-ledger-v1.md`, `tools/check-first-party-ui-clean-room.ps1`, and `tools/check-ai-integration-123-runtime-ui-platform-production.ps1`. The static-contract ledger discovers the new chapter automatically by numeric prefix; the entry script remains unchanged by design.

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-first-party-ui-clean-room.ps1` | Passed; printed `first-party-ui-clean-room: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed after the new chapter invoked the clean-room guard. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed; no manifest compose drift. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` | Passed; no dependency records or vcpkg feature changes were required. |
| `git diff --check` | Passed. |

## Task 2 - Runtime UI Platform Production Gate

**Goal:** Add one exact production gate that names every previously unclaimed UI platform capability and rejects broad claims unless selected evidence exists.

**Files:**

- Create: `engine/ui/include/mirakana/ui/runtime_ui_platform_production.hpp`
- Create: `engine/ui/src/runtime_ui_platform_production.cpp`
- Modify: `engine/ui/include/mirakana/ui/runtime_ui_production_stack.hpp`
- Modify: `engine/ui/src/runtime_ui_production_stack.cpp`
- Modify: `engine/ui/CMakeLists.txt`
- Create: `tests/unit/runtime_ui_platform_production_tests.cpp`

- [x] Add public first-party types with these names: `RuntimeUiPlatformProductionFeature`, `RuntimeUiPlatformProductionProofKind`, `RuntimeUiPlatformProductionEvidenceRow`, `RuntimeUiPlatformProductionResult`, and `evaluate_runtime_ui_platform_production`.
- [x] Required feature enum rows: `visible_ui_editor`, `production_text_shaping`, `real_font_loading`, `font_rasterization`, `native_ime_session`, `os_accessibility_publication`, `renderer_texture_upload_execution`, `clean_room_provenance`, `external_engine_parity_non_claim`.
- [x] Required proof enum rows: `first_party_contract`, `official_sdk_adapter`, `audited_dependency_adapter`, `selected_package_counter`, `visible_editor_shell`, `host_gate`, `dependency_gate`, `unsupported_non_claim`.
- [x] Add diagnostics for missing feature rows, duplicate row ids, public native handles, dependency not recorded, host evidence missing, renderer upload missing, external engine parity claim, middleware API exposure, copied external source, copied external asset, and row budget overflow.
- [x] Add RED tests that fail before implementation for missing `external_engine_parity_non_claim`, missing `clean_room_provenance`, missing renderer upload execution, native-handle exposure, and a broad production claim without all required selected rows.
- [x] Implement the gate so `result.ready == true` only when all selected Windows/D3D12 rows are ready and all cross-platform rows are either host-gated/dependency-gated with explicit blockers or unsupported non-claims.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_platform_production_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_platform_production_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

Validation evidence:

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_platform_production_tests` failed before implementation with missing `mirakana/ui/runtime_ui_platform_production.hpp`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_platform_production_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_platform_production_tests"` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_ui_production_stack_tests` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_ui_production_stack_tests|MK_runtime_ui_platform_production_tests"` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.

**Expected:** Focused tests pass; public API boundary check confirms no native/platform/middleware types in public `mirakana::ui` headers.

**Done When:** The engine has a single fail-closed production-readiness evaluator that prevents value-only rows from becoming production UI platform claims.

## Task 3 - Windows DirectWrite Text Shaping Adapter

**Goal:** Turn production text shaping from a value-row claim into selected Windows official-SDK adapter evidence.

**Files:**

- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_ui_text_font.hpp`
- Create: `engine/platform/win32/src/win32_ui_text_font.cpp`
- Modify: `engine/platform/win32/CMakeLists.txt`
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Create: `tests/unit/win32_ui_text_font_tests.cpp`
- Modify: `tests/unit/runtime_ui_platform_production_tests.cpp`

- [x] Add `Win32UiTextShapeRequest`, `Win32UiTextShapeResult`, `Win32UiTextGlyphRow`, `Win32UiTextBoundaryRow`, and `shape_win32_ui_text_with_directwrite`.
- [x] Keep all COM and DirectWrite interfaces inside `.cpp` or private implementation details; the public header may contain only first-party value rows, strings, numbers, and booleans.
- [x] Require strict UTF-8 input, font family id, pixel size, direction, language tag, script tag, max width, and row budget.
- [x] The result must include glyph ids, cluster byte offsets on UTF-8 scalar boundaries, advance/offset pairs, fallback family rows, direction/script/language rows, line-break rows, bidi run rows, and diagnostic rows.
- [x] Add tests for ASCII, Japanese text, mixed LTR/RTL row evidence, missing font family, invalid UTF-8, duplicate clusters, missing fallback rows, and public native handle access set to zero.
- [x] Connect successful Windows proof rows into `RuntimeUiPlatformProductionEvidenceRow` as `production_text_shaping` with `official_sdk_adapter`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_font_tests MK_runtime_ui_platform_production_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_ui_text_font_tests|MK_runtime_ui_platform_production_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Expected:** Windows DirectWrite adapter evidence passes on Windows hosts; non-Windows hosts fail closed as host-gated without promoting readiness.

**Done When:** `production_text_shaping` can become ready only through selected Windows DirectWrite evidence, while Core Text and HarfBuzz/Fontconfig rows remain explicitly gated.

**Implementation Evidence (2026-06-24):** Task 3 adds `engine/platform/win32/include/mirakana/platform/win32/win32_ui_text_font.hpp`, `engine/platform/win32/src/win32_ui_text_font.cpp`, `MK_win32_ui_text_font_tests`, `Win32UiTextFallbackFamilyRow`, `validate_win32_ui_text_shape_result_rows`, `make_win32_directwrite_text_shaping_production_evidence`, and selected `runtime-ui-platform.text-shaping.win32.directwrite` evidence conversion. The adapter uses Windows SDK DirectWrite behind private `.cpp` COM boundaries, emits value-only glyph, fallback, script/language, line-break, bidi, diagnostics, and no-public-native-handle rows, and keeps Core Text plus HarfBuzz/Fontconfig rows unclaimed.

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_font_tests` before implementation | Failed as expected on missing `mirakana/platform/win32/win32_ui_text_font.hpp`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_font_tests MK_runtime_ui_platform_production_tests MK_ui_renderer_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_ui_text_font_tests|MK_runtime_ui_platform_production_tests|MK_ui_renderer_tests"` | Passed. |

## Task 4 - Real Font Loading And Glyph Rasterization

**Goal:** Prove real font loading and rasterization through official/adapted font engines, not through placeholder glyph rows.

**Files:**

- Modify: `engine/platform/win32/include/mirakana/platform/win32/win32_ui_text_font.hpp`
- Modify: `engine/platform/win32/src/win32_ui_text_font.cpp`
- Modify: `tests/unit/win32_ui_text_font_tests.cpp`
- Modify: `tests/unit/runtime_ui_platform_production_tests.cpp`
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`

- [x] Add `Win32UiFontSourceKind`, `Win32UiFontLicenseStatus`, `Win32UiFontLoadRequest`, `Win32UiFontFaceRow`, `Win32UiGlyphRasterRequest`, `Win32UiGlyphRasterResult`, and `rasterize_win32_ui_glyph`.
- [x] Require font family, resolved face id, source kind (`system_font_collection` or `project_font_asset`), glyph id, pixel size, DPI scale, pixel format, bitmap dimensions, bearing, advance, atlas padding, and row budget.
- [x] Reject embedded or distributable fonts without a source/provenance row and license status.
- [x] Add tests for missing font provenance, missing face id, zero pixel size, missing bitmap pixels, invalid metrics, color glyph row handling, atlas overflow, and zero public native handle exposure.
- [x] Add sample package counters: `runtime_ui_font_loading_rows`, `runtime_ui_glyph_raster_rows`, `runtime_ui_glyph_bitmap_rows`, `runtime_ui_glyph_metrics_rows`, `runtime_ui_font_license_rows`, and `runtime_ui_font_native_handles_exposed=0`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_font_tests MK_runtime_ui_platform_production_tests MK_ui_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_ui_text_font_tests|MK_runtime_ui_platform_production_tests|MK_ui_renderer_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package
Push-Location out/build/desktop-runtime/games/Debug/sample_2d_desktop_runtime_package
.\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-font-rasterization
Pop-Location
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Expected:** Real glyph bitmap/metrics rows pass on supported Windows hosts; package counters remain zero/host-gated when host font evidence is unavailable.

**Done When:** `real_font_loading` and `font_rasterization` are separate evidence rows and neither can be inferred from text shaping alone.

**Implementation Evidence (2026-06-24):** Task 4 adds selected Windows DirectWrite font-loading and glyph-rasterization evidence through `Win32UiFontSourceKind`, `Win32UiFontLicenseStatus`, `Win32UiFontLoadRequest`, `Win32UiFontFaceRow`, `Win32UiGlyphRasterRequest`, `Win32UiGlyphRasterResult`, `Win32UiFontDiagnostic`, `load_win32_ui_font_face`, `rasterize_win32_ui_glyph`, `validate_win32_ui_font_load_result_rows`, `validate_win32_ui_glyph_raster_result_rows`, `make_win32_directwrite_font_loading_production_evidence`, and `make_win32_directwrite_font_rasterization_production_evidence`. The adapter uses the Windows SDK system font collection and DirectWrite glyph run analysis behind private `.cpp` COM boundaries, emits value-only provenance/license, face, bitmap, metrics, color-check, diagnostics, and no-public-native-handle rows, and keeps project font asset loading plus non-Windows font engines unclaimed.

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_font_tests` before implementation | Failed as expected on missing Task 4 font/raster APIs. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_ui_text_font_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_ui_text_font_tests"` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package` | Passed. |
| `.\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-font-rasterization` from the packaged sample directory | Passed with `runtime_ui_font_loading_rows=1`, `runtime_ui_glyph_raster_rows=1`, `runtime_ui_glyph_bitmap_rows=1`, `runtime_ui_glyph_metrics_rows=1`, `runtime_ui_font_license_rows=1`, `runtime_ui_font_native_handles_exposed=0`, and `runtime_ui_font_rasterization_diagnostics=0`. |

## Task 5 - Native Windows TSF IME Session

**Goal:** Promote runtime text input from copied Win32 messages/value rows to a selected native TSF session proof.

**Files:**

- Modify: `engine/platform/win32/include/mirakana/platform/win32/win32_text_input.hpp`
- Modify: `engine/platform/win32/src/win32_text_input.cpp`
- Modify: `tests/unit/win32_platform_tests.cpp`
- Modify: `tests/unit/runtime_ui_platform_production_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`

- [x] Add `Win32TsfTextSessionDesc`, `Win32TsfCompositionRow`, `Win32TsfCandidateIntentRow`, `Win32TsfTextAreaRow`, and `plan_win32_tsf_text_session`.
- [x] Record TSF thread manager availability, document manager availability, context availability, focus sink rows, text store lock rows, composition begin/update/end rows, committed text rows, candidate intent rows, caret/text area rectangles, and unsupported native candidate UI parity rows.
- [x] Keep TSF COM interfaces private to `.cpp` implementation details.
- [x] Add tests for missing active target, invalid caret rect, invalid UTF-8 surrounding text, composition update without begin row, committed text target mismatch, candidate rows without session, public native handle exposure, and broad IME parity claim.
- [x] Add package counters: `runtime_ui_tsf_session_ready`, `runtime_ui_tsf_composition_rows`, `runtime_ui_tsf_candidate_intent_rows`, `runtime_ui_tsf_text_area_rows`, `runtime_ui_ime_native_candidate_ui_ready=0`, `runtime_ui_ime_cross_platform_ready=0`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_platform_production_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_platform_tests|MK_runtime_ui_platform_production_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package
out/build/desktop-runtime/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-tsf-session
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Expected:** Windows TSF proof rows pass or report exact host gate; no macOS/Linux/mobile IME readiness is inferred.

**Done When:** `native_ime_session` has selected Windows TSF evidence and all other platform IME rows remain host-gated with exact blockers.

**Implementation Evidence (2026-06-24):** Task 5 adds selected Windows TSF native IME session evidence through `Win32TsfTextSessionDesc`, `Win32TsfCompositionRow`, `Win32TsfCandidateIntentRow`, `Win32TsfTextAreaRow`, `Win32TsfTextSessionResult`, `plan_win32_tsf_text_session`, and `make_win32_tsf_native_ime_production_evidence`. The adapter creates the TSF thread manager, document manager, context, and a private minimal `ITextStoreACP` / `ITfContextOwnerCompositionSink` implementation inside `win32_text_input.cpp`, emits value-only thread/document/context/focus/text-store-lock/composition/committed/candidate/text-area rows, and keeps TSF COM interfaces, HWND/native handles, native candidate UI, reconversion, virtual keyboard policy, and all non-Windows IME readiness unclaimed.

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_platform_production_tests` before implementation | Failed as expected on missing Task 5 TSF APIs. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_platform_production_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_platform_tests|MK_runtime_ui_platform_production_tests"` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package` | Passed. |
| `out/build/desktop-runtime/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-tsf-session` | Passed with `status=completed`, `runtime_ui_tsf_session_ready=1`, `runtime_ui_tsf_composition_rows=3`, `runtime_ui_tsf_candidate_intent_rows=1`, `runtime_ui_tsf_text_area_rows=1`, `runtime_ui_ime_native_candidate_ui_ready=0`, `runtime_ui_ime_cross_platform_ready=0`, and `runtime_ui_tsf_diagnostics=0`. |

## Task 6 - Runtime Windows UI Automation Publication

**Goal:** Publish runtime UI accessibility through a selected Windows UIA provider proof while keeping semantic rows platform-neutral.

**Files:**

- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_ui_accessibility.hpp`
- Create: `engine/platform/win32/src/win32_ui_accessibility.cpp`
- Modify: `engine/platform/win32/CMakeLists.txt`
- Modify: `tests/unit/win32_platform_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`

- [x] Add `Win32UiaRuntimeNodeRow`, `Win32UiaProviderPublicationDesc`, `Win32UiaProviderPublicationResult`, `publish_runtime_ui_to_win32_uia`, and `make_win32_uia_accessibility_publication_production_evidence`.
- [x] Require role, name, description, state, focus, action rows, parent/child relationships, reading order, live region status, keyboard pattern, screen-space bounds, provider root id, child runtime ids, and event-publication counters.
- [x] Keep `IRawElementProviderSimple`, COM, `HWND`, and UIA types out of public `mirakana::ui` and generated-game headers.
- [x] Add tests for missing accessible names, duplicate runtime ids, invalid bounds, focusable node without action pattern, child without parent, unsupported pattern claim, event claim without provider root, and public native handle exposure.
- [x] Add package counters: `runtime_ui_uia_provider_ready`, `runtime_ui_accessibility_nodes`, `runtime_ui_accessibility_action_rows`, `runtime_ui_accessibility_event_rows`, `runtime_ui_accessibility_cross_platform_ready=0`, `runtime_ui_accessibility_native_handles_exposed=0`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_platform_production_tests sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_platform_tests|MK_runtime_ui_platform_production_tests|sample_2d_desktop_runtime_package"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Expected:** UIA publication evidence passes on Windows hosts; NSAccessibility, AT-SPI2, Android, and iOS rows remain host-gated.

**Done When:** Runtime UI OS accessibility publication is ready only for the selected Windows UIA proof row and cannot be inferred from semantic accessibility rows alone.

### Task 6 Implementation Evidence - 2026-06-24

| Command / check | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_platform_production_tests` before implementation | RED: failed as expected because `mirakana/platform/win32/win32_ui_accessibility.hpp` did not exist. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_platform_tests MK_runtime_ui_platform_production_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_platform_tests|MK_runtime_ui_platform_production_tests"` | Passed: 2/2 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package` | Passed after the first long build completed and the incremental rerun finished cleanly. |
| `out/build/desktop-runtime/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-uia-publication` | Passed with `status=completed`, `runtime_ui_uia_provider_ready=1`, `runtime_ui_accessibility_nodes=2`, `runtime_ui_accessibility_action_rows=1`, `runtime_ui_accessibility_event_rows=1`, `runtime_ui_accessibility_cross_platform_ready=0`, `runtime_ui_accessibility_native_handles_exposed=0`, and `runtime_ui_uia_diagnostics=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed: `first-party-ui-clean-room: ok`, `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/platform/win32/src/win32_ui_accessibility.cpp,tests/unit/win32_platform_tests.cpp"` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files "games/sample_2d_desktop_runtime_package/main.cpp"` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: static checks, build, and 154/154 CTest tests. |

## Task 7 - UI Atlas Renderer Texture Upload Execution

**Goal:** Promote UI atlas handoff into actual selected renderer texture upload execution without exposing upload APIs to gameplay UI.

**Files:**

- Modify: `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp`
- Modify: `engine/ui_renderer/src/ui_renderer.cpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp`
- Modify: `engine/runtime_rhi/src/runtime_upload.cpp`
- Modify: `games/CMakeLists.txt`
- Modify: `tests/unit/ui_renderer_tests.cpp`
- Modify: `tests/unit/runtime_rhi_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`

- [x] Add `RuntimeUiAtlasTextureUploadDesc`, `RuntimeUiAtlasTextureUploadResult`, and `execute_runtime_ui_atlas_texture_upload` under `mirakana::runtime_rhi`.
- [x] Input must be cooked `GameEngine.UiAtlas.v1` page payloads and first-party texture payloads already present in the runtime package; no runtime source image decode is allowed.
- [x] Require row pitch, upload buffer allocation, `CopyTextureRegion` or backend-equivalent copy count, resource transition count, sampled descriptor write count, fence, optional readback checksum, owner-device pointer retained only in private result evidence, and `native_handle_accessed=false`.
- [x] Add D3D12 selected proof first. Add Vulkan and Metal rows as host-gated until their backend-specific upload/readback tests land.
- [x] Add tests for missing atlas page payload, missing upload ring, row-pitch mismatch, descriptor write missing, readback checksum mismatch, backend inference, public native handle exposure, and gameplay-requested renderer upload API.
- [x] Add package counters: `runtime_ui_atlas_upload_ready`, `runtime_ui_atlas_upload_backend=d3d12`, `runtime_ui_atlas_upload_uploaded_bytes`, `runtime_ui_atlas_upload_copy_to_texture_count`, `runtime_ui_atlas_upload_resource_transitions`, `runtime_ui_atlas_upload_descriptor_writes`, `runtime_ui_atlas_upload_readback_checksum`, `runtime_ui_renderer_texture_upload_public_api=0`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests MK_ui_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_tests|MK_ui_renderer_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package
out/build/desktop-runtime/games/Debug/sample_2d_desktop_runtime_package/sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-atlas-upload --require-runtime-ui-renderer-atlas-handoff
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package_smoke"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Implementation Evidence (2026-06-24):**

| Validation | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests MK_ui_renderer_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_rhi_tests|MK_ui_renderer_tests"` | Passed: 2/2 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package` | Passed after the first long build completed and the incremental rerun finished cleanly. |
| `sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-runtime-ui-atlas-upload --require-runtime-ui-renderer-atlas-handoff` | Passed with `runtime_ui_atlas_upload_ready=1`, `runtime_ui_atlas_upload_backend=d3d12`, `runtime_ui_atlas_upload_uploaded_bytes=256`, `runtime_ui_atlas_upload_readback_checksum_matched=1`, `runtime_ui_atlas_upload_sampled_descriptor_written=1`, `runtime_ui_atlas_upload_d3d12_selected_proof_ready=1`, `runtime_ui_renderer_texture_upload_public_api=0`, `runtime_ui_atlas_upload_native_handle_accessed=0`, `runtime_ui_renderer_atlas_handoff_texture_upload_execution_rows=1`, and `runtime_ui_atlas_upload_diagnostics=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package_smoke"` | Passed: 1/1 test. |

**Expected:** D3D12 upload execution passes on Windows/D3D12 hosts; Vulkan/Metal rows remain explicit host gates.

**Done When:** `renderer_texture_upload_execution` is backed by actual selected D3D12 upload/readback evidence and does not become a public gameplay UI upload API.

## Task 8 - GUI-Independent Runtime UI Authoring Model

**Goal:** Build the editor-core model for editing runtime UI documents/themes without adopting Unity/Unreal/Godot editor models.

**Files:**

- Create: `editor/core/include/mirakana/editor/runtime_ui_authoring.hpp`
- Create: `editor/core/src/runtime_ui_authoring.cpp`
- Modify: `editor/core/include/mirakana/editor/ui_model.hpp`
- Modify: `editor/core/src/ui_model.cpp`
- Modify: `editor/core/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add `EditorRuntimeUiDocumentModel`, `EditorRuntimeUiThemeModel`, `EditorRuntimeUiHierarchyRow`, `EditorRuntimeUiInspectorRow`, `EditorRuntimeUiStyleTokenRow`, `EditorRuntimeUiPreviewModel`, and `make_editor_runtime_ui_authoring_model`.
- [x] Add reviewed commands: `runtime_ui.element.add`, `runtime_ui.element.remove`, `runtime_ui.element.reorder`, `runtime_ui.element.select`, `runtime_ui.property.edit_text`, `runtime_ui.style.set_token`, `runtime_ui.preview.refresh`.
- [x] Commands must require expected document revision and must be undoable through existing editor history patterns.
- [x] Reject Unity/Godot/Unreal schema ids, copied external visual theme ids, native handle rows, renderer execution requests, package script execution, and validation recipe execution from editor-core command handling.
- [x] Add tests for hierarchy rows, inspector rows, style token editing, undo/redo, stale revision rejection, invalid schema rejection, external engine import token rejection, and preview model creation without native handles.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

**Expected:** Editor-core runtime UI authoring is deterministic and native-handle-free.

**Done When:** A visible shell can render runtime UI authoring without owning document semantics or copying external engine authoring models.

**Evidence:** Task 8 merged in PR #789 / merge `094f2ec56fcc08943c72129d6747d847e08a42d5` with hosted PR Gate, Windows MSVC, Windows Native Desktop Editor, Linux, and CodeQL passing.

## Task 9 - Visible First-Party UI Editor Panel

**Goal:** Expose an actual visible runtime UI editor panel inside `MK_editor` using first-party editor-core models.

**Files:**

- Modify: `editor/src/first_party_editor_document.hpp`
- Modify: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `editor/core/include/mirakana/editor/editor_panel.hpp`
- Modify: `editor/core/src/workspace.cpp`
- Modify: `editor/core/src/editor_dock_layout.cpp`
- Modify: `editor/core/src/ai_operation_surface.cpp`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `engine/agent/manifest.fragments/005-applications.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`

- [x] Add a retained panel id `runtime_ui_editor` to the first-party editor panel catalog.
- [x] Render hierarchy, inspector, style tokens, preview status, clean-room provenance status, text/font readiness, IME readiness, accessibility readiness, and renderer upload readiness rows from `EditorRuntimeUiPreviewModel` and the selected first-party shell evidence rows.
- [x] Use existing `mirakana::ui::UiDocument` rendering and `MK_ui_renderer` submissions for the panel.
- [x] Do not use Dear ImGui, Qt, Slint, RmlUi, NoesisGUI, Unity UI Builder, Unreal UMG Designer, Godot scene tree, external icons, copied layouts, or public native handles.
- [x] Add smoke counters: `editor_runtime_ui_editor_panel_visible=1`, `editor_runtime_ui_editor_hierarchy_rows`, `editor_runtime_ui_editor_inspector_rows`, `editor_runtime_ui_editor_style_rows`, `editor_runtime_ui_editor_preview_rows`, `editor_runtime_ui_editor_external_engine_parity_claim=0`, `editor_runtime_ui_editor_native_handles_exposed=0`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
```

**Task 9 Focused Evidence (2026-06-24):**

| Validation | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_native_shell_tests"` | Passed: 1/1 test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1` | Passed: `desktop-editor` configured/built and CTest passed 155/155, including `MK_editor_smoke`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |

**Expected:** The visible editor shell reports the new runtime UI editor panel and zero external parity/native handle rows.

**Done When:** The actual visible UI editor claim is backed by editor-core tests, native shell tests, and `tools/build-editor.ps1`.

## Task 10 - Cross-Platform Adapter Gates

**Goal:** Record exact platform/dependency gates without pretending Windows proof is universal.

**Files:**

- Modify: `engine/ui/include/mirakana/ui/runtime_ui_platform_production.hpp`
- Modify: `engine/ui/src/runtime_ui_platform_production.cpp`
- Modify: `docs/ui.md`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `engine/agent/manifest.fragments/006-runtimeBackendReadiness.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`

- [ ] Add gate ids:

```text
runtime_ui.adapter.windows.directwrite
runtime_ui.adapter.windows.tsf
runtime_ui.adapter.windows.uia
runtime_ui.upload.windows.d3d12
runtime_ui.adapter.macos.core_text
runtime_ui.adapter.macos.input_method_kit
runtime_ui.adapter.macos.nsaccessibility
runtime_ui.adapter.linux.harfbuzz_fontconfig
runtime_ui.adapter.linux.freetype
runtime_ui.adapter.linux.at_spi
runtime_ui.adapter.android.text_input
runtime_ui.adapter.android.accessibility
runtime_ui.adapter.ios.uitextinput
runtime_ui.adapter.ios.uiaccessibility
runtime_ui.upload.vulkan
runtime_ui.upload.metal
```

- [ ] Mark only implemented Windows/D3D12 rows as selected proof. Mark all unimplemented rows as host-gated or dependency-gated with exact blocker strings.
- [ ] Do not add optional dependencies in this task.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

**Expected:** Manifest/docs identify selected, host-gated, dependency-gated, and unsupported rows without changing unrelated production gap state.

**Done When:** Cross-platform UI capability claims are machine-readable and fail closed.

## Task 11 - Optional Dependency Adapter Selection Gate

**Goal:** Define the exact rule for HarfBuzz/FreeType/Fontconfig/ICU-style dependencies without adding them implicitly.

**Files:**

- Modify: `vcpkg.json` only if a dependency is selected.
- Modify: `docs/dependencies.md` only if a dependency is selected.
- Modify: `docs/legal-and-licensing.md` only if a dependency is selected.
- Modify: `THIRD_PARTY_NOTICES.md` only if a dependency is selected.
- Modify: `tools/bootstrap-deps.ps1` only if a vcpkg feature is selected.
- Modify: `tools/check-dependency-policy.ps1` only if dependency policy needs new assertions.

- [ ] If HarfBuzz is selected, add it behind a non-default vcpkg feature named `runtime-ui-harfbuzz`.
- [ ] If FreeType is selected, add it behind a non-default vcpkg feature named `runtime-ui-freetype`.
- [ ] If Fontconfig is selected, add it behind a non-default vcpkg feature named `runtime-ui-fontconfig`.
- [ ] Add notices with name, source URL, retrieved date, version or commit, copyright holder, SPDX license, modification status, and distribution target.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

**Expected:** Dependency selection is explicit, optional, reproducible, and legally recorded.

**Done When:** No optional text/font dependency can be used by production UI code without manifest, dependency, legal, notice, and bootstrap evidence.

## Task 12 - Package Smoke And Validation Wrapper

**Goal:** Make the selected runtime UI platform production proof executable through one supported repository entrypoint.

**Files:**

- Create: `tools/validate-runtime-ui-platform-production.ps1`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `tools/check-ai-integration-123-runtime-ui-platform-production.ps1`

- [ ] Add `tools/validate-runtime-ui-platform-production.ps1` with parameters `-RequireReady`, `-SkipEditor`, `-SkipPackage`, and `-StaticOnly`.
- [ ] The wrapper must build and run selected tests:

```powershell
MK_runtime_ui_platform_production_tests
MK_win32_ui_text_font_tests
MK_win32_platform_tests
MK_ui_renderer_tests
MK_runtime_rhi_tests
MK_editor_core_tests
MK_editor_native_shell_tests
```

- [ ] The wrapper must run `sample_2d_desktop_runtime_package --smoke --require-runtime-ui-platform-production` unless `-SkipPackage` is passed.
- [ ] Required package counters under `-RequireReady`:

```text
runtime_ui_platform_production_ready=1
runtime_ui_platform_clean_room_ready=1
runtime_ui_platform_external_engine_parity_claim=0
runtime_ui_platform_public_native_handles_exposed=0
runtime_ui_text_shaping_selected_adapter=directwrite
runtime_ui_font_rasterization_selected_adapter=directwrite
runtime_ui_ime_selected_adapter=tsf
runtime_ui_accessibility_selected_adapter=uia
runtime_ui_renderer_upload_selected_backend=d3d12
editor_runtime_ui_editor_panel_visible=1
```

- [ ] Add fail-closed diagnostics for missing counters, nonzero parity claims, nonzero public native handles, missing package rows, and host-gated selected rows under `-RequireReady`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-runtime-ui-platform-production.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-runtime-ui-platform-production.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

**Expected:** Default mode can report host/dependency gates; `-RequireReady` passes only after selected Windows/D3D12/editor package proof is complete.

**Done When:** Agents and CI can validate this milestone through one explicit wrapper.

## Task 13 - Docs, Manifest, Static Guards, And Claim Closeout

**Goal:** Align human docs, machine-readable manifest, static checks, and current capability claims.

**Files:**

- Modify: `docs/ui.md`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/006-runtimeBackendReadiness.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only if this plan is selected as active.
- Generate: `engine/agent/manifest.json`

- [ ] Update docs to say exactly which rows are ready: Windows DirectWrite text shaping, Windows real font load/raster proof, Windows TSF native IME session, Windows UIA runtime accessibility publication, D3D12 UI atlas upload execution, and visible runtime UI editor panel.
- [ ] Update docs to say exactly which rows remain host-gated or dependency-gated: Core Text, InputMethodKit, NSAccessibility, HarfBuzz/FreeType/Fontconfig, AT-SPI2, Android/iOS text input/accessibility, Vulkan upload, Metal upload.
- [ ] Preserve non-claims for Unity/UE/Godot compatibility, visual parity, API parity, serialized import parity, editor workflow parity, middleware readiness, public native handles, and all-platform UI parity.
- [ ] Compose the manifest:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
git diff --check
```

**Expected:** Docs and manifest describe the same ready/gated/unsupported surface.

**Done When:** No current-truth doc or agent surface can imply broader UI readiness than validated evidence.

## Task 14 - Final Full Validation And Publication

**Goal:** Close the selected milestone only after local validation and reviewable publication evidence exist.

- [ ] Run full local validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-runtime-ui-platform-production.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/runtime-ui-editor-platform-production
```

- [ ] Stage only task-owned files.
- [ ] Commit a validated checkpoint with a message that names the selected ready rows.
- [ ] Push the branch.
- [ ] Open a draft PR if any host-gated row remains, or a ready PR if all selected rows pass hosted checks.
- [ ] Do not mark the PR ready until selected hosted lanes and `PR Gate` pass for the PR head SHA.

**Expected:** The milestone has a validated local checkpoint and a reviewable PR with evidence.

**Done When:** The selected milestone is either merged with hosted evidence or explicitly blocked with exact host/dependency evidence, and `currentActivePlan` no longer points at a completed plan.

## Explicit Non-Claims After This Plan

Even after selected Windows/D3D12/editor proof passes, these remain unsupported until separate selected evidence lands:

- Unity UI Toolkit/uGUI/IMGUI compatibility.
- Unreal UMG/Slate/Common UI compatibility.
- Godot Control/Container/CanvasLayer compatibility.
- External engine visual parity, API parity, serialized asset import parity, or workflow parity.
- UI middleware readiness.
- macOS/Linux/Android/iOS runtime UI platform parity.
- Vulkan/Metal UI atlas upload readiness unless the corresponding backend task passes.
- Native IME candidate-window parity across all operating systems.
- Full screen-reader parity across all operating systems.
- Broad commercial UI quality beyond the exact validated rows.

## Final Verification Checklist

- [ ] `tools/check-first-party-ui-clean-room.ps1` passes.
- [ ] `tools/validate-runtime-ui-platform-production.ps1 -RequireReady` passes or records an exact host/dependency blocker.
- [ ] `tools/check-public-api-boundaries.ps1` proves no native/middleware/external-engine public type leakage.
- [ ] `tools/check-dependency-policy.ps1` passes if any optional dependency is selected.
- [ ] `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, and `tools/check-agents.ps1` pass.
- [ ] `tools/validate.ps1` passes for code/runtime/editor/public-contract slices.
- [ ] Docs, manifest fragments, validation recipes, package counters, current capabilities, roadmap, and this plan agree on ready/gated/unsupported rows.
