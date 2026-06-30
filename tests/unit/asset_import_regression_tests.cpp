// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/assets/asset_import_regression_corpus.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/tools/asset_import_regression_runner.hpp"

#include <algorithm>
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

int main() {
    return mirakana::test::run_all();
}
