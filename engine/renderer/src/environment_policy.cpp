// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/environment_policy.hpp"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(EnvironmentPolicyPlan& plan, EnvironmentPolicyDiagnosticCode code, std::size_t feature_index,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(EnvironmentPolicyDiagnostic{
        .code = code,
        .feature_index = feature_index,
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool is_valid_feature(EnvironmentRenderFeature feature) noexcept {
    switch (feature) {
    case EnvironmentRenderFeature::physical_sky:
    case EnvironmentRenderFeature::sky_lighting:
    case EnvironmentRenderFeature::height_fog:
    case EnvironmentRenderFeature::volumetric_fog:
    case EnvironmentRenderFeature::cloud_layer:
    case EnvironmentRenderFeature::volumetric_clouds:
    case EnvironmentRenderFeature::precipitation:
    case EnvironmentRenderFeature::surface_wetness:
        return true;
    case EnvironmentRenderFeature::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool is_supported_backend(rhi::BackendKind backend) noexcept {
    switch (backend) {
    case rhi::BackendKind::null:
    case rhi::BackendKind::d3d12:
    case rhi::BackendKind::vulkan:
    case rhi::BackendKind::metal:
        return true;
    }
    return false;
}

[[nodiscard]] bool valid_frame_extent(Extent2D extent) noexcept {
    return extent.width > 0 && extent.height > 0;
}

} // namespace

bool EnvironmentPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

EnvironmentPolicyPlan plan_environment_render_policy(const EnvironmentPolicyDesc& desc) {
    EnvironmentPolicyPlan plan{
        .frame_extent = desc.frame_extent,
        .backend = desc.backend,
        .package_counter_evidence_ready = desc.package_counter_evidence_ready,
    };

    if (desc.features.empty()) {
        add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::no_features, 0, 0,
                       "environment render policy requires at least one enabled feature row");
    }
    if (!valid_frame_extent(desc.frame_extent)) {
        add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::invalid_frame_extent, 0, 0,
                       "environment render policy requires a positive frame extent");
    }
    if (!is_supported_backend(desc.backend)) {
        add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::unsupported_backend, 0, 0,
                       "environment render policy backend is unsupported");
    }
    if (desc.request_backend_inheritance) {
        add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::unsupported_backend_inheritance, 0, 0,
                       "environment render policy does not infer backend readiness from another backend");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::unsupported_native_handle_claim, 0, 0,
                       "environment render policy must not request native handle access");
    }

    bool requires_backend_shader_evidence = false;
    bool missing_backend_shader_evidence = false;
    bool requires_backend_validation_evidence = false;
    bool missing_backend_validation_evidence = false;
    std::unordered_set<std::uint8_t> seen_features;
    for (std::size_t index = 0; index < desc.features.size(); ++index) {
        const auto& feature = desc.features[index];
        if (!feature.enabled) {
            continue;
        }
        if (!is_valid_feature(feature.feature)) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::invalid_feature, index, feature.source_index,
                           "environment render policy feature is unsupported");
            continue;
        }
        if (!seen_features.insert(static_cast<std::uint8_t>(feature.feature)).second) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::duplicate_feature, index, feature.source_index,
                           "environment render policy feature is duplicated");
            continue;
        }

        const auto pass_index = plan.environment_pass_count;
        plan.feature_rows.push_back(EnvironmentRenderFeatureRow{
            .feature = feature.feature,
            .uses_scene_depth = feature.requires_scene_depth,
            .requires_backend_shader_evidence = feature.requires_backend_shader_evidence,
            .requires_backend_validation_evidence = feature.requires_backend_validation_evidence,
            .backend_shader_evidence_ready = feature.backend_shader_evidence_ready,
            .backend_validation_evidence_ready = feature.backend_validation_evidence_ready,
            .raymarch_step_budget = feature.raymarch_step_budget,
            .pass_index = pass_index,
            .source_index = feature.source_index,
        });
        ++plan.feature_count;
        ++plan.environment_pass_count;
        ++plan.frame_graph_pass_count;
        plan.frame_graph_barrier_step_budget += 2U;
        plan.scene_color_required = true;
        plan.scene_depth_required = plan.scene_depth_required || feature.requires_scene_depth;
        requires_backend_shader_evidence = requires_backend_shader_evidence || feature.requires_backend_shader_evidence;
        missing_backend_shader_evidence =
            missing_backend_shader_evidence ||
            (feature.requires_backend_shader_evidence && !feature.backend_shader_evidence_ready);
        requires_backend_validation_evidence =
            requires_backend_validation_evidence || feature.requires_backend_validation_evidence;
        missing_backend_validation_evidence =
            missing_backend_validation_evidence ||
            (feature.requires_backend_validation_evidence && !feature.backend_validation_evidence_ready);

        if (!desc.scene_color_available) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::missing_scene_color, index, feature.source_index,
                           "environment render policy requires scene color input");
        }
        if (feature.requires_scene_depth && !desc.scene_depth_available) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::missing_scene_depth, index, feature.source_index,
                           "environment render policy feature requires scene depth input");
        }
        if (feature.requires_backend_shader_evidence && !feature.backend_shader_evidence_ready) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::missing_backend_shader_evidence, index,
                           feature.source_index, "environment render policy feature requires backend shader evidence");
        }
        if (feature.requires_backend_validation_evidence && !feature.backend_validation_evidence_ready) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::missing_backend_validation_evidence, index,
                           feature.source_index,
                           "environment render policy feature requires backend validation evidence");
        }
        if (feature.raymarch_step_budget > desc.max_raymarch_step_budget) {
            add_diagnostic(plan, EnvironmentPolicyDiagnosticCode::excessive_raymarch_step_budget, index,
                           feature.source_index,
                           "environment render policy raymarch step budget exceeds the selected maximum");
        }
    }

    plan.backend_shader_evidence_ready = requires_backend_shader_evidence && !missing_backend_shader_evidence;
    plan.backend_validation_evidence_ready =
        requires_backend_validation_evidence && !missing_backend_validation_evidence;

    return plan;
}

bool has_environment_policy_diagnostic(const EnvironmentPolicyPlan& plan,
                                       EnvironmentPolicyDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
