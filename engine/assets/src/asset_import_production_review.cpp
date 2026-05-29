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
        return row_has_exact_dependency_ids(row, {"vcpkg.ktx"});
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

constexpr std::array<KtxBasisTextureReviewFeature, 7U> ktx_required_feature_set = {
    KtxBasisTextureReviewFeature::container_validation,    KtxBasisTextureReviewFeature::supercompression_policy,
    KtxBasisTextureReviewFeature::transcode_target_policy, KtxBasisTextureReviewFeature::gpu_target_compatibility,
    KtxBasisTextureReviewFeature::source_provenance,       KtxBasisTextureReviewFeature::package_output,
    KtxBasisTextureReviewFeature::host_tool_gate,
};

[[nodiscard]] bool ktx_valid_feature(KtxBasisTextureReviewFeature feature) noexcept {
    return std::ranges::find(ktx_required_feature_set, feature) != ktx_required_feature_set.end();
}

[[nodiscard]] bool valid_optional_token(std::string_view value) noexcept {
    return value.empty() || (valid_token(value) && !contains_unsafe_token(value));
}

[[nodiscard]] bool ktx_feature_requires_dependency_legal_record(KtxBasisTextureReviewFeature feature) noexcept {
    return feature != KtxBasisTextureReviewFeature::host_tool_gate;
}

[[nodiscard]] bool ktx_row_is_host_gated(const KtxBasisTextureReviewRow& row) noexcept {
    return row.host_tool_gate_required && !row.host_tool_validated;
}

[[nodiscard]] bool ktx_row_is_dependency_gated(const KtxBasisTextureReviewRow& row) noexcept {
    return row.dependency_gate_required && ktx_feature_requires_dependency_legal_record(row.feature) &&
           !row.dependency_legal_evidence;
}

[[nodiscard]] bool ktx_row_has_exact_dependency_id(const KtxBasisTextureReviewRow& row,
                                                   std::string_view expected_id) noexcept {
    return row.dependency_ids.size() == 1U && std::string_view{row.dependency_ids.front()} == expected_id;
}

[[nodiscard]] bool ktx_row_has_required_dependency_evidence(const KtxBasisTextureReviewRow& row) noexcept {
    return !ktx_feature_requires_dependency_legal_record(row.feature) ||
           (row.dependency_legal_evidence && ktx_row_has_exact_dependency_id(row, "vcpkg.ktx"));
}

[[nodiscard]] bool ktx_row_has_unsupported_claim(const KtxBasisTextureReviewRow& row) noexcept {
    return row.request_runtime_transcoding || row.request_gpu_upload || row.request_compression_execution ||
           row.request_native_handle_access || row.request_broad_texture_codec_claim;
}

[[nodiscard]] bool ktx_row_base_is_valid(const KtxBasisTextureReviewRow& row) noexcept {
    return ktx_valid_feature(row.feature) && valid_token(row.row_id) && !contains_unsafe_token(row.row_id) &&
           valid_optional_token(row.supercompression_scheme) && valid_optional_token(row.transcode_policy) &&
           valid_optional_token(row.transcode_target) && valid_optional_token(row.gpu_target) &&
           valid_optional_token(row.deterministic_content_hash) && valid_token_list(row.container_validator_ids) &&
           valid_token_list(row.source_provenance_ids) && valid_token_list(row.package_output_rows) &&
           valid_token_list(row.dependency_ids) && valid_token_list(row.license_ids);
}

void append_ktx_diagnostic(std::vector<KtxBasisTextureReviewDiagnostic>& diagnostics,
                           KtxBasisTextureReviewDiagnosticCode code, const KtxBasisTextureReviewRow& row,
                           std::string message) {
    diagnostics.push_back(KtxBasisTextureReviewDiagnostic{
        .code = code,
        .feature = row.feature,
        .row_id = row.row_id,
        .message = std::move(message),
        .source_index = row.source_index,
    });
}

void append_ktx_diagnostic(std::vector<KtxBasisTextureReviewDiagnostic>& diagnostics,
                           KtxBasisTextureReviewDiagnosticCode code, KtxBasisTextureReviewFeature feature,
                           std::string message) {
    diagnostics.push_back(KtxBasisTextureReviewDiagnostic{
        .code = code,
        .feature = feature,
        .row_id = {},
        .message = std::move(message),
        .source_index = 0U,
    });
}

[[nodiscard]] bool ktx_row_is_ready(const KtxBasisTextureReviewRow& row) noexcept {
    if (ktx_row_has_unsupported_claim(row) || ktx_row_is_dependency_gated(row) || ktx_row_is_host_gated(row) ||
        !row.reviewed || !ktx_row_has_required_dependency_evidence(row)) {
        return false;
    }

    switch (row.feature) {
    case KtxBasisTextureReviewFeature::container_validation:
        return row.container_validation_evidence && !row.container_validator_ids.empty();
    case KtxBasisTextureReviewFeature::supercompression_policy:
        return row.supercompression_policy_evidence && valid_token(row.supercompression_scheme) &&
               !contains_unsafe_token(row.supercompression_scheme);
    case KtxBasisTextureReviewFeature::transcode_target_policy:
        return row.transcode_target_evidence && valid_token(row.transcode_policy) &&
               !contains_unsafe_token(row.transcode_policy) && valid_token(row.transcode_target) &&
               !contains_unsafe_token(row.transcode_target);
    case KtxBasisTextureReviewFeature::gpu_target_compatibility:
        return row.gpu_target_compatibility_evidence && valid_token(row.gpu_target) &&
               !contains_unsafe_token(row.gpu_target);
    case KtxBasisTextureReviewFeature::source_provenance:
        return row.source_provenance_evidence && !row.source_provenance_ids.empty() && !row.license_ids.empty() &&
               valid_token(row.deterministic_content_hash) && !contains_unsafe_token(row.deterministic_content_hash);
    case KtxBasisTextureReviewFeature::package_output:
        return row.package_output_evidence && !row.package_output_rows.empty() &&
               valid_token(row.deterministic_content_hash) && !contains_unsafe_token(row.deterministic_content_hash);
    case KtxBasisTextureReviewFeature::host_tool_gate:
        return row.host_tool_validated && !row.host_tool_gate_required;
    }
    return false;
}

[[nodiscard]] std::uint64_t hash_ktx_row(std::uint64_t hash, const KtxBasisTextureReviewRow& row) noexcept {
    hash = hash_byte(hash, static_cast<std::uint8_t>(row.feature));
    hash = hash_string(hash, row.row_id);
    hash = hash_string_list(hash, row.container_validator_ids);
    hash = hash_string(hash, row.supercompression_scheme);
    hash = hash_string(hash, row.transcode_policy);
    hash = hash_string(hash, row.transcode_target);
    hash = hash_string(hash, row.gpu_target);
    hash = hash_string_list(hash, row.source_provenance_ids);
    hash = hash_string_list(hash, row.package_output_rows);
    hash = hash_string_list(hash, row.dependency_ids);
    hash = hash_string_list(hash, row.license_ids);
    hash = hash_string(hash, row.deterministic_content_hash);
    hash = hash_bool(hash, row.reviewed);
    hash = hash_bool(hash, row.container_validation_evidence);
    hash = hash_bool(hash, row.supercompression_policy_evidence);
    hash = hash_bool(hash, row.transcode_target_evidence);
    hash = hash_bool(hash, row.gpu_target_compatibility_evidence);
    hash = hash_bool(hash, row.source_provenance_evidence);
    hash = hash_bool(hash, row.package_output_evidence);
    hash = hash_bool(hash, row.dependency_legal_evidence);
    hash = hash_bool(hash, row.dependency_gate_required);
    hash = hash_bool(hash, row.host_tool_validated);
    hash = hash_bool(hash, row.host_tool_gate_required);
    hash = hash_bool(hash, row.request_runtime_transcoding);
    hash = hash_bool(hash, row.request_gpu_upload);
    hash = hash_bool(hash, row.request_compression_execution);
    hash = hash_bool(hash, row.request_native_handle_access);
    hash = hash_bool(hash, row.request_broad_texture_codec_claim);
    hash = hash_u32(hash, row.source_index);
    return hash;
}

[[nodiscard]] std::uint64_t build_ktx_replay_hash(const KtxBasisTextureReviewRequest& request) noexcept {
    auto hash = fnv_offset;
    hash ^= request.seed;
    hash *= fnv_prime;
    for (const auto feature : request.required_features) {
        hash = hash_byte(hash, static_cast<std::uint8_t>(feature));
    }
    for (const auto& row : request.rows) {
        hash = hash_ktx_row(hash, row);
    }
    return hash == 0U ? fnv_offset : hash;
}

[[nodiscard]] bool has_ready_ktx_feature(const std::vector<KtxBasisTextureReviewRow>& rows,
                                         KtxBasisTextureReviewFeature feature) noexcept {
    return std::ranges::any_of(rows, [feature](const KtxBasisTextureReviewRow& row) {
        return row.feature == feature && ktx_row_is_ready(row);
    });
}

constexpr std::array<GltfSceneImportReviewFeature, 8U> gltf_required_feature_set = {
    GltfSceneImportReviewFeature::source_root_policy, GltfSceneImportReviewFeature::parser_validation,
    GltfSceneImportReviewFeature::geometry_payload,   GltfSceneImportReviewFeature::material_payload,
    GltfSceneImportReviewFeature::animation_payload,  GltfSceneImportReviewFeature::external_resource_policy,
    GltfSceneImportReviewFeature::source_provenance,  GltfSceneImportReviewFeature::package_output,
};

[[nodiscard]] bool gltf_valid_feature(GltfSceneImportReviewFeature feature) noexcept {
    return std::ranges::find(gltf_required_feature_set, feature) != gltf_required_feature_set.end();
}

[[nodiscard]] bool gltf_row_is_dependency_gated(const GltfSceneImportReviewRow& row) noexcept {
    return row.dependency_gate_required && !row.dependency_legal_evidence;
}

[[nodiscard]] bool gltf_row_has_exact_dependency_id(const GltfSceneImportReviewRow& row,
                                                    std::string_view expected_id) noexcept {
    return row.dependency_ids.size() == 1U && std::string_view{row.dependency_ids.front()} == expected_id;
}

[[nodiscard]] bool gltf_row_has_required_dependency_evidence(const GltfSceneImportReviewRow& row) noexcept {
    return row.dependency_legal_evidence && gltf_row_has_exact_dependency_id(row, "vcpkg.asset-importers");
}

[[nodiscard]] bool gltf_row_requests_arbitrary_extension(const GltfSceneImportReviewRow& row) noexcept {
    if (row.request_arbitrary_extension) {
        return true;
    }
    return !std::ranges::all_of(row.declared_extensions, [](const std::string& extension) {
        return std::string_view{extension} == ".gltf" || std::string_view{extension} == ".glb";
    });
}

[[nodiscard]] bool gltf_row_has_unsupported_claim(const GltfSceneImportReviewRow& row) noexcept {
    return gltf_row_requests_arbitrary_extension(row) || row.request_external_network_fetch ||
           row.request_runtime_source_parsing || row.request_parser_type_access || row.request_native_handle_access ||
           row.request_broad_scene_import_claim || row.request_package_mutation;
}

[[nodiscard]] bool gltf_row_base_is_valid(const GltfSceneImportReviewRow& row) noexcept {
    return gltf_valid_feature(row.feature) && valid_token(row.row_id) && !contains_unsafe_token(row.row_id) &&
           valid_optional_token(row.source_root) && valid_optional_token(row.importer_id) &&
           valid_token_list(row.declared_extensions) && valid_token_list(row.validator_ids) &&
           valid_token_list(row.dependency_ids) && valid_token_list(row.license_ids) &&
           valid_token_list(row.provenance_ids) && valid_token_list(row.package_output_rows) &&
           valid_optional_token(row.deterministic_content_hash) && valid_optional_token(row.external_resource_policy);
}

void append_gltf_diagnostic(std::vector<GltfSceneImportReviewDiagnostic>& diagnostics,
                            GltfSceneImportReviewDiagnosticCode code, const GltfSceneImportReviewRow& row,
                            std::string message) {
    diagnostics.push_back(GltfSceneImportReviewDiagnostic{
        .code = code,
        .feature = row.feature,
        .row_id = row.row_id,
        .message = std::move(message),
        .source_index = row.source_index,
    });
}

void append_gltf_diagnostic(std::vector<GltfSceneImportReviewDiagnostic>& diagnostics,
                            GltfSceneImportReviewDiagnosticCode code, GltfSceneImportReviewFeature feature,
                            std::string message) {
    diagnostics.push_back(GltfSceneImportReviewDiagnostic{
        .code = code,
        .feature = feature,
        .row_id = {},
        .message = std::move(message),
        .source_index = 0U,
    });
}

[[nodiscard]] bool gltf_row_is_ready(const GltfSceneImportReviewRow& row) noexcept {
    if (gltf_row_has_unsupported_claim(row) || gltf_row_is_dependency_gated(row) || !row.reviewed ||
        !gltf_row_has_required_dependency_evidence(row)) {
        return false;
    }

    switch (row.feature) {
    case GltfSceneImportReviewFeature::source_root_policy:
        return row.source_root_evidence && !row.source_root.empty() && !row.declared_extensions.empty() &&
               !gltf_row_requests_arbitrary_extension(row);
    case GltfSceneImportReviewFeature::parser_validation:
        return row.parser_validation_evidence && !row.importer_id.empty() && !row.validator_ids.empty();
    case GltfSceneImportReviewFeature::geometry_payload:
        return row.geometry_payload_evidence && !row.deterministic_content_hash.empty();
    case GltfSceneImportReviewFeature::material_payload:
        return row.material_payload_evidence && !row.deterministic_content_hash.empty();
    case GltfSceneImportReviewFeature::animation_payload:
        return row.animation_payload_evidence && !row.deterministic_content_hash.empty();
    case GltfSceneImportReviewFeature::external_resource_policy:
        return row.external_resource_policy_evidence && !row.external_resource_policy.empty();
    case GltfSceneImportReviewFeature::source_provenance:
        return row.source_provenance_evidence && !row.provenance_ids.empty() && !row.license_ids.empty() &&
               !row.deterministic_content_hash.empty();
    case GltfSceneImportReviewFeature::package_output:
        return row.package_output_evidence && !row.package_output_rows.empty() &&
               !row.deterministic_content_hash.empty();
    }
    return false;
}

[[nodiscard]] std::uint64_t hash_gltf_row(std::uint64_t hash, const GltfSceneImportReviewRow& row) noexcept {
    hash = hash_byte(hash, static_cast<std::uint8_t>(row.feature));
    hash = hash_string(hash, row.row_id);
    hash = hash_string(hash, row.source_root);
    hash = hash_string(hash, row.importer_id);
    hash = hash_string_list(hash, row.declared_extensions);
    hash = hash_string_list(hash, row.validator_ids);
    hash = hash_string_list(hash, row.dependency_ids);
    hash = hash_string_list(hash, row.license_ids);
    hash = hash_string_list(hash, row.provenance_ids);
    hash = hash_string_list(hash, row.package_output_rows);
    hash = hash_string(hash, row.deterministic_content_hash);
    hash = hash_string(hash, row.external_resource_policy);
    hash = hash_bool(hash, row.reviewed);
    hash = hash_bool(hash, row.source_root_evidence);
    hash = hash_bool(hash, row.parser_validation_evidence);
    hash = hash_bool(hash, row.geometry_payload_evidence);
    hash = hash_bool(hash, row.material_payload_evidence);
    hash = hash_bool(hash, row.animation_payload_evidence);
    hash = hash_bool(hash, row.external_resource_policy_evidence);
    hash = hash_bool(hash, row.source_provenance_evidence);
    hash = hash_bool(hash, row.package_output_evidence);
    hash = hash_bool(hash, row.dependency_legal_evidence);
    hash = hash_bool(hash, row.dependency_gate_required);
    hash = hash_bool(hash, row.request_arbitrary_extension);
    hash = hash_bool(hash, row.request_external_network_fetch);
    hash = hash_bool(hash, row.request_runtime_source_parsing);
    hash = hash_bool(hash, row.request_parser_type_access);
    hash = hash_bool(hash, row.request_native_handle_access);
    hash = hash_bool(hash, row.request_broad_scene_import_claim);
    hash = hash_bool(hash, row.request_package_mutation);
    hash = hash_u32(hash, row.source_index);
    return hash;
}

[[nodiscard]] std::uint64_t build_gltf_replay_hash(const GltfSceneImportReviewRequest& request) noexcept {
    auto hash = fnv_offset;
    hash ^= request.seed;
    hash *= fnv_prime;
    for (const auto feature : request.required_features) {
        hash = hash_byte(hash, static_cast<std::uint8_t>(feature));
    }
    for (const auto& row : request.rows) {
        hash = hash_gltf_row(hash, row);
    }
    return hash == 0U ? fnv_offset : hash;
}

[[nodiscard]] bool has_ready_gltf_feature(const std::vector<GltfSceneImportReviewRow>& rows,
                                          GltfSceneImportReviewFeature feature) noexcept {
    return std::ranges::any_of(rows, [feature](const GltfSceneImportReviewRow& row) {
        return row.feature == feature && gltf_row_is_ready(row);
    });
}

constexpr std::array<SourceImageAudioCodecReviewFeature, 6U> source_codec_required_feature_set = {
    SourceImageAudioCodecReviewFeature::png_decode_adapter,   SourceImageAudioCodecReviewFeature::png_pixel_format,
    SourceImageAudioCodecReviewFeature::audio_decode_adapter, SourceImageAudioCodecReviewFeature::audio_sample_format,
    SourceImageAudioCodecReviewFeature::source_provenance,    SourceImageAudioCodecReviewFeature::package_output,
};

[[nodiscard]] bool source_codec_valid_feature(SourceImageAudioCodecReviewFeature feature) noexcept {
    return std::ranges::find(source_codec_required_feature_set, feature) != source_codec_required_feature_set.end();
}

[[nodiscard]] bool source_codec_feature_is_png(SourceImageAudioCodecReviewFeature feature) noexcept {
    return feature == SourceImageAudioCodecReviewFeature::png_decode_adapter ||
           feature == SourceImageAudioCodecReviewFeature::png_pixel_format;
}

[[nodiscard]] bool source_codec_feature_is_audio(SourceImageAudioCodecReviewFeature feature) noexcept {
    return feature == SourceImageAudioCodecReviewFeature::audio_decode_adapter ||
           feature == SourceImageAudioCodecReviewFeature::audio_sample_format;
}

[[nodiscard]] bool source_codec_feature_uses_both_dependencies(SourceImageAudioCodecReviewFeature feature) noexcept {
    return feature == SourceImageAudioCodecReviewFeature::source_provenance ||
           feature == SourceImageAudioCodecReviewFeature::package_output;
}

[[nodiscard]] bool source_codec_row_is_dependency_gated(const SourceImageAudioCodecReviewRow& row) noexcept {
    return row.dependency_gate_required && !row.dependency_legal_evidence;
}

[[nodiscard]] bool
source_codec_row_has_exact_dependency_ids(const SourceImageAudioCodecReviewRow& row,
                                          std::initializer_list<std::string_view> expected_ids) noexcept {
    if (row.dependency_ids.size() != expected_ids.size()) {
        return false;
    }
    return std::ranges::all_of(expected_ids, [&row](std::string_view expected_id) {
        return has_exact_dependency_id(row.dependency_ids, expected_id);
    });
}

[[nodiscard]] bool
source_codec_row_has_required_dependency_evidence(const SourceImageAudioCodecReviewRow& row) noexcept {
    if (!row.dependency_legal_evidence || row.license_ids.empty()) {
        return false;
    }
    if (source_codec_feature_is_png(row.feature)) {
        return source_codec_row_has_exact_dependency_ids(row, {"vcpkg.libspng"});
    }
    if (source_codec_feature_is_audio(row.feature)) {
        return source_codec_row_has_exact_dependency_ids(row, {"vcpkg.miniaudio"});
    }
    if (source_codec_feature_uses_both_dependencies(row.feature)) {
        return source_codec_row_has_exact_dependency_ids(row, {"vcpkg.libspng", "vcpkg.miniaudio"});
    }
    return false;
}

[[nodiscard]] bool
source_codec_row_extensions_are_selected(const SourceImageAudioCodecReviewRow& row,
                                         std::initializer_list<std::string_view> selected_extensions) noexcept {
    return !row.declared_extensions.empty() &&
           std::ranges::all_of(row.declared_extensions, [selected_extensions](const std::string& extension) {
               return std::ranges::find(selected_extensions, std::string_view{extension}) != selected_extensions.end();
           });
}

[[nodiscard]] bool source_codec_row_requests_svg_vector(const SourceImageAudioCodecReviewRow& row) noexcept {
    if (row.request_svg_vector_codec) {
        return true;
    }
    return std::ranges::any_of(row.declared_extensions,
                               [](const std::string& extension) { return std::string_view{extension} == ".svg"; });
}

[[nodiscard]] bool source_codec_row_requests_broad_image_codec(const SourceImageAudioCodecReviewRow& row) noexcept {
    if (row.request_broad_image_codec) {
        return true;
    }
    return source_codec_feature_is_png(row.feature) &&
           std::ranges::any_of(row.declared_extensions, [](const std::string& extension) {
               return std::string_view{extension} != ".png" && std::string_view{extension} != ".svg";
           });
}

[[nodiscard]] bool source_codec_row_requests_broad_audio_codec(const SourceImageAudioCodecReviewRow& row) noexcept {
    if (row.request_broad_audio_codec) {
        return true;
    }
    return source_codec_feature_is_audio(row.feature) &&
           !source_codec_row_extensions_are_selected(row, {".wav", ".flac", ".mp3"});
}

[[nodiscard]] bool source_codec_row_has_unsupported_claim(const SourceImageAudioCodecReviewRow& row) noexcept {
    return source_codec_row_requests_svg_vector(row) || source_codec_row_requests_broad_image_codec(row) ||
           source_codec_row_requests_broad_audio_codec(row) || row.request_background_decode_streaming ||
           row.request_hrtf_middleware || row.request_runtime_source_parsing || row.request_native_handle_access ||
           row.request_package_mutation;
}

[[nodiscard]] bool source_codec_row_base_is_valid(const SourceImageAudioCodecReviewRow& row) noexcept {
    return source_codec_valid_feature(row.feature) && valid_token(row.row_id) && !contains_unsafe_token(row.row_id) &&
           valid_token(row.source_root) && !contains_unsafe_token(row.source_root) && valid_token(row.importer_id) &&
           !contains_unsafe_token(row.importer_id) && valid_token_list(row.declared_extensions) &&
           valid_token_list(row.dependency_ids) && valid_token_list(row.license_ids) &&
           valid_token_list(row.provenance_ids) && valid_token_list(row.package_output_rows) &&
           valid_token(row.deterministic_content_hash) && !contains_unsafe_token(row.deterministic_content_hash) &&
           valid_optional_token(row.image_pixel_format) && valid_optional_token(row.audio_sample_format);
}

void append_source_codec_diagnostic(std::vector<SourceImageAudioCodecReviewDiagnostic>& diagnostics,
                                    SourceImageAudioCodecReviewDiagnosticCode code,
                                    const SourceImageAudioCodecReviewRow& row, std::string message) {
    diagnostics.push_back(SourceImageAudioCodecReviewDiagnostic{
        .code = code,
        .feature = row.feature,
        .row_id = row.row_id,
        .message = std::move(message),
        .source_index = row.source_index,
    });
}

void append_source_codec_diagnostic(std::vector<SourceImageAudioCodecReviewDiagnostic>& diagnostics,
                                    SourceImageAudioCodecReviewDiagnosticCode code,
                                    SourceImageAudioCodecReviewFeature feature, std::string message) {
    diagnostics.push_back(SourceImageAudioCodecReviewDiagnostic{
        .code = code,
        .feature = feature,
        .row_id = {},
        .message = std::move(message),
        .source_index = 0U,
    });
}

[[nodiscard]] bool source_codec_row_has_selected_png_source(const SourceImageAudioCodecReviewRow& row) noexcept {
    return row.source_root_evidence && source_codec_row_extensions_are_selected(row, {".png"}) &&
           valid_token(row.source_root) && valid_token(row.importer_id);
}

[[nodiscard]] bool source_codec_row_has_selected_audio_source(const SourceImageAudioCodecReviewRow& row) noexcept {
    return row.source_root_evidence && source_codec_row_extensions_are_selected(row, {".wav", ".flac", ".mp3"}) &&
           valid_token(row.source_root) && valid_token(row.importer_id);
}

[[nodiscard]] bool source_codec_row_has_hash(const SourceImageAudioCodecReviewRow& row) noexcept {
    return row.deterministic_hash_evidence && valid_token(row.deterministic_content_hash) &&
           !contains_unsafe_token(row.deterministic_content_hash);
}

[[nodiscard]] bool source_codec_row_is_ready(const SourceImageAudioCodecReviewRow& row) noexcept {
    if (source_codec_row_has_unsupported_claim(row) || source_codec_row_is_dependency_gated(row) || !row.reviewed ||
        !source_codec_row_has_required_dependency_evidence(row) || !source_codec_row_has_hash(row)) {
        return false;
    }

    switch (row.feature) {
    case SourceImageAudioCodecReviewFeature::png_decode_adapter:
        return source_codec_row_has_selected_png_source(row) && row.decode_adapter_evidence;
    case SourceImageAudioCodecReviewFeature::png_pixel_format:
        return source_codec_row_has_selected_png_source(row) && row.pixel_format_evidence &&
               std::string_view{row.image_pixel_format} == "rgba8_unorm" && row.image_width > 0U &&
               row.image_height > 0U;
    case SourceImageAudioCodecReviewFeature::audio_decode_adapter:
        return source_codec_row_has_selected_audio_source(row) && row.decode_adapter_evidence;
    case SourceImageAudioCodecReviewFeature::audio_sample_format:
        return source_codec_row_has_selected_audio_source(row) && row.sample_format_evidence &&
               std::string_view{row.audio_sample_format} == "float32" && row.audio_channels > 0U &&
               row.audio_sample_rate > 0U && row.audio_frame_count > 0U;
    case SourceImageAudioCodecReviewFeature::source_provenance:
        return row.source_root_evidence && row.source_provenance_evidence && !row.provenance_ids.empty() &&
               !row.license_ids.empty();
    case SourceImageAudioCodecReviewFeature::package_output:
        return row.package_output_evidence && !row.package_output_rows.empty();
    }
    return false;
}

[[nodiscard]] std::uint64_t hash_source_codec_row(std::uint64_t hash,
                                                  const SourceImageAudioCodecReviewRow& row) noexcept {
    hash = hash_byte(hash, static_cast<std::uint8_t>(row.feature));
    hash = hash_string(hash, row.row_id);
    hash = hash_string(hash, row.source_root);
    hash = hash_string(hash, row.importer_id);
    hash = hash_string_list(hash, row.declared_extensions);
    hash = hash_string_list(hash, row.dependency_ids);
    hash = hash_string_list(hash, row.license_ids);
    hash = hash_string_list(hash, row.provenance_ids);
    hash = hash_string_list(hash, row.package_output_rows);
    hash = hash_string(hash, row.deterministic_content_hash);
    hash = hash_string(hash, row.image_pixel_format);
    hash = hash_u32(hash, row.image_width);
    hash = hash_u32(hash, row.image_height);
    hash = hash_string(hash, row.audio_sample_format);
    hash = hash_u32(hash, row.audio_channels);
    hash = hash_u32(hash, row.audio_sample_rate);
    hash = hash_u32(hash, row.audio_frame_count);
    hash = hash_bool(hash, row.reviewed);
    hash = hash_bool(hash, row.source_root_evidence);
    hash = hash_bool(hash, row.decode_adapter_evidence);
    hash = hash_bool(hash, row.pixel_format_evidence);
    hash = hash_bool(hash, row.sample_format_evidence);
    hash = hash_bool(hash, row.source_provenance_evidence);
    hash = hash_bool(hash, row.package_output_evidence);
    hash = hash_bool(hash, row.deterministic_hash_evidence);
    hash = hash_bool(hash, row.dependency_legal_evidence);
    hash = hash_bool(hash, row.dependency_gate_required);
    hash = hash_bool(hash, row.request_svg_vector_codec);
    hash = hash_bool(hash, row.request_broad_image_codec);
    hash = hash_bool(hash, row.request_broad_audio_codec);
    hash = hash_bool(hash, row.request_background_decode_streaming);
    hash = hash_bool(hash, row.request_hrtf_middleware);
    hash = hash_bool(hash, row.request_runtime_source_parsing);
    hash = hash_bool(hash, row.request_native_handle_access);
    hash = hash_bool(hash, row.request_package_mutation);
    hash = hash_u32(hash, row.source_index);
    return hash;
}

[[nodiscard]] std::uint64_t build_source_codec_replay_hash(const SourceImageAudioCodecReviewRequest& request) noexcept {
    auto hash = fnv_offset;
    hash ^= request.seed;
    hash *= fnv_prime;
    for (const auto feature : request.required_features) {
        hash = hash_byte(hash, static_cast<std::uint8_t>(feature));
    }
    for (const auto& row : request.rows) {
        hash = hash_source_codec_row(hash, row);
    }
    return hash == 0U ? fnv_offset : hash;
}

[[nodiscard]] bool has_ready_source_codec_feature(const std::vector<SourceImageAudioCodecReviewRow>& rows,
                                                  SourceImageAudioCodecReviewFeature feature) noexcept {
    return std::ranges::any_of(rows, [feature](const SourceImageAudioCodecReviewRow& row) {
        return row.feature == feature && source_codec_row_is_ready(row);
    });
}

} // namespace

bool AssetImportProductionReview::succeeded() const noexcept {
    return status == AssetImportProductionStatus::ready || status == AssetImportProductionStatus::no_rows;
}

bool KtxBasisTextureReview::succeeded() const noexcept {
    return status == KtxBasisTextureReviewStatus::ready || status == KtxBasisTextureReviewStatus::no_rows;
}

bool GltfSceneImportReview::succeeded() const noexcept {
    return status == GltfSceneImportReviewStatus::ready || status == GltfSceneImportReviewStatus::no_rows;
}

bool SourceImageAudioCodecReview::succeeded() const noexcept {
    return status == SourceImageAudioCodecReviewStatus::ready || status == SourceImageAudioCodecReviewStatus::no_rows;
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

KtxBasisTextureReview review_ktx_basis_texture_readiness(const KtxBasisTextureReviewRequest& request) {
    KtxBasisTextureReview review;
    review.required_features = request.required_features;
    review.row_count = request.rows.size();

    if (request.rows.empty() && request.required_features.empty()) {
        review.status = KtxBasisTextureReviewStatus::no_rows;
        return review;
    }

    if (request.rows.size() > request.row_budget) {
        append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::row_budget_exceeded,
                              KtxBasisTextureReviewFeature::container_validation,
                              "KTX2/Basis texture review row budget is exceeded");
    }

    for (std::size_t index = 0; index < request.required_features.size(); ++index) {
        const auto feature = request.required_features[index];
        if (!ktx_valid_feature(feature)) {
            append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::invalid_required_feature,
                                  feature, "required KTX2/Basis review feature is invalid");
            continue;
        }
        for (std::size_t other = index + 1U; other < request.required_features.size(); ++other) {
            if (feature == request.required_features[other]) {
                append_ktx_diagnostic(review.diagnostics,
                                      KtxBasisTextureReviewDiagnosticCode::duplicate_required_feature, feature,
                                      "required KTX2/Basis review feature is duplicated");
                break;
            }
        }
    }

    for (const auto feature : request.required_features) {
        if (ktx_valid_feature(feature) &&
            std::ranges::none_of(request.rows,
                                 [feature](const KtxBasisTextureReviewRow& row) { return row.feature == feature; })) {
            append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::missing_required_feature_row,
                                  feature, "required KTX2/Basis review feature row is missing");
        }
    }

    for (std::size_t index = 0; index < request.rows.size(); ++index) {
        const auto& row = request.rows[index];
        if (ktx_row_is_host_gated(row)) {
            ++review.host_gated_row_count;
        } else if (ktx_row_is_dependency_gated(row)) {
            ++review.dependency_gated_row_count;
        } else if (ktx_row_has_unsupported_claim(row)) {
            ++review.unsupported_claim_row_count;
        }

        switch (row.feature) {
        case KtxBasisTextureReviewFeature::container_validation:
            ++review.container_validation_rows;
            break;
        case KtxBasisTextureReviewFeature::supercompression_policy:
            ++review.supercompression_policy_rows;
            break;
        case KtxBasisTextureReviewFeature::transcode_target_policy:
            ++review.transcode_target_policy_rows;
            break;
        case KtxBasisTextureReviewFeature::gpu_target_compatibility:
            ++review.gpu_target_compatibility_rows;
            break;
        case KtxBasisTextureReviewFeature::source_provenance:
            ++review.source_provenance_rows;
            break;
        case KtxBasisTextureReviewFeature::package_output:
            ++review.package_output_rows;
            break;
        case KtxBasisTextureReviewFeature::host_tool_gate:
            ++review.host_tool_gate_rows;
            break;
        }

        if (!ktx_row_base_is_valid(row)) {
            append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::invalid_row, row,
                                  "KTX2/Basis texture review row is invalid");
        }
        for (std::size_t other = index + 1U; other < request.rows.size(); ++other) {
            const auto& other_row = request.rows[other];
            if (row.feature == other_row.feature) {
                append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::duplicate_feature_row,
                                      row, "KTX2/Basis texture review row is duplicated");
                break;
            }
        }

        if (row.request_runtime_transcoding) {
            append_ktx_diagnostic(review.diagnostics,
                                  KtxBasisTextureReviewDiagnosticCode::unsupported_runtime_transcoding, row,
                                  "runtime KTX2/Basis transcoding is unsupported in the reviewed cook lane");
        }
        if (row.request_gpu_upload) {
            append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::unsupported_gpu_upload, row,
                                  "KTX2/Basis GPU upload is unsupported in the reviewed cook lane");
        }
        if (row.request_compression_execution) {
            append_ktx_diagnostic(review.diagnostics,
                                  KtxBasisTextureReviewDiagnosticCode::unsupported_compression_execution, row,
                                  "KTX2/Basis compression tool execution requires a separate host-evidence lane");
        }
        if (row.request_native_handle_access) {
            append_ktx_diagnostic(review.diagnostics,
                                  KtxBasisTextureReviewDiagnosticCode::unsupported_native_handle_claim, row,
                                  "native texture handles are unsupported in KTX2/Basis review rows");
        }
        if (row.request_broad_texture_codec_claim) {
            append_ktx_diagnostic(review.diagnostics,
                                  KtxBasisTextureReviewDiagnosticCode::unsupported_broad_texture_codec_claim, row,
                                  "broad texture codec readiness requires explicit reviewed evidence");
        }

        if (!row.reviewed) {
            append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::invalid_row, row,
                                  "KTX2/Basis texture review evidence is missing");
        }

        if (ktx_row_is_host_gated(row)) {
            continue;
        }

        if (ktx_feature_requires_dependency_legal_record(row.feature) && !ktx_row_is_dependency_gated(row) &&
            !ktx_row_has_required_dependency_evidence(row)) {
            append_ktx_diagnostic(review.diagnostics,
                                  KtxBasisTextureReviewDiagnosticCode::missing_dependency_legal_record, row,
                                  "KTX2/Basis dependency and legal evidence is missing");
        }

        switch (row.feature) {
        case KtxBasisTextureReviewFeature::container_validation:
            if (!row.container_validation_evidence || row.container_validator_ids.empty()) {
                append_ktx_diagnostic(review.diagnostics,
                                      KtxBasisTextureReviewDiagnosticCode::missing_container_validation, row,
                                      "KTX2 container validation evidence is missing");
            }
            break;
        case KtxBasisTextureReviewFeature::supercompression_policy:
            if (!row.supercompression_policy_evidence || row.supercompression_scheme.empty()) {
                append_ktx_diagnostic(review.diagnostics,
                                      KtxBasisTextureReviewDiagnosticCode::missing_supercompression_policy, row,
                                      "Basis supercompression policy evidence is missing");
            }
            break;
        case KtxBasisTextureReviewFeature::transcode_target_policy:
            if (!row.transcode_target_evidence || row.transcode_policy.empty() || row.transcode_target.empty()) {
                append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::missing_transcode_target,
                                      row, "KTX2/Basis transcode target policy evidence is missing");
            }
            break;
        case KtxBasisTextureReviewFeature::gpu_target_compatibility:
            if (!row.gpu_target_compatibility_evidence || row.gpu_target.empty()) {
                append_ktx_diagnostic(review.diagnostics,
                                      KtxBasisTextureReviewDiagnosticCode::missing_gpu_target_compatibility, row,
                                      "GPU target compatibility evidence is missing");
            }
            break;
        case KtxBasisTextureReviewFeature::source_provenance:
            if (!row.source_provenance_evidence || row.source_provenance_ids.empty() || row.license_ids.empty() ||
                row.deterministic_content_hash.empty()) {
                append_ktx_diagnostic(review.diagnostics,
                                      KtxBasisTextureReviewDiagnosticCode::missing_source_provenance, row,
                                      "KTX2/Basis source provenance evidence is missing");
            }
            break;
        case KtxBasisTextureReviewFeature::package_output:
            if (!row.package_output_evidence || row.package_output_rows.empty() ||
                row.deterministic_content_hash.empty()) {
                append_ktx_diagnostic(review.diagnostics, KtxBasisTextureReviewDiagnosticCode::missing_package_output,
                                      row, "KTX2/Basis package output evidence is missing");
            }
            break;
        case KtxBasisTextureReviewFeature::host_tool_gate:
            if (!row.host_tool_validated && !row.host_tool_gate_required) {
                append_ktx_diagnostic(review.diagnostics,
                                      KtxBasisTextureReviewDiagnosticCode::missing_host_tool_evidence, row,
                                      "KTX2/Basis host tool evidence is missing");
            }
            break;
        }
    }

    if (!review.diagnostics.empty()) {
        review.status = KtxBasisTextureReviewStatus::invalid_request;
        return review;
    }

    review.rows = request.rows;
    for (const auto& row : review.rows) {
        if (ktx_row_is_ready(row)) {
            ++review.ready_row_count;
        }
    }

    review.container_validation_ready =
        has_ready_ktx_feature(review.rows, KtxBasisTextureReviewFeature::container_validation);
    review.supercompression_policy_ready =
        has_ready_ktx_feature(review.rows, KtxBasisTextureReviewFeature::supercompression_policy);
    review.transcode_target_policy_ready =
        has_ready_ktx_feature(review.rows, KtxBasisTextureReviewFeature::transcode_target_policy);
    review.gpu_target_compatibility_ready =
        has_ready_ktx_feature(review.rows, KtxBasisTextureReviewFeature::gpu_target_compatibility);
    review.source_provenance_ready =
        has_ready_ktx_feature(review.rows, KtxBasisTextureReviewFeature::source_provenance);
    review.package_output_ready = has_ready_ktx_feature(review.rows, KtxBasisTextureReviewFeature::package_output);
    review.dependency_legal_records_ready = std::ranges::all_of(review.rows, [](const auto& row) {
        return ktx_row_is_host_gated(row) || !ktx_feature_requires_dependency_legal_record(row.feature) ||
               ktx_row_has_required_dependency_evidence(row);
    });
    review.selected_package_evidence_ready =
        review.container_validation_ready && review.supercompression_policy_ready &&
        review.transcode_target_policy_ready && review.gpu_target_compatibility_ready &&
        review.source_provenance_ready && review.package_output_ready && review.dependency_legal_records_ready &&
        review.unsupported_claim_row_count == 0U && review.dependency_gated_row_count == 0U;
    review.ktx_basis_review_ready = review.selected_package_evidence_ready;
    review.broad_texture_codec_ready = false;
    review.replay_hash = build_ktx_replay_hash(request);

    if (review.host_gated_row_count > 0U) {
        review.status = KtxBasisTextureReviewStatus::host_evidence_required;
    } else if (review.dependency_gated_row_count > 0U) {
        review.status = KtxBasisTextureReviewStatus::dependency_evidence_required;
    } else {
        review.status = KtxBasisTextureReviewStatus::ready;
    }
    return review;
}

GltfSceneImportReview review_gltf_scene_import_readiness(const GltfSceneImportReviewRequest& request) {
    GltfSceneImportReview review;
    review.required_features = request.required_features;
    review.row_count = request.rows.size();

    if (request.rows.empty() && request.required_features.empty()) {
        review.status = GltfSceneImportReviewStatus::no_rows;
        return review;
    }

    if (request.rows.size() > request.row_budget) {
        append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::row_budget_exceeded,
                               GltfSceneImportReviewFeature::source_root_policy,
                               "glTF scene import review row budget is exceeded");
    }

    for (std::size_t index = 0; index < request.required_features.size(); ++index) {
        const auto feature = request.required_features[index];
        if (!gltf_valid_feature(feature)) {
            append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::invalid_required_feature,
                                   feature, "required glTF scene import review feature is invalid");
            continue;
        }
        for (std::size_t other = index + 1U; other < request.required_features.size(); ++other) {
            if (feature == request.required_features[other]) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::duplicate_required_feature, feature,
                                       "required glTF scene import review feature is duplicated");
                break;
            }
        }
    }

    for (const auto feature : request.required_features) {
        if (gltf_valid_feature(feature) &&
            std::ranges::none_of(request.rows,
                                 [feature](const GltfSceneImportReviewRow& row) { return row.feature == feature; })) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::missing_required_feature_row, feature,
                                   "required glTF scene import review feature row is missing");
        }
    }

    for (std::size_t index = 0; index < request.rows.size(); ++index) {
        const auto& row = request.rows[index];
        if (gltf_row_is_dependency_gated(row)) {
            ++review.dependency_gated_row_count;
        } else if (gltf_row_has_unsupported_claim(row)) {
            ++review.unsupported_claim_row_count;
        }

        switch (row.feature) {
        case GltfSceneImportReviewFeature::source_root_policy:
            ++review.source_root_rows;
            break;
        case GltfSceneImportReviewFeature::parser_validation:
            ++review.parser_validation_rows;
            break;
        case GltfSceneImportReviewFeature::geometry_payload:
            ++review.geometry_payload_rows;
            break;
        case GltfSceneImportReviewFeature::material_payload:
            ++review.material_payload_rows;
            break;
        case GltfSceneImportReviewFeature::animation_payload:
            ++review.animation_payload_rows;
            break;
        case GltfSceneImportReviewFeature::external_resource_policy:
            ++review.external_resource_policy_rows;
            break;
        case GltfSceneImportReviewFeature::source_provenance:
            ++review.source_provenance_rows;
            break;
        case GltfSceneImportReviewFeature::package_output:
            ++review.package_output_rows;
            break;
        }

        if (!gltf_row_base_is_valid(row)) {
            append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::invalid_row, row,
                                   "glTF scene import review row is invalid");
        }
        for (std::size_t other = index + 1U; other < request.rows.size(); ++other) {
            const auto& other_row = request.rows[other];
            if (row.feature == other_row.feature) {
                append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::duplicate_feature_row,
                                       row, "glTF scene import review row is duplicated");
                break;
            }
        }

        if (gltf_row_requests_arbitrary_extension(row)) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_arbitrary_extension, row,
                                   "arbitrary glTF source extensions are unsupported");
        }
        if (row.request_external_network_fetch) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_external_network_fetch, row,
                                   "external network fetches are unsupported for glTF scene import review rows");
        }
        if (row.request_runtime_source_parsing) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_runtime_source_parsing, row,
                                   "runtime glTF source parsing is unsupported");
        }
        if (row.request_parser_type_access) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_parser_type_leakage, row,
                                   "glTF parser type access is unsupported in public review rows");
        }
        if (row.request_native_handle_access) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_native_handle_claim, row,
                                   "native handle access is unsupported in glTF scene import review rows");
        }
        if (row.request_broad_scene_import_claim) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_broad_scene_import_claim, row,
                                   "broad scene import readiness requires separate reviewed evidence");
        }
        if (row.request_package_mutation) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::unsupported_package_mutation, row,
                                   "package mutation is unsupported in glTF scene import review rows");
        }

        if (!row.reviewed) {
            append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::invalid_row, row,
                                   "glTF scene import review evidence is missing");
        }

        if (!gltf_row_is_dependency_gated(row) && !gltf_row_has_required_dependency_evidence(row)) {
            append_gltf_diagnostic(review.diagnostics,
                                   GltfSceneImportReviewDiagnosticCode::missing_dependency_legal_record, row,
                                   "glTF scene import dependency and legal evidence is missing");
        }

        switch (row.feature) {
        case GltfSceneImportReviewFeature::source_root_policy:
            if (!row.source_root_evidence || row.source_root.empty() || row.declared_extensions.empty()) {
                append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::missing_source_root,
                                       row, "glTF source-root evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::parser_validation:
            if (!row.parser_validation_evidence || row.importer_id.empty() || row.validator_ids.empty()) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::missing_parser_validation, row,
                                       "glTF parser validation evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::geometry_payload:
            if (!row.geometry_payload_evidence || row.deterministic_content_hash.empty()) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::missing_geometry_payload, row,
                                       "glTF geometry payload evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::material_payload:
            if (!row.material_payload_evidence || row.deterministic_content_hash.empty()) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::missing_material_payload, row,
                                       "glTF material payload evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::animation_payload:
            if (!row.animation_payload_evidence || row.deterministic_content_hash.empty()) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::missing_animation_payload, row,
                                       "glTF animation payload evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::external_resource_policy:
            if (!row.external_resource_policy_evidence || row.external_resource_policy.empty()) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::missing_external_resource_policy, row,
                                       "glTF external resource policy evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::source_provenance:
            if (!row.source_provenance_evidence || row.provenance_ids.empty() || row.license_ids.empty() ||
                row.deterministic_content_hash.empty()) {
                append_gltf_diagnostic(review.diagnostics,
                                       GltfSceneImportReviewDiagnosticCode::missing_source_provenance, row,
                                       "glTF source provenance evidence is missing");
            }
            break;
        case GltfSceneImportReviewFeature::package_output:
            if (!row.package_output_evidence || row.package_output_rows.empty() ||
                row.deterministic_content_hash.empty()) {
                append_gltf_diagnostic(review.diagnostics, GltfSceneImportReviewDiagnosticCode::missing_package_output,
                                       row, "glTF package output evidence is missing");
            }
            break;
        }
    }

    if (!review.diagnostics.empty()) {
        review.status = GltfSceneImportReviewStatus::invalid_request;
        return review;
    }

    review.rows = request.rows;
    for (const auto& row : review.rows) {
        if (gltf_row_is_ready(row)) {
            ++review.ready_row_count;
        }
    }

    review.source_root_ready = has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::source_root_policy);
    review.parser_validation_ready =
        has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::parser_validation);
    review.geometry_payload_ready = has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::geometry_payload);
    review.material_payload_ready = has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::material_payload);
    review.animation_payload_ready =
        has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::animation_payload);
    review.external_resource_policy_ready =
        has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::external_resource_policy);
    review.source_provenance_ready =
        has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::source_provenance);
    review.package_output_ready = has_ready_gltf_feature(review.rows, GltfSceneImportReviewFeature::package_output);
    review.dependency_legal_records_ready = std::ranges::all_of(
        review.rows, [](const auto& row) { return gltf_row_has_required_dependency_evidence(row); });
    review.selected_package_evidence_ready =
        review.source_root_ready && review.parser_validation_ready && review.geometry_payload_ready &&
        review.material_payload_ready && review.animation_payload_ready && review.external_resource_policy_ready &&
        review.source_provenance_ready && review.package_output_ready && review.dependency_legal_records_ready &&
        review.unsupported_claim_row_count == 0U && review.dependency_gated_row_count == 0U;
    review.gltf_scene_import_ready = review.selected_package_evidence_ready;
    review.broad_scene_import_ready = false;
    review.replay_hash = build_gltf_replay_hash(request);

    if (review.dependency_gated_row_count > 0U) {
        review.status = GltfSceneImportReviewStatus::dependency_evidence_required;
    } else {
        review.status = GltfSceneImportReviewStatus::ready;
    }
    return review;
}

SourceImageAudioCodecReview
review_source_image_audio_codec_readiness(const SourceImageAudioCodecReviewRequest& request) {
    SourceImageAudioCodecReview review;
    review.required_features = request.required_features;
    review.row_count = request.rows.size();

    if (request.rows.empty() && request.required_features.empty()) {
        review.status = SourceImageAudioCodecReviewStatus::no_rows;
        return review;
    }

    if (request.rows.size() > request.row_budget) {
        append_source_codec_diagnostic(review.diagnostics,
                                       SourceImageAudioCodecReviewDiagnosticCode::row_budget_exceeded,
                                       SourceImageAudioCodecReviewFeature::png_decode_adapter,
                                       "source image/audio codec review row budget is exceeded");
    }

    for (std::size_t index = 0; index < request.required_features.size(); ++index) {
        const auto feature = request.required_features[index];
        if (!source_codec_valid_feature(feature)) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::invalid_required_feature, feature,
                                           "required source image/audio codec review feature is invalid");
            continue;
        }
        for (std::size_t other = index + 1U; other < request.required_features.size(); ++other) {
            if (feature == request.required_features[other]) {
                append_source_codec_diagnostic(
                    review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::duplicate_required_feature, feature,
                    "required source image/audio codec review feature is duplicated");
                break;
            }
        }
    }

    for (const auto feature : request.required_features) {
        if (source_codec_valid_feature(feature) &&
            std::ranges::none_of(request.rows, [feature](const SourceImageAudioCodecReviewRow& row) {
                return row.feature == feature;
            })) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::missing_required_feature_row,
                                           feature, "required source image/audio codec review feature row is missing");
        }
    }

    for (std::size_t index = 0; index < request.rows.size(); ++index) {
        const auto& row = request.rows[index];
        if (source_codec_row_is_dependency_gated(row)) {
            ++review.dependency_gated_row_count;
        } else if (source_codec_row_has_unsupported_claim(row)) {
            ++review.unsupported_claim_row_count;
        }

        switch (row.feature) {
        case SourceImageAudioCodecReviewFeature::png_decode_adapter:
            ++review.png_decode_rows;
            break;
        case SourceImageAudioCodecReviewFeature::png_pixel_format:
            ++review.png_pixel_format_rows;
            break;
        case SourceImageAudioCodecReviewFeature::audio_decode_adapter:
            ++review.audio_decode_rows;
            break;
        case SourceImageAudioCodecReviewFeature::audio_sample_format:
            ++review.audio_sample_format_rows;
            break;
        case SourceImageAudioCodecReviewFeature::source_provenance:
            ++review.source_provenance_rows;
            break;
        case SourceImageAudioCodecReviewFeature::package_output:
            ++review.package_output_rows;
            break;
        }

        if (!source_codec_row_base_is_valid(row)) {
            append_source_codec_diagnostic(review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::invalid_row,
                                           row, "source image/audio codec review row is invalid");
        }
        for (std::size_t other = index + 1U; other < request.rows.size(); ++other) {
            const auto& other_row = request.rows[other];
            if (row.feature == other_row.feature) {
                append_source_codec_diagnostic(review.diagnostics,
                                               SourceImageAudioCodecReviewDiagnosticCode::duplicate_feature_row, row,
                                               "source image/audio codec review row is duplicated");
                break;
            }
        }

        if (source_codec_row_requests_svg_vector(row)) {
            append_source_codec_diagnostic(
                review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::unsupported_svg_vector_codec, row,
                "SVG/vector source decoding is unsupported in source image/audio codec review rows");
        }
        if (source_codec_row_requests_broad_image_codec(row)) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::unsupported_broad_image_codec,
                                           row, "broad image codec readiness requires a separate reviewed lane");
        }
        if (source_codec_row_requests_broad_audio_codec(row)) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::unsupported_broad_audio_codec,
                                           row, "broad audio codec readiness requires a separate reviewed lane");
        }
        if (row.request_background_decode_streaming) {
            append_source_codec_diagnostic(
                review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::unsupported_background_decode_streaming,
                row, "background decode streaming is unsupported in source image/audio codec review rows");
        }
        if (row.request_hrtf_middleware) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::unsupported_hrtf_middleware, row,
                                           "HRTF or audio middleware execution requires a separate reviewed lane");
        }
        if (row.request_runtime_source_parsing) {
            append_source_codec_diagnostic(
                review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::unsupported_runtime_source_parsing, row,
                "runtime source parsing is unsupported for source image/audio codec review rows");
        }
        if (row.request_native_handle_access) {
            append_source_codec_diagnostic(
                review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::unsupported_native_handle_claim, row,
                "native handle access is unsupported in source image/audio codec review rows");
        }
        if (row.request_package_mutation) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::unsupported_package_mutation, row,
                                           "package mutation is unsupported in source image/audio codec review rows");
        }

        if (!row.reviewed) {
            append_source_codec_diagnostic(review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::invalid_row,
                                           row, "source image/audio codec review evidence is missing");
        }

        if (!source_codec_row_is_dependency_gated(row) && !source_codec_row_has_required_dependency_evidence(row)) {
            append_source_codec_diagnostic(review.diagnostics,
                                           SourceImageAudioCodecReviewDiagnosticCode::missing_dependency_legal_record,
                                           row, "source image/audio codec dependency and legal evidence is missing");
        }

        switch (row.feature) {
        case SourceImageAudioCodecReviewFeature::png_decode_adapter:
            if (!source_codec_row_has_selected_png_source(row) || !row.decode_adapter_evidence) {
                append_source_codec_diagnostic(review.diagnostics,
                                               SourceImageAudioCodecReviewDiagnosticCode::missing_decode_adapter, row,
                                               "selected PNG decode adapter evidence is missing");
            }
            break;
        case SourceImageAudioCodecReviewFeature::png_pixel_format:
            if (!source_codec_row_has_selected_png_source(row) || !row.pixel_format_evidence ||
                std::string_view{row.image_pixel_format} != "rgba8_unorm" || row.image_width == 0U ||
                row.image_height == 0U) {
                append_source_codec_diagnostic(
                    review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::missing_pixel_format_diagnostics,
                    row, "selected PNG RGBA8 pixel-format evidence is missing");
            }
            break;
        case SourceImageAudioCodecReviewFeature::audio_decode_adapter:
            if (!source_codec_row_has_selected_audio_source(row) || !row.decode_adapter_evidence) {
                append_source_codec_diagnostic(review.diagnostics,
                                               SourceImageAudioCodecReviewDiagnosticCode::missing_decode_adapter, row,
                                               "selected audio decode adapter evidence is missing");
            }
            break;
        case SourceImageAudioCodecReviewFeature::audio_sample_format:
            if (!source_codec_row_has_selected_audio_source(row) || !row.sample_format_evidence ||
                std::string_view{row.audio_sample_format} != "float32" || row.audio_channels == 0U ||
                row.audio_sample_rate == 0U || row.audio_frame_count == 0U) {
                append_source_codec_diagnostic(
                    review.diagnostics, SourceImageAudioCodecReviewDiagnosticCode::missing_sample_format_diagnostics,
                    row, "selected audio float32 sample-format evidence is missing");
            }
            break;
        case SourceImageAudioCodecReviewFeature::source_provenance:
            if (!row.source_root_evidence || !row.source_provenance_evidence || row.provenance_ids.empty() ||
                row.license_ids.empty() || !source_codec_row_has_hash(row)) {
                append_source_codec_diagnostic(review.diagnostics,
                                               SourceImageAudioCodecReviewDiagnosticCode::missing_source_provenance,
                                               row, "source image/audio codec source provenance evidence is missing");
            }
            break;
        case SourceImageAudioCodecReviewFeature::package_output:
            if (!row.package_output_evidence || row.package_output_rows.empty() || !source_codec_row_has_hash(row)) {
                append_source_codec_diagnostic(review.diagnostics,
                                               SourceImageAudioCodecReviewDiagnosticCode::missing_package_output, row,
                                               "source image/audio codec package output evidence is missing");
            }
            break;
        }
    }

    if (!review.diagnostics.empty()) {
        review.status = SourceImageAudioCodecReviewStatus::invalid_request;
        return review;
    }

    review.rows = request.rows;
    for (const auto& row : review.rows) {
        if (source_codec_row_is_ready(row)) {
            ++review.ready_row_count;
        }
    }

    review.png_decode_ready =
        has_ready_source_codec_feature(review.rows, SourceImageAudioCodecReviewFeature::png_decode_adapter);
    review.png_rgba8_pixel_format_ready =
        has_ready_source_codec_feature(review.rows, SourceImageAudioCodecReviewFeature::png_pixel_format);
    review.audio_decode_ready =
        has_ready_source_codec_feature(review.rows, SourceImageAudioCodecReviewFeature::audio_decode_adapter);
    review.audio_sample_format_ready =
        has_ready_source_codec_feature(review.rows, SourceImageAudioCodecReviewFeature::audio_sample_format);
    review.source_provenance_ready =
        has_ready_source_codec_feature(review.rows, SourceImageAudioCodecReviewFeature::source_provenance);
    review.package_output_ready =
        has_ready_source_codec_feature(review.rows, SourceImageAudioCodecReviewFeature::package_output);
    review.dependency_legal_records_ready = std::ranges::all_of(review.rows, [](const auto& row) {
        return !source_codec_row_is_dependency_gated(row) && source_codec_row_has_required_dependency_evidence(row);
    });
    review.selected_package_evidence_ready =
        review.png_decode_ready && review.png_rgba8_pixel_format_ready && review.audio_decode_ready &&
        review.audio_sample_format_ready && review.source_provenance_ready && review.package_output_ready &&
        review.dependency_legal_records_ready && review.unsupported_claim_row_count == 0U &&
        review.dependency_gated_row_count == 0U;
    review.source_image_audio_codec_ready = review.selected_package_evidence_ready;
    review.broad_image_codec_ready = false;
    review.broad_audio_codec_ready = false;
    review.replay_hash = build_source_codec_replay_hash(request);

    if (review.dependency_gated_row_count > 0U) {
        review.status = SourceImageAudioCodecReviewStatus::dependency_evidence_required;
    } else {
        review.status = SourceImageAudioCodecReviewStatus::ready;
    }
    return review;
}

} // namespace mirakana
