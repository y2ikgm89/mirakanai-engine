// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/scene_scale_policy.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool is_supported_group_kind(SceneScaleDrawGroupKind kind) noexcept {
    switch (kind) {
    case SceneScaleDrawGroupKind::sprite:
    case SceneScaleDrawGroupKind::static_mesh:
    case SceneScaleDrawGroupKind::skinned_mesh:
    case SceneScaleDrawGroupKind::morph_mesh:
        return true;
    case SceneScaleDrawGroupKind::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool is_supported_culling(SceneScaleCullingMode mode) noexcept {
    return mode == SceneScaleCullingMode::none || mode == SceneScaleCullingMode::cpu_frustum;
}

[[nodiscard]] bool is_supported_batching(SceneScaleBatchingMode mode) noexcept {
    return mode == SceneScaleBatchingMode::none || mode == SceneScaleBatchingMode::instanced_draw;
}

[[nodiscard]] bool is_supported_lod(SceneScaleLodMode mode) noexcept {
    return mode == SceneScaleLodMode::none || mode == SceneScaleLodMode::distance_band ||
           mode == SceneScaleLodMode::screen_size;
}

[[nodiscard]] std::string_view group_kind_name(SceneScaleDrawGroupKind kind) noexcept {
    switch (kind) {
    case SceneScaleDrawGroupKind::sprite:
        return "sprite";
    case SceneScaleDrawGroupKind::static_mesh:
        return "static_mesh";
    case SceneScaleDrawGroupKind::skinned_mesh:
        return "skinned_mesh";
    case SceneScaleDrawGroupKind::morph_mesh:
        return "morph_mesh";
    case SceneScaleDrawGroupKind::unknown:
        return "unknown";
    }
    return "unknown";
}

[[nodiscard]] std::string_view culling_name(SceneScaleCullingMode mode) noexcept {
    switch (mode) {
    case SceneScaleCullingMode::none:
        return "none";
    case SceneScaleCullingMode::cpu_frustum:
        return "cpu_frustum";
    case SceneScaleCullingMode::gpu_indirect:
        return "gpu_indirect";
    }
    return "unknown";
}

[[nodiscard]] std::string_view batching_name(SceneScaleBatchingMode mode) noexcept {
    switch (mode) {
    case SceneScaleBatchingMode::none:
        return "none";
    case SceneScaleBatchingMode::instanced_draw:
        return "instanced_draw";
    case SceneScaleBatchingMode::gpu_indirect:
        return "gpu_indirect";
    }
    return "unknown";
}

[[nodiscard]] std::string_view lod_name(SceneScaleLodMode mode) noexcept {
    switch (mode) {
    case SceneScaleLodMode::none:
        return "none";
    case SceneScaleLodMode::distance_band:
        return "distance_band";
    case SceneScaleLodMode::screen_size:
        return "screen_size";
    case SceneScaleLodMode::gpu_driven:
        return "gpu_driven";
    }
    return "unknown";
}

[[nodiscard]] std::string_view backend_name(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::null:
        return "null";
    case rhi::BackendKind::d3d12:
        return "d3d12";
    case rhi::BackendKind::vulkan:
        return "vulkan";
    case rhi::BackendKind::metal:
        return "metal";
    }
    return "unknown";
}

void add_diagnostic(SceneScalePolicyPlan& plan, SceneScaleDiagnosticCode code, std::size_t group_index,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(SceneScaleDiagnostic{
        .code = code,
        .group_index = group_index,
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool valid_lod_count(SceneScaleLodMode mode, std::uint32_t lod_count) noexcept {
    if (mode == SceneScaleLodMode::none) {
        return lod_count <= 1U;
    }
    return lod_count >= 1U && lod_count <= 8U;
}

[[nodiscard]] bool can_use_instancing(const SceneScaleDrawGroupDesc& group,
                                      bool backend_instancing_evidence_ready) noexcept {
    return group.batching == SceneScaleBatchingMode::instanced_draw && backend_instancing_evidence_ready &&
           group.visible_instance_count > 1U && group.scene_resources_available && is_supported_group_kind(group.kind);
}

} // namespace

SceneScalePolicyPlan plan_scene_scale_policy(const SceneScalePolicyDesc& desc) {
    SceneScalePolicyPlan plan;
    plan.frame_extent = desc.frame_extent;
    plan.backend = desc.backend;
    plan.backend_instancing_evidence_ready = desc.backend_instancing_evidence_ready;
    plan.performance_measurement_ready = desc.performance_measurement_ready;

    if (desc.frame_extent.width == 0 || desc.frame_extent.height == 0) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::invalid_frame_extent, 0, 0,
                       "scene scale policy requires a non-zero frame extent");
    }
    if (desc.draw_groups.empty()) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::no_draw_groups, 0, 0,
                       "scene scale policy requires at least one draw group");
    }
    if (desc.max_draw_group_count == 0 || desc.draw_groups.size() > desc.max_draw_group_count) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::too_many_draw_groups, desc.draw_groups.size(), 0,
                       "scene scale policy exceeds max_draw_group_count");
    }
    if (desc.require_backend_instancing_evidence && !desc.backend_instancing_evidence_ready) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::missing_backend_instancing_evidence, 0, 0,
                       std::string{"scene scale policy requires instancing evidence for "} +
                           std::string{backend_name(desc.backend)});
    }
    if (desc.require_performance_measurement && !desc.performance_measurement_ready) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::missing_performance_measurement, 0, 0,
                       "scene scale policy performance claims require measurement evidence");
    }

    for (std::size_t index = 0; index < desc.draw_groups.size(); ++index) {
        const auto& group = desc.draw_groups[index];
        const auto source_index = group.source_index;

        if (!is_supported_group_kind(group.kind)) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::invalid_draw_group, index, source_index,
                           std::string{"unsupported scene scale draw group "} +
                               std::string{group_kind_name(group.kind)});
        }
        if (group.instance_count == 0) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::zero_instance_count, index, source_index,
                           "scene scale draw group requires at least one instance");
        }
        if (group.visible_instance_count > group.instance_count) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::invalid_visible_instance_count, index, source_index,
                           "scene scale visible_instance_count cannot exceed instance_count");
        }
        if (!group.scene_resources_available) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::missing_scene_resources, index, source_index,
                           "scene scale draw group requires available scene resources");
        }
        if (!is_supported_batching(group.batching)) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::unsupported_batching_mode, index, source_index,
                           std::string{"unsupported scene scale batching mode "} +
                               std::string{batching_name(group.batching)});
        }
        if (!is_supported_culling(group.culling)) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::unsupported_culling_mode, index, source_index,
                           std::string{"unsupported scene scale culling mode "} +
                               std::string{culling_name(group.culling)});
        }
        if (!is_supported_lod(group.lod)) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::unsupported_lod_mode, index, source_index,
                           std::string{"unsupported scene scale lod mode "} + std::string{lod_name(group.lod)});
        }
        if (!valid_lod_count(group.lod, group.lod_count)) {
            add_diagnostic(plan, SceneScaleDiagnosticCode::invalid_lod_count, index, source_index,
                           "scene scale lod_count must match the selected lod policy");
        }

        const std::uint32_t visible_count = group.visible_instance_count;
        const std::uint32_t culled_count =
            group.instance_count >= visible_count ? group.instance_count - visible_count : 0U;
        const bool uses_instancing = can_use_instancing(group, desc.backend_instancing_evidence_ready);
        const std::uint32_t draw_calls = visible_count == 0 ? 0U : uses_instancing ? 1U : visible_count;

        plan.draw_group_rows.push_back(SceneScaleDrawGroupRow{
            .kind = group.kind,
            .requested_instance_count = group.instance_count,
            .visible_instance_count = visible_count,
            .culled_instance_count = culled_count,
            .draw_call_count = draw_calls,
            .instanced_visible_instance_count = uses_instancing ? visible_count : 0U,
            .culling = group.culling,
            .batching = group.batching,
            .lod = group.lod,
            .lod_count = group.lod_count,
            .uses_instancing = uses_instancing,
            .source_index = source_index,
        });

        ++plan.draw_group_count;
        plan.requested_instance_count += group.instance_count;
        plan.visible_instance_count += visible_count;
        plan.culled_instance_count += culled_count;
        plan.draw_call_count += draw_calls;
        if (uses_instancing) {
            ++plan.instanced_draw_call_count;
            plan.instanced_visible_instance_count += visible_count;
        }
        if (group.lod != SceneScaleLodMode::none) {
            ++plan.lod_group_count;
        }
        if (group.culling == SceneScaleCullingMode::cpu_frustum) {
            ++plan.cpu_culling_group_count;
        }
    }

    if (plan.visible_instance_count > desc.max_visible_instance_count) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::too_many_visible_instances, 0, 0,
                       "scene scale policy exceeds max_visible_instance_count");
    }
    if (plan.draw_call_count > desc.max_draw_call_count) {
        add_diagnostic(plan, SceneScaleDiagnosticCode::too_many_draw_calls, 0, 0,
                       "scene scale policy exceeds max_draw_call_count");
    }

    return plan;
}

bool has_scene_scale_policy_diagnostic(const SceneScalePolicyPlan& plan, SceneScaleDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const SceneScaleDiagnostic& diagnostic) { return diagnostic.code == code; });
}

bool scene_scale_policy_backend_instancing_evidence_ready(
    const SceneScaleBackendInstancingEvidenceDesc& desc) noexcept {
    const auto instanced_draws_ready = desc.instanced_draw_calls > 0 && desc.instanced_indexed_draw_calls > 0;
    const auto instances_ready = desc.instanced_instances_submitted > 0;
    switch (desc.backend) {
    case rhi::BackendKind::d3d12:
    case rhi::BackendKind::vulkan:
        return instanced_draws_ready && instances_ready;
    case rhi::BackendKind::null:
    case rhi::BackendKind::metal:
        return false;
    }
    return false;
}

} // namespace mirakana
