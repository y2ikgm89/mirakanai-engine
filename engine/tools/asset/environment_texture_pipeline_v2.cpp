// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/tools/environment_texture_pipeline_v2.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr std::array<EnvironmentTexturePipelineV2EvidenceKind, 14> kRequiredRows{
    EnvironmentTexturePipelineV2EvidenceKind::openexr_scanline_rgba16f,
    EnvironmentTexturePipelineV2EvidenceKind::openexr_tiled_rgba16f,
    EnvironmentTexturePipelineV2EvidenceKind::openexr_multipart,
    EnvironmentTexturePipelineV2EvidenceKind::openexr_metadata_preservation,
    EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected,
    EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_etc1s_transcode,
    EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_uastc_transcode,
    EnvironmentTexturePipelineV2EvidenceKind::ktx2_mip_level_validation,
    EnvironmentTexturePipelineV2EvidenceKind::ktx2_color_space_metadata,
    EnvironmentTexturePipelineV2EvidenceKind::d3d12_bc7_target,
    EnvironmentTexturePipelineV2EvidenceKind::vulkan_bc7_target,
    EnvironmentTexturePipelineV2EvidenceKind::metal_astc_target,
    EnvironmentTexturePipelineV2EvidenceKind::android_vulkan_astc_target,
    EnvironmentTexturePipelineV2EvidenceKind::runtime_cooked_only_ingest,
};

[[nodiscard]] bool clean_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] std::string artifact_id(std::string_view prefix, EnvironmentTexturePipelineV2EvidenceKind kind) {
    std::string value{prefix};
    value += '.';
    value += environment_texture_pipeline_v2_evidence_kind_name(kind);
    return value;
}

[[nodiscard]] std::string package_counter_id(EnvironmentTexturePipelineV2EvidenceKind kind) {
    if (kind == EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected) {
        return "openexr_deep_image_rejected_with_diagnostic=1";
    }
    std::string value{environment_texture_pipeline_v2_evidence_kind_name(kind)};
    value += "_ready=1";
    return value;
}

[[nodiscard]] std::string_view official_source_id(EnvironmentTexturePipelineV2EvidenceKind kind) noexcept {
    switch (kind) {
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_scanline_rgba16f:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_tiled_rgba16f:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_multipart:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_metadata_preservation:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected:
        return "context7.openexr.technical-introduction";
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_etc1s_transcode:
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_uastc_transcode:
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_mip_level_validation:
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_color_space_metadata:
        return "context7.ktx-software.ktx2-basis";
    case EnvironmentTexturePipelineV2EvidenceKind::d3d12_bc7_target:
    case EnvironmentTexturePipelineV2EvidenceKind::vulkan_bc7_target:
    case EnvironmentTexturePipelineV2EvidenceKind::metal_astc_target:
    case EnvironmentTexturePipelineV2EvidenceKind::android_vulkan_astc_target:
        return "official.backend-format-support";
    case EnvironmentTexturePipelineV2EvidenceKind::runtime_cooked_only_ingest:
        return "project.runtime-cooked-only-ingest";
    }
    return "";
}

void add_diagnostic(EnvironmentTexturePipelineV2Result& result, EnvironmentTexturePipelineV2DiagnosticCode code,
                    std::string message, std::string evidence_id = {}) {
    result.diagnostics.push_back(EnvironmentTexturePipelineV2Diagnostic{
        .code = code,
        .message = std::move(message),
        .evidence_id = std::move(evidence_id),
    });
}

[[nodiscard]] std::size_t row_index(EnvironmentTexturePipelineV2EvidenceKind kind) noexcept {
    return static_cast<std::size_t>(kind);
}

[[nodiscard]] bool required_kind(EnvironmentTexturePipelineV2EvidenceKind kind) noexcept {
    return std::ranges::find(kRequiredRows, kind) != kRequiredRows.end();
}

void count_category(EnvironmentTexturePipelineV2Result& result,
                    EnvironmentTexturePipelineV2EvidenceKind kind) noexcept {
    switch (kind) {
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_scanline_rgba16f:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_tiled_rgba16f:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_multipart:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_metadata_preservation:
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected:
        ++result.openexr_rows;
        return;
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_etc1s_transcode:
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_uastc_transcode:
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_mip_level_validation:
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_color_space_metadata:
        ++result.ktx2_basis_rows;
        return;
    case EnvironmentTexturePipelineV2EvidenceKind::d3d12_bc7_target:
    case EnvironmentTexturePipelineV2EvidenceKind::vulkan_bc7_target:
    case EnvironmentTexturePipelineV2EvidenceKind::metal_astc_target:
    case EnvironmentTexturePipelineV2EvidenceKind::android_vulkan_astc_target:
        ++result.backend_target_rows;
        return;
    case EnvironmentTexturePipelineV2EvidenceKind::runtime_cooked_only_ingest:
        ++result.runtime_rows;
        return;
    }
}

} // namespace

std::string_view
environment_texture_pipeline_v2_evidence_kind_name(EnvironmentTexturePipelineV2EvidenceKind kind) noexcept {
    switch (kind) {
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_scanline_rgba16f:
        return "openexr_scanline_rgba16f";
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_tiled_rgba16f:
        return "openexr_tiled_rgba16f";
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_multipart:
        return "openexr_multipart";
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_metadata_preservation:
        return "openexr_metadata_preservation";
    case EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected:
        return "openexr_deep_image_rejected";
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_etc1s_transcode:
        return "ktx2_basis_etc1s_transcode";
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_uastc_transcode:
        return "ktx2_basis_uastc_transcode";
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_mip_level_validation:
        return "ktx2_mip_level_validation";
    case EnvironmentTexturePipelineV2EvidenceKind::ktx2_color_space_metadata:
        return "ktx2_color_space_metadata";
    case EnvironmentTexturePipelineV2EvidenceKind::d3d12_bc7_target:
        return "d3d12_bc7_target";
    case EnvironmentTexturePipelineV2EvidenceKind::vulkan_bc7_target:
        return "vulkan_bc7_target";
    case EnvironmentTexturePipelineV2EvidenceKind::metal_astc_target:
        return "metal_astc_target";
    case EnvironmentTexturePipelineV2EvidenceKind::android_vulkan_astc_target:
        return "android_vulkan_astc_target";
    case EnvironmentTexturePipelineV2EvidenceKind::runtime_cooked_only_ingest:
        return "runtime_cooked_only_ingest";
    }
    return "unknown";
}

EnvironmentTexturePipelineV2Result
evaluate_environment_texture_pipeline_v2_full(const EnvironmentTexturePipelineV2Desc& desc) {
    EnvironmentTexturePipelineV2Result result;
    result.required_rows = static_cast<std::uint32_t>(kRequiredRows.size());

    if (desc.optional_dependency_feature != "asset-importers" || !desc.openexr_dependency_recorded ||
        !desc.ktx_dependency_recorded || !desc.bootstrap_entrypoint_ready || !desc.build_asset_importers_ready) {
        add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::invalid_dependency_gate,
                       "full OpenEXR/KTX2/Basis pipeline requires reviewed asset-importers dependency gates");
    }
    if (desc.cmake_configure_installs_dependencies) {
        add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::configure_time_dependency_install,
                       "CMake configure must not install or restore optional asset-importers dependencies");
    }

    result.runtime_source_parsing = desc.runtime_source_parsing ? 1U : 0U;
    result.runtime_optional_codec_execution = desc.runtime_optional_codec_execution ? 1U : 0U;
    result.native_handle_access = desc.native_handle_access ? 1U : 0U;
    result.gpu_command_executed = desc.gpu_command_executed ? 1U : 0U;

    if (desc.runtime_source_parsing) {
        add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::runtime_source_parsing,
                       "runtime must ingest cooked payloads only and must not parse OpenEXR/KTX2 source files");
    }
    if (desc.runtime_optional_codec_execution) {
        add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::runtime_optional_codec_execution,
                       "runtime must not execute optional OpenEXR or Basis codec/transcode paths");
    }
    if (desc.native_handle_access) {
        add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::native_handle_access,
                       "asset-pipeline readiness evidence must not expose native handles");
    }
    if (desc.gpu_command_executed) {
        add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::gpu_command_executed,
                       "value-only full asset-pipeline readiness must not execute GPU commands");
    }

    std::array<bool, kRequiredRows.size()> seen{};
    for (const auto& row : desc.evidence_rows) {
        if (!required_kind(row.kind)) {
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::invalid_row,
                           "unknown full asset-pipeline evidence row", row.evidence_id);
            continue;
        }

        const auto index = row_index(row.kind);
        if (seen[index]) {
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::duplicate_row,
                           "duplicate full asset-pipeline evidence row", row.evidence_id);
            continue;
        }
        seen[index] = true;
        count_category(result, row.kind);

        if (row.ready) {
            ++result.ready_rows;
        } else {
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::not_ready_row,
                           "full asset-pipeline evidence row is not ready", row.evidence_id);
        }
        if (row.dependency_gated) {
            ++result.dependency_gated_rows;
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::dependency_gated_row,
                           "full asset-pipeline evidence row is still dependency-gated", row.evidence_id);
        }
        if (row.package_visible) {
            ++result.package_visible_rows;
        }
        if (row.host_validated) {
            ++result.host_validated_rows;
        }

        const auto source_artifact_ok =
            row.source_artifact_validated &&
            row.source_artifact_id == artifact_id("environment.asset_pipeline.source", row.kind);
        const auto cooked_artifact_ok =
            row.cooked_artifact_validated &&
            row.cooked_artifact_id == artifact_id("environment.asset_pipeline.cooked", row.kind);
        const auto package_counter_ok =
            row.package_counter_validated && row.package_counter_id == package_counter_id(row.kind);

        if (source_artifact_ok) {
            ++result.source_artifact_rows;
        }
        if (cooked_artifact_ok) {
            ++result.cooked_artifact_rows;
        }
        if (package_counter_ok) {
            ++result.package_counter_rows;
        }
        if (row.replay_hash_recorded) {
            ++result.replay_hash_rows;
        }
        if (row.kind == EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected &&
            row.rejection_diagnostic_recorded) {
            ++result.rejection_diagnostic_rows;
        }

        const auto deep_rejection_ok =
            row.kind != EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected ||
            row.rejection_diagnostic_recorded;
        const auto unexpected_rejection =
            row.kind != EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected &&
            row.rejection_diagnostic_recorded;

        if (!clean_token(row.evidence_id) || row.official_source_id != official_source_id(row.kind) ||
            !row.package_visible || !row.host_validated || !source_artifact_ok || !cooked_artifact_ok ||
            !package_counter_ok || !row.replay_hash_recorded || !deep_rejection_ok || unexpected_rejection) {
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::invalid_row,
                           "full asset-pipeline evidence row requires exact official source, artifact, package "
                           "counter, replay hash, and deep-rejection diagnostic evidence",
                           row.evidence_id);
        }
        if (row.runtime_source_parsing) {
            ++result.runtime_source_parsing;
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::runtime_source_parsing,
                           "evidence row claims runtime source parsing", row.evidence_id);
        }
        if (row.runtime_optional_codec_execution) {
            ++result.runtime_optional_codec_execution;
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::runtime_optional_codec_execution,
                           "evidence row claims runtime optional codec execution", row.evidence_id);
        }
        if (row.native_handle_access) {
            ++result.native_handle_access;
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::native_handle_access,
                           "evidence row exposes native handles", row.evidence_id);
        }
        if (row.gpu_command_executed) {
            ++result.gpu_command_executed;
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::gpu_command_executed,
                           "evidence row executed GPU commands", row.evidence_id);
        }
    }

    for (const auto kind : kRequiredRows) {
        if (!seen[row_index(kind)]) {
            ++result.missing_required_rows;
            add_diagnostic(result, EnvironmentTexturePipelineV2DiagnosticCode::missing_required_row,
                           "missing required full asset-pipeline evidence row",
                           std::string{environment_texture_pipeline_v2_evidence_kind_name(kind)});
        }
    }

    result.full_ready =
        result.diagnostics.empty() && result.ready_rows == result.required_rows && result.missing_required_rows == 0U &&
        result.dependency_gated_rows == 0U && result.package_visible_rows == result.required_rows &&
        result.host_validated_rows == result.required_rows && result.source_artifact_rows == result.required_rows &&
        result.cooked_artifact_rows == result.required_rows && result.package_counter_rows == result.required_rows &&
        result.replay_hash_rows == result.required_rows && result.rejection_diagnostic_rows == 1U &&
        result.runtime_source_parsing == 0U && result.runtime_optional_codec_execution == 0U &&
        result.native_handle_access == 0U && result.gpu_command_executed == 0U;
    return result;
}

bool has_environment_texture_pipeline_v2_diagnostic(const EnvironmentTexturePipelineV2Result& result,
                                                    EnvironmentTexturePipelineV2DiagnosticCode code) noexcept {
    return std::ranges::any_of(result.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
