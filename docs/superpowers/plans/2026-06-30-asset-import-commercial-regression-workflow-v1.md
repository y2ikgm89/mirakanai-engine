# Asset Import Commercial Regression Workflow v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a clean-break commercial importer regression workflow for large real glTF, texture, material, and animation asset sets, with deterministic failure diagnostics, batch reimport, preset diff, and axis/unit preview. The workflow must preserve the existing cooked-only runtime policy, consume `GameEngine.AssetImportPresets.v1`, and provide enough evidence for commercial-quality importer claims without copying Unity, Unreal Engine, Godot, or other engine behavior, UI expression, asset schemas, code, samples, marks, or compatibility claims.

**Architecture:** `MK_assets` owns value contracts and deterministic text/JSON surfaces. `MK_tools` owns optional `asset-importers` execution and converts external source formats into first-party source documents and cooked payloads. `MK_editor_core` owns retained, GUI-independent asset browser workflow rows. Native editor shell and CI scripts only execute reviewed commands. Runtime/game modules continue to consume cooked artifacts only and never parse glTF, PNG, EXR, KTX, audio, or third-party source formats.

**Tech Stack:** C++23, first-party `mirakana::` APIs, `MK_assets`, `MK_tools`, `MK_editor_core`, optional `asset-importers` vcpkg feature (`fastgltf`, `libspng`, `OpenEXR`, `KTX Software`, `miniaudio`), `AssetImportPresets.v1`, `AssetImportProvenance.v1`, `AssetImportProductionReview v1`, PowerShell 7 validation scripts, JSON Schema, CTest, and the existing first-party UI retained model.

---

## Authoring Status

- Date: 2026-07-01.
- Status: candidate implementation plan with Tasks 1-14 implemented as the host-independent foundation plus Task 16 implemented as the visible native Assets panel smoke. Task 13 real-corpus `-RequireReady` execution, Task 15 real-corpus reimport/diff/preview loops, and Task 17 commercial evidence promotion remain the commercial-operation closeout and are not selected by `currentActivePlan`.
- Selected project coordinate convention: right-handed, `+Y` up, meters, matching the current `AssetImportPresets.v1` and `AssetCoordinateNormalizationPlan` implementation.
- Current readiness estimate after the 2026-07-01 Task 16 implementation: core design and safe import contract about 90%, visible Assets panel integration 80-85%, whole commercial asset browser/import regression product about 60%. These are planning estimates only; no readiness counter changes until Task 17 real-corpus evidence lands.
- Current hard blocker: no approved large real-asset corpus has been run through `tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -RequireReady`, and the optional `asset-importers` lane still needs a dependency-ready host where `tools/bootstrap-deps.ps1 -Feature asset-importers` and `tools/build-asset-importers.ps1` pass.

## Sources Reviewed

- Project files: `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.fragments/004-modules.json`, `engine/agent/manifest.fragments/007-importerCapabilities.json`, `engine/assets/include/mirakana/assets/asset_import_presets.hpp`, `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`, `engine/assets/include/mirakana/assets/asset_import_provenance.hpp`, `engine/assets/include/mirakana/assets/asset_import_production_review.hpp`, `engine/tools/include/mirakana/tools/asset_coordinate_normalization.hpp`, `engine/tools/include/mirakana/tools/asset_import_tool.hpp`, `engine/tools/include/mirakana/tools/asset_import_adapters.hpp`, `engine/tools/include/mirakana/tools/gltf_*`, `editor/core/include/mirakana/editor/asset_browser_production.hpp`, `tests/unit/tools_tests.cpp`, and `tests/unit/editor_core_tests.cpp`.
- 2026-07-01 project re-audit files for Task 14: `engine/assets/include/mirakana/assets/asset_import_regression_corpus.hpp`, `engine/assets/src/asset_import_regression_corpus.cpp`, `engine/tools/include/mirakana/tools/asset_import_regression_runner.hpp`, `engine/tools/asset/asset_import_regression_runner.cpp`, `editor/core/include/mirakana/editor/asset_import_regression_workflow.hpp`, `editor/core/src/asset_import_regression_workflow.cpp`, `tests/unit/asset_import_regression_tests.cpp`, `tests/unit/editor_core_tests.cpp`, `tools/run-asset-import-regression-corpus.ps1`, and `tools/validate-asset-import-regression-corpus.ps1`.
- 2026-07-01 Task 16 implementation files: `editor/src/native_editor_launch.*`, `editor/src/native_editor_app.cpp`, `editor/src/first_party_editor_document.*`, `editor/src/main.cpp`, and `tests/unit/editor_native_shell_tests.cpp`.
- Context7 `/spnda/fastgltf`: `fastgltf::Parser`, `GltfDataBuffer`, `Options::LoadExternalBuffers`, `Options::LoadExternalImages`, `Options::DecomposeNodeMatrices`, `Options::GenerateMeshIndices`, `fastgltf::validate`, accessor iteration, material/image inspection, and parser reuse constraints.
- Context7 `/kitware/cmake`: add focused executables/tests with `add_executable`, `target_link_libraries(... PRIVATE ...)`, and `add_test`; use imported/library targets where dependency packages already exist and do not make CMake configure install external packages.
- Context7 `/kitware/cmake`: CMake/CTest presets support `cmake --preset <preset>`, `cmake --build --preset <preset> --target <target>`, and `ctest --preset <preset> -R <regex>` style focused validation; the repository wrappers keep those official entrypoints normalized.
- Context7 `/microsoftdocs/powershell-docs`: validation scripts use explicit parameters, `$ErrorActionPreference = 'Stop'`, deterministic `Write-Output` counters, `Write-Error` for failed checks, and `ShouldProcess` only for host-visible mutations. The Task 14 operator-loop check script is read/validate-only and must not mutate source/project outputs.
- Context7 `/khronosgroup/ktx-software`: `ktxTexture2_CreateFromNamedFile`, `ktxTexture2_NeedsTranscoding`, `ktxTexture2_TranscodeBasis`, level/layer/face/dimension metadata, selected transcode targets, and native handle isolation.
- Context7 `/mackron/miniaudio`: `ma_decoder_init_file`, `ma_decoder_init_memory`, `ma_decoder_read_pcm_frames`, `ma_data_source_get_length_in_pcm_frames`, `ma_decoder_seek_to_pcm_frame`, `ma_decoder_uninit`, WAV/FLAC/MP3 decoder availability, and private decoder lifetime boundaries.
- Khronos glTF 2.0 specification: right-handed coordinates, `+Y` up, meters, radians, glTF scenes/nodes/meshes/materials/animations, and external resource rules. Official reference: `https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html`.
- Khronos glTF registry and June 2026 glTF 2.1 planning blog: the official registry checked on 2026-07-01 still identifies glTF 2.0 as the current registry specification, while the 2.1 work is a focused future/backward-compatible large-scene plan. This plan stays on glTF 2.0 import execution and treats glTF 2.1 BVH/encapsulated/progressive-scene topics as future corpus observation rows only. Non-Khronos reporting is not an implementation source. Official references: `https://registry.khronos.org/glTF/` and `https://www.khronos.org/blog/introducing-gltf-2.1-with-complex-scenes`.
- Khronos glTF Sample Assets: per-asset license metadata is required; the top-level README is not sufficient for redistribution approval. Official reference: `https://github.com/KhronosGroup/glTF-Sample-Assets/blob/main/README.md`.
- Khronos glTF Asset Generator: useful for generated specification-coverage cases, but generated outputs still need exact source revision, generator command, license, and expected SHA-256 rows before corpus use. Official reference: `https://github.com/KhronosGroup/glTF-Asset-Generator`.
- Khronos KTX Software/libktx: KTX 1/2 loading, Basis Universal transcode, KTX2 metadata, OpenGL/Vulkan upload helpers, and KTX/native handle boundaries. Official reference: `https://github.khronos.org/KTX-Software/libktx/index.html`.
- Khronos KTX 2.0 specification: texture dimensions, levels, layers, faces, `vkFormat`, supercompression, data format descriptor, and registered metadata keys must be captured as review rows before KTX2/Basis package evidence. Official reference: `https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html`.
- W3C PNG Third Edition: PNG corpus rows must record color type, bit depth, alpha, transparency, gamma/sRGB/color-management metadata, and decoded RGBA8 policy before package evidence. Official reference: `https://www.w3.org/TR/png-3/`.
- OpenEXR documentation: scene-linear HDR semantics and EXR metadata/window/channel concepts. Official references: `https://openexr.com/en/latest/SceneLinear.html` and `https://openexr.com/en/latest/TechnicalIntroduction.html`.
- Audio format references: IETF FLAC RFC 9639 for FLAC stream/metadata/frame diagnostics, Microsoft RIFF services documentation for chunked WAV container diagnostics, and miniaudio Context7 docs for private source decode lifecycle. Official references: `https://www.rfc-editor.org/rfc/rfc9639.html` and `https://learn.microsoft.com/en-us/windows/win32/multimedia/resource-interchange-file-format-services`.
- License/corpus references: SPDX License List identifiers, Creative Commons license terms, Poly Haven CC0 asset license page, and Khronos glTF Sample Assets per-model license README requirements. Official references: `https://spdx.org/licenses/`, `https://creativecommons.org/share-your-work/cclicenses/`, `https://polyhaven.com/license`, and `https://github.com/KhronosGroup/glTF-Sample-Assets`.
- Legal/trademark category references: Unity trademark guidelines `https://unity.com/legal/trademarks`, Epic Content License Agreement `https://www.unrealengine.com/eula/content`, Epic non-licensee trademark guidelines `https://www.epic.com/epic/page/trademark-usage-guidelines-non-licensee/`, Godot license documentation `https://godotengine.org/license/`, and Godot license compliance documentation `https://docs.godotengine.org/en/stable/about/complying_with_licenses.html`.

## Current Baseline

- `MK_assets` already has `GameEngine.AssetImportPresets.v1`, `AssetImportAction::mesh_preset`, `GameEngine.AssetImportProvenance.v1`, source/cooked asset documents, and value-only production import review APIs.
- `MK_tools` already has optional adapters for PNG, glTF mesh, glTF morph mesh CPU, glTF node transform animation, glTF skin/morph animation helpers, OpenEXR environment texture source review/payload decode, KTX2/Basis environment texture metadata review/payload transcode, and common audio.
- `MK_tools` coordinate normalization already consumes `mesh.unit_scale` and `mesh.up_axis` for selected glTF mesh, morph mesh, skin, node transform, and animation paths.
- `MK_editor_core` already has a first-party asset browser production model, import review, reimport/recook/hot-reload planning, provenance/legal rows, KTX2/Basis source review rows, OpenEXR source review rows, import jobs, folder/drag-drop source review, and retained command rows.
- Existing validation covers selected fixtures and package-review evidence, but it does not yet provide a large real-asset corpus manifest, corpus runner, per-run diagnostic reports, batch reimport execution plans over a corpus, preset diff reports, or asset-browser axis/unit preview rows.
- Tasks 1-11 now provide the deterministic corpus/report contract, tiny first-party fixture corpus, host-corpus validator, `MK_tools` runner foundation, failure diagnostics, batch reimport staging, preset diff, axis/unit preview, and editor retained workflow rows.
- Remaining commercial gap: no large approved host corpus has been executed end-to-end across real glTF scenes/meshes/animations, PNG/OpenEXR/KTX2/Basis textures, material documents, and WAV/MP3/FLAC audio; no retained report proves zero failures, zero legal blockers, zero nondeterminism, and successful operator triage/reimport/preset-diff/axis-preview loops.

## Commercial Closeout Delta

| Area | Current state | Commercial closeout requirement |
| --- | --- | --- |
| Safe import contract | Value contracts, legal gates, deterministic report text, and fail-closed schemas are implemented. | Run a dependency-ready optional importer lane plus a real host corpus and retain `report.gereport`, replay hash, source notices, and corpus manifest evidence. |
| glTF and animation | Selected glTF mesh, morph, node transform, quaternion animation, and coordinate normalization paths are implemented for glTF 2.0. | Corpus must include external buffers/images, PBR material variants, skins, morph targets, quaternion TRS, invalid diagnostics, large scenes, and glTF 2.1 observation-only assets that remain non-promoting until parser support is explicit. |
| Texture/material | PNG, OpenEXR, KTX2/Basis review/transcode paths exist as optional or value-only surfaces. | Corpus must include color-management PNGs, HDR EXR metadata/window/channel cases, KTX2/Basis needs-transcoding and backend-target cases, material texture dependency rows, and package-output hash checks. |
| Audio | Common-audio optional adapter and source document paths exist. | Corpus must include WAV RIFF/chunk variants, MP3, FLAC RFC 9639 metadata/frame cases, static PCM and streaming-review preset rows, decode error rows, and cooked output hash checks. |
| Editor workflow | Source Pulse rows expose corpus run, report, batch reimport, preset diff, and axis/unit preview as value-only commands. | Visible shell must show retained workflow rows from a real corpus report, keep command enable/disable state accurate, and smoke positive row counters without executing importers in `editor/core`. |
| Legal/originality | Corpus policy blocks NC/ND, missing notice, external-engine, marketplace, trademark-heavy, and unapproved AI-generated material. | Every third-party asset row must have per-asset SPDX/custom license, copyright holder, source URL, retrieved date, version/commit, modification status, distribution target, expected SHA-256, and notice row before execution. |

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

- [x] Add `engine/assets/include/mirakana/assets/asset_import_regression_corpus.hpp`.
- [x] Add `engine/assets/src/asset_import_regression_corpus.cpp`.
- [x] Add the new header/source to `engine/assets/CMakeLists.txt`.
- [x] Add `schemas/asset-import-regression-corpus.schema.json`.
- [x] Add `schemas/asset-import-regression-report.schema.json`.
- [x] Add focused tests in `tests/unit/asset_import_regression_tests.cpp` and register a new CMake test target `MK_asset_import_regression_tests`.

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

- [x] Add tiny generated fixture builders to `tests/unit/asset_import_regression_tests.cpp` instead of committing unreviewed binary assets.
- [x] Add `tests/fixtures/asset_import_regression/README.md`.
- [x] Add `tests/fixtures/asset_import_regression/first_party_corpus.gecorpus` only if it is text-only and references generated or tiny first-party assets.

Slice 2026-06-30 candidate 2 completes the source-tree text fixture corpus for glTF mesh, glTF animation, glTF diagnostic intent cases, and a first-party material document. PNG, OpenEXR, KTX2/Basis, audio, and large real-asset importer execution remain gated by Task 4 and the optional `asset-importers` host corpus; no binary third-party assets are tracked in this slice.

Required fixture coverage:

- Valid glTF mesh with positions, normals, UVs, tangents, material reference, external buffer, and no external network resource.
- Valid glTF animation with translation, quaternion rotation, scale, skin joint tracks, morph weights, and node transform channels.
- Invalid glTF cases for unsafe external path, missing buffer, unsupported interpolation, unsupported extension, invalid quaternion, mismatched accessor counts, duplicate animation channel, unsupported skin/morph combination, and malformed material texture index.
- PNG RGBA8 texture fixture through `decode_audited_png_rgba8`.
- OpenEXR and KTX2/Basis fixtures generated through existing optional test helpers when `MK_HAS_ASSET_IMPORTERS=1`.
- First-party material document fixture with texture dependencies and missing texture diagnostics.

### Task 3: Host-Owned Large Corpus Intake Policy

- [x] Add `tools/check-asset-import-regression-corpus.ps1` for default static/schema/test checks.
- [x] Add `tools/validate-asset-import-regression-corpus.ps1`.
- [x] Add ignored output folders through existing ignore policy only if needed: `out/asset-import-regression/` and `out/host-artifacts/asset-import-regression-corpus/`.
- [x] Document host corpus layout in `tests/fixtures/asset_import_regression/README.md`.

The root `.gitignore` already ignores `out/`, so no narrower ignore rule is required for `out/asset-import-regression/` or `out/host-artifacts/asset-import-regression-corpus/`.

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

- [x] Add `engine/tools/include/mirakana/tools/asset_import_regression_runner.hpp`.
- [x] Add `engine/tools/asset/asset_import_regression_runner.cpp`.
- [x] Add `asset_import_regression_runner.cpp` to `engine/tools/asset/CMakeLists.txt`.
- [x] Add tests in `tests/unit/asset_import_regression_tests.cpp`.

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

Slice 2026-06-30 candidate 3 adds `run_asset_import_regression_corpus` in `MK_tools`. The runner validates the manifest before execution, resolves `AssetImportPresets.v1` per asset, records source SHA-256 and preset SHA-256, executes existing `execute_asset_import_plan` paths and optional `ExternalAssetImportAdapters` through a private overlay filesystem twice for deterministic output evidence, writes only caller-selected `output_root` staging artifacts when requested, and returns first-party `GameEngine.AssetImportRegressionReport.v1` rows without parser/native/third-party type leakage. Large real-asset commercial promotion still requires host-owned corpus evidence and `validate-asset-import-regression-corpus.ps1 -RequireReady`.

Candidate 3 validation evidence: `tools/check-format.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-asset-import-regression-corpus.ps1`, and full `tools/validate.ps1` pass. `tools/build-asset-importers.ps1` is blocked in this session before optional importer execution because `SPNG` is not installed in `vcpkg_installed` and `tools/bootstrap-deps.ps1 -Feature asset-importers` requires an approval prompt that is unavailable under the current `never` approval policy.

### Task 5: Failure Diagnostics And Repro Commands

- [x] Extend `AssetImportExecutionResult` mapping through a new helper in `asset_import_regression_runner.cpp`.
- [x] Add deterministic diagnostic code labels in `asset_import_regression_corpus.cpp`.
- [x] Add report-row tests for every diagnostic code listed in Task 1.

Slice 2026-06-30 candidate 4 adds direct report-row round-trip coverage for every
`AssetImportRegressionDiagnosticCode` value, including label uniqueness and deterministic report text assertions.
Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests`
and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_import_regression_tests`.

Each failure row must include `asset_id`, `source_path`, `source_sha256`, `preset_sha256`, `importer_id`, `phase`, `code`, `message`, and deterministic output hash when available.

### Task 6: Batch Reimport Planner And Transaction Staging

- [x] Add `engine/assets/include/mirakana/assets/asset_import_batch_reimport.hpp`.
- [x] Add `engine/assets/src/asset_import_batch_reimport.cpp`.
- [x] Add `engine/tools/include/mirakana/tools/asset_import_batch_reimport_tool.hpp`.
- [x] Add `engine/tools/asset/asset_import_batch_reimport_tool.cpp`.
- [x] Add planner, executor, and editor-facing dry-run row tests in `tests/unit/asset_import_regression_tests.cpp`.

Execution rules: dry-run is mandatory before apply; apply writes to `out/asset-import-regression/staging/<run-id>/` first; project outputs are replaced only after all selected rows pass hash, legal, preset, importer, and output validation; failed apply leaves project outputs unchanged.

Slice 2026-06-30 candidate 5 adds `AssetImportBatchReimportDesc` / `AssetImportBatchReimportPlan` as value-only asset-layer contracts and `execute_asset_import_batch_reimport` in `MK_tools` as the only staging/apply executor. The planner treats empty selection as explicit select-all, rejects unsafe or duplicate selections, blocks legal-policy diagnostics, source-path drift, missing import actions, failed corpus rows, unsafe paths, and stale output hashes, and only exposes `mutates_project_outputs` when an acknowledged apply is fully allowed. The executor imports selected rows into the staging root first, validates every staged output, then replaces project outputs all-or-nothing; failed validation leaves existing project outputs unchanged.

Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/asset_import_batch_reimport.cpp,engine/tools/asset/asset_import_batch_reimport_tool.cpp,tests/unit/asset_import_regression_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

Slice gate validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 7: Preset Diff Engine

- [x] Add `engine/assets/include/mirakana/assets/asset_import_preset_diff.hpp`.
- [x] Add `engine/assets/src/asset_import_preset_diff.cpp`.
- [x] Add the files to `engine/assets/CMakeLists.txt`.
- [x] Add tests in `tests/unit/asset_import_regression_tests.cpp`.

Diff rules: `mesh.unit_scale` and `mesh.up_axis` always mark mesh, morph mesh, skin, and transform animation rows as cooked output changes; texture preset fields map to texture source/cooked/package impacts; unsupported combinations return `review_blocked`; the diff engine never executes importers.

Slice 2026-06-30 candidate 6 adds the value-only `AssetImportPresetDiffDesc` / `AssetImportPresetDiff` API in `MK_assets`. The diff engine compares effective `AssetImportPresets.v1` defaults and per-`AssetKeyV2` overrides against caller-provided asset rows plus an existing `AssetImportPlan`; it never reads source assets, hashes outputs, executes importers, downloads assets, exposes parser/native handles, or claims external-engine compatibility. Mesh coordinate convention changes (`mesh.unit_scale`, `mesh.up_axis`) mark mesh, morph mesh CPU, and transform animation rows as cooked/package output changes. Texture fields map to source-review, cooked-output, and package-output impact rows. Invalid preset combinations, unsupported action kinds, failed/legal latest regression rows, and missing latest regression evidence for affected rows fail closed as `review_blocked`.

Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/asset_import_preset_diff.cpp,tests/unit/asset_import_regression_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

Slice gate validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 8: Axis And Unit Preview

- [x] Add `engine/tools/include/mirakana/tools/asset_axis_unit_preview.hpp`.
- [x] Add `engine/tools/asset/asset_axis_unit_preview.cpp`.
- [x] Add tests in `tests/unit/asset_import_regression_tests.cpp`.
- [x] Wire preview rows into the editor model in Task 9.

Preview rules: use the same normalization helpers as import, include before/after bounds, a basis triad, unit scale, sample vertex/joint rows, and fail closed on unsupported sources.

Slice 2026-06-30 candidate 7 adds `AssetAxisUnitPreviewDesc` / `AssetAxisUnitPreview` in `MK_tools` as a value-only preview over caller-supplied vertex and joint samples. The implementation uses the same `make_asset_coordinate_normalization_plan`, `normalize_asset_position`, `normalize_asset_direction`, and `normalize_asset_rotation` helpers as import/cook, emits source/project bounds, a three-axis basis triad, unit scale, up-axis, sample vertex/joint rows, and fails closed for unsupported action kinds, unsafe source paths, invalid mesh presets, missing samples, and row-budget overages. It does not parse glTF, read source files, execute importers, expose parser/native handles, or copy external-engine preview semantics.

Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/tools/asset/asset_axis_unit_preview.cpp,tests/unit/asset_import_regression_tests.cpp`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.

Slice gate validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 9: Editor Asset Browser Workflow Rows

- [x] Add `editor/core/include/mirakana/editor/asset_import_regression_workflow.hpp`.
- [x] Add `editor/core/src/asset_import_regression_workflow.cpp`.
- [x] Add the files to `editor/CMakeLists.txt`.
- [x] Extend `editor/core/include/mirakana/editor/asset_browser_production.hpp` only with retained row inputs, not importer execution.
- [x] Add tests in `tests/unit/editor_core_tests.cpp`.

Required editor command ids: `asset_browser.importer_corpus.run`, `asset_browser.importer_corpus.open_report`, `asset_browser.import.batch_reimport`, `asset_browser.import.preset_diff`, and `asset_browser.import.axis_unit_preview`.

Slice 2026-06-30 candidate 8 adds `EditorAssetImportRegressionWorkflowDesc` /
`EditorAssetImportRegressionWorkflowModel` in `MK_editor_core` and wires the
workflow into Source Pulse through `EditorAssetBrowserImportWorkflowRow` retained
rows. The workflow projects corpus/report evidence, batch reimport plans, preset
diff rows, and axis/unit preview rows into `asset_browser.import_workflow.*`, and
exposes the required command ids as value-only reviewed command rows. Editor core
does not execute importers, parse glTF, mutate project files, run package scripts
or validation recipes, expose native handles, download external assets, or claim
Unity/Unreal/Godot compatibility.

Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/core/src/asset_import_regression_workflow.cpp,editor/core/src/asset_browser_production.cpp,tests/unit/editor_core_tests.cpp`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

Slice gate validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_asset_import_regression_tests`, and full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 10: CI, Static Guards, And Validation Recipes

- [x] Add `tools/check-asset-import-regression-corpus.ps1`.
- [x] Add `tools/validate-asset-import-regression-corpus.ps1`.
- [x] Add a static-contract chapter after code lands.
- [x] Update `tools/check-ai-integration.ps1` discovery/needles if a new chapter is added.
- [x] Update JSON/static schema checks only when new schema entrypoints are added.

Required validation commands for implementation closeout:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

`tools/build-asset-importers.ps1` remains a host dependency gate in this session:
configure reaches the `asset-importers` preset but fails because `SPNGConfig.cmake`
is not present in the linked `vcpkg_installed` tree. No importer source execution
claim is made until `tools/bootstrap-deps.ps1 -Feature asset-importers` succeeds
on a dependency-ready host and the importer lane is rerun.

### Task 11: Docs, Manifest, And Legal Records

- [x] Update `docs/dependencies.md` only if a new dependency, feature flag, or optional package is added.
- [x] Update `docs/legal-and-licensing.md` with corpus policy, allowed licenses, rejected licenses, and external-engine clean-room restrictions.
- [x] Update `THIRD_PARTY_NOTICES.md` only if third-party corpus assets are tracked or distributed by the repository.
- [x] Update current capabilities, roadmap, game-development docs, plan registry, manifest fragments, and skills only after implementation evidence exists.

No dependency, feature flag, optional package, or third-party corpus asset was added in candidate 2, so `docs/dependencies.md` and `THIRD_PARTY_NOTICES.md` intentionally remain unchanged.

### Task 12: Approved Large Corpus Intake And Dependency-Ready Host

**Status:** implemented as the host-intake and dependency-ready host gate foundation. Real large-corpus execution remains blocked until an approved host-owned corpus and `asset-importers` dependency evidence exist.

**Files:**
- Create: `tools/generate-asset-import-regression-corpus-manifest.ps1`
- Modify: `tools/check-asset-import-regression-corpus.ps1`
- Modify: `tools/validate-asset-import-regression-corpus.ps1`
- Modify: `tests/fixtures/asset_import_regression/README.md`
- Modify only if third-party assets are tracked or redistributed: `THIRD_PARTY_NOTICES.md`, `docs/legal-and-licensing.md`, `docs/dependencies.md`

- [x] Add `tools/generate-asset-import-regression-corpus-manifest.ps1` with `-CorpusRoot`, `-SourcesRoot`, `-NoticesPath`, `-OutputManifest`, and `-FailOnMissingNotice` parameters. The script must not download assets or infer licenses; it only reads already-local source files and operator-supplied notice rows.
- [x] Require this host layout before execution:

```text
out/host-artifacts/asset-import-regression-corpus/
  corpus.gecorpus
  report.gereport
  sources/
    gltf/
    textures/
    materials/
    audio/
  notices/
    THIRD_PARTY_ASSET_NOTICES.md
  expected/
    hashes.gehashes
  retained/
    official-source-ledger.md
    corpus-selection-summary.md
```

- [x] Lock minimum corpus composition in `corpus-selection-summary.md`:
  - at least 40 glTF rows, including mesh-only, scene, skin, morph, material, external buffer, external image, animation TRS, quaternion animation, invalid external path, invalid extension, invalid accessor, and unsupported interpolation cases;
  - at least 30 texture rows across PNG, OpenEXR, and KTX2/Basis, including color-management, alpha, HDR metadata, mip/array/cubemap, needs-transcoding, and backend-target-policy cases;
  - at least 20 material rows covering factor-only, texture-backed, missing dependency, duplicated texture reference, unsupported graph/export, and package-output cases;
  - at least 20 animation rows covering node TRS, skin, morph weight, valid/invalid quaternion, unit/up-axis conversion, and unsupported scalar z-up rotation cases;
  - at least 20 audio rows across WAV, MP3, and FLAC, including static PCM, streaming-source-review, invalid/truncated decode, channel/sample-rate variance, and loop/normalization preset cases.
- [x] Allow Khronos glTF Sample Assets, glTF Asset Generator output, Poly Haven CC0 assets, or other public assets only when each asset row has a direct source URL, retrieved date, version or commit, author/copyright holder, SPDX/custom license id, modification status, distribution target, expected SHA-256, and notice id. The top-level project license alone is not enough when the asset directory has its own README/license.
- [x] Keep Unity, Unreal Engine, Godot, Fab, Asset Store, Starter Content, templates, marketplace packages, sample projects, serialized schemas, editor UI screenshots/icons, and trademark-heavy assets out of the corpus unless a separate legal approval artifact exists before execution.
- [ ] Run dependency bootstrap on an approval-capable dependency host:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
```

Expected ready evidence: `asset-importers` configure/build passes and does not add default-build dependencies or runtime source parsing claims. If bootstrap is blocked, record the exact missing vcpkg package or host error and do not attempt Task 13.

Slice 2026-06-30 candidate 12 adds the host-corpus intake generator and fail-closed `-RequireReady` layout gate. `tools/generate-asset-import-regression-corpus-manifest.ps1` accepts only canonical local `sources/`, `notices/THIRD_PARTY_ASSET_NOTICES.md`, and `corpus.gecorpus` paths plus operator-supplied notice rows, writes deterministic `corpus.gecorpus` and `expected/hashes.gehashes`, checks notice anchors with `-FailOnMissingNotice`, emits no download or license-inference counters, and leaves legal approval to retained notices. `tools/validate-asset-import-regression-corpus.ps1 -RequireReady` now also requires `report.gereport`, `expected/hashes.gehashes`, `notices/THIRD_PARTY_ASSET_NOTICES.md`, `sources/gltf`, `sources/textures`, `sources/materials`, `sources/audio`, `retained/official-source-ledger.md`, `retained/corpus-selection-summary.md`, notice-anchor completeness, minimum large-corpus composition evidence, and matching summary/category `required_feature` coverage before `asset_import_regression_corpus_ready=1` can be emitted. This does not add third-party assets, dependencies, or external-engine compatibility claims; real corpus execution remains Task 13+.

Dependency host evidence is still blocked in this session: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers` was blocked by the approval policy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` failed configure because `SPNGConfig.cmake` / `spng-config.cmake` is missing from `vcpkg_installed`. Do not attempt Task 13 real importer execution until an approval-capable host completes `asset-importers` bootstrap and build.

Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1`.

### Task 13: Real Corpus Runner CLI And Retained Report

**Status:** partially implemented. The host-independent reviewed CLI/wrapper gate is implemented and validated with a tiny generated corpus. Real large-corpus `-RequireReady` execution remains blocked until an approved host-owned corpus exists and the optional `asset-importers` dependency lane builds on a dependency-ready host.

**Files:**
- [x] Create: `engine/tools/asset/asset_import_regression_runner_cli.cpp`
- [x] Modify: `CMakeLists.txt`
- [x] Create: `tools/run-asset-import-regression-corpus.ps1`
- [x] Modify: `tools/check-asset-import-regression-corpus.ps1`
- [x] Modify: `tools/generate-asset-import-regression-corpus-manifest.ps1`
- [x] Modify: `tests/fixtures/asset_import_regression/README.md`
- [x] Modify: agent manifest/static surfaces
- [ ] Run on approved large real-asset corpus with dependency-ready `asset-importers` host evidence

- [x] Add a narrow `MK_asset_import_regression_runner` executable that accepts only reviewed arguments:

```text
--corpus-root <path>
--output-root <path>
--presets <path>
--write-report <path>
--write-cooked-outputs
--compare-expected-hashes
--collect-preview-rows
--row-budget <positive integer>
```

It must reject network URLs, absolute source paths from the manifest, parent traversal, reparse points, native handle requests, parser object dumping, and arbitrary importer plugin ids.

- [x] `tools/run-asset-import-regression-corpus.ps1` must call `tools/check-asset-import-regression-corpus.ps1 -CorpusRoot <path>` first, then run `MK_asset_import_regression_runner`, then run `tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot <path>`. With `-RequireReady`, it must pass through `-RequireReady` to the final validator and fail unless `asset_import_regression_corpus_ready=1`.
- [x] Runner output must include only first-party deterministic rows:

```text
asset_import_regression_report=<relative report path>
asset_import_regression_asset_count=<n>
asset_import_regression_succeeded_count=<n>
asset_import_regression_failed_count=<n>
asset_import_regression_legal_blocked_count=<n>
asset_import_regression_nondeterministic_count=<n>
asset_import_regression_replay_hash=sha256:<64 hex>
```

- [x] Report rows include `asset_id`, `kind`, `source_path`, `source_sha256`, `preset_sha256`, `importer_id`, `importer_version`, `phase`, `code`, `message`, `deterministic_output_hash`, `succeeded`, and `ready_for_commercial_evidence`.
- [x] Run each accepted asset twice in one process and once in a fresh process. Any same-process output hash mismatch sets `nondeterministic_output`; the wrapper fails on fresh-process report mismatch before final validation.
- [ ] Retain complete real-run evidence under `out/host-artifacts/asset-import-regression-corpus/retained/<run-id>/`. The host-independent wrapper currently retains `report.gereport`, runner stdout/stderr, fresh runner stdout/stderr, replay/report/manifest/notices hashes, and runner version evidence. Dependency versions and `asset-importers` build log remain dependency-host gated and must be captured with the real large-corpus run.

Slice 2026-06-30 candidate 13 adds `MK_asset_import_regression_runner`, `tools/run-asset-import-regression-corpus.ps1`, and static/manifest/docs sync. The CLI rejects network URLs, absolute/backslash/drive paths, parent traversal, reparse-point prefixes, missing required arguments, unknown arguments, and zero row budgets before execution; it writes deterministic `GameEngine.AssetImportRegressionReport.v1` rows and machine-readable counters including `asset_import_regression_replay_hash`. The wrapper runs the existing corpus check first, invokes the CLI, reruns it in a fresh process with a separate retained report, requires report text equality, retains stdout/stderr/hash evidence, and runs the final validator. `tools/generate-asset-import-regression-corpus-manifest.ps1` and the local smoke helper now write deterministic LF UTF-8 text so C++ parsers do not need CRLF tolerance. This does not add third-party assets, dependency installs, external-engine compatibility claims, or commercial corpus readiness.

Required validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -OutputRoot out/asset-import-regression/staging/large-corpus -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -RequireReady
```

Current validation evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1` passes and covers generator smoke, fixture validator, CMake configure/build for `MK_asset_import_regression_tests` and `MK_asset_import_regression_runner`, focused CTest, reviewed CLI counter smoke, wrapper fresh-process determinism, and URL rejection. Real-corpus `-RequireReady` remains not run because `out/host-artifacts/asset-import-regression-corpus` is absent and `tools/build-asset-importers.ps1` is blocked by missing `SPNGConfig.cmake` / `spng-config.cmake` until `asset-importers` bootstrap can run on an approval-capable dependency host.

### Task 14: Failure Diagnosis And Operator Triage Loop

**Status:** implemented and validated with synthetic first-party reports. This task is host-independent and does not require a real corpus.

**Design decision:** keep `GameEngine.AssetImportRegressionReport.v1` as raw measured evidence and add a separate first-party `GameEngine.AssetImportRegressionTriage.v1` value contract. This avoids mixing importer evidence with operator decisions, keeps failure diagnosis deterministic, and lets Task 15 consume triage rows without making editor-core execute importers.

**Files:**
- Create: `engine/assets/include/mirakana/assets/asset_import_regression_triage.hpp`
- Create: `engine/assets/src/asset_import_regression_triage.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Modify: `editor/core/include/mirakana/editor/asset_import_regression_workflow.hpp`
- Modify: `editor/core/src/asset_import_regression_workflow.cpp`
- Modify: `tools/check-asset-import-regression-corpus.ps1`
- Create: `tools/check-asset-import-regression-operator-loop.ps1`
- Test: `tests/unit/asset_import_regression_tests.cpp`
- Test: `tests/unit/editor_core_tests.cpp`
- Modify if new retained row ids become durable needles: `tools/check-ai-integration-147-asset-import-regression-corpus.ps1`
- Modify only if AI-operable command contracts change: `engine/agent/manifest.fragments/002-commands.json`, then run `tools/compose-agent-manifest.ps1 -Write`

Required public value types:

```cpp
namespace mirakana {

enum class AssetImportRegressionTriageSeverity : std::uint8_t {
    info,
    action_required,
    blocked,
};

enum class AssetImportRegressionRecommendedAction : std::uint8_t {
    none,
    fix_notice_or_remove_asset,
    refresh_corpus_manifest,
    inspect_source_asset,
    record_unsupported_or_reduce_source,
    open_axis_unit_preview,
    inspect_codec_dependency,
    rerun_isolated_and_compare_hashes,
    split_corpus_or_raise_reviewed_budget,
};

enum class AssetImportRegressionReimportDecision : std::uint8_t {
    not_needed,
    dry_run_allowed,
    blocked,
};

struct AssetImportRegressionTriageRowV1 {
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
    AssetImportRegressionTriageSeverity severity{AssetImportRegressionTriageSeverity::info};
    AssetImportRegressionRecommendedAction recommended_action{AssetImportRegressionRecommendedAction::none};
    AssetImportRegressionReimportDecision reimport_decision{AssetImportRegressionReimportDecision::not_needed};
    std::string repro_command_id;
    std::string repro_command;
    std::string source_excerpt_hash;
    bool preset_diff_required{false};
    bool axis_unit_preview_required{false};
    bool legal_blocked{false};
    bool nondeterministic{false};
};

struct AssetImportRegressionTriageDocumentV1 {
    std::string format{"GameEngine.AssetImportRegressionTriage.v1"};
    std::string corpus_id;
    std::string run_id;
    std::vector<AssetImportRegressionTriageRowV1> rows;
    std::size_t row_count{0U};
    std::size_t blocked_count{0U};
    std::size_t reimport_candidate_count{0U};
    std::size_t preset_diff_required_count{0U};
    std::size_t axis_unit_preview_required_count{0U};
    bool ready_for_operator_review{false};
};

[[nodiscard]] std::string_view
asset_import_regression_triage_severity_label(AssetImportRegressionTriageSeverity value) noexcept;
[[nodiscard]] std::string_view
asset_import_regression_recommended_action_label(AssetImportRegressionRecommendedAction value) noexcept;
[[nodiscard]] std::string_view
asset_import_regression_reimport_decision_label(AssetImportRegressionReimportDecision value) noexcept;

[[nodiscard]] AssetImportRegressionRecommendedAction
recommended_action_for_asset_import_regression_code(AssetImportRegressionDiagnosticCode code) noexcept;
[[nodiscard]] AssetImportRegressionTriageDocumentV1
make_asset_import_regression_triage_v1(const AssetImportRegressionReportV1& report);
[[nodiscard]] std::string serialize_asset_import_regression_triage_v1(const AssetImportRegressionTriageDocumentV1& document);
[[nodiscard]] AssetImportRegressionTriageDocumentV1
deserialize_asset_import_regression_triage_v1(std::string_view text);

} // namespace mirakana
```

Diagnostic-to-action table:

| Diagnostic code | Recommended action | Reimport decision | Extra flags |
| --- | --- | --- | --- |
| `none` | `none` | `not_needed` | none |
| `invalid_manifest`, `duplicate_asset_id`, `unsafe_source_path`, `row_budget_exceeded` | `split_corpus_or_raise_reviewed_budget` | `blocked` | none |
| `missing_source_file`, `source_hash_mismatch` | `refresh_corpus_manifest` | `blocked` | `preset_diff_required=false`, `axis_unit_preview_required=false` |
| `missing_license_provenance`, `rejected_license`, `external_engine_material` | `fix_notice_or_remove_asset` | `blocked` | `legal_blocked=true` |
| `unsupported_format`, `unsupported_extension`, `unsupported_animation_channel`, `unsupported_skin_or_morph_combination` | `record_unsupported_or_reduce_source` | `blocked` | none |
| `parser_error`, `validator_error`, `missing_external_resource`, `unsafe_external_resource_path` | `inspect_source_asset` | `dry_run_allowed` | none |
| `coordinate_normalization_failed` | `open_axis_unit_preview` | `dry_run_allowed` | `axis_unit_preview_required=true` |
| `material_extraction_failed` | `inspect_source_asset` | `dry_run_allowed` | `preset_diff_required=true` |
| `texture_decode_failed`, `texture_transcode_failed` | `inspect_codec_dependency` | `dry_run_allowed` | `preset_diff_required=true` |
| `cooked_output_mismatch`, `nondeterministic_output` | `rerun_isolated_and_compare_hashes` | `blocked` | `nondeterministic=true` only for `nondeterministic_output` |

Clean-room and legal rules:

- Triage rows are first-party labels and commands only. Do not use Unity, Unreal Engine, Godot, Fab, Asset Store, marketplace, Blender UI, or other product command names as row categories, labels, command ids, or statuses.
- Any row whose corpus/provenance source mentions external-engine content, marketplace-only terms, missing license, NC, ND, trademark-heavy content, or unclear AI-generated rights remains `fix_notice_or_remove_asset` and `reimport_decision=blocked`.
- `repro_command` must be a repository-relative PowerShell command template that never embeds absolute host paths, user profile paths, secrets, network URLs, native handles, parser dumps, or marketplace URLs.
- `source_excerpt_hash` is a hash of a bounded sanitized diagnostic/source excerpt. Do not serialize source snippets from third-party assets into the triage document.
- No broad compatibility or parity claim is created by triage. glTF 2.1 rows remain observation-only until Khronos publishes a current registry specification and the importer explicitly supports the relevant feature.

Implementation steps:

- [x] Add the `asset_import_regression_triage.hpp` public value contract exactly as listed above and include only first-party asset/report headers plus standard-library headers.
- [x] Add `asset_import_regression_triage.cpp` with label functions, a full `switch` for every `AssetImportRegressionDiagnosticCode`, deterministic triage construction, and LF-only deterministic text serialization using the same `key=value` style as `asset_import_regression_corpus.cpp`.
- [x] Make `make_asset_import_regression_triage_v1` preserve report row order, copy report evidence fields unchanged, compute counts from rows, set `ready_for_operator_review=true` when `row_count > 0`, and reject duplicate/unsupported serialized triage keys during deserialization.
- [x] Generate `repro_command_id` as `asset_import_regression.repro.<sanitized-asset-id>` and `repro_command` as:

```text
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -OutputRoot out/asset-import-regression/staging/<run-id>
```

  Replace `<run-id>` with the sanitized `report.run_id`. The command reproduces the retained corpus run, not a hidden one-off asset import, because the current reviewed runner does not have a per-asset filter.

- [x] Add tests in `tests/unit/asset_import_regression_tests.cpp` before implementation:
  - every diagnostic code maps to one non-`invalid`, non-empty recommended action label;
  - a mixed report with `rejected_license`, `source_hash_mismatch`, `coordinate_normalization_failed`, `texture_decode_failed`, `nondeterministic_output`, and `none` serializes/deserializes deterministically;
  - the triage text contains no backslashes, drive prefixes, absolute paths, user profile path fragments, network URLs, source snippets, parser type names, or external engine compatibility labels;
  - `coordinate_normalization_failed` sets `axis_unit_preview_required=true` and `reimport_decision=dry_run_allowed`;
  - legal rows set `legal_blocked=true` and `reimport_decision=blocked`;
  - nondeterministic rows set `nondeterministic=true` and `reimport_decision=blocked`.
- [x] Extend `EditorAssetImportRegressionWorkflowDesc` with `const AssetImportRegressionTriageDocumentV1* triage{nullptr};`.
- [x] Extend editor-core retained rows under `asset_browser.import_workflow.failure.<asset_id>` with `category_label=triage_failure`, severity, phase, diagnostic code, recommended action, repro command id, and reimport decision. These rows must leave `mutates_project_files=false`, `executes_import_tools=false`, `executes_package_scripts=false`, `executes_validation_recipes=false`, and `exposes_native_handles=false`.
- [x] Update command enablement in `make_editor_asset_import_regression_workflow_model`:
  - `open_report` is enabled when a report exists;
  - `batch_reimport` is enabled only when triage exists and at least one row has `reimport_decision=dry_run_allowed` with no `legal_blocked` rows selected;
  - `preset_diff` is enabled when any triage row has `preset_diff_required=true`;
  - `axis_unit_preview` is enabled when any triage row has `axis_unit_preview_required=true`;
  - `run_corpus` remains gated by corpus legal readiness and still requires user confirmation.
- [x] Add editor-core tests proving legal-blocked rows disable reimport, codec/material rows enable preset diff only as retained command rows, coordinate rows enable axis/unit preview, and all safety booleans remain false.
- [x] Add `tools/check-asset-import-regression-operator-loop.ps1` with parameters `-ReportPath`, optional `-OutputRoot`, and switch `-SyntheticSmoke`. With `-SyntheticSmoke`, generate a small first-party report under `out/tmp/asset-import-regression-operator-loop-<pid>/`, run the focused CMake build/test for `MK_asset_import_regression_tests` and `MK_editor_core_tests`, validate deterministic triage serialization through the C++ unit tests and script-generated triage text, and emit:

```text
asset_import_regression_operator_loop_ready=1
asset_import_regression_operator_loop_report_rows=<positive>
asset_import_regression_operator_loop_blocked_rows=<n>
asset_import_regression_operator_loop_reimport_candidates=<n>
asset_import_regression_operator_loop_preset_diff_required=<n>
asset_import_regression_operator_loop_axis_unit_preview_required=<n>
asset_import_regression_operator_loop_legal_blocked_rows=<n>
asset_import_regression_operator_loop_nondeterministic_rows=<n>
asset_import_regression_operator_loop_editor_core_value_only=1
asset_import_regression_operator_loop_external_engine_claim=0
```

  Without `-SyntheticSmoke`, the script reads an existing retained report and fails if the triage cannot be parsed, if counters are missing, if unsafe paths/URLs appear, or if the report has no rows.

- [x] Update `tools/check-asset-import-regression-corpus.ps1` to call `tools/check-asset-import-regression-operator-loop.ps1 -SyntheticSmoke` after the existing runner CLI smoke.
- [x] Run the targeted agent-surface drift check. The new script name, command counters, public triage APIs, and retained row ids are durable AI-operable contracts, so `tools/check-ai-integration-147-asset-import-regression-corpus.ps1` and manifest fragments were updated.

Required validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests MK_editor_core_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_asset_import_regression_tests|MK_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-operator-loop.ps1 -SyntheticSmoke
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Slice completion rules:

- Do not run or require a real large corpus in this task.
- Do not change readiness counters or commercial claims.
- Do not add external assets, dependency installs, external-engine compatibility, runtime source parsing, native handle exposure, or marketplace/network acquisition.
- If any public header or CMake target layout changes beyond the triage contract, also run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- If Task 14 touches durable command or AI-operable manifest contracts, also run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` after composing the manifest.

### Task 15: Batch Reimport, Preset Diff, And Axis/Unit Preview On Real Corpus

**Status:** planned, host-gated by the approved large corpus and optional `asset-importers` dependency host. Task 14 already added retained triage command enablement for synthetic reports; this task is limited to real-corpus retained evidence and apply/diff/preview closeout.

**Files:**
- Modify: `engine/assets/include/mirakana/assets/asset_import_batch_reimport.hpp`
- Modify: `engine/assets/src/asset_import_batch_reimport.cpp`
- Modify: `engine/assets/include/mirakana/assets/asset_import_preset_diff.hpp`
- Modify: `engine/assets/src/asset_import_preset_diff.cpp`
- Modify: `engine/tools/include/mirakana/tools/asset_axis_unit_preview.hpp`
- Modify: `engine/tools/asset/asset_axis_unit_preview.cpp`
- Modify: `tools/check-asset-import-regression-operator-loop.ps1`
- Test: `tests/unit/asset_import_regression_tests.cpp`

- [ ] Extend the Task 14 operator-loop validation script with `-CorpusRoot` and `-RequireReady` so it consumes one retained successful run and one retained failure run from the large corpus. The script must prove:
  - batch reimport dry-run rejects stale hashes and legal blockers;
  - apply writes only to `out/asset-import-regression/staging/<run-id>/` until every selected row validates;
  - preset diff lists all affected outputs for `mesh.unit_scale`, `mesh.up_axis`, texture color/compression policy, and audio decode mode changes;
  - axis/unit preview uses the same normalization helper as import and emits before/after bounds, basis triad, unit scale, sample vertices, sample joints, and unsupported-source blockers;
  - no editor-core command executes importers, package scripts, validation recipes, or native handles.
- [ ] Add real-corpus regression fixtures by retaining compact `*.gereport` and `*.gepreview` text outputs only. Do not commit third-party source assets or large binary outputs.
- [ ] Consume retained real-corpus `preset_diff_required=true` and `axis_unit_preview_required=true` rows through the already implemented Source Pulse command enablement, then retain real-corpus diff/preview output evidence without committing third-party sources or large binaries.

Required validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-operator-loop.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

### Task 16: Visible Assets Panel Commercial Workflow Smoke

**Status:** implemented and validated with synthetic first-party retained reports plus native shell smoke. This task is host-independent and does not require a real corpus.

**Files:**
- Modify: `editor/src/native_editor_app.cpp`
- Modify: `editor/src/native_editor_launch.hpp`
- Modify: `editor/src/native_editor_launch.cpp`
- Modify: `editor/src/first_party_editor_document.hpp`
- Modify: `editor/src/first_party_editor_document.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/editor_native_shell_tests.cpp`
- Modify only if retained ids become static needles: `tools/check-ai-integration-147-asset-import-regression-corpus.ps1`

- [x] Add shell-owned import-regression workflow handoff that reads retained report text from `--asset-import-regression-report <project-relative-report>`, sanitizes visible report rows, derives `GameEngine.AssetImportRegressionTriage.v1`, and feeds `EditorAssetImportRegressionWorkflowDesc`. It must not run importers from `editor/core`.
- [x] Extend editor smoke output with exact counters:

```text
editor_asset_import_regression_workflow_visible=1
editor_asset_import_regression_workflow_rows=<positive>
editor_asset_import_regression_failed_rows=<n>
editor_asset_import_regression_reimport_command_enabled=<0|1>
editor_asset_import_regression_preset_diff_command_enabled=<0|1>
editor_asset_import_regression_axis_unit_preview_command_enabled=<0|1>
editor_asset_import_regression_importers_executed_in_core=0
editor_asset_import_regression_native_handles_exposed=0
editor_asset_import_regression_external_engine_claim=0
```

- [x] Add native-shell tests proving safe project-relative launch parsing, unsafe path rejection, retained report projection into `asset_browser.import_workflow.*`, preset-diff/axis-preview command enablement, legal-blocked reimport disablement, and no absolute host corpus paths exposed in retained UI text.

Required validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1
out\build\desktop-editor\editor\Debug\MK_editor.exe --smoke-frames 1 --no-user-config
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests
```

### Task 17: Commercial Evidence Promotion And Documentation Sync

**Status:** planned, not implemented.

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/editor.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/007-importerCapabilities.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only if this plan becomes active or readiness counters change
- Modify: `engine/agent/manifest.json` only by running `tools/compose-agent-manifest.ps1 -Write`
- Modify only when notices change: `THIRD_PARTY_NOTICES.md`

- [ ] Promote only the exact evidenced claim: "large approved host-owned asset import regression corpus ready for selected source importers." Do not promote broad asset browser commercial readiness, marketplace ingestion, external-engine compatibility, runtime source parsing, or all-format importer readiness.
- [ ] Update manifest importer capability rows with retained run id, corpus asset counts, ready/failure/legal/nondeterminism counters, replay hash, optional dependency feature status, and unsupported claims.
- [ ] If any third-party corpus asset is tracked, packaged, or distributed by the repository, update `THIRD_PARTY_NOTICES.md` with name, source URL, retrieved date, version or commit, author/copyright holder, SPDX/custom license, modification status, and distribution target before readiness promotion.
- [ ] Run the agent-surface drift check and repository consistency checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Before publication, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` and publish through the standard PR flow. Do not push directly to `main`.

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
- Commercial promotion requires Tasks 12-17 to produce retained real-corpus evidence with `asset_import_regression_large_corpus_present=1`, `asset_import_regression_failed_count=0`, `asset_import_regression_legal_blocked_count=0`, `asset_import_regression_nondeterministic_count=0`, and `asset_import_regression_corpus_ready=1`.
- Any asset or official-source row that mentions Unity, Unreal Engine, Godot, Fab, Asset Store, or marketplace material remains a rejection/category/legal row, not an import compatibility or implementation source row.

## Final Validation Evidence Table

| Command | Required before completion | Observed 2026-07-01 |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Every slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Plan/tool text slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Schema/manifest/static-contract slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Manifest/docs/agent-surface slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Tool/script/agent hygiene slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Public C++ API slice | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-operator-loop.ps1 -SyntheticSmoke` | Task 14 operator triage loop | Pass; emitted `asset_import_regression_operator_loop_report_rows=6`, `failed_rows=5`, `legal_blocked_rows=1`, `nondeterministic_rows=1`, `reimport_candidates=2`, `blocked_rows=3`, `preset_diff_required=1`, `axis_unit_preview_required=1`, `editor_core_value_only=1`, and `external_engine_claim=0` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-asset-import-regression-corpus.ps1` | Corpus workflow slice | Pass; existing generator/runner/fresh-process checks passed and the integrated operator-loop synthetic smoke passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_asset_import_regression_tests MK_editor_core_tests` | Asset triage and editor-core retained row C++ contract | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_asset_import_regression_tests|MK_editor_core_tests"` | Asset triage and editor-core retained row C++ contract | Pass; 2/2 tests passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/asset_import_regression_triage.cpp,editor/core/src/asset_import_regression_workflow.cpp,tests/unit/asset_import_regression_tests.cpp,tests/unit/editor_core_tests.cpp` | Focused C++ static analysis | Pass; 4 files checked |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files editor/src/native_editor_launch.cpp,editor/src/native_editor_app.cpp,editor/src/first_party_editor_document.cpp,tests/unit/editor_native_shell_tests.cpp` | Task 16 focused C++ static analysis | Pass; 4 files checked; `editor/src/main.cpp` is outside the `dev` compile database and is covered by `tools/build-editor.ps1` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_editor_native_shell_tests` | Task 16 visible native Assets panel smoke C++ contract | Pass |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_editor_native_shell_tests` | Task 16 visible native Assets panel smoke C++ contract | Pass; 1/1 test target passed |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-editor.ps1` | Task 16 native shell executable and smoke lane | Pass; `MK_editor.exe`, `MK_editor_native_shell_tests`, and desktop-editor tests passed |
| `out\build\desktop-editor\editor\Debug\MK_editor.exe --smoke-frames 1 --no-user-config` | Task 16 direct shell smoke counters | Pass; emitted `editor_asset_import_regression_*` counters with no report path, workflow rows `0`, and importer/native-handle/external-engine counters `0` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature asset-importers` | Optional importer dependency host | Blocked by approval policy in this session; rerun on an approval-capable host |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` | Optional importer execution slice | Blocked by missing `SPNGConfig.cmake` in `vcpkg_installed`; rerun after `asset-importers` bootstrap succeeds |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-asset-import-regression-corpus.ps1 -CorpusRoot out/host-artifacts/asset-import-regression-corpus -RequireReady` | Commercial corpus promotion | Not run; requires host-owned approved large corpus |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Final implementation closeout | Pass; 161/161 tests passed |
| Tasks 13 real-corpus `-RequireReady` remainder, Task 15 real-corpus loops, and Task 17 closeout | Commercial regression workflow promotion | Planned/host-gated; no readiness counter change |
