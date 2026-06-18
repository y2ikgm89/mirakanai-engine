// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/tools/environment_texture_pipeline_v2.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace {

[[nodiscard]] std::string expected_official_source_id(mirakana::EnvironmentTexturePipelineV2EvidenceKind kind) {
    switch (kind) {
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_scanline_rgba16f:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_tiled_rgba16f:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_multipart:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_metadata_preservation:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected:
        return "context7.openexr.technical-introduction";
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_etc1s_transcode:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::ktx2_basis_uastc_transcode:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::ktx2_mip_level_validation:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::ktx2_color_space_metadata:
        return "context7.ktx-software.ktx2-basis";
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::d3d12_bc7_target:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::vulkan_bc7_target:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::metal_astc_target:
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::android_vulkan_astc_target:
        return "official.backend-format-support";
    case mirakana::EnvironmentTexturePipelineV2EvidenceKind::runtime_cooked_only_ingest:
        return "project.runtime-cooked-only-ingest";
    }
    return {};
}

[[nodiscard]] std::string expected_package_counter_id(mirakana::EnvironmentTexturePipelineV2EvidenceKind kind) {
    if (kind == mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected) {
        return "openexr_deep_image_rejected_with_diagnostic=1";
    }
    return std::string{mirakana::environment_texture_pipeline_v2_evidence_kind_name(kind)} + "_ready=1";
}

[[nodiscard]] mirakana::EnvironmentTexturePipelineV2EvidenceRow
ready_row(mirakana::EnvironmentTexturePipelineV2EvidenceKind kind) {
    const auto row_name = std::string{mirakana::environment_texture_pipeline_v2_evidence_kind_name(kind)};
    return mirakana::EnvironmentTexturePipelineV2EvidenceRow{
        .kind = kind,
        .evidence_id = "task9." + row_name,
        .official_source_id = expected_official_source_id(kind),
        .source_artifact_id = "environment.asset_pipeline.source." + row_name,
        .cooked_artifact_id = "environment.asset_pipeline.cooked." + row_name,
        .package_counter_id = expected_package_counter_id(kind),
        .ready = true,
        .dependency_gated = false,
        .package_visible = true,
        .host_validated = true,
        .source_artifact_validated = true,
        .cooked_artifact_validated = true,
        .package_counter_validated = true,
        .replay_hash_recorded = true,
        .rejection_diagnostic_recorded =
            kind == mirakana::EnvironmentTexturePipelineV2EvidenceKind::openexr_deep_image_rejected,
        .runtime_source_parsing = false,
        .runtime_optional_codec_execution = false,
        .native_handle_access = false,
        .gpu_command_executed = false,
    };
}

[[nodiscard]] mirakana::EnvironmentTexturePipelineV2Desc make_ready_pipeline_desc() {
    using Kind = mirakana::EnvironmentTexturePipelineV2EvidenceKind;

    return mirakana::EnvironmentTexturePipelineV2Desc{
        .optional_dependency_feature = "asset-importers",
        .openexr_dependency_recorded = true,
        .ktx_dependency_recorded = true,
        .bootstrap_entrypoint_ready = true,
        .build_asset_importers_ready = true,
        .cmake_configure_installs_dependencies = false,
        .runtime_source_parsing = false,
        .runtime_optional_codec_execution = false,
        .native_handle_access = false,
        .gpu_command_executed = false,
        .evidence_rows =
            {
                ready_row(Kind::openexr_scanline_rgba16f),
                ready_row(Kind::openexr_tiled_rgba16f),
                ready_row(Kind::openexr_multipart),
                ready_row(Kind::openexr_metadata_preservation),
                ready_row(Kind::openexr_deep_image_rejected),
                ready_row(Kind::ktx2_basis_etc1s_transcode),
                ready_row(Kind::ktx2_basis_uastc_transcode),
                ready_row(Kind::ktx2_mip_level_validation),
                ready_row(Kind::ktx2_color_space_metadata),
                ready_row(Kind::d3d12_bc7_target),
                ready_row(Kind::vulkan_bc7_target),
                ready_row(Kind::metal_astc_target),
                ready_row(Kind::android_vulkan_astc_target),
                ready_row(Kind::runtime_cooked_only_ingest),
            },
    };
}

} // namespace

MK_TEST("environment texture pipeline v2 promotes only the full official source matrix") {
    const auto result = mirakana::evaluate_environment_texture_pipeline_v2_full(make_ready_pipeline_desc());

    MK_REQUIRE(result.full_ready);
    MK_REQUIRE(result.required_rows == 14U);
    MK_REQUIRE(result.ready_rows == 14U);
    MK_REQUIRE(result.openexr_rows == 5U);
    MK_REQUIRE(result.ktx2_basis_rows == 4U);
    MK_REQUIRE(result.backend_target_rows == 4U);
    MK_REQUIRE(result.runtime_rows == 1U);
    MK_REQUIRE(result.dependency_gated_rows == 0U);
    MK_REQUIRE(result.source_artifact_rows == 14U);
    MK_REQUIRE(result.cooked_artifact_rows == 14U);
    MK_REQUIRE(result.package_counter_rows == 14U);
    MK_REQUIRE(result.replay_hash_rows == 14U);
    MK_REQUIRE(result.rejection_diagnostic_rows == 1U);
    MK_REQUIRE(result.runtime_source_parsing == 0U);
    MK_REQUIRE(result.runtime_optional_codec_execution == 0U);
    MK_REQUIRE(result.native_handle_access == 0U);
    MK_REQUIRE(result.gpu_command_executed == 0U);
    MK_REQUIRE(result.diagnostics.empty());
}

MK_TEST("environment texture pipeline v2 rejects selected closeout evidence as incomplete") {
    using Kind = mirakana::EnvironmentTexturePipelineV2EvidenceKind;
    auto desc = make_ready_pipeline_desc();
    desc.evidence_rows = {
        ready_row(Kind::openexr_scanline_rgba16f), ready_row(Kind::ktx2_basis_uastc_transcode),
        ready_row(Kind::d3d12_bc7_target),         ready_row(Kind::vulkan_bc7_target),
        ready_row(Kind::metal_astc_target),        ready_row(Kind::runtime_cooked_only_ingest),
    };

    const auto result = mirakana::evaluate_environment_texture_pipeline_v2_full(desc);

    MK_REQUIRE(!result.full_ready);
    MK_REQUIRE(result.required_rows == 14U);
    MK_REQUIRE(result.ready_rows == 6U);
    MK_REQUIRE(result.missing_required_rows == 8U);
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::missing_required_row));
}

MK_TEST("environment texture pipeline v2 fails closed on dependency gates and runtime parsing") {
    using Kind = mirakana::EnvironmentTexturePipelineV2EvidenceKind;
    auto desc = make_ready_pipeline_desc();
    desc.runtime_source_parsing = true;
    desc.evidence_rows[static_cast<std::size_t>(Kind::ktx2_basis_etc1s_transcode)].dependency_gated = true;
    desc.evidence_rows[static_cast<std::size_t>(Kind::openexr_deep_image_rejected)].ready = false;

    const auto result = mirakana::evaluate_environment_texture_pipeline_v2_full(desc);

    MK_REQUIRE(!result.full_ready);
    MK_REQUIRE(result.dependency_gated_rows == 1U);
    MK_REQUIRE(result.runtime_source_parsing == 1U);
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::dependency_gated_row));
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::runtime_source_parsing));
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::not_ready_row));
}

MK_TEST("environment texture pipeline v2 requires concrete artifact counters and deep rejection diagnostic") {
    using Kind = mirakana::EnvironmentTexturePipelineV2EvidenceKind;
    auto desc = make_ready_pipeline_desc();
    desc.evidence_rows[static_cast<std::size_t>(Kind::openexr_scanline_rgba16f)].source_artifact_validated = false;
    desc.evidence_rows[static_cast<std::size_t>(Kind::ktx2_basis_etc1s_transcode)].package_counter_id.clear();
    desc.evidence_rows[static_cast<std::size_t>(Kind::ktx2_basis_uastc_transcode)].package_counter_validated = false;
    desc.evidence_rows[static_cast<std::size_t>(Kind::runtime_cooked_only_ingest)].replay_hash_recorded = false;
    desc.evidence_rows[static_cast<std::size_t>(Kind::openexr_deep_image_rejected)].rejection_diagnostic_recorded =
        false;

    const auto result = mirakana::evaluate_environment_texture_pipeline_v2_full(desc);

    MK_REQUIRE(!result.full_ready);
    MK_REQUIRE(result.source_artifact_rows == 13U);
    MK_REQUIRE(result.package_counter_rows == 12U);
    MK_REQUIRE(result.replay_hash_rows == 13U);
    MK_REQUIRE(result.rejection_diagnostic_rows == 0U);
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::invalid_row));
}

MK_TEST("environment texture pipeline v2 rejects duplicate rows and unsafe execution claims") {
    using Kind = mirakana::EnvironmentTexturePipelineV2EvidenceKind;
    auto desc = make_ready_pipeline_desc();
    desc.cmake_configure_installs_dependencies = true;
    desc.runtime_optional_codec_execution = true;
    desc.native_handle_access = true;
    desc.gpu_command_executed = true;
    desc.evidence_rows.push_back(ready_row(Kind::openexr_scanline_rgba16f));

    const auto result = mirakana::evaluate_environment_texture_pipeline_v2_full(desc);

    MK_REQUIRE(!result.full_ready);
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::duplicate_row));
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::configure_time_dependency_install));
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::runtime_optional_codec_execution));
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::native_handle_access));
    MK_REQUIRE(mirakana::has_environment_texture_pipeline_v2_diagnostic(
        result, mirakana::EnvironmentTexturePipelineV2DiagnosticCode::gpu_command_executed));
}

int main() {
    return mirakana::test::run_all();
}
