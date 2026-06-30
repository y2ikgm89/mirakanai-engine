// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_import_batch_reimport.hpp"
#include "mirakana/assets/asset_import_preset_diff.hpp"
#include "mirakana/assets/asset_import_regression_corpus.hpp"
#include "mirakana/assets/asset_import_regression_triage.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/asset_axis_unit_preview.hpp"
#include "mirakana/tools/asset_import_batch_reimport_tool.hpp"
#include "mirakana/tools/asset_import_regression_runner.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::AssetImportProvenanceRowV1 first_party_provenance() {
    return mirakana::AssetImportProvenanceRowV1{
        .asset_key = mirakana::AssetKeyV2{.value = "meshes/hero"},
        .origin = mirakana::AssetImportProvenanceOrigin::first_party,
        .source_url = "first-party://asset-import-regression/hero",
        .retrieved_date = "2026-06-30",
        .version_or_commit = "generated-fixture-v1",
        .copyright_holder = "GameEngine contributors",
        .license_id = "LicenseRef-Proprietary",
        .modification_status = "generated first-party fixture",
        .distribution_target = "source-tree regression fixture",
        .notice_id = "THIRD_PARTY_NOTICES.md#first-party",
        .notice_complete = true,
        .external_engine_material = false,
    };
}

[[nodiscard]] mirakana::AssetImportRegressionCorpusAssetV1 good_corpus_asset() {
    return mirakana::AssetImportRegressionCorpusAssetV1{
        .asset_id = "mesh.hero",
        .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
        .asset_key = mirakana::AssetKeyV2{.value = "meshes/hero"},
        .source_path = "sources/gltf/hero.gltf",
        .expected_sha256 = "sha256:hero-source",
        .expected_output_kinds = {"GameEngine.MeshSource.v2", "GameEngine.CookedMesh.v2"},
        .required_features = {"gltf_geometry", "parser_validation"},
        .mesh_preset =
            mirakana::AssetImportMeshPresetV1{
                .unit_scale = 0.01F,
                .up_axis = mirakana::AssetImportMeshUpAxis::z,
                .triangulate = true,
                .generate_normals = true,
                .generate_tangents = true,
                .material_extraction = mirakana::AssetImportMeshMaterialExtraction::source_references,
            },
        .preset_metadata = {"mesh.unit_scale=0.01", "mesh.up_axis=z"},
        .provenance = first_party_provenance(),
        .license_policy = mirakana::AssetImportRegressionLicensePolicy::accepted_for_source_tree,
        .allow_external_resources = true,
        .allow_checked_in_distribution = true,
    };
}

[[nodiscard]] mirakana::AssetImportRegressionCorpusDocumentV1 good_corpus() {
    return mirakana::AssetImportRegressionCorpusDocumentV1{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .corpus_version = "1",
        .root_path = "tests/fixtures/asset_import_regression",
        .assets = {good_corpus_asset()},
        .row_budget = 100U,
    };
}

[[nodiscard]] bool contains(const std::vector<std::string>& values, const std::string& expected) {
    return std::ranges::find(values, expected) != values.end();
}

[[nodiscard]] bool near(float lhs, float rhs) {
    return std::abs(lhs - rhs) <= 0.00001F;
}

[[nodiscard]] bool near(mirakana::Vec3 lhs, mirakana::Vec3 rhs) {
    return near(lhs.x, rhs.x) && near(lhs.y, rhs.y) && near(lhs.z, rhs.z);
}

[[nodiscard]] bool contains_preset_diff_field(const std::vector<mirakana::AssetImportPresetDiffFieldChange>& values,
                                              const std::string& expected) {
    return std::ranges::find(values, expected, &mirakana::AssetImportPresetDiffFieldChange::field) != values.end();
}

[[nodiscard]] std::vector<mirakana::AssetImportRegressionDiagnosticCode> all_diagnostic_codes() {
    return {
        mirakana::AssetImportRegressionDiagnosticCode::none,
        mirakana::AssetImportRegressionDiagnosticCode::invalid_manifest,
        mirakana::AssetImportRegressionDiagnosticCode::duplicate_asset_id,
        mirakana::AssetImportRegressionDiagnosticCode::unsafe_source_path,
        mirakana::AssetImportRegressionDiagnosticCode::missing_source_file,
        mirakana::AssetImportRegressionDiagnosticCode::source_hash_mismatch,
        mirakana::AssetImportRegressionDiagnosticCode::missing_license_provenance,
        mirakana::AssetImportRegressionDiagnosticCode::rejected_license,
        mirakana::AssetImportRegressionDiagnosticCode::external_engine_material,
        mirakana::AssetImportRegressionDiagnosticCode::unsupported_format,
        mirakana::AssetImportRegressionDiagnosticCode::parser_error,
        mirakana::AssetImportRegressionDiagnosticCode::validator_error,
        mirakana::AssetImportRegressionDiagnosticCode::missing_external_resource,
        mirakana::AssetImportRegressionDiagnosticCode::unsafe_external_resource_path,
        mirakana::AssetImportRegressionDiagnosticCode::unsupported_extension,
        mirakana::AssetImportRegressionDiagnosticCode::unsupported_animation_channel,
        mirakana::AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination,
        mirakana::AssetImportRegressionDiagnosticCode::coordinate_normalization_failed,
        mirakana::AssetImportRegressionDiagnosticCode::material_extraction_failed,
        mirakana::AssetImportRegressionDiagnosticCode::texture_decode_failed,
        mirakana::AssetImportRegressionDiagnosticCode::texture_transcode_failed,
        mirakana::AssetImportRegressionDiagnosticCode::cooked_output_mismatch,
        mirakana::AssetImportRegressionDiagnosticCode::nondeterministic_output,
        mirakana::AssetImportRegressionDiagnosticCode::row_budget_exceeded,
    };
}

[[nodiscard]] std::filesystem::path find_repo_root() {
#ifdef MK_SOURCE_DIR
    return std::filesystem::path{MK_SOURCE_DIR};
#else
    auto path = std::filesystem::current_path();
    while (!path.empty()) {
        if (std::filesystem::exists(path / "CMakeLists.txt") && std::filesystem::exists(path / "tests")) {
            return path;
        }
        const auto parent = path.parent_path();
        if (parent == path) {
            break;
        }
        path = parent;
    }
    throw std::runtime_error("repo root was not found");
#endif
}

[[nodiscard]] std::string read_text_file(const std::filesystem::path& path) {
    std::ifstream input{path, std::ios::binary};
    if (!input) {
        throw std::runtime_error("failed to read text file: " + path.generic_string());
    }
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

} // namespace

MK_TEST("asset import regression corpus serializes deterministically and round trips") {
    const auto corpus = good_corpus();

    const auto text = mirakana::serialize_asset_import_regression_corpus_v1(corpus);
    const auto parsed = mirakana::deserialize_asset_import_regression_corpus_v1(text);
    const auto text_again = mirakana::serialize_asset_import_regression_corpus_v1(parsed);

    MK_REQUIRE(text == text_again);
    MK_REQUIRE(text.contains("format=GameEngine.AssetImportRegressionCorpus.v1\n"));
    MK_REQUIRE(text.contains("asset.count=1\n"));
    MK_REQUIRE(text.contains("asset.0.mesh.unit_scale=0.01\n"));
    MK_REQUIRE(parsed.assets.size() == 1U);
    MK_REQUIRE(parsed.assets[0].asset_id == "mesh.hero");
    MK_REQUIRE(parsed.assets[0].kind == mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh);
    MK_REQUIRE(parsed.assets[0].mesh_preset.up_axis == mirakana::AssetImportMeshUpAxis::z);
    MK_REQUIRE(parsed.assets[0].provenance.license_id == "LicenseRef-Proprietary");
}

MK_TEST("asset import regression corpus rejects unsafe paths and legal blockers") {
    auto corpus = good_corpus();
    auto duplicate = good_corpus_asset();
    duplicate.source_path = "../outside.gltf";
    duplicate.license_policy = mirakana::AssetImportRegressionLicensePolicy::rejected;
    duplicate.provenance.external_engine_material = true;
    corpus.assets.push_back(duplicate);

    auto third_party = good_corpus_asset();
    third_party.asset_id = "mesh.third_party";
    third_party.asset_key.value = "meshes/third_party";
    third_party.expected_sha256.clear();
    third_party.provenance.origin = mirakana::AssetImportProvenanceOrigin::third_party;
    third_party.provenance.asset_key = third_party.asset_key;
    third_party.provenance.license_id = "CC-BY-NC-4.0";
    third_party.provenance.notice_complete = false;
    corpus.assets.push_back(third_party);
    corpus.row_budget = 2U;

    const auto diagnostics = mirakana::validate_asset_import_regression_corpus_v1(corpus);

    MK_REQUIRE(contains(diagnostics, "asset.1.duplicate_asset_id"));
    MK_REQUIRE(contains(diagnostics, "asset.1.unsafe_source_path"));
    MK_REQUIRE(contains(diagnostics, "asset.1.rejected_license"));
    MK_REQUIRE(contains(diagnostics, "asset.1.external_engine_material"));
    MK_REQUIRE(contains(diagnostics, "asset.2.third_party_missing_expected_sha256"));
    MK_REQUIRE(contains(diagnostics, "asset.2.rejected_license"));
    MK_REQUIRE(contains(diagnostics, "corpus.row_budget_exceeded"));
}

MK_TEST("asset import regression corpus deserialize rejects duplicate keys") {
    const auto corpus = good_corpus();
    auto text = mirakana::serialize_asset_import_regression_corpus_v1(corpus);
    text += "asset.0.asset_id=duplicate\n";

    bool threw = false;
    try {
        (void)mirakana::deserialize_asset_import_regression_corpus_v1(text);
    } catch (const std::invalid_argument&) {
        threw = true;
    }

    MK_REQUIRE(threw);
}

MK_TEST("asset import regression report serializes deterministic failure rows") {
    const mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-001",
        .rows =
            {
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "mesh.hero",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                    .asset = mirakana::AssetId{42U},
                    .source_path = "sources/gltf/hero.gltf",
                    .source_sha256 = "sha256:hero-source",
                    .preset_sha256 = "sha256:hero-preset",
                    .importer_id = "reviewed.gltf-mesh",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "parser",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::parser_error,
                    .message = "failed to parse glTF",
                    .deterministic_output_hash = "",
                    .succeeded = false,
                    .ready_for_commercial_evidence = false,
                },
            },
        .asset_count = 1U,
        .succeeded_count = 0U,
        .failed_count = 1U,
        .legal_blocked_count = 0U,
        .nondeterministic_count = 0U,
        .ready = false,
    };

    const auto text = mirakana::serialize_asset_import_regression_report_v1(report);
    const auto parsed = mirakana::deserialize_asset_import_regression_report_v1(text);

    MK_REQUIRE(text == mirakana::serialize_asset_import_regression_report_v1(parsed));
    MK_REQUIRE(text.contains("format=GameEngine.AssetImportRegressionReport.v1\n"));
    MK_REQUIRE(text.contains("row.0.code=parser_error\n"));
    MK_REQUIRE(parsed.rows.size() == 1U);
    MK_REQUIRE(parsed.rows[0].asset.value == 42U);
    MK_REQUIRE(parsed.rows[0].code == mirakana::AssetImportRegressionDiagnosticCode::parser_error);
    MK_REQUIRE(!parsed.ready);
}

MK_TEST("asset import regression report round trips every diagnostic code row") {
    const auto codes = all_diagnostic_codes();
    mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-all-diagnostic-codes",
        .asset_count = codes.size(),
        .succeeded_count = 1U,
        .failed_count = codes.size() - 1U,
        .legal_blocked_count = 3U,
        .nondeterministic_count = 1U,
        .ready = false,
    };
    report.rows.reserve(codes.size());

    for (std::size_t index = 0U; index < codes.size(); ++index) {
        const auto code = codes[index];
        const auto label = mirakana::asset_import_regression_diagnostic_code_label(code);
        report.rows.push_back(mirakana::AssetImportRegressionReportRowV1{
            .asset_id = "diagnostic." + std::to_string(index),
            .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
            .asset = mirakana::AssetId{100U + index},
            .source_path = "sources/gltf/diagnostic-" + std::to_string(index) + ".gltf",
            .source_sha256 = "sha256:source-" + std::to_string(index),
            .preset_sha256 = "sha256:preset-" + std::to_string(index),
            .importer_id = "mirakana.importer.test",
            .importer_version = "asset-import-regression-v1",
            .phase = code == mirakana::AssetImportRegressionDiagnosticCode::none ? "cook" : "diagnostic",
            .code = code,
            .message = "diagnostic code row: " + std::string{label},
            .deterministic_output_hash =
                code == mirakana::AssetImportRegressionDiagnosticCode::none ? "fnv64:1234" : "",
            .succeeded = code == mirakana::AssetImportRegressionDiagnosticCode::none,
            .ready_for_commercial_evidence = code == mirakana::AssetImportRegressionDiagnosticCode::none,
        });
    }

    const auto text = mirakana::serialize_asset_import_regression_report_v1(report);
    const auto parsed = mirakana::deserialize_asset_import_regression_report_v1(text);

    MK_REQUIRE(parsed.rows.size() == codes.size());
    MK_REQUIRE(text == mirakana::serialize_asset_import_regression_report_v1(parsed));

    std::vector<std::string> labels;
    labels.reserve(codes.size());
    for (std::size_t index = 0U; index < codes.size(); ++index) {
        const auto label = std::string{mirakana::asset_import_regression_diagnostic_code_label(codes[index])};
        labels.push_back(label);
        MK_REQUIRE(label != "invalid");
        MK_REQUIRE(text.contains("row." + std::to_string(index) + ".code=" + label + "\n"));
        MK_REQUIRE(parsed.rows[index].code == codes[index]);
        MK_REQUIRE(parsed.rows[index].message.contains(label));
    }
    std::ranges::sort(labels);
    MK_REQUIRE(std::ranges::adjacent_find(labels) == labels.end());
}

MK_TEST("asset import regression triage maps every diagnostic to a deterministic operator action") {
    const auto codes = all_diagnostic_codes();
    for (const auto code : codes) {
        const auto action = mirakana::recommended_action_for_asset_import_regression_code(code);
        const auto action_label = mirakana::asset_import_regression_recommended_action_label(action);
        MK_REQUIRE(action_label != "invalid");
        MK_REQUIRE(!action_label.empty());
        if (code == mirakana::AssetImportRegressionDiagnosticCode::none) {
            MK_REQUIRE(action == mirakana::AssetImportRegressionRecommendedAction::none);
        } else {
            MK_REQUIRE(action != mirakana::AssetImportRegressionRecommendedAction::none);
        }
    }
}

MK_TEST("asset import regression triage serializes safe deterministic operator rows") {
    const mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-triage-smoke",
        .rows =
            {
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "mesh.legal",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                    .asset = mirakana::AssetId::from_name("mesh.legal"),
                    .source_path = "sources/gltf/legal.gltf",
                    .source_sha256 = "sha256:legal",
                    .preset_sha256 = "sha256:preset-legal",
                    .importer_id = "mirakana.importer.gltf_mesh",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "legal",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::rejected_license,
                    .message = "license row rejected after source review",
                },
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "mesh.hash",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                    .asset = mirakana::AssetId::from_name("mesh.hash"),
                    .source_path = "sources/gltf/hash.gltf",
                    .source_sha256 = "sha256:hash",
                    .preset_sha256 = "sha256:preset-hash",
                    .importer_id = "mirakana.importer.gltf_mesh",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "source_hash",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::source_hash_mismatch,
                    .message = "source hash changed",
                },
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "mesh.axis",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                    .asset = mirakana::AssetId::from_name("mesh.axis"),
                    .source_path = "sources/gltf/axis.gltf",
                    .source_sha256 = "sha256:axis",
                    .preset_sha256 = "sha256:preset-axis",
                    .importer_id = "mirakana.importer.gltf_mesh",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "normalization",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::coordinate_normalization_failed,
                    .message = "coordinate normalization failed",
                },
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "texture.codec",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::png_texture,
                    .asset = mirakana::AssetId::from_name("texture.codec"),
                    .source_path = "sources/textures/codec.png",
                    .source_sha256 = "sha256:codec",
                    .preset_sha256 = "sha256:preset-codec",
                    .importer_id = "mirakana.importer.png_texture",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "decode",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::texture_decode_failed,
                    .message = "texture decode failed",
                },
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "mesh.nondeterministic",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                    .asset = mirakana::AssetId::from_name("mesh.nondeterministic"),
                    .source_path = "sources/gltf/nondeterministic.gltf",
                    .source_sha256 = "sha256:nondeterministic",
                    .preset_sha256 = "sha256:preset-nondeterministic",
                    .importer_id = "mirakana.importer.gltf_mesh",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "determinism",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::nondeterministic_output,
                    .message = "cooked output changed between two executions",
                },
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "material.ready",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::material_document,
                    .asset = mirakana::AssetId::from_name("material.ready"),
                    .source_path = "sources/materials/ready.material",
                    .source_sha256 = "sha256:ready",
                    .preset_sha256 = "sha256:preset-ready",
                    .importer_id = "mirakana.importer.material_document",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "cook",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::none,
                    .message = "asset import succeeded",
                    .deterministic_output_hash = "fnv64:1234",
                    .succeeded = true,
                    .ready_for_commercial_evidence = true,
                },
            },
        .asset_count = 6U,
        .succeeded_count = 1U,
        .failed_count = 5U,
        .legal_blocked_count = 1U,
        .nondeterministic_count = 1U,
    };

    const auto triage = mirakana::make_asset_import_regression_triage_v1(report);
    const auto text = mirakana::serialize_asset_import_regression_triage_v1(triage);
    const auto parsed = mirakana::deserialize_asset_import_regression_triage_v1(text);

    MK_REQUIRE(text == mirakana::serialize_asset_import_regression_triage_v1(parsed));
    MK_REQUIRE(text.contains("format=GameEngine.AssetImportRegressionTriage.v1\n"));
    MK_REQUIRE(text.contains("row.0.recommended_action=fix_notice_or_remove_asset\n"));
    MK_REQUIRE(text.contains("row.2.axis_unit_preview_required=true\n"));
    MK_REQUIRE(text.contains("row.3.preset_diff_required=true\n"));
    MK_REQUIRE(text.contains("row.4.nondeterministic=true\n"));
    MK_REQUIRE(!text.contains('\\'));
    MK_REQUIRE(!text.contains("C:"));
    MK_REQUIRE(!text.contains("Users/"));
    MK_REQUIRE(!text.contains("https://"));
    MK_REQUIRE(!text.contains("Unity"));
    MK_REQUIRE(!text.contains("Unreal"));
    MK_REQUIRE(!text.contains("Godot"));
    MK_REQUIRE(!text.contains("fastgltf::"));
    MK_REQUIRE(parsed.rows.size() == 6U);
    MK_REQUIRE(parsed.blocked_count == 3U);
    MK_REQUIRE(parsed.reimport_candidate_count == 2U);
    MK_REQUIRE(parsed.preset_diff_required_count == 1U);
    MK_REQUIRE(parsed.axis_unit_preview_required_count == 1U);
    MK_REQUIRE(parsed.rows[0].legal_blocked);
    MK_REQUIRE(parsed.rows[0].reimport_decision == mirakana::AssetImportRegressionReimportDecision::blocked);
    MK_REQUIRE(parsed.rows[2].axis_unit_preview_required);
    MK_REQUIRE(parsed.rows[2].reimport_decision == mirakana::AssetImportRegressionReimportDecision::dry_run_allowed);
    MK_REQUIRE(parsed.rows[4].nondeterministic);
    MK_REQUIRE(parsed.rows[4].reimport_decision == mirakana::AssetImportRegressionReimportDecision::blocked);
    MK_REQUIRE(parsed.rows[5].recommended_action == mirakana::AssetImportRegressionRecommendedAction::none);
    MK_REQUIRE(parsed.rows[5].reimport_decision == mirakana::AssetImportRegressionReimportDecision::not_needed);
    MK_REQUIRE(parsed.rows[0].source_excerpt_hash.starts_with("sha256:"));
    MK_REQUIRE(parsed.rows[0].repro_command.contains("tools/run-asset-import-regression-corpus.ps1"));
}

MK_TEST("asset import regression committed first-party corpus fixture validates expected coverage") {
    const auto fixture = find_repo_root() / "tests/fixtures/asset_import_regression/first_party_corpus.gecorpus";
    MK_REQUIRE(std::filesystem::exists(fixture));

    const auto parsed = mirakana::deserialize_asset_import_regression_corpus_v1(read_text_file(fixture));
    const auto diagnostics = mirakana::validate_asset_import_regression_corpus_v1(parsed);

    MK_REQUIRE(diagnostics.empty());
    MK_REQUIRE(parsed.root_path == "tests/fixtures/asset_import_regression");

    const std::vector<std::string> required_asset_ids{
        "gltf.animation.valid",
        "gltf.invalid.duplicate_animation_channel",
        "gltf.invalid.invalid_quaternion",
        "gltf.invalid.malformed_material_texture_index",
        "gltf.invalid.mismatched_accessor_counts",
        "gltf.invalid.missing_buffer",
        "gltf.invalid.unsafe_external_path",
        "gltf.invalid.unsupported_extension",
        "gltf.invalid.unsupported_interpolation",
        "gltf.invalid.unsupported_skin_morph_combination",
        "gltf.mesh.valid",
        "material.first_party",
    };

    std::vector<std::string> asset_ids;
    bool saw_mesh_fixture = false;
    for (const auto& asset : parsed.assets) {
        asset_ids.push_back(asset.asset_id);
        if (asset.asset_id == "gltf.mesh.valid") {
            saw_mesh_fixture = true;
            MK_REQUIRE(contains(asset.required_features, "gltf_valid_mesh"));
        }
        MK_REQUIRE(asset.license_policy == mirakana::AssetImportRegressionLicensePolicy::accepted_for_source_tree);
        MK_REQUIRE(asset.allow_checked_in_distribution);
        MK_REQUIRE(!asset.provenance.external_engine_material);
    }

    for (const auto& asset_id : required_asset_ids) {
        MK_REQUIRE(contains(asset_ids, asset_id));
    }
    MK_REQUIRE(saw_mesh_fixture);
}

MK_TEST("asset import regression runner rejects invalid manifests without touching source files") {
    mirakana::MemoryFileSystem fs;
    auto corpus = good_corpus();
    corpus.assets[0].source_path = "../outside.gltf";

    const auto report =
        mirakana::run_asset_import_regression_corpus(fs, corpus,
                                                     mirakana::AssetImportRegressionRunnerOptions{
                                                         .corpus_root = "tests/fixtures/asset_import_regression",
                                                         .output_root = "out/asset-import-regression/tests",
                                                     });

    MK_REQUIRE(report.corpus_id == "GameEngine.AssetImportRegressionCorpus.v1");
    MK_REQUIRE(report.asset_count == 1U);
    MK_REQUIRE(report.rows.size() == 1U);
    MK_REQUIRE(report.failed_count == 1U);
    MK_REQUIRE(!report.ready);
    MK_REQUIRE(report.rows[0].asset_id == "manifest");
    MK_REQUIRE(report.rows[0].phase == "manifest");
    MK_REQUIRE(report.rows[0].code == mirakana::AssetImportRegressionDiagnosticCode::invalid_manifest);
    MK_REQUIRE(report.rows[0].message.contains("asset.0.unsafe_source_path"));
}

MK_TEST("asset import regression runner maps corpus assets to deterministic success and failure rows") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("corpus/sources/gltf/hero.gltf",
                  "format=GameEngine.MeshSource.v2\nmesh.vertex_count=3\nmesh.index_count=3\n"
                  "mesh.has_normals=false\nmesh.has_uvs=false\nmesh.has_tangent_frame=false\n"
                  "mesh.vertex_data_hex=000102000000000000000000000000000000000000000000000000000000000000000000\n"
                  "mesh.index_data_hex=000000000100000002000000\n");
    fs.write_text("corpus/sources/materials/player.material",
                  "format=GameEngine.Material.v1\nmaterial.id=1\nmaterial.name=Player\nmaterial.shading=lit\n"
                  "material.surface=opaque\nmaterial.double_sided=false\nfactor.base_color=1,1,1,1\n"
                  "factor.emissive=0,0,0\nfactor.metallic=0\nfactor.roughness=1\ntexture.count=0\n");

    auto corpus = good_corpus();
    corpus.root_path = "corpus";
    corpus.assets[0].source_path = "sources/gltf/hero.gltf";
    corpus.assets[0].expected_sha256 = "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    corpus.assets[0].expected_output_kinds = {"GameEngine.CookedMesh.v2"};
    corpus.assets[0].required_features = {"gltf_valid_mesh"};
    corpus.assets[0].preset_metadata = {"mesh.unit_scale=0.01", "mesh.up_axis=z"};

    auto material = good_corpus_asset();
    material.asset_id = "material.player";
    material.kind = mirakana::AssetImportRegressionCorpusAssetKind::material_document;
    material.asset_key.value = "materials/player";
    material.source_path = "sources/materials/player.material";
    material.expected_sha256 = "sha256:bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    material.expected_output_kinds = {"GameEngine.Material.v1"};
    material.required_features = {"material_document"};
    material.provenance.asset_key = material.asset_key;
    corpus.assets.push_back(material);

    auto missing = good_corpus_asset();
    missing.asset_id = "texture.missing";
    missing.kind = mirakana::AssetImportRegressionCorpusAssetKind::png_texture;
    missing.asset_key.value = "textures/missing";
    missing.source_path = "sources/textures/missing.png";
    missing.expected_sha256 = "sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
    missing.expected_output_kinds = {"GameEngine.CookedTexture.v1"};
    missing.required_features = {"png_decode"};
    missing.provenance.asset_key = missing.asset_key;
    corpus.assets.push_back(missing);
    corpus.row_budget = 8U;
    std::ranges::sort(corpus.assets, {}, &mirakana::AssetImportRegressionCorpusAssetV1::asset_id);

    const auto report =
        mirakana::run_asset_import_regression_corpus(fs, corpus,
                                                     mirakana::AssetImportRegressionRunnerOptions{
                                                         .corpus_root = "corpus",
                                                         .output_root = "out/asset-import-regression/tests",
                                                         .write_cooked_outputs = true,
                                                         .compare_expected_hashes = false,
                                                     });
    const auto text = mirakana::serialize_asset_import_regression_report_v1(report);
    const auto parsed = mirakana::deserialize_asset_import_regression_report_v1(text);

    MK_REQUIRE(text == mirakana::serialize_asset_import_regression_report_v1(parsed));
    MK_REQUIRE(report.asset_count == 3U);
    MK_REQUIRE(report.rows.size() == 3U);
    MK_REQUIRE(report.succeeded_count == 2U);
    MK_REQUIRE(report.failed_count == 1U);
    MK_REQUIRE(!report.ready);
    MK_REQUIRE(report.rows[0].asset_id == "material.player");
    MK_REQUIRE(report.rows[0].phase == "cook");
    MK_REQUIRE(report.rows[0].code == mirakana::AssetImportRegressionDiagnosticCode::none);
    MK_REQUIRE(report.rows[0].succeeded);
    MK_REQUIRE(!report.rows[0].preset_sha256.empty());
    MK_REQUIRE(!report.rows[0].deterministic_output_hash.empty());
    MK_REQUIRE(report.rows[1].asset_id == "mesh.hero");
    MK_REQUIRE(report.rows[1].importer_id == "mirakana.importer.gltf_mesh");
    MK_REQUIRE(report.rows[2].asset_id == "texture.missing");
    MK_REQUIRE(report.rows[2].phase == "source");
    MK_REQUIRE(report.rows[2].code == mirakana::AssetImportRegressionDiagnosticCode::missing_source_file);
    MK_REQUIRE(!report.rows[2].succeeded);
    MK_REQUIRE(fs.exists("out/asset-import-regression/tests/material.player.cooked"));
    MK_REQUIRE(fs.exists("out/asset-import-regression/tests/mesh.hero.cooked"));
}

MK_TEST("asset import batch reimport planner requires dry run before apply") {
    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = material,
        .kind = mirakana::AssetImportActionKind::material,
        .source_path = "sources/materials/player.material",
        .output_path = "runtime/materials/player.material",
    });

    mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-batch",
        .rows =
            {
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "material.player",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::material_document,
                    .asset = material,
                    .source_path = "sources/materials/player.material",
                    .source_sha256 = "sha256:source",
                    .preset_sha256 = "sha256:preset",
                    .importer_id = "mirakana.importer.material_document",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "cook",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::none,
                    .message = "asset import succeeded",
                    .deterministic_output_hash = "",
                    .succeeded = true,
                    .ready_for_commercial_evidence = true,
                },
            },
        .asset_count = 1U,
        .succeeded_count = 1U,
        .ready = true,
    };

    const auto blocked = mirakana::plan_asset_import_batch_reimport(mirakana::AssetImportBatchReimportDesc{
        .run_id = "run-batch",
        .staging_root = "out/asset-import-regression/staging",
        .import_plan = import_plan,
        .report = report,
        .selected_asset_ids = {"material.player"},
        .apply_requested = true,
        .dry_run_acknowledged = false,
    });

    MK_REQUIRE(blocked.selected_count == 1U);
    MK_REQUIRE(blocked.ready_row_count == 1U);
    MK_REQUIRE(blocked.blocked_row_count == 0U);
    MK_REQUIRE(blocked.dry_run_required);
    MK_REQUIRE(!blocked.apply_allowed);
    MK_REQUIRE(!blocked.ready_for_apply);
    MK_REQUIRE(blocked.rows.size() == 1U);
    MK_REQUIRE(blocked.rows[0].status == mirakana::AssetImportBatchReimportRowStatus::ready);
    MK_REQUIRE(blocked.rows[0].staging_path == "out/asset-import-regression/staging/run-batch/material.player.cooked");

    const auto allowed = mirakana::plan_asset_import_batch_reimport(mirakana::AssetImportBatchReimportDesc{
        .run_id = "run-batch",
        .staging_root = "out/asset-import-regression/staging",
        .import_plan = import_plan,
        .report = report,
        .selected_asset_ids = {"material.player"},
        .apply_requested = true,
        .dry_run_acknowledged = true,
    });

    MK_REQUIRE(allowed.apply_allowed);
    MK_REQUIRE(allowed.ready_for_apply);
    MK_REQUIRE(allowed.mutates_project_outputs);
    MK_REQUIRE(allowed.all_or_nothing);

    auto drifted_import_plan = import_plan;
    drifted_import_plan.actions[0].source_path = "sources/materials/other.material";
    const auto drifted = mirakana::plan_asset_import_batch_reimport(mirakana::AssetImportBatchReimportDesc{
        .run_id = "run-batch",
        .staging_root = "out/asset-import-regression/staging",
        .import_plan = drifted_import_plan,
        .report = report,
        .selected_asset_ids = {"material.player"},
        .apply_requested = false,
        .dry_run_acknowledged = false,
    });

    MK_REQUIRE(!drifted.ready());
    MK_REQUIRE(drifted.blocked_row_count == 1U);
    MK_REQUIRE(drifted.rows[0].status == mirakana::AssetImportBatchReimportRowStatus::blocked);
    MK_REQUIRE(drifted.rows[0].diagnostic == "source path drift");
}

MK_TEST("asset import batch reimport dry run rows stay editor-facing and non mutating") {
    const auto mesh = mirakana::AssetId::from_name("meshes/hero");
    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = mesh,
        .kind = mirakana::AssetImportActionKind::mesh,
        .source_path = "source/meshes/hero.gltf",
        .output_path = "runtime/meshes/hero.mesh",
    });

    const mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-editor-batch-dry",
        .rows =
            {
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "mesh.hero",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                    .asset = mesh,
                    .source_path = "source/meshes/hero.gltf",
                    .source_sha256 = "sha256:source",
                    .preset_sha256 = "sha256:preset",
                    .importer_id = "mirakana.importer.gltf_mesh",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "cook",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::none,
                    .message = "asset import succeeded",
                    .deterministic_output_hash = "fnv64:0123456789abcdef",
                    .succeeded = true,
                    .ready_for_commercial_evidence = true,
                },
            },
        .asset_count = 1U,
        .succeeded_count = 1U,
        .ready = true,
    };

    const auto plan = mirakana::plan_asset_import_batch_reimport(mirakana::AssetImportBatchReimportDesc{
        .run_id = "run-editor-batch-dry",
        .staging_root = "out/asset-import-regression/staging",
        .import_plan = import_plan,
        .report = report,
        .selected_asset_ids = {"mesh.hero"},
        .apply_requested = false,
        .dry_run_acknowledged = false,
    });

    MK_REQUIRE(plan.ready());
    MK_REQUIRE(plan.selected_count == 1U);
    MK_REQUIRE(plan.ready_row_count == 1U);
    MK_REQUIRE(plan.blocked_row_count == 0U);
    MK_REQUIRE(!plan.apply_allowed);
    MK_REQUIRE(!plan.mutates_project_outputs);
    MK_REQUIRE(plan.rows.size() == 1U);
    MK_REQUIRE(plan.rows[0].status == mirakana::AssetImportBatchReimportRowStatus::ready);
    MK_REQUIRE(plan.rows[0].source_path == "source/meshes/hero.gltf");
    MK_REQUIRE(plan.rows[0].output_path == "runtime/meshes/hero.mesh");
    MK_REQUIRE(plan.rows[0].staging_path ==
               "out/asset-import-regression/staging/run-editor-batch-dry/mesh.hero.cooked");
    MK_REQUIRE(plan.rows[0].expected_output_hash == "fnv64:0123456789abcdef");
    MK_REQUIRE(plan.rows[0].output_validation_required);
}

MK_TEST("asset import batch reimport tool stages before applying project outputs") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("sources/materials/player.material",
                  "format=GameEngine.Material.v1\nmaterial.id=7\nmaterial.name=Player\nmaterial.shading=lit\n"
                  "material.surface=opaque\nmaterial.double_sided=false\nfactor.base_color=1,1,1,1\n"
                  "factor.emissive=0,0,0\nfactor.metallic=0\nfactor.roughness=1\ntexture.count=0\n");
    fs.write_text("runtime/materials/player.material", "old material");

    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = material,
        .kind = mirakana::AssetImportActionKind::material,
        .source_path = "sources/materials/player.material",
        .output_path = "runtime/materials/player.material",
    });

    mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-batch-apply",
        .rows =
            {
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "material.player",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::material_document,
                    .asset = material,
                    .source_path = "sources/materials/player.material",
                    .source_sha256 = "sha256:source",
                    .preset_sha256 = "sha256:preset",
                    .importer_id = "mirakana.importer.material_document",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "cook",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::none,
                    .message = "asset import succeeded",
                    .deterministic_output_hash = "",
                    .succeeded = true,
                    .ready_for_commercial_evidence = true,
                },
            },
        .asset_count = 1U,
        .succeeded_count = 1U,
        .ready = true,
    };

    const auto applied =
        mirakana::execute_asset_import_batch_reimport(fs, mirakana::AssetImportBatchReimportDesc{
                                                              .run_id = "run-batch-apply",
                                                              .staging_root = "out/asset-import-regression/staging",
                                                              .import_plan = import_plan,
                                                              .report = report,
                                                              .selected_asset_ids = {"material.player"},
                                                              .apply_requested = true,
                                                              .dry_run_acknowledged = true,
                                                          });

    MK_REQUIRE(applied.succeeded());
    MK_REQUIRE(applied.staged);
    MK_REQUIRE(applied.applied);
    MK_REQUIRE(applied.project_outputs_mutated);
    MK_REQUIRE(fs.exists("out/asset-import-regression/staging/run-batch-apply/material.player.cooked"));
    MK_REQUIRE(fs.read_text("runtime/materials/player.material").contains("material.name=Player"));
}

MK_TEST("asset import batch reimport tool keeps project outputs unchanged on validation failure") {
    mirakana::MemoryFileSystem fs;
    fs.write_text("sources/materials/player.material",
                  "format=GameEngine.Material.v1\nmaterial.id=7\nmaterial.name=Player\nmaterial.shading=lit\n"
                  "material.surface=opaque\nmaterial.double_sided=false\nfactor.base_color=1,1,1,1\n"
                  "factor.emissive=0,0,0\nfactor.metallic=0\nfactor.roughness=1\ntexture.count=0\n");
    fs.write_text("runtime/materials/player.material", "old material");

    const auto material = mirakana::AssetId::from_name("materials/player");
    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = material,
        .kind = mirakana::AssetImportActionKind::material,
        .source_path = "sources/materials/player.material",
        .output_path = "runtime/materials/player.material",
    });

    mirakana::AssetImportRegressionReportV1 report{
        .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
        .run_id = "run-batch-failed",
        .rows =
            {
                mirakana::AssetImportRegressionReportRowV1{
                    .asset_id = "material.player",
                    .kind = mirakana::AssetImportRegressionCorpusAssetKind::material_document,
                    .asset = material,
                    .source_path = "sources/materials/player.material",
                    .source_sha256 = "sha256:source",
                    .preset_sha256 = "sha256:preset",
                    .importer_id = "mirakana.importer.material_document",
                    .importer_version = "asset-import-regression-v1",
                    .phase = "cook",
                    .code = mirakana::AssetImportRegressionDiagnosticCode::none,
                    .message = "asset import succeeded",
                    .deterministic_output_hash = "fnv64:0000000000000000",
                    .succeeded = true,
                    .ready_for_commercial_evidence = true,
                },
            },
        .asset_count = 1U,
        .succeeded_count = 1U,
        .ready = true,
    };

    const auto failed =
        mirakana::execute_asset_import_batch_reimport(fs, mirakana::AssetImportBatchReimportDesc{
                                                              .run_id = "run-batch-failed",
                                                              .staging_root = "out/asset-import-regression/staging",
                                                              .import_plan = import_plan,
                                                              .report = report,
                                                              .selected_asset_ids = {"material.player"},
                                                              .apply_requested = true,
                                                              .dry_run_acknowledged = true,
                                                          });

    MK_REQUIRE(!failed.succeeded());
    MK_REQUIRE(failed.staged);
    MK_REQUIRE(!failed.applied);
    MK_REQUIRE(!failed.project_outputs_mutated);
    MK_REQUIRE(fs.exists("out/asset-import-regression/staging/run-batch-failed/material.player.cooked"));
    MK_REQUIRE(fs.read_text("runtime/materials/player.material") == "old material");
    MK_REQUIRE(!failed.diagnostics.empty());
}

MK_TEST("asset import preset diff marks coordinate convention changes as cooked output impacts") {
    const auto mesh = mirakana::AssetId::from_name("meshes/hero");
    const auto morph = mirakana::AssetId::from_name("morphs/hero");
    const auto translation = mirakana::AssetId::from_name("animations/hero_translation");
    const auto rotation = mirakana::AssetId::from_name("animations/hero_rotation");

    mirakana::AssetImportPlan import_plan;
    import_plan.actions = {
        mirakana::AssetImportAction{
            .id = mesh,
            .kind = mirakana::AssetImportActionKind::mesh,
            .source_path = "sources/gltf/hero.gltf",
            .output_path = "runtime/meshes/hero.mesh",
        },
        mirakana::AssetImportAction{
            .id = morph,
            .kind = mirakana::AssetImportActionKind::morph_mesh_cpu,
            .source_path = "sources/gltf/hero.gltf",
            .output_path = "runtime/morphs/hero.morph",
        },
        mirakana::AssetImportAction{
            .id = translation,
            .kind = mirakana::AssetImportActionKind::animation_float_clip,
            .source_path = "sources/gltf/hero.gltf",
            .output_path = "runtime/animations/hero_translation.animf",
        },
        mirakana::AssetImportAction{
            .id = rotation,
            .kind = mirakana::AssetImportActionKind::animation_quaternion_clip,
            .source_path = "sources/gltf/hero.gltf",
            .output_path = "runtime/animations/hero_rotation.animq",
        },
    };

    mirakana::AssetImportPresetsDocumentV1 before;
    mirakana::AssetImportPresetsDocumentV1 after = before;
    after.defaults.mesh.unit_scale = 0.01F;
    after.defaults.mesh.up_axis = mirakana::AssetImportMeshUpAxis::z;

    const auto diff = mirakana::diff_asset_import_presets(mirakana::AssetImportPresetDiffDesc{
        .import_plan = import_plan,
        .assets =
            {
                mirakana::AssetImportPresetDiffAssetV1{
                    .asset_id = "mesh.hero",
                    .asset_key = mirakana::AssetKeyV2{.value = "meshes/hero"},
                    .asset = mesh,
                    .corpus_kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                },
                mirakana::AssetImportPresetDiffAssetV1{
                    .asset_id = "morph.hero",
                    .asset_key = mirakana::AssetKeyV2{.value = "morphs/hero"},
                    .asset = morph,
                    .corpus_kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_mesh,
                },
                mirakana::AssetImportPresetDiffAssetV1{
                    .asset_id = "animation.hero_translation",
                    .asset_key = mirakana::AssetKeyV2{.value = "animations/hero_translation"},
                    .asset = translation,
                    .corpus_kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_animation,
                },
                mirakana::AssetImportPresetDiffAssetV1{
                    .asset_id = "animation.hero_rotation",
                    .asset_key = mirakana::AssetKeyV2{.value = "animations/hero_rotation"},
                    .asset = rotation,
                    .corpus_kind = mirakana::AssetImportRegressionCorpusAssetKind::gltf_animation,
                },
            },
        .before = before,
        .after = after,
    });

    MK_REQUIRE(diff.ready());
    MK_REQUIRE(!diff.executes_importers);
    MK_REQUIRE(diff.affected_count == 4U);
    MK_REQUIRE(diff.cooked_output_change_count == 4U);
    MK_REQUIRE(diff.package_output_change_count == 4U);
    MK_REQUIRE(diff.review_blocked_count == 0U);
    MK_REQUIRE(diff.rows.size() == 4U);

    for (const auto& row : diff.rows) {
        MK_REQUIRE(row.status == mirakana::AssetImportPresetDiffRowStatus::changed);
        MK_REQUIRE(row.cooked_output_changes);
        MK_REQUIRE(row.package_output_changes);
        MK_REQUIRE(!row.source_review_required);
        MK_REQUIRE(!row.review_blocked);
        MK_REQUIRE(contains_preset_diff_field(row.field_changes, "mesh.unit_scale"));
        MK_REQUIRE(contains_preset_diff_field(row.field_changes, "mesh.up_axis"));
    }
}

MK_TEST("asset import preset diff maps texture impacts and blocks unsupported preset combinations") {
    const auto texture = mirakana::AssetId::from_name("textures/player_albedo");
    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "sources/textures/player_albedo.png",
        .output_path = "runtime/textures/player_albedo.tex",
    });

    mirakana::AssetImportPresetsDocumentV1 before;
    mirakana::AssetImportPresetsDocumentV1 after = before;
    after.defaults.texture.color_space = mirakana::AssetImportTextureColorSpace::linear;
    after.defaults.texture.mipmap_policy = mirakana::AssetImportTextureMipmapPolicy::generate_offline;
    after.defaults.texture.alpha_policy = mirakana::AssetImportTextureAlphaPolicy::premultiplied;
    after.defaults.texture.compression_intent = mirakana::AssetImportTextureCompressionIntent::basis_reviewed;

    const auto diff = mirakana::diff_asset_import_presets(mirakana::AssetImportPresetDiffDesc{
        .import_plan = import_plan,
        .assets =
            {
                mirakana::AssetImportPresetDiffAssetV1{
                    .asset_id = "texture.player_albedo",
                    .asset_key = mirakana::AssetKeyV2{.value = "textures/player_albedo"},
                    .asset = texture,
                    .corpus_kind = mirakana::AssetImportRegressionCorpusAssetKind::png_texture,
                },
            },
        .before = before,
        .after = after,
    });

    MK_REQUIRE(!diff.ready());
    MK_REQUIRE(!diff.executes_importers);
    MK_REQUIRE(diff.affected_count == 1U);
    MK_REQUIRE(diff.source_review_count == 1U);
    MK_REQUIRE(diff.cooked_output_change_count == 1U);
    MK_REQUIRE(diff.package_output_change_count == 1U);
    MK_REQUIRE(diff.review_blocked_count == 1U);
    MK_REQUIRE(diff.rows.size() == 1U);
    MK_REQUIRE(diff.rows[0].status == mirakana::AssetImportPresetDiffRowStatus::review_blocked);
    MK_REQUIRE(diff.rows[0].source_review_required);
    MK_REQUIRE(diff.rows[0].cooked_output_changes);
    MK_REQUIRE(diff.rows[0].package_output_changes);
    MK_REQUIRE(diff.rows[0].review_blocked);
    MK_REQUIRE(contains_preset_diff_field(diff.rows[0].field_changes, "texture.color_space"));
    MK_REQUIRE(contains_preset_diff_field(diff.rows[0].field_changes, "texture.mipmap_policy"));
    MK_REQUIRE(contains_preset_diff_field(diff.rows[0].field_changes, "texture.alpha_policy"));
    MK_REQUIRE(contains_preset_diff_field(diff.rows[0].field_changes, "texture.compression_intent"));
    MK_REQUIRE(contains(diff.rows[0].diagnostics,
                        "asset.texture.basis_reviewed_premultiplied_alpha_requires_transcode_review"));
}

MK_TEST("asset import preset diff blocks affected rows missing latest regression evidence") {
    const auto texture = mirakana::AssetId::from_name("textures/missing_report");
    mirakana::AssetImportPlan import_plan;
    import_plan.actions.push_back(mirakana::AssetImportAction{
        .id = texture,
        .kind = mirakana::AssetImportActionKind::texture,
        .source_path = "sources/textures/missing_report.png",
        .output_path = "runtime/textures/missing_report.tex",
    });

    mirakana::AssetImportPresetsDocumentV1 before;
    mirakana::AssetImportPresetsDocumentV1 after = before;
    after.defaults.texture.mipmap_policy = mirakana::AssetImportTextureMipmapPolicy::generate_offline;

    const auto diff = mirakana::diff_asset_import_presets(mirakana::AssetImportPresetDiffDesc{
        .import_plan = import_plan,
        .assets =
            {
                mirakana::AssetImportPresetDiffAssetV1{
                    .asset_id = "texture.missing_report",
                    .asset_key = mirakana::AssetKeyV2{.value = "textures/missing_report"},
                    .asset = texture,
                    .corpus_kind = mirakana::AssetImportRegressionCorpusAssetKind::png_texture,
                },
            },
        .before = before,
        .after = after,
        .latest_report =
            mirakana::AssetImportRegressionReportV1{
                .corpus_id = "GameEngine.AssetImportRegressionCorpus.v1",
                .run_id = "run-missing-report",
                .ready = true,
            },
    });

    MK_REQUIRE(!diff.ready());
    MK_REQUIRE(diff.affected_count == 1U);
    MK_REQUIRE(diff.review_blocked_count == 1U);
    MK_REQUIRE(diff.rows.size() == 1U);
    MK_REQUIRE(diff.rows[0].status == mirakana::AssetImportPresetDiffRowStatus::review_blocked);
    MK_REQUIRE(diff.rows[0].cooked_output_changes);
    MK_REQUIRE(diff.rows[0].package_output_changes);
    MK_REQUIRE(contains(diff.rows[0].diagnostics, "latest_report.missing_asset_row"));
}

MK_TEST("asset axis unit preview uses import coordinate normalization for bounds basis and samples") {
    const auto
        preview =
            mirakana::build_asset_axis_unit_preview(
                mirakana::AssetAxisUnitPreviewDesc{
                    .asset_id = "mesh.hero",
                    .source_path = "sources/gltf/hero.gltf",
                    .action_kind = mirakana::AssetImportActionKind::mesh,
                    .mesh_preset =
                        mirakana::AssetImportMeshPresetV1{
                            .unit_scale = 0.01F,
                            .up_axis = mirakana::AssetImportMeshUpAxis::z,
                            .triangulate = true,
                            .generate_normals = true,
                            .generate_tangents = false,
                            .material_extraction = mirakana::AssetImportMeshMaterialExtraction::source_references,
                        },
                    .vertex_samples =
                        {
                            mirakana::AssetAxisUnitPreviewSample{
                                .label = "v0",
                                .position = mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 3.0F},
                                .direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 1.0F},
                                .has_direction = true,
                            },
                            mirakana::AssetAxisUnitPreviewSample{
                                .label = "v1",
                                .position = mirakana::Vec3{.x = -1.0F, .y = 0.0F, .z = 5.0F},
                            },
                        },
                    .joint_samples =
                        {
                            mirakana::AssetAxisUnitPreviewSample{
                                .label = "root",
                                .position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F},
                                .rotation = mirakana::Quat::identity(),
                                .has_rotation = true,
                            },
                        },
                });

    MK_REQUIRE(preview.ready());
    MK_REQUIRE(preview.changes_coordinates);
    MK_REQUIRE(near(preview.unit_scale, 0.01F));
    MK_REQUIRE(preview.up_axis == mirakana::AssetImportMeshUpAxis::z);
    MK_REQUIRE(preview.source_bounds.valid);
    MK_REQUIRE(preview.project_bounds.valid);
    MK_REQUIRE(near(preview.source_bounds.min, mirakana::Vec3{.x = -1.0F, .y = 0.0F, .z = 3.0F}));
    MK_REQUIRE(near(preview.source_bounds.max, mirakana::Vec3{.x = 1.0F, .y = 2.0F, .z = 5.0F}));
    MK_REQUIRE(near(preview.project_bounds.min, mirakana::Vec3{.x = -0.01F, .y = 0.03F, .z = -0.02F}));
    MK_REQUIRE(near(preview.project_bounds.max, mirakana::Vec3{.x = 0.01F, .y = 0.05F, .z = 0.0F}));
    MK_REQUIRE(preview.basis.size() == 3U);
    MK_REQUIRE(near(preview.basis[0].project_axis, mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F}));
    MK_REQUIRE(near(preview.basis[1].project_axis, mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = -1.0F}));
    MK_REQUIRE(near(preview.basis[2].project_axis, mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}));
    MK_REQUIRE(preview.rows.size() == 3U);
    MK_REQUIRE(preview.rows[0].kind == mirakana::AssetAxisUnitPreviewSampleKind::vertex);
    MK_REQUIRE(near(preview.rows[0].project_position, mirakana::Vec3{.x = 0.01F, .y = 0.03F, .z = -0.02F}));
    MK_REQUIRE(near(preview.rows[0].project_direction, mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F}));
    MK_REQUIRE(preview.rows[2].kind == mirakana::AssetAxisUnitPreviewSampleKind::joint);
    MK_REQUIRE(near(preview.rows[2].project_position, mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = -0.01F}));
}

MK_TEST("asset axis unit preview fails closed for unsupported source action") {
    const auto preview = mirakana::build_asset_axis_unit_preview(mirakana::AssetAxisUnitPreviewDesc{
        .asset_id = "material.player",
        .source_path = "sources/materials/player.material",
        .action_kind = mirakana::AssetImportActionKind::material,
        .vertex_samples =
            {
                mirakana::AssetAxisUnitPreviewSample{
                    .label = "ignored",
                    .position = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                },
            },
    });

    MK_REQUIRE(!preview.ready());
    MK_REQUIRE(contains(preview.diagnostics, "source.unsupported_for_axis_unit_preview"));
    MK_REQUIRE(preview.rows.empty());
}

int main() {
    return mirakana::test::run_all();
}
