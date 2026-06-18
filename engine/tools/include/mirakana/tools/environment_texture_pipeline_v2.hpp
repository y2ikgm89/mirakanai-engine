// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class EnvironmentTexturePipelineV2EvidenceKind : std::uint8_t {
    openexr_scanline_rgba16f,
    openexr_tiled_rgba16f,
    openexr_multipart,
    openexr_metadata_preservation,
    openexr_deep_image_rejected,
    ktx2_basis_etc1s_transcode,
    ktx2_basis_uastc_transcode,
    ktx2_mip_level_validation,
    ktx2_color_space_metadata,
    d3d12_bc7_target,
    vulkan_bc7_target,
    metal_astc_target,
    android_vulkan_astc_target,
    runtime_cooked_only_ingest,
};

enum class EnvironmentTexturePipelineV2DiagnosticCode : std::uint8_t {
    none,
    invalid_dependency_gate,
    configure_time_dependency_install,
    invalid_row,
    duplicate_row,
    missing_required_row,
    not_ready_row,
    dependency_gated_row,
    runtime_source_parsing,
    runtime_optional_codec_execution,
    native_handle_access,
    gpu_command_executed,
};

struct EnvironmentTexturePipelineV2EvidenceRow {
    EnvironmentTexturePipelineV2EvidenceKind kind{EnvironmentTexturePipelineV2EvidenceKind::openexr_scanline_rgba16f};
    std::string evidence_id;
    std::string official_source_id;
    std::string source_artifact_id;
    std::string cooked_artifact_id;
    std::string package_counter_id;
    bool ready{false};
    bool dependency_gated{true};
    bool package_visible{false};
    bool host_validated{false};
    bool source_artifact_validated{false};
    bool cooked_artifact_validated{false};
    bool package_counter_validated{false};
    bool replay_hash_recorded{false};
    bool rejection_diagnostic_recorded{false};
    bool runtime_source_parsing{false};
    bool runtime_optional_codec_execution{false};
    bool native_handle_access{false};
    bool gpu_command_executed{false};
};

struct EnvironmentTexturePipelineV2Desc {
    std::string optional_dependency_feature;
    bool openexr_dependency_recorded{false};
    bool ktx_dependency_recorded{false};
    bool bootstrap_entrypoint_ready{false};
    bool build_asset_importers_ready{false};
    bool cmake_configure_installs_dependencies{true};
    bool runtime_source_parsing{true};
    bool runtime_optional_codec_execution{true};
    bool native_handle_access{true};
    bool gpu_command_executed{true};
    std::vector<EnvironmentTexturePipelineV2EvidenceRow> evidence_rows;
};

struct EnvironmentTexturePipelineV2Diagnostic {
    EnvironmentTexturePipelineV2DiagnosticCode code{EnvironmentTexturePipelineV2DiagnosticCode::none};
    std::string message;
    std::string evidence_id;
};

struct EnvironmentTexturePipelineV2Result {
    bool full_ready{false};
    std::uint32_t required_rows{0};
    std::uint32_t ready_rows{0};
    std::uint32_t missing_required_rows{0};
    std::uint32_t dependency_gated_rows{0};
    std::uint32_t package_visible_rows{0};
    std::uint32_t host_validated_rows{0};
    std::uint32_t source_artifact_rows{0};
    std::uint32_t cooked_artifact_rows{0};
    std::uint32_t package_counter_rows{0};
    std::uint32_t replay_hash_rows{0};
    std::uint32_t rejection_diagnostic_rows{0};
    std::uint32_t openexr_rows{0};
    std::uint32_t ktx2_basis_rows{0};
    std::uint32_t backend_target_rows{0};
    std::uint32_t runtime_rows{0};
    std::uint32_t runtime_source_parsing{0};
    std::uint32_t runtime_optional_codec_execution{0};
    std::uint32_t native_handle_access{0};
    std::uint32_t gpu_command_executed{0};
    std::vector<EnvironmentTexturePipelineV2Diagnostic> diagnostics;
};

[[nodiscard]] std::string_view
environment_texture_pipeline_v2_evidence_kind_name(EnvironmentTexturePipelineV2EvidenceKind kind) noexcept;
[[nodiscard]] EnvironmentTexturePipelineV2Result
evaluate_environment_texture_pipeline_v2_full(const EnvironmentTexturePipelineV2Desc& desc);
[[nodiscard]] bool
has_environment_texture_pipeline_v2_diagnostic(const EnvironmentTexturePipelineV2Result& result,
                                               EnvironmentTexturePipelineV2DiagnosticCode code) noexcept;

} // namespace mirakana
