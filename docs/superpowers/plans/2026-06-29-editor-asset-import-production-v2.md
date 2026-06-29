# Editor Asset Import Production v2 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn the current Source Pulse asset browser import handoff into a complete, reviewed, legally gated, project-safe asset import workflow with explicit high, medium, and deferred implementation priorities.

**Architecture:** `MK_editor_core` owns deterministic value models, import candidate review rows, legal/provenance review, command planning, and retained `mirakana::ui` output. The native `MK_editor` shell owns host file dialogs, project-root canonicalization, external-file copy transactions, registry writes, importer execution, job execution, and filesystem mutation through reviewed services. `MK_tools` continues to own optional codec adapters behind the existing `asset-importers` vcpkg feature; runtime/game code consumes only cooked artifacts and never parses external source formats.

**Tech Stack:** C++23, `MK_editor_core`, `MK_editor`, `MK_assets`, `MK_tools`, `GameEngine.SourceAssetRegistry.v1`, `AssetKeyV2`, optional `asset-importers` vcpkg feature (`libspng`, `fastgltf`, `OpenEXR`, `KTX Software`, `miniaudio`), first-party `mirakana::ui`, repository PowerShell validation wrappers.

---

## Status

- **Plan ID:** `editor-asset-import-production-v2`
- **Status:** Candidate follow-up plan, not selected by `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.
- **Date:** 2026-06-29
- **Scope:** Asset import workflow after Source Pulse asset browser production v1. This plan does not replace the active renderer commercial-readiness plan and does not reopen `unsupportedProductionGaps`.
- **Priority buckets:** High priority implements a complete safe import path. Medium priority improves operator productivity and live iteration. Deferred work remains explicit non-scope until separate legal, dependency, and host evidence exists.

## Locked Current Project Facts

- `editor/src/native_editor_app.cpp` currently routes `Browse Import Sources` through `IFileDialogService`, normalizes accepted paths to project-relative paths, rejects outside-project/device/invalid paths, and invokes `execute_asset_import_plan` only after `asset_browser.import.execute_reviewed_plan` passes generation and user-confirmation review.
- `editor/core/src/content_browser_import_panel.cpp` accepts `.texture`, `.mesh`, `.material`, `.scene`, `.audio_source`, `.png`, `.gltf`, `.glb`, `.wav`, `.mp3`, and `.flac` as reviewed source selections.
- `editor/src/native_editor_app.cpp::make_default_asset_browser_import_plan` currently maps source-registry rows for `AssetKind::texture`, `AssetKind::material`, and `AssetKind::scene` only; mesh and audio rows can be visible but not imported through the default visible-shell plan.
- `engine/tools/asset/asset_import_adapters.cpp` already contains optional `PngTextureExternalAssetImporter`, `GltfMeshExternalAssetImporter`, `GltfMorphMeshCpuExternalAssetImporter`, and `AudioExternalAssetImporter` behind `MK_HAS_ASSET_IMPORTERS`.
- `engine/tools/asset/source_asset_registration_tool.cpp` already provides reviewed `plan_source_asset_registration` / `apply_source_asset_registration` for `GameEngine.SourceAssetRegistry.v1`, but the visible editor import handoff does not yet apply selected sources into the registry.
- `editor/core/src/asset_browser_production.cpp::review_editor_asset_browser_legal_provenance` already rejects missing licenses, incomplete provenance, NC/ND licenses, and external-engine material, but this is not yet a mandatory gate for source registration, external copy, import execution, and package readiness.
- `vcpkg.json` already has the optional `asset-importers` feature with `libspng`, `fastgltf`, `openexr`, `ktx`, and `miniaudio`; this plan does not add a new third-party dependency in high or medium priority work.

## Official Source Ledger

All source rows were reviewed on 2026-06-29. They are allowed only as factual API, format, dependency, or legal context. They are not sample-code, UI-expression, asset, icon, schema, or compatibility sources.

| Source | URL or Context7 ID | Implementation decision locked |
| --- | --- | --- |
| Context7 `/microsoft/vcpkg` | `/microsoft/vcpkg` | Optional importer dependencies remain behind vcpkg manifest features. CMake configure must not install packages in this repository; `tools/bootstrap-deps.ps1` remains the installer surface and presets keep `VCPKG_MANIFEST_INSTALL=OFF`. |
| Context7 `/kitware/cmake` | `/kitware/cmake` | Any CMake changes are target-scoped. Importer code is linked only from targets that opt into `MK_ENABLE_ASSET_IMPORTERS`; no global flags or default dependency leakage. |
| Context7 `/assimp/assimp` | `/assimp/assimp` | Assimp is not selected for high or medium priority because existing `fastgltf` covers the selected glTF path and broad-format importer adoption would expand legal/dependency scope. If evaluated later, it must be an optional isolated adapter with no parser type leakage. |
| Microsoft vcpkg manifest mode | `https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode` | `vcpkg.json` and its `builtin-baseline` remain the dependency authority; optional features, dependency docs, and notices must be updated together. |
| Microsoft vcpkg CMake integration | `https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration` | CMake consumes already-bootstrapped packages from `vcpkg_installed`; configure-time install is not allowed for importer work. |
| Khronos glTF 2.0 specification | `https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html` | glTF import remains source-to-first-party mesh/morph/animation document conversion. No external network fetches, runtime source parsing, parser type leakage, or Unity/Unreal/Godot workflow copying. |
| W3C PNG Third Edition | `https://www.w3.org/TR/png-3/` | PNG import accepts only audited image decode into first-party texture source/cooked rows with explicit color/pixel diagnostics. No broad image-codec readiness claim. |
| Khronos KTX 2.0 specification | `https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html` | KTX2/Basis remains explicit review/transcode-target evidence. High priority does not add editor-core upload or runtime source parsing. |
| RFC 9639 FLAC | `https://www.rfc-editor.org/rfc/rfc9639.html` | FLAC is treated as an optional source-audio input through the existing `miniaudio` adapter, not runtime codec readiness. |
| Microsoft RIFF/WAVE multimedia reference | `https://learn.microsoft.com/en-us/windows/win32/multimedia/resource-interchange-file-format` | WAV handling stays source import only, with cooked first-party audio artifacts as runtime input. |
| SPDX License List | `https://spdx.org/licenses/` | License ids in provenance rows use SPDX ids when possible; custom license references require explicit notice records. |
| Creative Commons license guide | `https://creativecommons.org/share-your-work/cclicenses/` | NC and ND asset licenses are blocked for production package use because commercial use or modification can be incompatible with engine distribution. |
| Unity trademarks | `https://unity.com/legal/trademarks` | Unity marks, logos, product expression, endorsement language, and compatibility claims are blocked. |
| Unreal Engine Content EULA | `https://www.unrealengine.com/en-US/eula/content` | Epic/Fab/Unreal marketplace or sample content is blocked unless a separate legal and technical approval record exists before implementation. |
| Godot license compliance | `https://docs.godotengine.org/en/stable/about/complying_with_licenses.html` | Godot MIT-licensed material still needs notices and cannot be copied as editor UI expression, schema compatibility, or trademarked product behavior. |

This plan is not legal advice. Any shipped external asset, font, model, texture, audio file, source code, binary, or generated asset still requires `THIRD_PARTY_NOTICES.md` records before distribution.

## Clean-Room Product Rules

- The feature remains MIRAIKANAI-first `Source Pulse`; do not use Unity Project window, Unreal Content Browser, Godot FileSystem, or any marketplace name as product branding, retained row ids, shortcuts, icons, query syntax, serialized fields, or compatibility language.
- External engine source code, sample assets, starter content, marketplace/Fab/Asset Store content, editor screenshots, icons, visual layouts, project schemas, and trademarked material are rejected by default.
- `editor/core` must never receive Win32 handles, filesystem-native paths, parser-native types, `fastgltf::*`, `spng_*`, `ktx*`, `ma_decoder`, renderer/RHI resources, process handles, package script handles, validation-recipe execution handles, or native UIA/COM handles.
- No backward-compatibility shims, aliases, migration layers, or duplicate command ids are added. If an old behavior is replaced during execution, tests must assert the old path is absent or blocked.

## File Map

High priority creates focused files instead of expanding large files:

- Create: `editor/core/include/mirakana/editor/asset_import_review.hpp`
  Owns value-only candidate rows, output-path decisions, asset-kind mapping, source-registration request planning, and import gate diagnostics.
- Create: `editor/core/src/asset_import_review.cpp`
  Implements deterministic candidate review without filesystem mutation or codec execution.
- Create: `editor/src/native_asset_import_copy.hpp`
  Declares shell-private external source copy transaction inputs/results.
- Create: `editor/src/native_asset_import_copy.cpp`
  Implements host filesystem canonicalization, target containment, temp-write/rename copy, collision policy, and failure diagnostics.
- Create: `engine/assets/include/mirakana/assets/asset_import_provenance.hpp`
  Declares `GameEngine.AssetImportProvenance.v1` rows keyed by `AssetKeyV2`.
- Create: `engine/assets/src/asset_import_provenance.cpp`
  Implements deterministic text serialization, validation, and projection to `EditorAssetBrowserLegalProvenanceRow`.
- Modify: `editor/src/native_editor_app.hpp`
  Adds reviewed source registration, copy execution, import job/result, and provenance binding entrypoints.
- Modify: `editor/src/native_editor_app.cpp`
  Wires the value review model to source-registry writes, external copy execution, import-plan rebuild, import execution, browser refresh, and generation increments.
- Modify: `editor/core/include/mirakana/editor/asset_browser_production.hpp` and `editor/core/src/asset_browser_production.cpp`
  Adds import-specific command ids only when they are not covered by existing command ids, and exposes import provenance rows in the retained model.
- Modify: `editor/core/include/mirakana/editor/content_browser_import_panel.hpp` and `editor/core/src/content_browser_import_panel.cpp`
  Reuses existing open/copy rows, adds collision/duplicate/content-hash diagnostics, and keeps all rows retained-mode.
- Modify: `editor/CMakeLists.txt`, `engine/assets/CMakeLists.txt`, and test CMake entries if new test files are split out.
- Test: `tests/unit/editor_core_tests.cpp`, `tests/unit/editor_native_shell_tests.cpp`, and `tests/unit/core_tests.cpp` or new focused test files registered under existing test targets.
- Docs/agent surface: `docs/editor.md`, `docs/current-capabilities.md`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `engine/agent/manifest.fragments/007-importerCapabilities.json`, `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, and `docs/superpowers/plans/README.md` only when the implemented behavior changes durable claims or dependencies.

## Priority Summary

### High Priority

1. Source import candidate review and deterministic output-path planning.
2. Full visible-shell import-plan mapping for texture, mesh, audio, material, scene, and supported source documents.
3. Reviewed source registration into `GameEngine.SourceAssetRegistry.v1`.
4. External source copy transaction with project-root containment and collision policy.
5. Persistent provenance/legal gate before registration, copy, import, and package readiness.
6. Import execution result refresh, diagnostics, and generated asset-record registration.
7. Focused docs, manifest, and validation sync for the completed high-priority slice.

### Medium Priority

8. Import job queue, progress snapshots, cancellation, retry, and per-file diagnostics.
9. Reviewed reimport/recook/hot-reload action rows from source hashes and dependency closure.
10. Import presets for texture, mesh, and audio with project defaults plus per-asset overrides.
11. Folder/drag-drop batch import review using the same candidate pipeline.
12. Duplicate detection, rename suggestions, and content-hash reuse.

### Deferred

13. Broad codec expansion beyond existing `asset-importers` packages.
14. Marketplace/library connectors and third-party asset catalogs.
15. External-engine project/schema import compatibility or migration.

Deferred rows are not hidden requirements. They are explicit non-scope until a separate plan updates legal records, dependency records, official-source research, validation recipes, and clean-room approval records.

## High Priority Tasks

### Task 1: Source Import Candidate Review

**Files:**
- Create: `editor/core/include/mirakana/editor/asset_import_review.hpp`
- Create: `editor/core/src/asset_import_review.cpp`
- Modify: `editor/CMakeLists.txt`
- Test: `tests/unit/editor_core_tests.cpp`

- [x] **Step 1: Write failing editor-core tests**

Add tests named:

```cpp
MK_TEST("editor asset import review maps selected sources to registry requests");
MK_TEST("editor asset import review rejects duplicate target paths and unsupported sources");
MK_TEST("editor asset import review blocks legal rows before registration");
```

The first test constructs selected project paths:

```text
assets/imported_sources/hero.png
assets/imported_sources/robot.gltf
assets/imported_sources/theme.wav
assets/materials/hero.material
assets/scenes/level.scene
```

Expected rows:

```text
hero.png -> AssetKind::texture -> GameEngine.TextureSource.v1 -> assets/imported/hero.texture
robot.gltf -> AssetKind::mesh -> GameEngine.MeshSource.v2 -> assets/imported/robot.mesh
theme.wav -> AssetKind::audio -> GameEngine.AudioSource.v1 -> assets/imported/theme.audio
hero.material -> AssetKind::material -> GameEngine.Material.v1 -> assets/materials/hero.material
level.scene -> AssetKind::scene -> GameEngine.Scene.v1 -> assets/scenes/level.scene
```

Expected command:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
```

Expected before implementation: the new tests fail to compile because `review_editor_asset_import_candidates` is missing.

- [x] **Step 2: Add the value-only API**

Create these declarations in `editor/core/include/mirakana/editor/asset_import_review.hpp`:

```cpp
namespace mirakana::editor {

enum class EditorAssetImportCandidateStatus : std::uint8_t {
    ready,
    blocked,
};

struct EditorAssetImportCandidateInput {
    std::string source_path;
    EditorAssetBrowserLegalProvenanceRow provenance;
    bool source_exists{false};
};

struct EditorAssetImportCandidateRow {
    std::string id;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetKind asset_kind{AssetKind::unknown};
    AssetImportActionKind action_kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::string status_label;
    std::string diagnostic;
    bool can_register{false};
    bool can_import{false};
    bool blocked_by_legal{false};
};

struct EditorAssetImportReviewRequest {
    std::string asset_root{"assets"};
    std::string imported_output_root{"assets/imported"};
    std::string source_registry_path{"source/assets/package.geassets"};
    std::string source_registry_content;
    std::vector<EditorAssetImportCandidateInput> sources;
};

struct EditorAssetImportReviewModel {
    std::vector<EditorAssetImportCandidateRow> rows;
    std::vector<SourceAssetRegistrationRequest> registration_requests;
    AssetImportPlan import_plan;
    std::vector<std::string> diagnostics;
    bool ready{false};
};

[[nodiscard]] AssetImportActionKind editor_asset_import_action_kind_for_asset_kind(AssetKind kind) noexcept;
[[nodiscard]] AssetKind editor_asset_import_asset_kind_for_source_path(std::string_view path) noexcept;
[[nodiscard]] std::string editor_asset_import_source_format_for_path(std::string_view path);
[[nodiscard]] std::string editor_asset_import_output_path_for_source_path(std::string_view output_root,
                                                                          std::string_view source_path,
                                                                          AssetKind kind);
[[nodiscard]] EditorAssetImportReviewModel
review_editor_asset_import_candidates(const EditorAssetImportReviewRequest& request);

} // namespace mirakana::editor
```

- [x] **Step 3: Implement deterministic mapping**

Implement the mapping exactly:

```text
.texture -> AssetKind::texture -> AssetImportActionKind::texture -> GameEngine.TextureSource.v1
.png -> AssetKind::texture -> AssetImportActionKind::texture -> GameEngine.TextureSource.v1
.mesh -> AssetKind::mesh -> AssetImportActionKind::mesh -> GameEngine.MeshSource.v2
.gltf/.glb -> AssetKind::mesh -> AssetImportActionKind::mesh -> GameEngine.MeshSource.v2
.audio_source -> AssetKind::audio -> AssetImportActionKind::audio -> GameEngine.AudioSource.v1
.wav/.mp3/.flac -> AssetKind::audio -> AssetImportActionKind::audio -> GameEngine.AudioSource.v1
.material -> AssetKind::material -> AssetImportActionKind::material -> GameEngine.Material.v1
.scene -> AssetKind::scene -> AssetImportActionKind::scene -> GameEngine.Scene.v1
```

Output path rule:

```text
First-party cooked source documents keep their explicit project path when the source path already ends in .material or .scene.
Texture, mesh, and audio source imports write cooked outputs under assets/imported/<stem>.<texture|mesh|audio>.
The stem is the sanitized final filename without extension, lower ASCII, with spaces converted to underscores.
Duplicate output paths are blocked with diagnostic duplicate_import_target_path.
```

- [x] **Step 4: Run focused editor-core tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
```

Expected: the three new tests pass and no existing editor-core tests regress.

Task 1 validation evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
git diff --check
```

Result on 2026-06-29: PASS. Read-only subagent re-review returned `SPEC_APPROVED` after the `:` path and `.geassets` source-registry follow-up fixes.

### Task 2: Complete Visible Import Plan Mapping

**Files:**
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/core/include/mirakana/editor/asset_import_review.hpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [x] **Step 1: Write failing native-shell test**

Add:

```cpp
MK_TEST("native asset browser import plan includes texture mesh audio material and scene rows");
```

The test creates a `SourceAssetRegistryDocumentV1` with texture, mesh, audio, material, and scene rows, launches `NativeEditorApp`, and asserts its visible import queue or reviewed execution plan contains five actions, including `AssetImportActionKind::mesh` and `AssetImportActionKind::audio`.

Expected before implementation: mesh and audio are missing because `asset_import_action_kind_for_asset_browser` returns `unknown`.

- [x] **Step 2: Replace the local incomplete mapping**

Remove the private three-kind switch from `editor/src/native_editor_app.cpp` and call `editor_asset_import_action_kind_for_asset_kind` from Task 1. The mapping must include:

```text
texture, mesh, audio, material, scene
```

Unsupported asset kinds stay excluded with no compatibility alias.

- [x] **Step 3: Verify**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
```

Expected: the new native-shell test passes; import execution still requires user confirmation and matching generation.

Task 2 validation evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
git diff --check
```

Result on 2026-06-29: PASS. The native default Source Pulse registry now exposes texture, mesh, audio, material, and scene rows, and all five are planned by the shared clean-break mapping from Task 1.

### Task 3: Reviewed Source Registry Registration

**Files:**
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/core/src/asset_import_review.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [x] **Step 1: Write failing registration tests**

Add:

```cpp
MK_TEST("native asset browser applies reviewed import sources to source registry");
MK_TEST("native asset browser source registration rejects stale generation and legal blockers");
```

The success test uses `MemoryFileSystem` with:

```text
source/assets/package.geassets
assets/imported_sources/hero.png
```

It calls the new apply entrypoint with `user_confirmed=true` and asserts:

```text
registration_result.applied == true
source registry contains hero texture row
asset browser generation increments by 1
import plan contains one texture action
```

- [x] **Step 2: Add native API structs**

In `editor/src/native_editor_app.hpp` add:

```cpp
struct NativeEditorAssetBrowserSourceRegistrationRequest {
    std::uint64_t expected_generation{0};
    std::vector<std::string> project_source_paths;
    std::vector<EditorAssetBrowserLegalProvenanceRow> provenance_rows;
    bool user_confirmed{false};
};

struct NativeEditorAssetBrowserSourceRegistrationResult {
    EditorAssetBrowserCommandPlan command;
    EditorAssetImportReviewModel review;
    bool applied{false};
    std::size_t registered_count{0};
    std::vector<std::string> diagnostics;
};
```

Add method:

```cpp
[[nodiscard]] NativeEditorAssetBrowserSourceRegistrationResult
apply_reviewed_asset_browser_import_sources(NativeEditorAssetBrowserSourceRegistrationRequest request);
```

- [x] **Step 3: Implement apply through existing source asset registration tool**

Implementation rules:

```text
Use review_editor_asset_import_candidates to create SourceAssetRegistrationRequest rows.
Use plan_editor_asset_browser_command with a new or existing confirmation-gated command row before mutation.
Call apply_source_asset_registration for each reviewed request.
Stop on the first diagnostic and leave the in-memory browser model unchanged when any request fails.
After success, reload source registry content, refresh ContentBrowserState, rebuild AssetImportPlan, refresh Source Pulse model, and increment generation.
```

Do not write `.geindex`, package manifests, or cooked artifacts in this task.

- [x] **Step 4: Verify**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
```

Expected: registration success, stale-generation rejection, and legal-blocker tests pass.

Task 3 validation evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
git diff --check
```

Result on 2026-06-29: PASS. Source registration is now an explicit confirmation-gated `asset_browser.import.register_sources` command, legal/provenance blockers prevent mutation, and successful registration refreshes Source Pulse generation and import planning.

### Task 4: External Source Copy Transaction

**Files:**
- Create: `editor/src/native_asset_import_copy.hpp`
- Create: `editor/src/native_asset_import_copy.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/CMakeLists.txt`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [x] **Step 1: Write failing copy execution tests**

Add:

```cpp
MK_TEST("native asset browser copies reviewed external source through temp target");
MK_TEST("native asset browser copy rejects symlink device traversal and collisions");
```

The success test uses a temporary host directory with:

```text
external/hero.png
project/assets/imported_sources/
```

Expected result:

```text
assets/imported_sources/hero.png exists
copy_result.copied_count == 1
copy_result.target_project_paths[0] == "assets/imported_sources/hero.png"
temporary .copying file is absent after success
```

- [x] **Step 2: Add shell-private transaction API**

Create `editor/src/native_asset_import_copy.hpp`:

```cpp
namespace mirakana::editor {

struct NativeAssetImportExternalCopyInput {
    std::string absolute_source_path;
    std::string target_project_path;
};

struct NativeAssetImportExternalCopyResultRow {
    std::string source_path;
    std::string target_project_path;
    std::string diagnostic;
    bool copied{false};
};

struct NativeAssetImportExternalCopyResult {
    std::vector<NativeAssetImportExternalCopyResultRow> rows;
    std::vector<std::string> target_project_paths;
    std::vector<std::string> diagnostics;
    std::size_t copied_count{0};
    bool succeeded{false};
};

[[nodiscard]] NativeAssetImportExternalCopyResult copy_reviewed_external_asset_sources_to_project(
    std::string_view project_root,
    std::span<const NativeAssetImportExternalCopyInput> inputs);

} // namespace mirakana::editor
```

- [x] **Step 3: Implement fail-closed filesystem policy**

Rules:

```text
Canonicalize project_root and every target path with std::filesystem.
Reject device paths, parent traversal, absolute target paths, empty filenames, line separators, '=', ';', and backslashes in project-relative targets.
Reject source symlinks and target symlinks.
Reject source paths that are not regular files.
Reject targets outside project_root.
Reject existing targets unless a later rename-suggestion task has produced a reviewed alternate target.
Copy to <target>.copying-<process-id-or-monotonic-counter>, then promote to the final target with no-overwrite finalization.
On failure, remove only exact temp files and final links created by this transaction.
Never delete or overwrite the source file.
```

- [x] **Step 4: Wire explicit copy command**

Add `NativeEditorAssetBrowserExternalSourceCopyExecutionRequest` / result structs to `native_editor_app.hpp`, call `plan_editor_asset_browser_command` with `copy_external_sources`, require `user_confirmed=true`, then call `copy_reviewed_external_asset_sources_to_project`.

After success, feed returned `target_project_paths` into Task 3 registration. Do not automatically import cooked artifacts after copy.

- [x] **Step 5: Verify**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-editor --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-editor --output-on-failure -R MK_editor_native_shell_tests
```

Expected: copy succeeds only for reviewed safe sources and leaves no temp file behind after success or failure.

Result on 2026-06-29: PASS. The native shell now exposes a confirmed
`copy_reviewed_asset_browser_external_sources` flow backed by shell-private
`copy_reviewed_external_asset_sources_to_project`. External files copy into
`assets/imported_sources` through `.copying-*` temp files with no-overwrite
hard-link promotion, reject device paths, parent traversal, source/target
symlinks, non-regular sources, status-query errors, and target collisions,
require legal provenance before mutation, roll back exact temp/final files
created by the transaction on failure, and feed copied project-relative targets
into the reviewed `asset_browser.import.register_sources` registration path
without invoking import tools. Verification run:
`tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests`,
`tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests`,
`tools/check-format.ps1`, `tools/check-json-contracts.ps1`,
`tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, and
`git diff --check`.

### Task 5: Persistent Asset Import Provenance Gate

**Files:**
- Create: `engine/assets/include/mirakana/assets/asset_import_provenance.hpp`
- Create: `engine/assets/src/asset_import_provenance.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Existing gate verified: `editor/core/src/asset_import_review.cpp`
- Modify: `editor/core/src/asset_browser_production.cpp`
- Test: `tests/unit/core_tests.cpp`
- Test: `tests/unit/editor_core_tests.cpp`

- [x] **Step 1: Write failing provenance tests**

Add:

```cpp
MK_TEST("asset import provenance document serializes validates and round trips");
MK_TEST("asset import provenance rejects missing license restricted licenses and external engine material");
MK_TEST("editor import candidates require accepted provenance before registration");
```

Result on 2026-06-29: PASS. `MK_core_tests` failed before implementation because
`mirakana/assets/asset_import_provenance.hpp` was missing; `MK_editor_core_tests`
already compiled and the new editor review test captured the existing mandatory
legal gate.

- [x] **Step 2: Add first-party provenance document**

Declare:

```cpp
namespace mirakana {

enum class AssetImportProvenanceOrigin : std::uint8_t {
    first_party,
    third_party,
    generated_ai,
};

struct AssetImportProvenanceRowV1 {
    AssetKeyV2 asset_key;
    AssetImportProvenanceOrigin origin{AssetImportProvenanceOrigin::first_party};
    std::string source_url;
    std::string retrieved_date;
    std::string version_or_commit;
    std::string copyright_holder;
    std::string license_id{"LicenseRef-Proprietary"};
    std::string modification_status{"unmodified"};
    std::string distribution_target{"editor_source"};
    std::string notice_id;
    bool notice_complete{false};
    bool external_engine_material{false};
};

struct AssetImportProvenanceDocumentV1 {
    std::vector<AssetImportProvenanceRowV1> rows;
};

[[nodiscard]] std::string serialize_asset_import_provenance_document(
    const AssetImportProvenanceDocumentV1& document);
[[nodiscard]] AssetImportProvenanceDocumentV1 deserialize_asset_import_provenance_document(std::string_view text);
[[nodiscard]] std::vector<std::string> validate_asset_import_provenance_document(
    const AssetImportProvenanceDocumentV1& document);

} // namespace mirakana
```

Format header:

```text
format=GameEngine.AssetImportProvenance.v1
```

Result on 2026-06-29: PASS. `MK_assets` now owns
`AssetImportProvenanceDocumentV1`, `AssetImportProvenanceRowV1`,
`AssetImportProvenanceOrigin`, deterministic text IO, and fail-closed validation
for `GameEngine.AssetImportProvenance.v1`.

- [x] **Step 3: Connect provenance to existing legal review**

Rules:

```text
First-party project assets use LicenseRef-Proprietary and notice_complete=true.
Third-party or generated AI assets require source_url, retrieved_date, copyright_holder, license_id, distribution_target, and notice_id.
SPDX ids are accepted when present in the SPDX license list or already documented in docs/legal-and-licensing.md.
LicenseRef-* ids require a matching THIRD_PARTY_NOTICES.md or LICENSES entry before package acceptance.
CC-BY-NC, CC-BY-ND, CC-NC, CC-ND, unknown marketplace licenses, license-less material, and external_engine_material=true are blocked.
Unity Asset Store, Epic/Fab/Unreal Marketplace, copied editor UI expression, engine logos/trademarks, and external engine project schemas are blocked by default.
```

Result on 2026-06-29: PASS. Validation accepts documented first-party
`LicenseRef-Proprietary` rows, selected allowed SPDX ids and simple `AND`/`OR`
expressions, and `LicenseRef-*` only with a `LICENSES/` or
`THIRD_PARTY_NOTICES.md` notice reference. It rejects missing/unknown licenses,
NC/ND licenses, marketplace-only license labels, and external-engine project,
asset, UI, trademark, or schema markers. The existing editor legal review was
extended with the same external-engine and marketplace markers while preserving
the existing clean-room Godot-reference test.

Review follow-up on 2026-06-29: PASS. `version_or_commit` is now validated before
serialization, invalid `AssetImportProvenanceOrigin` values produce
`invalid_origin`, `asset_import_provenance_origin_label` no longer silently maps
invalid enum values to `third_party`, and editor-core projection uses
`make_editor_asset_browser_legal_provenance_row` so persistent
`GameEngine.AssetImportProvenance.v1` rows become legal review rows through one
fail-closed path. Pre-blocked projected rows remain blocked when reviewed.

- [x] **Step 4: Make provenance mandatory for registration**

`review_editor_asset_import_candidates` must set:

```text
blocked_by_legal=true
can_register=false
can_import=false
diagnostic=asset_import_provenance_blocked
```

when the matching provenance row does not pass `review_editor_asset_browser_legal_provenance`.

Result on 2026-06-29: PASS. `review_editor_asset_import_candidates` already
blocked source registration/import planning through the legal review. The new
test proves restricted and external-engine provenance rows produce
`asset_import_provenance_blocked`, empty registration requests, and empty import
actions.

- [x] **Step 5: Verify legal and dependency checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-license.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

Expected: tests pass; no new dependency records are required because high priority adds no third-party packages or distributable assets.

Result on 2026-06-29: PASS.

```text
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-license.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/assets/src/asset_import_provenance.cpp,editor/core/src/asset_browser_production.cpp"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

No new third-party package, copied asset, external code, Unity/Unreal/Godot
sample, UI expression, trademark, or marketplace material was added.

### Task 6: Import Execution Refresh And Diagnostics

**Files:**
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/core/include/mirakana/editor/asset_pipeline.hpp`
- Modify: `editor/core/src/asset_pipeline.cpp`
- Test: `tests/unit/editor_core_tests.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [x] **Step 1: Write failing result refresh tests**

Add:

```cpp
MK_TEST("editor asset import execution registers imported cooked records and refreshes browser");
MK_TEST("editor asset import execution keeps browser unchanged on failed import batch");
```

The failure test includes one missing source and asserts no cooked outputs are written and no imported records are registered.

- [x] **Step 2: Extend result mapping**

Use existing helpers:

```text
make_imported_asset_records
add_imported_asset_records
execute_asset_import_plan
ExternalAssetImportAdapters::options
```

Rules:

```text
If execute_asset_import_plan reports any failure, do not add records to the visible asset registry.
If it succeeds, add imported records, refresh ContentBrowserState from the active SourceAssetRegistryDocumentV1, refresh Source Pulse model, update import diagnostics, and increment generation.
Preserve the all-or-nothing behavior of execute_asset_import_plan.
```

- [x] **Step 3: Verify default and optional importer lanes**

Run default focused tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
```

Run optional importer lane when host dependencies are bootstrapped:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
```

Expected: default tests pass without optional packages; optional importer lane passes or reports the exact missing bootstrap/toolchain blocker.

Result on 2026-06-29: PASS for the default focused lane. RED failed first on the missing
`NativeEditorAssetBrowserImportExecutionResult::registered_imported_count` and
`browser_refreshed` fields. The implementation now keeps a native-shell imported asset
registry and `AssetPipelineState`, maps successful import execution into imported asset
records, Source Pulse `imported` labels, browser generation increments, and active
`GameEngine.SourceAssetRegistry.v1` browser refresh. Failed batches apply diagnostics
without visible registry mutation, keep Source Pulse generation unchanged, and preserve
`execute_asset_import_plan` all-or-nothing writes. Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
```

Optional importer lane blocker: `tools/build-asset-importers.ps1` reached CMake configure but
failed because `SPNGConfig.cmake` / `spng-config.cmake` was not installed in the
worktree vcpkg package root. The supported remediation is
`tools/bootstrap-deps.ps1 -Feature asset-importers`; this session could not run that
bootstrap command because the active approval policy rejected the dependency bootstrap
operation before execution.

### Task 7: High-Priority Docs, Manifest, And Closeout Validation

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/2026-06-29-editor-asset-import-production-v2.md`
- Modify when durable behavior changes: `engine/agent/manifest.fragments/007-importerCapabilities.json`
- Modify when active plan status changes: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify only if dependencies or notices change: `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `vcpkg.json`

- [x] **Step 1: Update current-truth docs**

Document exact ready surface:

```text
Source Pulse import can review selected project or external source files, copy external files into assets/imported_sources, require provenance, register SourceAssetRegistry rows, execute reviewed import plans, refresh browser state, and preserve explicit non-claims.
```

Non-claims that must remain:

```text
arbitrary importer plugins
automatic import execution
package script execution
validation recipe execution from editor import
broad runtime package streaming
runtime source parsing
renderer/RHI residency from editor core
public native handles
external engine compatibility/equivalence/parity
Unity/Unreal/Godot asset/project/schema import compatibility
marketplace connector readiness
legal advice
```

- [x] **Step 2: Run agent-surface drift checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: all pass. If manifest fragments were changed, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

- [x] **Step 3: Run slice validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/asset_import_review.cpp editor/src/native_asset_import_copy.cpp editor/src/native_editor_app.cpp engine/assets/src/asset_import_provenance.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: full validation passes or records an exact toolchain/host blocker with all focused checks above.

Result on 2026-06-29: PASS. `docs/editor.md`, `docs/current-capabilities.md`,
`engine/agent/manifest.fragments/004-modules.json`,
`engine/agent/manifest.fragments/014-gameCodeGuidance.json`, and composed
`engine/agent/manifest.json` now state the exact Source Pulse import ready surface:
reviewed source selection, external copy, provenance-gated registration, reviewed
import execution, success-only imported record registration/browser refresh/generation
advance, failure all-or-nothing/no-visible-registry mutation, and Unity/Unreal/Godot
compatibility non-claims. Validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "editor/core/src/asset_browser_production.cpp,editor/src/native_editor_app.cpp"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-license.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Medium Priority Tasks

### Task 8: Import Job Queue And Progress

**Files:**
- Create: `editor/core/include/mirakana/editor/asset_import_jobs.hpp`
- Create: `editor/core/src/asset_import_jobs.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Test: `tests/unit/editor_core_tests.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [x] Add `EditorAssetImportJobSnapshot`, `EditorAssetImportJobRow`, `EditorAssetImportJobCommandKind`, and `make_editor_asset_import_job_model`.
- [x] Native shell starts one reviewed job per import batch after Task 6 gates pass.
- [x] Job rows expose `queued`, `copying`, `registering`, `importing`, `refreshing`, `succeeded`, `failed`, and `canceled` states.
- [x] Cancel requests are cooperative and stop before the next filesystem mutation boundary.
- [x] Retry creates a new generation-checked job and never mutates the completed job snapshot.
- [x] Tests prove deterministic job row ordering, cancellation before import writes, retry after failed source, and no native handles in job rows.
- [x] Verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
```

Result on 2026-06-29: PASS for the focused build/test lane. RED failed first on the
missing `mirakana/editor/asset_import_jobs.hpp` contract. The implementation adds
value-only `EditorAssetImportJobSnapshot` / `EditorAssetImportJobRow` rows,
generation-checked cancel/retry command planning, deterministic row ordering,
progress/state labels, no native handle exposure, and retry rows that append a new
queued job while preserving the completed failed/canceled job. The native shell now
records one job per reviewed external-copy or import-execution batch after the
existing legal/provenance, generation, user-confirmation, and filesystem gates pass;
copy batches progress through `copying` / `registering`, import batches through
`importing` / `refreshing`, and both finish as `succeeded` or `failed`. Cooperative
cancel applies only to queued/active jobs before the next mutation boundary and retry
creates a new generation-checked queued job. No new dependency, third-party asset,
external engine material, compatibility claim, or legal-advice claim was added.

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Strict -Files "editor/core/src/asset_import_jobs.cpp,editor/src/native_editor_app.cpp,tests/unit/editor_core_tests.cpp,tests/unit/editor_native_shell_tests.cpp"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-license.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

### Task 9: Reviewed Reimport, Recook, And Hot Reload

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_import_review.hpp`
- Modify: `editor/core/src/asset_import_review.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Test: `tests/unit/editor_core_tests.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [ ] Add reviewed command ids for `asset_browser.import.reimport_selected`, `asset_browser.import.recook_stale`, and `asset_browser.import.stage_hot_reload`.
- [ ] Build stale rows from `AssetHotReloadTracker`, source content hash, output content hash, and `build_asset_recook_plan`.
- [ ] Reimport selected assets by `AssetKeyV2`; dependency expansion is explicit and defaults to selected only.
- [ ] Recook uses existing `execute_asset_runtime_recook` only after import plan review and user confirmation.
- [ ] Hot reload stages replacements through `AssetRuntimeReplacementState` and commits only at a caller-owned safe point.
- [ ] Tests prove selected-only reimport, dependency-closure reimport when explicitly requested, failed recook rollback, and no runtime source parsing.
- [ ] Verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests|MK_core_tests"
```

### Task 10: Import Presets

**Files:**
- Create: `engine/assets/include/mirakana/assets/asset_import_presets.hpp`
- Create: `engine/assets/src/asset_import_presets.cpp`
- Modify: `editor/core/include/mirakana/editor/asset_import_review.hpp`
- Modify: `editor/core/src/asset_import_review.cpp`
- Test: `tests/unit/core_tests.cpp`
- Test: `tests/unit/editor_core_tests.cpp`

- [ ] Add `GameEngine.AssetImportPresets.v1` with project defaults and per-asset overrides keyed by `AssetKeyV2`.
- [ ] Texture preset fields: `color_space` (`srgb`, `linear`), `mipmap_policy` (`none`, `generate_offline`), `alpha_policy` (`opaque`, `premultiplied`, `straight`), `compression_intent` (`none`, `bc7`, `astc`, `basis_reviewed`).
- [ ] Mesh preset fields: `unit_scale`, `up_axis`, `triangulate`, `generate_normals`, `generate_tangents`, `material_extraction`.
- [ ] Audio preset fields: `decode_mode` (`static_pcm`, `streaming_source_review`), `sample_format`, `loop`, `normalize_peak`.
- [ ] Presets influence action metadata and review rows only until a matching tool implementation consumes the field.
- [ ] Tests prove unsupported preset combinations block import rather than being ignored.
- [ ] Verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_editor_core_tests"
```

### Task 11: Folder And Drag-Drop Batch Review

**Files:**
- Modify: `editor/core/include/mirakana/editor/content_browser_import_panel.hpp`
- Modify: `editor/core/src/content_browser_import_panel.cpp`
- Modify: `editor/src/native_editor_app.hpp`
- Modify: `editor/src/native_editor_app.cpp`
- Test: `tests/unit/editor_core_tests.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [ ] Add a folder scan request that lists files through shell-owned filesystem services and then feeds the same `EditorAssetImportCandidateInput` rows from Task 1.
- [ ] Scan limits: maximum 500 candidate files, maximum 4 directory levels, maximum 256 MiB per file unless a reviewed override exists.
- [ ] Drag/drop is normalized to the same reviewed project/external source selection path.
- [ ] No drag/drop or folder scan executes copy, registration, or import automatically.
- [ ] Tests prove scan limits, unsupported file diagnostics, duplicate target blocking, and same retained UI ids for dialog and drag/drop candidates.
- [ ] Verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
```

### Task 12: Duplicate Detection And Rename Suggestions

**Files:**
- Modify: `editor/core/include/mirakana/editor/asset_import_review.hpp`
- Modify: `editor/core/src/asset_import_review.cpp`
- Modify: `editor/src/native_asset_import_copy.cpp`
- Test: `tests/unit/editor_core_tests.cpp`
- Test: `tests/unit/editor_native_shell_tests.cpp`

- [ ] Detect duplicate source path, duplicate output path, duplicate `AssetKeyV2`, duplicate imported path, and duplicate content hash.
- [ ] Suggest deterministic alternates: `<stem>_2`, `<stem>_3`, up to `<stem>_99`.
- [ ] The suggestion is a review row only. The operator must explicitly select it before copy or registration.
- [ ] Content-hash duplicates can reuse an existing source only when provenance rows match and the operator selects `reuse_existing_source`.
- [ ] Tests prove collision blocking, deterministic suggestions, hash reuse blocking on provenance mismatch, and no implicit overwrite.
- [ ] Verification:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_editor_core_tests|MK_editor_native_shell_tests"
```

## Deferred Tasks

### Task 13: Broad Codec Expansion Gate

**Deferred scope:** AVIF, JPEG, TIFF, USD, FBX, OBJ, Ogg/Vorbis, Opus, EXR-to-runtime import, KTX2 runtime transcode, and Assimp-based broad format import are not part of high or medium priority work.

- [ ] Before selecting any codec or broad importer, update `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, and the relevant manifest fragments.
- [ ] Use Context7 or official upstream docs for the selected dependency.
- [ ] Keep each dependency behind a non-default vcpkg feature.
- [ ] Add tests proving parser/native types never leak into public APIs.
- [ ] Add validation recipe gates proving dependency bootstrap, focused build, license policy, and no default build dependency leakage.

### Task 14: Marketplace And Asset Catalog Gate

**Deferred scope:** Online catalogs, marketplace import, license checkout, account-bound assets, cloud downloads, and package-store connectors are not part of this plan.

- [ ] Any connector requires a separate legal and security design.
- [ ] The connector must not download assets in the editor core.
- [ ] The connector must record account, license, source URL, retrieved date, version, author, and distribution target before assets can enter package-ready state.
- [ ] Unity Asset Store, Unreal Marketplace, Fab, and engine sample content remain blocked unless a separate legal approval record exists before implementation.

### Task 15: External Engine Project Compatibility Gate

**Deferred scope:** Importing Unity projects, Unreal projects, Godot projects, `.meta`, `.uasset`, `.umap`, `.tscn`, `.tres`, external engine scenes, external engine prefab/blueprint/node schemas, and compatibility/equivalence/parity claims.

- [ ] Any clean-room comparison must use public documentation only and first-party MIRAIKANAI assets.
- [ ] Do not parse or mimic external engine private formats.
- [ ] Do not use external engine names in command ids, serialized schemas, row ids, or product-facing compatibility language.
- [ ] Do not claim replacement, parity, equivalence, superiority, or compatibility without explicit legal and technical approval.

## Validation Closeout

At the end of each implementation checkpoint:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

At the high-priority slice close:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Optional importer evidence:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
```

Before staging, pushing, PR creation, or ready conversion:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

## Final Acceptance Checklist

- [ ] High-priority import can review source files, copy external files, require provenance, register source registry rows, import cooked artifacts, refresh browser state, and record diagnostics without automatic execution.
- [ ] Mesh and audio rows are no longer dropped from the visible-shell default import plan.
- [ ] External copy is transactional, root-contained, collision-safe, and leaves no temp files on success or known failure.
- [ ] Provenance blocks license-less, NC/ND, external-engine, marketplace, trademark, copied UI expression, and incomplete notice rows.
- [ ] Existing optional `asset-importers` dependency gate is reused; no new high-priority dependency is introduced.
- [ ] Runtime/game code consumes cooked artifacts only and still does not parse external source formats.
- [ ] `editor/core` remains GUI-independent and free of native handles, parser types, filesystem mutation, process execution, renderer/RHI work, package scripts, and validation recipe execution.
- [ ] Unity, Unreal Engine, and Godot are not copied or mimicked; they remain official legal/category research sources only.
- [ ] Current-truth docs, plan registry, and agent-surface contracts match the implemented behavior.
