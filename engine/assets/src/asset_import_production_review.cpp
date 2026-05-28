// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_import_production_review.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr std::array<AssetImportProductionFeatureKind, 9U> required_feature_set = {
    AssetImportProductionFeatureKind::source_root_policy,
    AssetImportProductionFeatureKind::gltf_geometry,
    AssetImportProductionFeatureKind::gltf_animation,
    AssetImportProductionFeatureKind::ktx_texture,
    AssetImportProductionFeatureKind::source_image,
    AssetImportProductionFeatureKind::source_audio,
    AssetImportProductionFeatureKind::material_source,
    AssetImportProductionFeatureKind::shader_offline_compile_request,
    AssetImportProductionFeatureKind::package_cook_output,
};

constexpr std::uint64_t fnv_offset = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

[[nodiscard]] bool valid_feature(AssetImportProductionFeatureKind feature) noexcept {
    return std::ranges::find(required_feature_set, feature) != required_feature_set.end();
}

[[nodiscard]] bool feature_requires_validator(AssetImportProductionFeatureKind feature) noexcept {
    return feature == AssetImportProductionFeatureKind::gltf_geometry ||
           feature == AssetImportProductionFeatureKind::gltf_animation ||
           feature == AssetImportProductionFeatureKind::ktx_texture ||
           feature == AssetImportProductionFeatureKind::shader_offline_compile_request;
}

[[nodiscard]] bool feature_requires_dependency_legal_record(AssetImportProductionFeatureKind feature) noexcept {
    return feature == AssetImportProductionFeatureKind::gltf_geometry ||
           feature == AssetImportProductionFeatureKind::gltf_animation ||
           feature == AssetImportProductionFeatureKind::ktx_texture ||
           feature == AssetImportProductionFeatureKind::source_image ||
           feature == AssetImportProductionFeatureKind::source_audio ||
           feature == AssetImportProductionFeatureKind::shader_offline_compile_request;
}

[[nodiscard]] bool feature_requires_command_review(AssetImportProductionFeatureKind feature) noexcept {
    return feature == AssetImportProductionFeatureKind::shader_offline_compile_request;
}

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] char ascii_lower(char value) noexcept {
    if (value >= 'A' && value <= 'Z') {
        return static_cast<char>(value + ('a' - 'A'));
    }
    return value;
}

[[nodiscard]] bool contains_ascii_case_insensitive(std::string_view value, std::string_view token) noexcept {
    if (token.empty() || token.size() > value.size()) {
        return false;
    }
    for (std::size_t offset = 0; offset <= value.size() - token.size(); ++offset) {
        bool matched{true};
        for (std::size_t token_index = 0; token_index < token.size(); ++token_index) {
            if (ascii_lower(value[offset + token_index]) != ascii_lower(token[token_index])) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool contains_unsafe_token(std::string_view value) noexcept {
    constexpr std::string_view unsafe_tokens[] = {
        "ID3D12",        "D3D12_",    "Vk",     "vk",         "MTL",         "SDL_",    "ImGui",
        "native_handle", "HWND",      "HANDLE", "dlopen",     "LoadLibrary", "system(", "ShellExecute",
        "CreateProcess", "fastgltf",  "cgltf",  "ktxTexture", "KtxTexture",  "IDxc",    "DxcCompiler",
        "spng_",         "ma_decoder"};
    return std::ranges::any_of(
        unsafe_tokens, [value](std::string_view token) { return contains_ascii_case_insensitive(value, token); });
}

[[nodiscard]] bool valid_token_list(const std::vector<std::string>& values) noexcept {
    return std::ranges::all_of(
        values, [](const std::string& value) { return valid_token(value) && !contains_unsafe_token(value); });
}

void append_diagnostic(std::vector<AssetImportProductionDiagnostic>& diagnostics,
                       AssetImportProductionDiagnosticCode code, const AssetImportProductionEvidenceRow& row,
                       std::string message) {
    diagnostics.push_back(AssetImportProductionDiagnostic{
        .code = code,
        .feature = row.feature,
        .capability_id = row.capability_id,
        .message = std::move(message),
        .source_index = row.source_index,
    });
}

void append_diagnostic(std::vector<AssetImportProductionDiagnostic>& diagnostics,
                       AssetImportProductionDiagnosticCode code, AssetImportProductionFeatureKind feature,
                       std::string message) {
    diagnostics.push_back(AssetImportProductionDiagnostic{
        .code = code,
        .feature = feature,
        .capability_id = {},
        .message = std::move(message),
        .source_index = 0U,
    });
}

[[nodiscard]] bool row_base_is_valid(const AssetImportProductionEvidenceRow& row) noexcept {
    return valid_feature(row.feature) && valid_token(row.capability_id) && !contains_unsafe_token(row.capability_id) &&
           valid_token(row.importer_id) && !contains_unsafe_token(row.importer_id) && valid_token(row.source_root) &&
           !contains_unsafe_token(row.source_root) && valid_token(row.deterministic_content_hash) &&
           !contains_unsafe_token(row.deterministic_content_hash) && valid_token_list(row.declared_extensions) &&
           valid_token_list(row.output_package_rows) && valid_token_list(row.validator_ids) &&
           valid_token_list(row.reviewed_command_ids) && valid_token_list(row.dependency_ids) &&
           valid_token_list(row.license_ids) && valid_token_list(row.provenance_ids);
}

[[nodiscard]] bool row_is_host_gated(const AssetImportProductionEvidenceRow& row) noexcept {
    return row.host_gate_required && !row.host_validated;
}

[[nodiscard]] bool row_is_dependency_gated(const AssetImportProductionEvidenceRow& row) noexcept {
    return row.dependency_gate_required && feature_requires_dependency_legal_record(row.feature) &&
           !row.dependency_legal_evidence;
}

[[nodiscard]] bool has_exact_dependency_id(const std::vector<std::string>& values,
                                           std::string_view expected_id) noexcept {
    return std::ranges::any_of(
        values, [expected_id](const std::string& value) { return std::string_view{value} == expected_id; });
}

[[nodiscard]] bool row_has_exact_dependency_ids(const AssetImportProductionEvidenceRow& row,
                                                std::initializer_list<std::string_view> expected_ids) noexcept {
    if (row.dependency_ids.size() != expected_ids.size()) {
        return false;
    }
    return std::ranges::all_of(expected_ids, [&row](std::string_view expected_id) {
        return has_exact_dependency_id(row.dependency_ids, expected_id);
    });
}

[[nodiscard]] bool all_extensions_are_selected(const AssetImportProductionEvidenceRow& row,
                                               std::initializer_list<std::string_view> selected_extensions) noexcept {
    return std::ranges::all_of(row.declared_extensions, [selected_extensions](const std::string& extension) {
        return std::ranges::find(selected_extensions, std::string_view{extension}) != selected_extensions.end();
    });
}

[[nodiscard]] bool row_requests_broad_codec_claim(const AssetImportProductionEvidenceRow& row) noexcept {
    if (row.request_broad_codec_claim) {
        return true;
    }
    if (row.feature == AssetImportProductionFeatureKind::source_image) {
        return !all_extensions_are_selected(row, {".png"});
    }
    if (row.feature == AssetImportProductionFeatureKind::source_audio) {
        return !all_extensions_are_selected(row, {".wav", ".flac", ".mp3"});
    }
    return false;
}

[[nodiscard]] bool row_has_unsupported_claim(const AssetImportProductionEvidenceRow& row) noexcept {
    return row.request_arbitrary_importer_plugin || row.request_external_download ||
           row.request_live_shader_generation || row.request_source_mutation_outside_roots ||
           row.request_native_handle_access || row.request_unreviewed_compiler_execution ||
           row.request_runtime_source_parsing || row_requests_broad_codec_claim(row);
}

[[nodiscard]] bool row_has_required_dependency_evidence(const AssetImportProductionEvidenceRow& row) noexcept {
    if (!feature_requires_dependency_legal_record(row.feature)) {
        return true;
    }
    if (!row.dependency_legal_evidence || row.dependency_ids.empty()) {
        return false;
    }
    if (row.feature == AssetImportProductionFeatureKind::gltf_geometry ||
        row.feature == AssetImportProductionFeatureKind::gltf_animation ||
        row.feature == AssetImportProductionFeatureKind::source_image ||
        row.feature == AssetImportProductionFeatureKind::source_audio) {
        return row_has_exact_dependency_ids(row, {"vcpkg.asset-importers"});
    }
    if (row.feature == AssetImportProductionFeatureKind::ktx_texture) {
        return row_has_exact_dependency_ids(row, {"vcpkg.ktx-software"});
    }
    if (row.feature == AssetImportProductionFeatureKind::shader_offline_compile_request) {
        return row_has_exact_dependency_ids(row, {"toolchain.dxc", "toolchain.spirv-tools"});
    }
    return true;
}

[[nodiscard]] AssetImportProductionExecutionReadiness
execution_readiness_for_row(const AssetImportProductionEvidenceRow& row) noexcept {
    if (row.request_package_mutation) {
        return AssetImportProductionExecutionReadiness::package_mutation_required;
    }
    if (row_has_unsupported_claim(row)) {
        return AssetImportProductionExecutionReadiness::unsupported_claim;
    }
    if (row_is_dependency_gated(row)) {
        return AssetImportProductionExecutionReadiness::dependency_evidence_required;
    }
    if (row_is_host_gated(row)) {
        return AssetImportProductionExecutionReadiness::host_evidence_required;
    }
    return AssetImportProductionExecutionReadiness::reviewed_execution;
}

[[nodiscard]] bool row_is_ready(const AssetImportProductionEvidenceRow& row) noexcept {
    if (execution_readiness_for_row(row) != AssetImportProductionExecutionReadiness::reviewed_execution) {
        return false;
    }
    return row.reviewed && row.host_validated && row.source_root_evidence && row.importer_declared &&
           row.extension_evidence && row.package_handoff_evidence && row.license_provenance_evidence &&
           row.deterministic_hash_evidence && (!feature_requires_validator(row.feature) || row.validator_evidence) &&
           row_has_required_dependency_evidence(row) &&
           (!feature_requires_command_review(row.feature) || row.command_review_evidence);
}

[[nodiscard]] std::uint64_t hash_byte(std::uint64_t hash, std::uint8_t value) noexcept {
    hash ^= value;
    return hash * fnv_prime;
}

[[nodiscard]] std::uint64_t hash_string(std::uint64_t hash, std::string_view value) noexcept {
    for (const auto character : value) {
        hash = hash_byte(hash, static_cast<std::uint8_t>(character));
    }
    return hash_byte(hash, 0xFFU);
}

[[nodiscard]] std::uint64_t hash_bool(std::uint64_t hash, bool value) noexcept {
    return hash_byte(hash, value ? 1U : 0U);
}

[[nodiscard]] std::uint64_t hash_u32(std::uint64_t hash, std::uint32_t value) noexcept {
    for (std::uint32_t shift = 0U; shift < 32U; shift += 8U) {
        hash = hash_byte(hash, static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
    return hash;
}

[[nodiscard]] std::uint64_t hash_string_list(std::uint64_t hash, const std::vector<std::string>& values) noexcept {
    hash = hash_u32(hash, static_cast<std::uint32_t>(values.size()));
    for (const auto& value : values) {
        hash = hash_string(hash, value);
    }
    return hash;
}

[[nodiscard]] std::uint64_t hash_row(std::uint64_t hash, const AssetImportProductionEvidenceRow& row) noexcept {
    hash = hash_byte(hash, static_cast<std::uint8_t>(row.feature));
    hash = hash_byte(hash, static_cast<std::uint8_t>(row.proof));
    hash = hash_string(hash, row.capability_id);
    hash = hash_string(hash, row.importer_id);
    hash = hash_string(hash, row.source_root);
    hash = hash_string_list(hash, row.declared_extensions);
    hash = hash_string_list(hash, row.output_package_rows);
    hash = hash_string_list(hash, row.validator_ids);
    hash = hash_string_list(hash, row.reviewed_command_ids);
    hash = hash_string_list(hash, row.dependency_ids);
    hash = hash_string_list(hash, row.license_ids);
    hash = hash_string_list(hash, row.provenance_ids);
    hash = hash_string(hash, row.deterministic_content_hash);
    hash = hash_bool(hash, row.reviewed);
    hash = hash_bool(hash, row.source_root_evidence);
    hash = hash_bool(hash, row.importer_declared);
    hash = hash_bool(hash, row.extension_evidence);
    hash = hash_bool(hash, row.package_handoff_evidence);
    hash = hash_bool(hash, row.license_provenance_evidence);
    hash = hash_bool(hash, row.deterministic_hash_evidence);
    hash = hash_bool(hash, row.validator_evidence);
    hash = hash_bool(hash, row.dependency_legal_evidence);
    hash = hash_bool(hash, row.dependency_gate_required);
    hash = hash_bool(hash, row.command_review_evidence);
    hash = hash_bool(hash, row.host_validated);
    hash = hash_bool(hash, row.host_gate_required);
    hash = hash_bool(hash, row.request_arbitrary_importer_plugin);
    hash = hash_bool(hash, row.request_external_download);
    hash = hash_bool(hash, row.request_live_shader_generation);
    hash = hash_bool(hash, row.request_source_mutation_outside_roots);
    hash = hash_bool(hash, row.request_package_mutation);
    hash = hash_bool(hash, row.request_native_handle_access);
    hash = hash_bool(hash, row.request_unreviewed_compiler_execution);
    hash = hash_bool(hash, row.request_runtime_source_parsing);
    hash = hash_bool(hash, row.request_broad_codec_claim);
    hash = hash_u32(hash, row.source_index);
    return hash;
}

[[nodiscard]] std::uint64_t build_replay_hash(const AssetImportProductionReviewRequest& request) noexcept {
    auto hash = fnv_offset;
    hash ^= request.seed;
    hash *= fnv_prime;
    for (const auto feature : request.required_features) {
        hash = hash_byte(hash, static_cast<std::uint8_t>(feature));
    }
    for (const auto& row : request.rows) {
        hash = hash_row(hash, row);
    }
    return hash == 0U ? fnv_offset : hash;
}

[[nodiscard]] bool has_ready_feature(const std::vector<AssetImportProductionEvidenceRow>& rows,
                                     AssetImportProductionFeatureKind feature) noexcept {
    return std::ranges::any_of(rows, [feature](const AssetImportProductionEvidenceRow& row) {
        return row.feature == feature && row_is_ready(row);
    });
}

[[nodiscard]] bool has_no_host_gate_for_feature(const std::vector<AssetImportProductionEvidenceRow>& rows,
                                                AssetImportProductionFeatureKind feature) noexcept {
    return std::ranges::none_of(rows, [feature](const AssetImportProductionEvidenceRow& row) {
        return row.feature == feature && row_is_host_gated(row);
    });
}

[[nodiscard]] std::size_t count_extensions(const std::vector<AssetImportProductionEvidenceRow>& rows) {
    std::vector<std::string> unique_extensions;
    for (const auto& row : rows) {
        if (!row_is_ready(row)) {
            continue;
        }
        for (const auto& extension : row.declared_extensions) {
            if (std::ranges::find(unique_extensions, extension) == unique_extensions.end()) {
                unique_extensions.push_back(extension);
            }
        }
    }
    return unique_extensions.size();
}

} // namespace

bool AssetImportProductionReview::succeeded() const noexcept {
    return status == AssetImportProductionStatus::ready || status == AssetImportProductionStatus::no_rows;
}

AssetImportProductionReview
review_asset_import_production_readiness(const AssetImportProductionReviewRequest& request) {
    AssetImportProductionReview review;
    review.required_features = request.required_features;
    review.row_count = request.rows.size();

    if (request.rows.empty() && request.required_features.empty()) {
        review.status = AssetImportProductionStatus::no_rows;
        return review;
    }

    if (request.rows.size() > request.row_budget) {
        append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::row_budget_exceeded,
                          AssetImportProductionFeatureKind::source_root_policy,
                          "asset import production evidence row budget is exceeded");
    }

    for (std::size_t index = 0; index < request.required_features.size(); ++index) {
        const auto feature = request.required_features[index];
        if (!valid_feature(feature)) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::invalid_required_feature,
                              feature, "required asset import production feature is invalid");
            continue;
        }
        for (std::size_t other = index + 1U; other < request.required_features.size(); ++other) {
            if (feature == request.required_features[other]) {
                append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::duplicate_required_feature,
                                  feature, "required asset import production feature is duplicated");
                break;
            }
        }
    }

    for (const auto feature : request.required_features) {
        if (valid_feature(feature) &&
            std::ranges::none_of(request.rows, [feature](const AssetImportProductionEvidenceRow& row) {
                return row.feature == feature;
            })) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_required_feature_row,
                              feature, "required asset import production feature row is missing");
        }
    }

    for (std::size_t index = 0; index < request.rows.size(); ++index) {
        const auto& row = request.rows[index];
        const auto row_readiness = execution_readiness_for_row(row);
        review.execution_readiness.push_back(AssetImportProductionExecutionReadinessRow{
            .feature = row.feature,
            .readiness = row_readiness,
            .capability_id = row.capability_id,
            .source_index = row.source_index,
        });
        if (row_readiness == AssetImportProductionExecutionReadiness::host_evidence_required) {
            ++review.host_gated_row_count;
        } else if (row_readiness == AssetImportProductionExecutionReadiness::dependency_evidence_required) {
            ++review.dependency_gated_row_count;
        } else if (row_readiness == AssetImportProductionExecutionReadiness::package_mutation_required) {
            ++review.package_mutation_request_count;
        } else if (row_readiness == AssetImportProductionExecutionReadiness::unsupported_claim) {
            ++review.unsupported_claim_row_count;
        }

        if (!row_base_is_valid(row)) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::invalid_evidence_row, row,
                              "asset import production evidence row is invalid");
        }
        for (std::size_t other = index + 1U; other < request.rows.size(); ++other) {
            const auto& other_row = request.rows[other];
            if (row.feature == other_row.feature) {
                append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::duplicate_feature_row, row,
                                  "asset import production evidence row is duplicated");
                break;
            }
        }

        if (row.request_arbitrary_importer_plugin) {
            append_diagnostic(review.diagnostics,
                              AssetImportProductionDiagnosticCode::unsupported_arbitrary_importer_plugin, row,
                              "arbitrary importer plugin execution is unsupported");
        }
        if (row.request_external_download) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::unsupported_external_download,
                              row, "external asset downloads are unsupported");
        }
        if (row.request_live_shader_generation) {
            append_diagnostic(review.diagnostics,
                              AssetImportProductionDiagnosticCode::unsupported_live_shader_generation, row,
                              "live shader generation is unsupported");
        }
        if (row.request_source_mutation_outside_roots) {
            append_diagnostic(review.diagnostics,
                              AssetImportProductionDiagnosticCode::unsupported_source_mutation_outside_roots, row,
                              "source mutation outside reviewed roots is unsupported");
        }
        if (row.request_package_mutation) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::unsupported_package_mutation,
                              row, "package mutation from importer review rows is unsupported");
        }
        if (row.request_native_handle_access) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::unsupported_native_handle_claim,
                              row, "native handle access is unsupported");
        }
        if (row.request_unreviewed_compiler_execution) {
            append_diagnostic(review.diagnostics,
                              AssetImportProductionDiagnosticCode::unsupported_unreviewed_compiler_execution, row,
                              "unreviewed compiler execution is unsupported");
        }
        if (row.request_runtime_source_parsing) {
            append_diagnostic(review.diagnostics,
                              AssetImportProductionDiagnosticCode::unsupported_runtime_source_parsing, row,
                              "runtime source parsing is unsupported");
        }
        if (row_requests_broad_codec_claim(row)) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::unsupported_broad_codec_claim,
                              row, "broad codec support claims require explicit reviewed rows");
        }

        if (row_is_host_gated(row)) {
            continue;
        }
        if (!row.reviewed) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_review_evidence, row,
                              "review evidence is missing");
        }
        if (!row.host_validated) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_host_validation_evidence,
                              row, "host validation evidence is missing");
        }
        if (!row.source_root_evidence) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_source_root_evidence,
                              row, "source root evidence is missing");
        }
        if (!row.importer_declared) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_importer_id, row,
                              "importer id evidence is missing");
        }
        if (!row.extension_evidence || row.declared_extensions.empty()) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_extension_evidence, row,
                              "extension evidence is missing");
        }
        if (!row.package_handoff_evidence || row.output_package_rows.empty()) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_output_package_row, row,
                              "output package row evidence is missing");
        }
        if (!row.license_provenance_evidence || row.license_ids.empty() || row.provenance_ids.empty()) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_license_provenance, row,
                              "license and provenance evidence is missing");
        }
        if (!row.deterministic_hash_evidence || row.deterministic_content_hash.empty()) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_deterministic_hash, row,
                              "deterministic content hash evidence is missing");
        }
        if (feature_requires_validator(row.feature) && (!row.validator_evidence || row.validator_ids.empty())) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_validator_evidence, row,
                              "validator evidence is missing");
        }
        if (feature_requires_dependency_legal_record(row.feature) && !row_is_dependency_gated(row) &&
            !row_has_required_dependency_evidence(row)) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_dependency_legal_record,
                              row, "dependency and legal evidence is missing");
        }
        if (feature_requires_command_review(row.feature) &&
            (!row.command_review_evidence || row.reviewed_command_ids.empty())) {
            append_diagnostic(review.diagnostics, AssetImportProductionDiagnosticCode::missing_command_review_evidence,
                              row, "reviewed command evidence is missing");
        }
    }

    if (!review.diagnostics.empty()) {
        review.status = AssetImportProductionStatus::invalid_request;
        return review;
    }

    review.rows = request.rows;
    for (const auto& row : review.rows) {
        if (row_is_ready(row)) {
            ++review.ready_row_count;
        }
        if (row.reviewed && row.importer_declared) {
            ++review.reviewed_importer_count;
        }
    }

    review.supported_source_format_count = count_extensions(review.rows);
    review.reviewed_source_roots_ready =
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::source_root_policy);
    review.gltf_ktx_import_review_ready =
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::gltf_geometry) &&
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::gltf_animation) &&
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::ktx_texture) &&
        has_no_host_gate_for_feature(review.rows, AssetImportProductionFeatureKind::ktx_texture);
    review.image_audio_import_review_ready =
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::source_image) &&
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::source_audio);
    review.shader_offline_compile_review_ready =
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::shader_offline_compile_request);
    review.package_cook_outputs_ready =
        has_ready_feature(review.rows, AssetImportProductionFeatureKind::package_cook_output);
    review.dependency_legal_records_ready = std::ranges::all_of(review.rows, [](const auto& row) {
        return row_is_host_gated(row) || !feature_requires_dependency_legal_record(row.feature) ||
               row_has_required_dependency_evidence(row);
    });
    review.deterministic_cook_ready = std::ranges::all_of(
        review.rows, [](const auto& row) { return row_is_host_gated(row) || row.deterministic_hash_evidence; });
    review.broad_asset_import_ready = review.ready_row_count == review.row_count &&
                                      review.reviewed_source_roots_ready && review.gltf_ktx_import_review_ready &&
                                      review.image_audio_import_review_ready &&
                                      review.shader_offline_compile_review_ready && review.package_cook_outputs_ready &&
                                      review.dependency_legal_records_ready && review.deterministic_cook_ready;
    review.replay_hash = build_replay_hash(request);
    if (review.host_gated_row_count > 0U) {
        review.status = AssetImportProductionStatus::host_evidence_required;
    } else if (review.dependency_gated_row_count > 0U) {
        review.status = AssetImportProductionStatus::dependency_evidence_required;
    } else {
        review.status = AssetImportProductionStatus::ready;
    }
    return review;
}

} // namespace mirakana
