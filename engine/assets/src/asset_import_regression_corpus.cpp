// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_regression_corpus.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view corpus_format_v1 = "GameEngine.AssetImportRegressionCorpus.v1";
constexpr std::string_view report_format_v1 = "GameEngine.AssetImportRegressionReport.v1";

struct CorpusAssetTextRow {
    bool has_asset_id{false};
    bool has_kind{false};
    bool has_asset_key{false};
    bool has_source_path{false};
    bool has_expected_sha256{false};
    bool has_expected_output_count{false};
    bool has_required_feature_count{false};
    bool has_preset_metadata_count{false};
    bool has_mesh_unit_scale{false};
    bool has_mesh_up_axis{false};
    bool has_mesh_triangulate{false};
    bool has_mesh_generate_normals{false};
    bool has_mesh_generate_tangents{false};
    bool has_mesh_material_extraction{false};
    bool has_license_policy{false};
    bool has_allow_external_resources{false};
    bool has_allow_checked_in_distribution{false};
    bool has_provenance_asset_key{false};
    bool has_provenance_origin{false};
    bool has_provenance_source_url{false};
    bool has_provenance_retrieved_date{false};
    bool has_provenance_version_or_commit{false};
    bool has_provenance_copyright_holder{false};
    bool has_provenance_license_id{false};
    bool has_provenance_modification_status{false};
    bool has_provenance_distribution_target{false};
    bool has_provenance_notice_id{false};
    bool has_provenance_notice_complete{false};
    bool has_provenance_external_engine_material{false};
    std::size_t expected_output_count{0U};
    std::size_t required_feature_count{0U};
    std::size_t preset_metadata_count{0U};
    AssetImportRegressionCorpusAssetV1 asset;
};

struct ReportTextRow {
    bool has_asset_id{false};
    bool has_kind{false};
    bool has_asset{false};
    bool has_source_path{false};
    bool has_source_sha256{false};
    bool has_preset_sha256{false};
    bool has_importer_id{false};
    bool has_importer_version{false};
    bool has_phase{false};
    bool has_code{false};
    bool has_message{false};
    bool has_deterministic_output_hash{false};
    bool has_succeeded{false};
    bool has_ready_for_commercial_evidence{false};
    AssetImportRegressionReportRowV1 row;
};

[[nodiscard]] bool is_ascii_control(char character) noexcept {
    const auto value = static_cast<unsigned char>(character);
    return value < 0x20U || value == 0x7FU;
}

[[nodiscard]] bool contains_ascii_whitespace(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
}

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0U;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool clean_text(std::string_view value) noexcept {
    return value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_id_token(std::string_view value) noexcept {
    if (value.empty() || !clean_text(value) || contains_ascii_whitespace(value) || contains_parent_segment(value)) {
        return false;
    }
    return std::ranges::all_of(value, [](char character) {
        return std::isalnum(static_cast<unsigned char>(character)) != 0 || character == '_' || character == '-' ||
               character == '.';
    });
}

[[nodiscard]] bool safe_relative_path(std::string_view value) noexcept {
    return !value.empty() && clean_text(value) && value.front() != '/' && value.find('\\') == std::string_view::npos &&
           value.find(':') == std::string_view::npos && !contains_parent_segment(value) &&
           !std::ranges::any_of(value, is_ascii_control);
}

[[nodiscard]] bool valid_vector_value(std::string_view value) noexcept {
    return !value.empty() && clean_text(value) && !contains_ascii_whitespace(value) && !contains_parent_segment(value);
}

[[nodiscard]] bool valid_sha256_label(std::string_view value) noexcept {
    return value.starts_with("sha256:") && value.size() > std::string_view{"sha256:"}.size() && clean_text(value) &&
           !contains_ascii_whitespace(value);
}

[[nodiscard]] bool valid_asset_kind(AssetImportRegressionCorpusAssetKind value) noexcept {
    switch (value) {
    case AssetImportRegressionCorpusAssetKind::gltf_scene:
    case AssetImportRegressionCorpusAssetKind::gltf_mesh:
    case AssetImportRegressionCorpusAssetKind::gltf_animation:
    case AssetImportRegressionCorpusAssetKind::png_texture:
    case AssetImportRegressionCorpusAssetKind::openexr_texture:
    case AssetImportRegressionCorpusAssetKind::ktx2_basis_texture:
    case AssetImportRegressionCorpusAssetKind::material_document:
    case AssetImportRegressionCorpusAssetKind::audio_source:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_license_policy(AssetImportRegressionLicensePolicy value) noexcept {
    switch (value) {
    case AssetImportRegressionLicensePolicy::accepted_for_source_tree:
    case AssetImportRegressionLicensePolicy::accepted_for_host_corpus_only:
    case AssetImportRegressionLicensePolicy::rejected:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_diagnostic_code(AssetImportRegressionDiagnosticCode value) noexcept {
    switch (value) {
    case AssetImportRegressionDiagnosticCode::none:
    case AssetImportRegressionDiagnosticCode::invalid_manifest:
    case AssetImportRegressionDiagnosticCode::duplicate_asset_id:
    case AssetImportRegressionDiagnosticCode::unsafe_source_path:
    case AssetImportRegressionDiagnosticCode::missing_source_file:
    case AssetImportRegressionDiagnosticCode::source_hash_mismatch:
    case AssetImportRegressionDiagnosticCode::missing_license_provenance:
    case AssetImportRegressionDiagnosticCode::rejected_license:
    case AssetImportRegressionDiagnosticCode::external_engine_material:
    case AssetImportRegressionDiagnosticCode::unsupported_format:
    case AssetImportRegressionDiagnosticCode::parser_error:
    case AssetImportRegressionDiagnosticCode::validator_error:
    case AssetImportRegressionDiagnosticCode::missing_external_resource:
    case AssetImportRegressionDiagnosticCode::unsafe_external_resource_path:
    case AssetImportRegressionDiagnosticCode::unsupported_extension:
    case AssetImportRegressionDiagnosticCode::unsupported_animation_channel:
    case AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination:
    case AssetImportRegressionDiagnosticCode::coordinate_normalization_failed:
    case AssetImportRegressionDiagnosticCode::material_extraction_failed:
    case AssetImportRegressionDiagnosticCode::texture_decode_failed:
    case AssetImportRegressionDiagnosticCode::texture_transcode_failed:
    case AssetImportRegressionDiagnosticCode::cooked_output_mismatch:
    case AssetImportRegressionDiagnosticCode::nondeterministic_output:
    case AssetImportRegressionDiagnosticCode::row_budget_exceeded:
        return true;
    }
    return false;
}

void push(std::vector<std::string>& diagnostics, std::string_view prefix, std::string_view code) {
    diagnostics.push_back(std::string{prefix} + "." + std::string{code});
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view message) {
    if (value.empty()) {
        throw std::invalid_argument(std::string{message});
    }
    std::uint64_t parsed = 0U;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(std::string{message});
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
            throw std::invalid_argument(std::string{message});
        }
        parsed = (parsed * 10U) + digit;
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_size(std::string_view value, std::string_view message) {
    const auto parsed = parse_u64(value, message);
    if (parsed > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument(std::string{message});
    }
    return static_cast<std::size_t>(parsed);
}

[[nodiscard]] float parse_float(std::string_view value) {
    if (value.empty() || !clean_text(value) || contains_ascii_whitespace(value)) {
        throw std::invalid_argument("asset import regression float value is invalid");
    }

    std::istringstream input{std::string{value}};
    input.imbue(std::locale::classic());
    float parsed = 0.0F;
    input >> parsed;
    if (input.fail() || !input.eof() || !std::isfinite(parsed)) {
        throw std::invalid_argument("asset import regression float value is invalid");
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument("asset import regression bool value is invalid");
}

[[nodiscard]] const char* bool_text(bool value) noexcept {
    return value ? "true" : "false";
}

[[nodiscard]] std::string float_text(float value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("asset import regression float value is invalid");
    }
    for (int precision = 1; precision <= std::numeric_limits<float>::max_digits10; ++precision) {
        std::ostringstream output;
        output.imbue(std::locale::classic());
        output << std::setprecision(precision) << value;
        const auto text = output.str();
        if (parse_float(text) == value) {
            return text;
        }
    }

    std::ostringstream fallback;
    fallback.imbue(std::locale::classic());
    fallback << std::setprecision(std::numeric_limits<float>::max_digits10) << value;
    return fallback.str();
}

[[nodiscard]] AssetImportRegressionCorpusAssetKind parse_asset_kind(std::string_view value) {
    if (value == "gltf_scene") {
        return AssetImportRegressionCorpusAssetKind::gltf_scene;
    }
    if (value == "gltf_mesh") {
        return AssetImportRegressionCorpusAssetKind::gltf_mesh;
    }
    if (value == "gltf_animation") {
        return AssetImportRegressionCorpusAssetKind::gltf_animation;
    }
    if (value == "png_texture") {
        return AssetImportRegressionCorpusAssetKind::png_texture;
    }
    if (value == "openexr_texture") {
        return AssetImportRegressionCorpusAssetKind::openexr_texture;
    }
    if (value == "ktx2_basis_texture") {
        return AssetImportRegressionCorpusAssetKind::ktx2_basis_texture;
    }
    if (value == "material_document") {
        return AssetImportRegressionCorpusAssetKind::material_document;
    }
    if (value == "audio_source") {
        return AssetImportRegressionCorpusAssetKind::audio_source;
    }
    throw std::invalid_argument("asset import regression asset kind is unsupported");
}

[[nodiscard]] AssetImportRegressionLicensePolicy parse_license_policy(std::string_view value) {
    if (value == "accepted_for_source_tree") {
        return AssetImportRegressionLicensePolicy::accepted_for_source_tree;
    }
    if (value == "accepted_for_host_corpus_only") {
        return AssetImportRegressionLicensePolicy::accepted_for_host_corpus_only;
    }
    if (value == "rejected") {
        return AssetImportRegressionLicensePolicy::rejected;
    }
    throw std::invalid_argument("asset import regression license policy is unsupported");
}

[[nodiscard]] AssetImportRegressionDiagnosticCode parse_diagnostic_code(std::string_view value) {
    for (const auto code : {
             AssetImportRegressionDiagnosticCode::none,
             AssetImportRegressionDiagnosticCode::invalid_manifest,
             AssetImportRegressionDiagnosticCode::duplicate_asset_id,
             AssetImportRegressionDiagnosticCode::unsafe_source_path,
             AssetImportRegressionDiagnosticCode::missing_source_file,
             AssetImportRegressionDiagnosticCode::source_hash_mismatch,
             AssetImportRegressionDiagnosticCode::missing_license_provenance,
             AssetImportRegressionDiagnosticCode::rejected_license,
             AssetImportRegressionDiagnosticCode::external_engine_material,
             AssetImportRegressionDiagnosticCode::unsupported_format,
             AssetImportRegressionDiagnosticCode::parser_error,
             AssetImportRegressionDiagnosticCode::validator_error,
             AssetImportRegressionDiagnosticCode::missing_external_resource,
             AssetImportRegressionDiagnosticCode::unsafe_external_resource_path,
             AssetImportRegressionDiagnosticCode::unsupported_extension,
             AssetImportRegressionDiagnosticCode::unsupported_animation_channel,
             AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination,
             AssetImportRegressionDiagnosticCode::coordinate_normalization_failed,
             AssetImportRegressionDiagnosticCode::material_extraction_failed,
             AssetImportRegressionDiagnosticCode::texture_decode_failed,
             AssetImportRegressionDiagnosticCode::texture_transcode_failed,
             AssetImportRegressionDiagnosticCode::cooked_output_mismatch,
             AssetImportRegressionDiagnosticCode::nondeterministic_output,
             AssetImportRegressionDiagnosticCode::row_budget_exceeded,
         }) {
        if (value == asset_import_regression_diagnostic_code_label(code)) {
            return code;
        }
    }
    throw std::invalid_argument("asset import regression diagnostic code is unsupported");
}

[[nodiscard]] AssetImportMeshUpAxis parse_mesh_up_axis(std::string_view value) {
    if (value == "y") {
        return AssetImportMeshUpAxis::y;
    }
    if (value == "z") {
        return AssetImportMeshUpAxis::z;
    }
    throw std::invalid_argument("asset import regression mesh up axis is unsupported");
}

[[nodiscard]] AssetImportMeshMaterialExtraction parse_mesh_material_extraction(std::string_view value) {
    if (value == "none") {
        return AssetImportMeshMaterialExtraction::none;
    }
    if (value == "source_references") {
        return AssetImportMeshMaterialExtraction::source_references;
    }
    throw std::invalid_argument("asset import regression mesh material extraction is unsupported");
}

[[nodiscard]] AssetImportProvenanceOrigin parse_origin(std::string_view value) {
    if (value == "first_party") {
        return AssetImportProvenanceOrigin::first_party;
    }
    if (value == "third_party") {
        return AssetImportProvenanceOrigin::third_party;
    }
    if (value == "generated_ai") {
        return AssetImportProvenanceOrigin::generated_ai;
    }
    throw std::invalid_argument("asset import regression provenance origin is unsupported");
}

void validate_mesh_preset(std::vector<std::string>& diagnostics, std::string_view prefix,
                          const AssetImportMeshPresetV1& preset) {
    if (!is_valid_asset_import_mesh_preset_v1(preset)) {
        push(diagnostics, prefix, "invalid_mesh_preset");
    }
}

void validate_vector(std::vector<std::string>& diagnostics, std::string_view prefix,
                     const std::vector<std::string>& values) {
    if (values.empty()) {
        push(diagnostics, prefix, "empty");
        return;
    }
    std::unordered_set<std::string> unique;
    for (std::size_t index = 0; index < values.size(); ++index) {
        const auto& value = values[index];
        const std::string item_prefix = std::string{prefix} + "." + std::to_string(index);
        if (!valid_vector_value(value)) {
            push(diagnostics, item_prefix, "invalid");
        } else if (!unique.insert(value).second) {
            push(diagnostics, item_prefix, "duplicate");
        }
    }
}

[[nodiscard]] bool text_row_complete(const CorpusAssetTextRow& row) noexcept {
    return row.has_asset_id && row.has_kind && row.has_asset_key && row.has_source_path && row.has_expected_sha256 &&
           row.has_expected_output_count && row.has_required_feature_count && row.has_preset_metadata_count &&
           row.has_mesh_unit_scale && row.has_mesh_up_axis && row.has_mesh_triangulate &&
           row.has_mesh_generate_normals && row.has_mesh_generate_tangents && row.has_mesh_material_extraction &&
           row.has_license_policy && row.has_allow_external_resources && row.has_allow_checked_in_distribution &&
           row.has_provenance_asset_key && row.has_provenance_origin && row.has_provenance_source_url &&
           row.has_provenance_retrieved_date && row.has_provenance_version_or_commit &&
           row.has_provenance_copyright_holder && row.has_provenance_license_id &&
           row.has_provenance_modification_status && row.has_provenance_distribution_target &&
           row.has_provenance_notice_id && row.has_provenance_notice_complete &&
           row.has_provenance_external_engine_material &&
           row.asset.expected_output_kinds.size() == row.expected_output_count &&
           row.asset.required_features.size() == row.required_feature_count &&
           row.asset.preset_metadata.size() == row.preset_metadata_count;
}

[[nodiscard]] bool text_row_complete(const ReportTextRow& row) noexcept {
    return row.has_asset_id && row.has_kind && row.has_asset && row.has_source_path && row.has_source_sha256 &&
           row.has_preset_sha256 && row.has_importer_id && row.has_importer_version && row.has_phase && row.has_code &&
           row.has_message && row.has_deterministic_output_hash && row.has_succeeded &&
           row.has_ready_for_commercial_evidence;
}

void parse_corpus_asset_field(CorpusAssetTextRow& row, std::string_view field, std::string_view value) {
    if (field == "asset_id") {
        row.has_asset_id = true;
        row.asset.asset_id = std::string{value};
    } else if (field == "kind") {
        row.has_kind = true;
        row.asset.kind = parse_asset_kind(value);
    } else if (field == "asset_key") {
        row.has_asset_key = true;
        row.asset.asset_key.value = std::string{value};
    } else if (field == "source_path") {
        row.has_source_path = true;
        row.asset.source_path = std::string{value};
    } else if (field == "expected_sha256") {
        row.has_expected_sha256 = true;
        row.asset.expected_sha256 = std::string{value};
    } else if (field == "expected_output_kind.count") {
        row.has_expected_output_count = true;
        row.expected_output_count = parse_size(value, "asset import regression expected output count is invalid");
        row.asset.expected_output_kinds.resize(row.expected_output_count);
    } else if (field.starts_with("expected_output_kind.")) {
        const auto index = parse_size(field.substr(std::string_view{"expected_output_kind."}.size()),
                                      "asset import regression expected output index is invalid");
        if (index >= row.asset.expected_output_kinds.size()) {
            throw std::invalid_argument("asset import regression expected output index is out of range");
        }
        row.asset.expected_output_kinds[index] = std::string{value};
    } else if (field == "required_feature.count") {
        row.has_required_feature_count = true;
        row.required_feature_count = parse_size(value, "asset import regression required feature count is invalid");
        row.asset.required_features.resize(row.required_feature_count);
    } else if (field.starts_with("required_feature.")) {
        const auto index = parse_size(field.substr(std::string_view{"required_feature."}.size()),
                                      "asset import regression required feature index is invalid");
        if (index >= row.asset.required_features.size()) {
            throw std::invalid_argument("asset import regression required feature index is out of range");
        }
        row.asset.required_features[index] = std::string{value};
    } else if (field == "preset_metadata.count") {
        row.has_preset_metadata_count = true;
        row.preset_metadata_count = parse_size(value, "asset import regression preset metadata count is invalid");
        row.asset.preset_metadata.resize(row.preset_metadata_count);
    } else if (field.starts_with("preset_metadata.")) {
        const auto index = parse_size(field.substr(std::string_view{"preset_metadata."}.size()),
                                      "asset import regression preset metadata index is invalid");
        if (index >= row.asset.preset_metadata.size()) {
            throw std::invalid_argument("asset import regression preset metadata index is out of range");
        }
        row.asset.preset_metadata[index] = std::string{value};
    } else if (field == "mesh.unit_scale") {
        row.has_mesh_unit_scale = true;
        row.asset.mesh_preset.unit_scale = parse_float(value);
    } else if (field == "mesh.up_axis") {
        row.has_mesh_up_axis = true;
        row.asset.mesh_preset.up_axis = parse_mesh_up_axis(value);
    } else if (field == "mesh.triangulate") {
        row.has_mesh_triangulate = true;
        row.asset.mesh_preset.triangulate = parse_bool(value);
    } else if (field == "mesh.generate_normals") {
        row.has_mesh_generate_normals = true;
        row.asset.mesh_preset.generate_normals = parse_bool(value);
    } else if (field == "mesh.generate_tangents") {
        row.has_mesh_generate_tangents = true;
        row.asset.mesh_preset.generate_tangents = parse_bool(value);
    } else if (field == "mesh.material_extraction") {
        row.has_mesh_material_extraction = true;
        row.asset.mesh_preset.material_extraction = parse_mesh_material_extraction(value);
    } else if (field == "license_policy") {
        row.has_license_policy = true;
        row.asset.license_policy = parse_license_policy(value);
    } else if (field == "allow_external_resources") {
        row.has_allow_external_resources = true;
        row.asset.allow_external_resources = parse_bool(value);
    } else if (field == "allow_checked_in_distribution") {
        row.has_allow_checked_in_distribution = true;
        row.asset.allow_checked_in_distribution = parse_bool(value);
    } else if (field == "provenance.asset_key") {
        row.has_provenance_asset_key = true;
        row.asset.provenance.asset_key.value = std::string{value};
    } else if (field == "provenance.origin") {
        row.has_provenance_origin = true;
        row.asset.provenance.origin = parse_origin(value);
    } else if (field == "provenance.source_url") {
        row.has_provenance_source_url = true;
        row.asset.provenance.source_url = std::string{value};
    } else if (field == "provenance.retrieved_date") {
        row.has_provenance_retrieved_date = true;
        row.asset.provenance.retrieved_date = std::string{value};
    } else if (field == "provenance.version_or_commit") {
        row.has_provenance_version_or_commit = true;
        row.asset.provenance.version_or_commit = std::string{value};
    } else if (field == "provenance.copyright_holder") {
        row.has_provenance_copyright_holder = true;
        row.asset.provenance.copyright_holder = std::string{value};
    } else if (field == "provenance.license_id") {
        row.has_provenance_license_id = true;
        row.asset.provenance.license_id = std::string{value};
    } else if (field == "provenance.modification_status") {
        row.has_provenance_modification_status = true;
        row.asset.provenance.modification_status = std::string{value};
    } else if (field == "provenance.distribution_target") {
        row.has_provenance_distribution_target = true;
        row.asset.provenance.distribution_target = std::string{value};
    } else if (field == "provenance.notice_id") {
        row.has_provenance_notice_id = true;
        row.asset.provenance.notice_id = std::string{value};
    } else if (field == "provenance.notice_complete") {
        row.has_provenance_notice_complete = true;
        row.asset.provenance.notice_complete = parse_bool(value);
    } else if (field == "provenance.external_engine_material") {
        row.has_provenance_external_engine_material = true;
        row.asset.provenance.external_engine_material = parse_bool(value);
    } else {
        throw std::invalid_argument("asset import regression corpus field is unsupported");
    }
}

void parse_report_row_field(ReportTextRow& row, std::string_view field, std::string_view value) {
    if (field == "asset_id") {
        row.has_asset_id = true;
        row.row.asset_id = std::string{value};
    } else if (field == "kind") {
        row.has_kind = true;
        row.row.kind = parse_asset_kind(value);
    } else if (field == "asset") {
        row.has_asset = true;
        row.row.asset = AssetId{parse_u64(value, "asset import regression report asset id is invalid")};
    } else if (field == "source_path") {
        row.has_source_path = true;
        row.row.source_path = std::string{value};
    } else if (field == "source_sha256") {
        row.has_source_sha256 = true;
        row.row.source_sha256 = std::string{value};
    } else if (field == "preset_sha256") {
        row.has_preset_sha256 = true;
        row.row.preset_sha256 = std::string{value};
    } else if (field == "importer_id") {
        row.has_importer_id = true;
        row.row.importer_id = std::string{value};
    } else if (field == "importer_version") {
        row.has_importer_version = true;
        row.row.importer_version = std::string{value};
    } else if (field == "phase") {
        row.has_phase = true;
        row.row.phase = std::string{value};
    } else if (field == "code") {
        row.has_code = true;
        row.row.code = parse_diagnostic_code(value);
    } else if (field == "message") {
        row.has_message = true;
        row.row.message = std::string{value};
    } else if (field == "deterministic_output_hash") {
        row.has_deterministic_output_hash = true;
        row.row.deterministic_output_hash = std::string{value};
    } else if (field == "succeeded") {
        row.has_succeeded = true;
        row.row.succeeded = parse_bool(value);
    } else if (field == "ready_for_commercial_evidence") {
        row.has_ready_for_commercial_evidence = true;
        row.row.ready_for_commercial_evidence = parse_bool(value);
    } else {
        throw std::invalid_argument("asset import regression report row field is unsupported");
    }
}

void write_vector(std::ostringstream& output, std::string_view prefix, const std::vector<std::string>& values) {
    output << prefix << ".count=" << values.size() << '\n';
    for (std::size_t index = 0; index < values.size(); ++index) {
        output << prefix << "." << index << "=" << values[index] << '\n';
    }
}

void write_mesh_preset(std::ostringstream& output, std::string_view prefix, const AssetImportMeshPresetV1& preset) {
    output << prefix << ".unit_scale=" << float_text(preset.unit_scale) << '\n';
    output << prefix << ".up_axis=" << asset_import_mesh_up_axis_label(preset.up_axis) << '\n';
    output << prefix << ".triangulate=" << bool_text(preset.triangulate) << '\n';
    output << prefix << ".generate_normals=" << bool_text(preset.generate_normals) << '\n';
    output << prefix << ".generate_tangents=" << bool_text(preset.generate_tangents) << '\n';
    output << prefix
           << ".material_extraction=" << asset_import_mesh_material_extraction_label(preset.material_extraction)
           << '\n';
}

void write_provenance(std::ostringstream& output, std::string_view prefix, const AssetImportProvenanceRowV1& row) {
    output << prefix << ".asset_key=" << row.asset_key.value << '\n';
    output << prefix << ".origin=" << asset_import_provenance_origin_label(row.origin) << '\n';
    output << prefix << ".source_url=" << row.source_url << '\n';
    output << prefix << ".retrieved_date=" << row.retrieved_date << '\n';
    output << prefix << ".version_or_commit=" << row.version_or_commit << '\n';
    output << prefix << ".copyright_holder=" << row.copyright_holder << '\n';
    output << prefix << ".license_id=" << row.license_id << '\n';
    output << prefix << ".modification_status=" << row.modification_status << '\n';
    output << prefix << ".distribution_target=" << row.distribution_target << '\n';
    output << prefix << ".notice_id=" << row.notice_id << '\n';
    output << prefix << ".notice_complete=" << bool_text(row.notice_complete) << '\n';
    output << prefix << ".external_engine_material=" << bool_text(row.external_engine_material) << '\n';
}

} // namespace

std::string_view asset_import_regression_asset_kind_label(AssetImportRegressionCorpusAssetKind value) noexcept {
    switch (value) {
    case AssetImportRegressionCorpusAssetKind::gltf_scene:
        return "gltf_scene";
    case AssetImportRegressionCorpusAssetKind::gltf_mesh:
        return "gltf_mesh";
    case AssetImportRegressionCorpusAssetKind::gltf_animation:
        return "gltf_animation";
    case AssetImportRegressionCorpusAssetKind::png_texture:
        return "png_texture";
    case AssetImportRegressionCorpusAssetKind::openexr_texture:
        return "openexr_texture";
    case AssetImportRegressionCorpusAssetKind::ktx2_basis_texture:
        return "ktx2_basis_texture";
    case AssetImportRegressionCorpusAssetKind::material_document:
        return "material_document";
    case AssetImportRegressionCorpusAssetKind::audio_source:
        return "audio_source";
    }
    return "invalid";
}

std::string_view asset_import_regression_license_policy_label(AssetImportRegressionLicensePolicy value) noexcept {
    switch (value) {
    case AssetImportRegressionLicensePolicy::accepted_for_source_tree:
        return "accepted_for_source_tree";
    case AssetImportRegressionLicensePolicy::accepted_for_host_corpus_only:
        return "accepted_for_host_corpus_only";
    case AssetImportRegressionLicensePolicy::rejected:
        return "rejected";
    }
    return "invalid";
}

std::string_view asset_import_regression_diagnostic_code_label(AssetImportRegressionDiagnosticCode value) noexcept {
    switch (value) {
    case AssetImportRegressionDiagnosticCode::none:
        return "none";
    case AssetImportRegressionDiagnosticCode::invalid_manifest:
        return "invalid_manifest";
    case AssetImportRegressionDiagnosticCode::duplicate_asset_id:
        return "duplicate_asset_id";
    case AssetImportRegressionDiagnosticCode::unsafe_source_path:
        return "unsafe_source_path";
    case AssetImportRegressionDiagnosticCode::missing_source_file:
        return "missing_source_file";
    case AssetImportRegressionDiagnosticCode::source_hash_mismatch:
        return "source_hash_mismatch";
    case AssetImportRegressionDiagnosticCode::missing_license_provenance:
        return "missing_license_provenance";
    case AssetImportRegressionDiagnosticCode::rejected_license:
        return "rejected_license";
    case AssetImportRegressionDiagnosticCode::external_engine_material:
        return "external_engine_material";
    case AssetImportRegressionDiagnosticCode::unsupported_format:
        return "unsupported_format";
    case AssetImportRegressionDiagnosticCode::parser_error:
        return "parser_error";
    case AssetImportRegressionDiagnosticCode::validator_error:
        return "validator_error";
    case AssetImportRegressionDiagnosticCode::missing_external_resource:
        return "missing_external_resource";
    case AssetImportRegressionDiagnosticCode::unsafe_external_resource_path:
        return "unsafe_external_resource_path";
    case AssetImportRegressionDiagnosticCode::unsupported_extension:
        return "unsupported_extension";
    case AssetImportRegressionDiagnosticCode::unsupported_animation_channel:
        return "unsupported_animation_channel";
    case AssetImportRegressionDiagnosticCode::unsupported_skin_or_morph_combination:
        return "unsupported_skin_or_morph_combination";
    case AssetImportRegressionDiagnosticCode::coordinate_normalization_failed:
        return "coordinate_normalization_failed";
    case AssetImportRegressionDiagnosticCode::material_extraction_failed:
        return "material_extraction_failed";
    case AssetImportRegressionDiagnosticCode::texture_decode_failed:
        return "texture_decode_failed";
    case AssetImportRegressionDiagnosticCode::texture_transcode_failed:
        return "texture_transcode_failed";
    case AssetImportRegressionDiagnosticCode::cooked_output_mismatch:
        return "cooked_output_mismatch";
    case AssetImportRegressionDiagnosticCode::nondeterministic_output:
        return "nondeterministic_output";
    case AssetImportRegressionDiagnosticCode::row_budget_exceeded:
        return "row_budget_exceeded";
    }
    return "invalid";
}

std::vector<std::string>
validate_asset_import_regression_corpus_v1(const AssetImportRegressionCorpusDocumentV1& document) {
    std::vector<std::string> diagnostics;
    if (document.corpus_id != corpus_format_v1) {
        diagnostics.push_back("corpus.invalid_id");
    }
    if (document.corpus_version != "1") {
        diagnostics.push_back("corpus.invalid_version");
    }
    if (!safe_relative_path(document.root_path)) {
        diagnostics.push_back("corpus.unsafe_root_path");
    }
    if (document.assets.size() > document.row_budget) {
        diagnostics.push_back("corpus.row_budget_exceeded");
    }

    std::unordered_set<std::string> asset_ids;
    std::string previous_asset_id;
    for (std::size_t index = 0; index < document.assets.size(); ++index) {
        const auto& asset = document.assets[index];
        const std::string prefix = "asset." + std::to_string(index);
        if (!valid_id_token(asset.asset_id)) {
            push(diagnostics, prefix, "invalid_asset_id");
        } else {
            if (!asset_ids.insert(asset.asset_id).second) {
                push(diagnostics, prefix, "duplicate_asset_id");
            }
            if (!previous_asset_id.empty() && previous_asset_id > asset.asset_id) {
                push(diagnostics, prefix, "asset_id_not_sorted");
            }
            previous_asset_id = asset.asset_id;
        }
        if (!valid_asset_kind(asset.kind)) {
            push(diagnostics, prefix, "invalid_kind");
        }
        if (!safe_relative_path(asset.asset_key.value)) {
            push(diagnostics, prefix, "invalid_asset_key");
        }
        if (!safe_relative_path(asset.source_path)) {
            push(diagnostics, prefix, "unsafe_source_path");
        }
        if (asset.expected_sha256.empty()) {
            if (asset.provenance.origin == AssetImportProvenanceOrigin::third_party) {
                push(diagnostics, prefix, "third_party_missing_expected_sha256");
            } else {
                push(diagnostics, prefix, "missing_expected_sha256");
            }
        } else if (!valid_sha256_label(asset.expected_sha256)) {
            push(diagnostics, prefix, "invalid_expected_sha256");
        }
        validate_vector(diagnostics, prefix + ".expected_output_kind", asset.expected_output_kinds);
        validate_vector(diagnostics, prefix + ".required_feature", asset.required_features);
        if (!asset.preset_metadata.empty()) {
            validate_vector(diagnostics, prefix + ".preset_metadata", asset.preset_metadata);
        }
        validate_mesh_preset(diagnostics, prefix, asset.mesh_preset);
        if (!valid_license_policy(asset.license_policy)) {
            push(diagnostics, prefix, "invalid_license_policy");
        }
        if (asset.license_policy == AssetImportRegressionLicensePolicy::rejected) {
            push(diagnostics, prefix, "rejected_license");
        }
        if (asset.provenance.asset_key.value != asset.asset_key.value) {
            push(diagnostics, prefix, "provenance_asset_key_mismatch");
        }
        if (asset.provenance.external_engine_material) {
            push(diagnostics, prefix, "external_engine_material");
        }
        if (asset.provenance.license_id.empty() || !asset.provenance.notice_complete) {
            push(diagnostics, prefix, "missing_license_provenance");
        }
        const AssetImportProvenanceDocumentV1 provenance_document{{asset.provenance}};
        if (!validate_asset_import_provenance_document(provenance_document).empty()) {
            push(diagnostics, prefix, "rejected_license");
        }
        if (asset.license_policy == AssetImportRegressionLicensePolicy::accepted_for_source_tree &&
            !asset.allow_checked_in_distribution) {
            push(diagnostics, prefix, "checked_in_distribution_not_allowed");
        }
    }

    return diagnostics;
}

std::string serialize_asset_import_regression_corpus_v1(const AssetImportRegressionCorpusDocumentV1& document) {
    const auto diagnostics = validate_asset_import_regression_corpus_v1(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset import regression corpus document is invalid");
    }

    std::ostringstream output;
    output << "format=" << corpus_format_v1 << '\n';
    output << "corpus.id=" << document.corpus_id << '\n';
    output << "corpus.version=" << document.corpus_version << '\n';
    output << "root_path=" << document.root_path << '\n';
    output << "row_budget=" << document.row_budget << '\n';
    output << "asset.count=" << document.assets.size() << '\n';
    for (std::size_t index = 0; index < document.assets.size(); ++index) {
        const auto& asset = document.assets[index];
        const std::string prefix = "asset." + std::to_string(index);
        output << prefix << ".asset_id=" << asset.asset_id << '\n';
        output << prefix << ".kind=" << asset_import_regression_asset_kind_label(asset.kind) << '\n';
        output << prefix << ".asset_key=" << asset.asset_key.value << '\n';
        output << prefix << ".source_path=" << asset.source_path << '\n';
        output << prefix << ".expected_sha256=" << asset.expected_sha256 << '\n';
        write_vector(output, prefix + ".expected_output_kind", asset.expected_output_kinds);
        write_vector(output, prefix + ".required_feature", asset.required_features);
        write_vector(output, prefix + ".preset_metadata", asset.preset_metadata);
        write_mesh_preset(output, prefix + ".mesh", asset.mesh_preset);
        output << prefix << ".license_policy=" << asset_import_regression_license_policy_label(asset.license_policy)
               << '\n';
        output << prefix << ".allow_external_resources=" << bool_text(asset.allow_external_resources) << '\n';
        output << prefix << ".allow_checked_in_distribution=" << bool_text(asset.allow_checked_in_distribution) << '\n';
        write_provenance(output, prefix + ".provenance", asset.provenance);
    }
    return output.str();
}

AssetImportRegressionCorpusDocumentV1 deserialize_asset_import_regression_corpus_v1(std::string_view text) {
    bool saw_format = false;
    bool saw_corpus_id = false;
    bool saw_corpus_version = false;
    bool saw_root_path = false;
    bool saw_row_budget = false;
    bool saw_asset_count = false;
    std::size_t asset_count = 0U;
    std::unordered_set<std::string> seen_keys;
    std::unordered_map<std::size_t, CorpusAssetTextRow> rows;
    AssetImportRegressionCorpusDocumentV1 document;

    std::size_t line_start = 0U;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("asset import regression corpus text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("asset import regression corpus line is missing '='");
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string{key}).second) {
            throw std::invalid_argument("asset import regression corpus text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != corpus_format_v1) {
                throw std::invalid_argument("asset import regression corpus format is unsupported");
            }
        } else if (key == "corpus.id") {
            saw_corpus_id = true;
            document.corpus_id = std::string{value};
        } else if (key == "corpus.version") {
            saw_corpus_version = true;
            document.corpus_version = std::string{value};
        } else if (key == "root_path") {
            saw_root_path = true;
            document.root_path = std::string{value};
        } else if (key == "row_budget") {
            saw_row_budget = true;
            document.row_budget = parse_u64(value, "asset import regression corpus row budget is invalid");
        } else if (key == "asset.count") {
            saw_asset_count = true;
            asset_count = parse_size(value, "asset import regression corpus asset count is invalid");
        } else if (key.starts_with("asset.")) {
            const auto after_prefix = key.substr(std::string_view{"asset."}.size());
            const auto dot = after_prefix.find('.');
            if (dot == std::string_view::npos) {
                throw std::invalid_argument("asset import regression corpus asset key is malformed");
            }
            const auto index =
                parse_size(after_prefix.substr(0, dot), "asset import regression corpus asset index is invalid");
            parse_corpus_asset_field(rows[index], after_prefix.substr(dot + 1U), value);
        } else {
            throw std::invalid_argument("asset import regression corpus key is unsupported");
        }
    }

    if (!saw_format || !saw_corpus_id || !saw_corpus_version || !saw_root_path || !saw_row_budget || !saw_asset_count) {
        throw std::invalid_argument("asset import regression corpus header is incomplete");
    }
    if (rows.size() != asset_count) {
        throw std::invalid_argument("asset import regression corpus asset count does not match rows");
    }

    document.assets.reserve(asset_count);
    for (std::size_t index = 0; index < asset_count; ++index) {
        const auto row = rows.find(index);
        if (row == rows.end() || !text_row_complete(row->second)) {
            throw std::invalid_argument("asset import regression corpus asset row is incomplete");
        }
        document.assets.push_back(row->second.asset);
    }

    const auto diagnostics = validate_asset_import_regression_corpus_v1(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset import regression corpus document is invalid");
    }
    return document;
}

std::string serialize_asset_import_regression_report_v1(const AssetImportRegressionReportV1& report) {
    std::ostringstream output;
    output << "format=" << report_format_v1 << '\n';
    output << "corpus_id=" << report.corpus_id << '\n';
    output << "run_id=" << report.run_id << '\n';
    output << "asset_count=" << report.asset_count << '\n';
    output << "succeeded_count=" << report.succeeded_count << '\n';
    output << "failed_count=" << report.failed_count << '\n';
    output << "legal_blocked_count=" << report.legal_blocked_count << '\n';
    output << "nondeterministic_count=" << report.nondeterministic_count << '\n';
    output << "ready=" << bool_text(report.ready) << '\n';
    output << "row.count=" << report.rows.size() << '\n';
    for (std::size_t index = 0; index < report.rows.size(); ++index) {
        const auto& row = report.rows[index];
        const std::string prefix = "row." + std::to_string(index);
        output << prefix << ".asset_id=" << row.asset_id << '\n';
        output << prefix << ".kind=" << asset_import_regression_asset_kind_label(row.kind) << '\n';
        output << prefix << ".asset=" << row.asset.value << '\n';
        output << prefix << ".source_path=" << row.source_path << '\n';
        output << prefix << ".source_sha256=" << row.source_sha256 << '\n';
        output << prefix << ".preset_sha256=" << row.preset_sha256 << '\n';
        output << prefix << ".importer_id=" << row.importer_id << '\n';
        output << prefix << ".importer_version=" << row.importer_version << '\n';
        output << prefix << ".phase=" << row.phase << '\n';
        output << prefix << ".code=" << asset_import_regression_diagnostic_code_label(row.code) << '\n';
        output << prefix << ".message=" << row.message << '\n';
        output << prefix << ".deterministic_output_hash=" << row.deterministic_output_hash << '\n';
        output << prefix << ".succeeded=" << bool_text(row.succeeded) << '\n';
        output << prefix << ".ready_for_commercial_evidence=" << bool_text(row.ready_for_commercial_evidence) << '\n';
    }
    return output.str();
}

AssetImportRegressionReportV1 deserialize_asset_import_regression_report_v1(std::string_view text) {
    bool saw_format = false;
    bool saw_row_count = false;
    std::size_t row_count = 0U;
    std::unordered_set<std::string> seen_keys;
    std::unordered_map<std::size_t, ReportTextRow> rows;
    AssetImportRegressionReportV1 report;

    std::size_t line_start = 0U;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("asset import regression report text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("asset import regression report line is missing '='");
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string{key}).second) {
            throw std::invalid_argument("asset import regression report text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != report_format_v1) {
                throw std::invalid_argument("asset import regression report format is unsupported");
            }
        } else if (key == "corpus_id") {
            report.corpus_id = std::string{value};
        } else if (key == "run_id") {
            report.run_id = std::string{value};
        } else if (key == "asset_count") {
            report.asset_count = parse_size(value, "asset import regression report asset count is invalid");
        } else if (key == "succeeded_count") {
            report.succeeded_count = parse_size(value, "asset import regression report succeeded count is invalid");
        } else if (key == "failed_count") {
            report.failed_count = parse_size(value, "asset import regression report failed count is invalid");
        } else if (key == "legal_blocked_count") {
            report.legal_blocked_count =
                parse_size(value, "asset import regression report legal blocked count is invalid");
        } else if (key == "nondeterministic_count") {
            report.nondeterministic_count =
                parse_size(value, "asset import regression report nondeterministic count is invalid");
        } else if (key == "ready") {
            report.ready = parse_bool(value);
        } else if (key == "row.count") {
            saw_row_count = true;
            row_count = parse_size(value, "asset import regression report row count is invalid");
        } else if (key.starts_with("row.")) {
            const auto after_prefix = key.substr(std::string_view{"row."}.size());
            const auto dot = after_prefix.find('.');
            if (dot == std::string_view::npos) {
                throw std::invalid_argument("asset import regression report row key is malformed");
            }
            const auto index =
                parse_size(after_prefix.substr(0, dot), "asset import regression report row index is invalid");
            parse_report_row_field(rows[index], after_prefix.substr(dot + 1U), value);
        } else {
            throw std::invalid_argument("asset import regression report key is unsupported");
        }
    }

    if (!saw_format || !saw_row_count || rows.size() != row_count) {
        throw std::invalid_argument("asset import regression report is incomplete");
    }
    report.rows.reserve(row_count);
    for (std::size_t index = 0; index < row_count; ++index) {
        const auto row = rows.find(index);
        if (row == rows.end() || !text_row_complete(row->second)) {
            throw std::invalid_argument("asset import regression report row is incomplete");
        }
        if (!valid_asset_kind(row->second.row.kind) || !valid_diagnostic_code(row->second.row.code)) {
            throw std::invalid_argument("asset import regression report row is invalid");
        }
        report.rows.push_back(row->second.row);
    }
    return report;
}

} // namespace mirakana
