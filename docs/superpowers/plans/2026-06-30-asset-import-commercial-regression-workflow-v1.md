# Asset Import Commercial Regression Workflow v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a clean-break commercial importer regression workflow for large real glTF, texture, material, and animation asset sets, with deterministic failure diagnostics, batch reimport, preset diff, and axis/unit preview. The workflow must preserve the existing cooked-only runtime policy, consume `GameEngine.AssetImportPresets.v1`, and provide enough evidence for commercial-quality importer claims without copying Unity, Unreal Engine, Godot, or other engine behavior, UI expression, asset schemas, code, samples, marks, or compatibility claims.

**Architecture:** `MK_assets` owns value contracts and deterministic text/JSON surfaces. `MK_tools` owns optional `asset-importers` execution and converts external source formats into first-party source documents and cooked payloads. `MK_editor_core` owns retained, GUI-independent asset browser workflow rows. Native editor shell and CI scripts only execute reviewed commands. Runtime/game modules continue to consume cooked artifacts only and never parse glTF, PNG, EXR, KTX, audio, or third-party source formats.

**Tech Stack:** C++23, first-party `mirakana::` APIs, `MK_assets`, `MK_tools`, `MK_editor_core`, optional `asset-importers` vcpkg feature (`fastgltf`, `libspng`, `OpenEXR`, `KTX Software`, `miniaudio`), `AssetImportPresets.v1`, `AssetImportProvenance.v1`, `AssetImportProductionReview v1`, PowerShell 7 validation scripts, JSON Schema, CTest, and the existing first-party UI retained model.

---

## Authoring Status

- Date: 2026-06-30.
- Status: candidate implementation plan. This file does not change `currentActivePlan`, readiness counters, `unsupportedProductionGaps`, dependencies, or legal notices by itself.
- Selected project coordinate convention: right-handed, `+Y` up, meters, matching the current `AssetImportPresets.v1` and `AssetCoordinateNormalizationPlan` implementation.

## Sources Reviewed

- Project files: `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/007-importerCapabilities.json`, `engine/assets/include/mirakana/assets/asset_import_presets.hpp`, `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`, `engine/assets/include/mirakana/assets/asset_import_provenance.hpp`, `engine/assets/include/mirakana/assets/asset_import_production_review.hpp`, `engine/tools/include/mirakana/tools/asset_coordinate_normalization.hpp`, `engine/tools/include/mirakana/tools/asset_import_tool.hpp`, `engine/tools/include/mirakana/tools/asset_import_adapters.hpp`, `engine/tools/include/mirakana/tools/gltf_*`, `editor/core/include/mirakana/editor/asset_browser_production.hpp`, `tests/unit/tools_tests.cpp`, and `tests/unit/editor_core_tests.cpp`.
- Context7 `/spnda/fastgltf`: `fastgltf::Parser`, `GltfDataBuffer`, `Options::LoadExternalBuffers`, `Options::LoadExternalImages`, `Options::DecomposeNodeMatrices`, `Options::GenerateMeshIndices`, `fastgltf::validate`, accessor iteration, material/image inspection, and parser reuse constraints.
- Context7 `/khronosgroup/ktx-software`: `ktxTexture2_CreateFromNamedFile`, `ktxTexture2_NeedsTranscoding`, `ktxTexture2_TranscodeBasis`, level/layer/face/dimension metadata, selected transcode targets, and native handle isolation.
- Khronos glTF 2.0 specification: right-handed coordinates, `+Y` up, meters, radians, glTF scenes/nodes/meshes/materials/animations, and external resource rules. Official reference: `https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html`.
- Khronos glTF Sample Assets: per-asset license metadata is required; the top-level README is not sufficient for redistribution approval. Official reference: `https://github.com/KhronosGroup/glTF-Sample-Assets/blob/main/README.md`.
- Khronos KTX Software/libktx: KTX 1/2 loading, Basis Universal transcode, KTX2 metadata, OpenGL/Vulkan upload helpers, and KTX/native handle boundaries. Official reference: `https://github.khronos.org/KTX-Software/libktx/index.html`.
- OpenEXR documentation: scene-linear HDR semantics and EXR metadata/window/channel concepts. Official references: `https://openexr.com/en/latest/SceneLinear.html` and `https://openexr.com/en/latest/TechnicalIntroduction.html`.
- Legal/trademark category references: Unity trademark guidelines `https://unity.com/legal/branding-trademarks`, Unreal Engine/Epic trademark guidance `https://www.unrealengine.com/release` and `https://www.epic.com/epic/page/trademark-usage-guidelines-non-licensee/`, Godot logo/trademark guidance `https://godotengine.org/press/`, and Godot license documentation `https://godotengine.org/license/`.

## Current Baseline

- `MK_assets` already has `GameEngine.AssetImportPresets.v1`, `AssetImportAction::mesh_preset`, `GameEngine.AssetImportProvenance.v1`, source/cooked asset documents, and value-only production import review APIs.
- `MK_tools` already has optional adapters for PNG, glTF mesh, glTF morph mesh CPU, glTF node transform animation, glTF skin/morph animation helpers, OpenEXR environment texture source review/payload decode, KTX2/Basis environment texture metadata review/payload transcode, and common audio.
- `MK_tools` coordinate normalization already consumes `mesh.unit_scale` and `mesh.up_axis` for selected glTF mesh, morph mesh, skin, node transform, and animation paths.
- `MK_editor_core` already has a first-party asset browser production model, import review, reimport/recook/hot-reload planning, provenance/legal rows, KTX2/Basis source review rows, OpenEXR source review rows, import jobs, folder/drag-drop source review, and retained command rows.
- Existing validation covers selected fixtures and package-review evidence, but it does not yet provide a large real-asset corpus manifest, corpus runner, per-run diagnostic reports, batch reimport execution plans over a corpus, preset diff reports, or asset-browser axis/unit preview rows.

## Non-Goals

- Do not implement Unity, Unreal Engine, Godot, Blender project/schema import, `.meta`, `.uasset`, `.umap`, `.uproject`, `.tscn`, `.tres`, or external engine compatibility.
- Do not copy external engine editor layout, UX expression, command names, serialized formats, screenshots, samples, shaders, trademarks, icons, or assets.
- Do not add marketplace connectors, network downloads, asset-store scraping, or automatic third-party asset acquisition in importer execution.
- Do not commit large external binary assets until legal review, provenance rows, notices, repo-size policy, and distribution policy are explicitly approved.
- Do not add runtime glTF/PNG/EXR/KTX/audio source parsing or runtime Basis/OpenEXR/PNG decode/transcode.
- Do not leak `fastgltf`, `ktxTexture*`, OpenEXR, libspng, miniaudio, native GPU handles, parser objects, or third-party pixel/channel types into public runtime/game APIs.
- Do not preserve backward compatibility shims. This is a greenfield clean-break v1 surface.

## Legal And Clean-Room Policy

- Corpus assets are accepted only from first-party generated fixtures, project-owned assets, or third-party assets with a per-asset allowlist entry.
- Accepted third-party asset licenses for checked-in or distributed corpus material are `CC0-1.0`, `MIT`, `BSD-2-Clause`, `BSD-3-Clause`, `Apache-2.0`, `Zlib`, or `CC-BY-4.0` with complete attribution and notice rows. Reject `CC-BY-NC-*`, `CC-BY-ND-*`, missing license, ambiguous marketplace license, AI-generated asset with unclear training/source rights, trademark/logo-heavy assets, and external-engine sample assets unless a separate legal approval record exists.
- Khronos glTF Sample Assets may be used only after reading the individual model directory README, recording the exact model path, commit, license, copyright holder, modifications, and distribution target.
- Large real-asset corpus bundles are host-owned or CI artifact-owned by default under `out/host-artifacts/asset-import-regression-corpus/`. The source tree stores schemas, tiny first-party generated fixtures, and manifests, not unreviewed large external assets.
- If any third-party corpus asset becomes tracked or distributed, update `THIRD_PARTY_NOTICES.md`, `docs/legal-and-licensing.md`, and the corpus manifest in the same implementation slice.
- Public documentation may reference Unity, Unreal Engine, and Godot only as legal/trademark/category research sources. It must not claim compatibility, equivalence, parity, migration, replacement, or marketplace ingestion.
- This plan is not legal advice; it defines repository gates that must pass before engineering readiness can be claimed.

## Implementation Tasks

### Task 1: Corpus Manifest And Report Contracts In `MK_assets`

- [ ] Add `engine/assets/include/mirakana/assets/asset_import_regression_corpus.hpp`.
- [ ] Add `engine/assets/src/asset_import_regression_corpus.cpp`.
- [ ] Add the new header/source to `engine/assets/CMakeLists.txt`.
- [ ] Add `schemas/asset-import-regression-corpus.schema.json`.
- [ ] Add `schemas/asset-import-regression-report.schema.json`.
- [ ] Add focused tests in `tests/unit/asset_import_regression_tests.cpp` and register a new CMake test target `MK_asset_import_regression_tests`.

Required public value types:

```cpp
namespace mirakana {

enum class AssetImportRegressionCorpusAssetKind : std::uint8_t {
    gltf_scene,
    gltf_mesh,
    gltf_animation,
    png_texture,
    openexr_texture,
    ktx2_basis_texture,
    material_document,
    audio_source
};

enum class AssetImportRegressionLicensePolicy : std::uint8_t {
    accepted_for_source_tree,
    accepted_for_host_corpus_only,
    rejected
};

enum class AssetImportRegressionDiagnosticCode : std::uint8_t {
    none,
    invalid_manifest,
    duplicate_asset_id,
    unsafe_source_path,
    missing_source_file,
    source_hash_mismatch,
    missing_license_provenance,
    rejected_license,
    external_engine_material,
    unsupported_format,
    parser_error,
    validator_error,
    missing_external_resource,
    unsafe_external_resource_path,
    unsupported_extension,
    unsupported_animation_channel,
    unsupported_skin_or_morph_combination,
    coordinate_normalization_failed,
    material_extraction_failed,
    texture_decode_failed,
    texture_transcode_failed,
    cooked_output_mismatch,
    nondeterministic_output,
    row_budget_exceeded
};

struct AssetImportRegressionCorpusAssetV1 {
    std::string asset_id;
    AssetImportRegressionCorpusAssetKind kind{AssetImportRegressionCorpusAssetKind::gltf_mesh};
    AssetKeyV2 asset_key;
    std::string source_path;
    std::string expected_sha256;
    std::vector<std::string> expected_output_kinds;
    std::vector<std::string> required_features;
    AssetImportMeshPresetV1 mesh_preset;
    std::vector<std::string> preset_metadata;
    AssetImportProvenanceRowV1 provenance;
    AssetImportRegressionLicensePolicy license_policy{AssetImportRegressionLicensePolicy::rejected};
    bool allow_external_resources{false};
    bool allow_checked_in_distribution{false};
};

struct AssetImportRegressionCorpusDocumentV1 {
    std::string corpus_id{"GameEngine.AssetImportRegressionCorpus.v1"};
    std::string corpus_version{"1"};
    std::string root_path;
    std::vector<AssetImportRegressionCorpusAssetV1> assets;
    std::uint64_t row_budget{10000U};
};

struct AssetImportRegressionReportRowV1 {
    std::string asset_id;
    AssetImportRegressionCorpusAssetKind kind{AssetImportRegressionCorpusAssetKind::gltf_mesh};
    AssetId asset;
    std::string source_path;
    std::string source_sha256;
    std::string preset_sha256;
    std::string importer_id;
    std::string importer_version;
    std::string phase;
    AssetImportRegressionDiagnosticCode code{AssetImportRegressionDiagnosticCode::none};
    std::string message;
    std::string deterministic_output_hash;
    bool succeeded{false};
    bool ready_for_commercial_evidence{false};
};

struct AssetImportRegressionReportV1 {
    std::string corpus_id;
    std::string run_id;
    std::vector<AssetImportRegressionReportRowV1> rows;
    std::size_t asset_count{0U};
    std::size_t succeeded_count{0U};
    std::size_t failed_count{0U};
    std::size_t legal_blocked_count{0U};
    std::size_t nondeterministic_count{0U};
    bool ready{false};
};

[[nodiscard]] std::vector<std::string>
validate_asset_import_regression_corpus_v1(const AssetImportRegressionCorpusDocumentV1& document);
[[nodiscard]] std::string
serialize_asset_import_regression_corpus_v1(const AssetImportRegressionCorpusDocumentV1& document);
[[nodiscard]] AssetImportRegressionCorpusDocumentV1
deserialize_asset_import_regression_corpus_v1(std::string_view text);
[[nodiscard]] std::string serialize_asset_import_regression_report_v1(const AssetImportRegressionReportV1& report);
[[nodiscard]] AssetImportRegressionReportV1 deserialize_asset_import_regression_report_v1(std::string_view text);

} // namespace mirakana
```

Contract rules:

- Validate deterministic row ordering by `asset_id`.
- Reject duplicate ids, absolute paths, parent traversal, backslashes in serialized corpus paths, missing hashes for third-party assets, missing provenance, rejected licenses, and `external_engine_material=true`.
- Keep all source paths relative to a caller-supplied corpus root.
- Serialize all reports as first-party deterministic text first; JSON schema mirrors are used for retained artifacts and CI.

### Task 2: First-Party Tiny Fixture Corpus

- [ ] Add tiny generated fixture builders to `tests/unit/asset_import_regression_tests.cpp` instead of committing unreviewed binary assets.
- [ ] Add `tests/fixtures/asset_import_regression/README.md`.
- [ ] Add `tests/fixtures/asset_import_regression/first_party_corpus.gecorpus` only if it is text-only and references generated or tiny first-party assets.

Required fixture coverage:

- Valid glTF mesh with positions, normals, UVs, tangents, material reference, external buffer, and no external network resource.
- Valid glTF animation with translation, quaternion rotation, scale, skin joint tracks, morph weights, and node transform channels.
- Invalid glTF cases for unsafe external path, missing buffer, unsupported interpolation, unsupported extension, invalid quaternion, mismatched accessor counts, duplicate animation channel, unsupported skin/morph combination, and malformed material texture index.
- PNG RGBA8 texture fixture through `decode_audited_png_rgba8`.
- OpenEXR and KTX2/Basis fixtures generated through existing optional test helpers when `MK_HAS_ASSET_IMPORTERS=1`.
- First-party material document fixture with texture dependencies and missing texture diagnostics.

### Task 3: Host-Owned Large Corpus Intake Policy

- [ ] Add `tools/check-asset-import-regression-corpus.ps1` for default static/schema/test checks.
- [ ] Add `tools/validate-asset-import-regression-corpus.ps1`.
- [ ] Add ignored output folders through existing ignore policy only if needed: `out/asset-import-regression/` and `out/host-artifacts/asset-import-regression-corpus/`.
- [ ] Document host corpus layout in `tests/fixtures/asset_import_regression/README.md`.

Required host corpus layout:

```text
out/host-artifacts/asset-import-regression-corpus/
  corpus.gecorpus
  sources/
    gltf/
    textures/
    materials/
    audio/
  notices/
    THIRD_PARTY_ASSET_NOTICES.md
  expected/
    hashes.gehashes
```

`tools/validate-asset-import-regression-corpus.ps1` behavior:

- Without `-CorpusRoot`, validate schemas and generated first-party corpus only.
- With `-CorpusRoot`, validate the manifest, per-asset provenance, SHA-256 hashes, and legal policy before any importer runs.
- With `-RequireReady`, fail if no host-owned large corpus is present or if any report row is failed, legal-blocked, or nondeterministic.
- Never download assets, scrape marketplaces, call network APIs, mutate source assets, or bypass notices.
- Emit machine-readable counters including `asset_import_regression_corpus_ready`, `asset_import_regression_large_corpus_present`, `asset_import_regression_legal_blocked_count`, `asset_import_regression_failed_count`, and `asset_import_regression_replay_hash`.

### Task 4: `MK_tools` Corpus Runner

- [ ] Add `engine/tools/include/mirakana/tools/asset_import_regression_runner.hpp`.
- [ ] Add `engine/tools/asset/asset_import_regression_runner.cpp`.
- [ ] Add `asset_import_regression_runner.cpp` to `engine/tools/asset/CMakeLists.txt`.
- [ ] Add tests in `tests/unit/asset_import_regression_tests.cpp`.

Required execution API:

```cpp
namespace mirakana {

struct AssetImportRegressionRunnerOptions {
    std::string corpus_root;
    std::string output_root;
    AssetImportPresetsDocumentV1 project_presets;
    AssetImportExecutionOptions import_execution_options;
    bool require_large_corpus{false};
    bool write_cooked_outputs{false};
    bool compare_expected_hashes{true};
    bool collect_preview_rows{true};
    std::uint64_t row_budget{10000U};
};

[[nodiscard]] AssetImportRegressionReportV1
run_asset_import_regression_corpus(IFileSystem& filesystem,
                                   const AssetImportRegressionCorpusDocumentV1& corpus,
                                   const AssetImportRegressionRunnerOptions& options);

} // namespace mirakana
```

Runner rules:

- Use existing `IExternalAssetImporter` adapters and `execute_asset_import_plan` where possible.
- For glTF, parse with `fastgltf` only inside `MK_tools` and record parser/validation diagnostics as first-party report rows.
- Use `Options::LoadExternalBuffers` for glTF external buffer tests and restrict external paths to the corpus root.
- Use `Options::LoadExternalImages` only for explicit image-dependency diagnostics; actual PNG/KTX/EXR decoding remains in selected adapters.
- Use `Options::DecomposeNodeMatrices` for transform import diagnostics so matrix/TRS mismatch cases are visible.
- Use `fastgltf::validate` as a development/corpus diagnostic gate, not as the only correctness gate.
- For KTX2/Basis, use existing environment texture source review/transcode helpers and record `ktxTexture2_CreateFromNamedFile`, `ktxTexture2_NeedsTranscoding`, and `ktxTexture2_TranscodeBasis` stages as first-party strings only.
- Destroy native KTX/OpenEXR/parser resources before returning. Report rows must not contain native pointers or third-party types.
- Hash deterministic first-party source documents and cooked outputs twice in one run to detect nondeterminism.
- On any failure, record a row and continue unless manifest validation failed. Do not write partial selected project outputs.

### Task 5: Failure Diagnostics And Repro Commands

- [ ] Extend `AssetImportExecutionResult` mapping through a new helper in `asset_import_regression_runner.cpp`.
- [ ] Add deterministic diagnostic code labels in `asset_import_regression_corpus.cpp`.
- [ ] Add report-row tests for every diagnostic code listed in Task 1.

Each failure row must include `asset_id`, `source_path`, `source_sha256`, `preset_sha256`, `importer_id`, `phase`, `code`, `message`, and deterministic output hash when available.

### Task 6: Batch Reimport Planner And Transaction Staging

- [ ] Add `engine/assets/include/mirakana/assets/asset_import_batch_reimport.hpp`.
- [ ] Add `engine/assets/src/asset_import_batch_reimport.cpp`.
- [ ] Add `engine/tools/include/mirakana/tools/asset_import_batch_reimport_tool.hpp`.
- [ ] Add `engine/tools/asset/asset_import_batch_reimport_tool.cpp`.
- [ ] Add tests in `tests/unit/asset_import_regression_tests.cpp` and `tests/unit/editor_core_tests.cpp`.

Execution rules: dry-run is mandatory before apply; apply writes to `out/asset-import-regression/staging/<run-id>/` first; project outputs are replaced only after all selected rows pass hash, legal, preset, importer, and output validation; failed apply leaves project outputs unchanged.

### Task 7: Preset Diff Engine

- [ ] Add `engine/assets/include/mirakana/assets/asset_import_preset_diff.hpp`.
- [ ] Add `engine/assets/src/asset_import_preset_diff.cpp`.
- [ ] Add the files to `engine/assets/CMakeLists.txt`.
- [ ] Add tests in `tests/unit/asset_import_regression_tests.cpp`.

Diff rules: `mesh.unit_scale` and `mesh.up_axis` always mark mesh, morph mesh, skin, and transform animation rows as cooked output changes; texture preset fields map to texture source/cooked/package impacts; unsupported combinations return `review_blocked`; the diff engine never executes importers.

### Task 8: Axis And Unit Preview

- [ ] Add `engine/tools/include/mirakana/tools/asset_axis_unit_preview.hpp`.
- [ ] Add `engine/tools/asset/asset_axis_unit_preview.cpp`.
- [ ] Add tests in `tests/unit/asset_import_regression_tests.cpp`.
- [ ] Wire preview rows into the editor model in Task 9.

Preview rules: use the same normalization helpers as import, include before/after bounds, a basis triad, unit scale, sample vertex/joint rows, and fail closed on unsupported sources.

### Task 9: Editor Asset Browser Workflow Rows

- [ ] Add `editor/core/include/mirakana/editor/asset_import_regression_workflow.hpp`.
- [ ] Add `editor/core/src/asset_import_regression_workflow.cpp`.
- [ ] Add the files to `editor/core/CMakeLists.txt`.
- [ ] Extend `editor/core/include/mirakana/editor/asset_browser_production.hpp` only with retained row inputs, not importer execution.
- [ ] Add tests in `tests/unit/editor_core_tests.cpp`.

Required editor command ids: `asset_browser.importer_corpus.run`, `asset_browser.importer_corpus.open_report`, `asset_browser.import.batch_reimport`, `asset_browser.import.preset_diff`, and `asset_browser.import.axis_unit_preview`.

### Task 10: CI, Static Guards, And Validation Recipes

- [ ] Add `tools/check-asset-import-regression-corpus.ps1`.
- [ ] Add `tools/validate-asset-import-regression-corpus.ps1`.
- [ ] Add a static-contract chapter after code lands.
- [ ] Update `tools/check-ai-integration.ps1` discovery/needles if a new chapter is added.
- [ ] Update JSON/static schema checks only when new schema entrypoints are added.

Required validation commands for implementation closeout:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

### Task 11: Docs, Manifest, And Legal Records

- [ ] Update `docs/dependencies.md` only if a new dependency, feature flag, or optional package is added.
- [ ] Update `docs/legal-and-licensing.md` with corpus policy, allowed licenses, rejected licenses, and external-engine clean-room restrictions.
- [ ] Update `THIRD_PARTY_NOTICES.md` only if third-party corpus assets are tracked or distributed by the repository.
- [ ] Update current capabilities, roadmap, game-development docs, plan registry, manifest fragments, and skills only after implementation evidence exists.

## Acceptance Criteria

- The source tree contains a deterministic first-party corpus contract, report contract, tests, and validation wrappers.
- A host-owned large corpus can be validated with no network downloads and with per-asset legal/provenance checks before importer execution.
- `MK_tools` can run the corpus and produce deterministic per-asset reports across supported glTF, texture, material, animation, and audio surfaces.
- Failures are diagnosable by stable code, phase, asset id, source hash, preset hash, importer id, and repro command.
- Batch reimport supports dry-run, changed-source/preset/dependency/importer detection, legal blocking, staging, and all-or-nothing apply.
- Preset diff shows exactly which assets and outputs are affected by `AssetImportPresets.v1` changes.
- Axis/unit preview shows before/after bounds, basis, unit scale, and sample rows using the same normalization code as import.
- Editor asset browser rows expose corpus runs, failure reports, preset diff, batch reimport, and axis/unit preview without parser/runtime/native handle leakage.
- Legal gates block unapproved, NC, ND, trademark-heavy, external-engine, or notice-missing assets before import.
- Static checks prevent future docs/manifest claims from expanding into runtime source parsing, external downloads, arbitrary importer plugins, native handles, or external-engine compatibility.

## Final Validation Evidence Table

| Command | Required before completion | Expected result |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Every slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Schema/manifest/static-contract slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Manifest/docs/agent-surface slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1` | Corpus workflow slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` | Optional importer execution slice | Pass or explicit missing `asset-importers` bootstrap blocker |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -RequireReady` | Commercial corpus promotion | Pass on host with approved corpus, otherwise explicit host/legal blocker |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Final implementation closeout | Pass |
