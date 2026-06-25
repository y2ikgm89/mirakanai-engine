// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/rhi/metal/metal_backend.hpp"

#include "metal_native_private.hpp"

#include <iterator>
#include <string>
#include <utility>

namespace mirakana::rhi::metal {

BackendKind backend_kind() noexcept {
    return BackendKind::metal;
}

std::string_view backend_name() noexcept {
    return "metal";
}

BackendCapabilityProfile capabilities() noexcept {
    return default_backend_capabilities(BackendKind::metal);
}

BackendProbePlan probe_plan() noexcept {
    return backend_probe_plan(BackendKind::metal);
}

bool supports_host(RhiHostPlatform host) noexcept {
    return is_backend_supported_on_host(BackendKind::metal, host);
}

BackendProbeResult make_probe_result(RhiHostPlatform host, BackendProbeStatus status, std::string_view diagnostic) {
    return make_backend_probe_result(BackendKind::metal, host, status, diagnostic);
}

namespace {

constexpr std::string_view kRendererMetalAppleHostEvidenceRecipeId{"renderer-metal-apple-host-evidence"};

[[nodiscard]] MetalResourceUsage resource_usage_for_state(ResourceState state) noexcept {
    switch (state) {
    case ResourceState::render_target:
        return MetalResourceUsage::render_target_write;
    case ResourceState::depth_write:
        return MetalResourceUsage::depth_write;
    case ResourceState::shader_read:
        return MetalResourceUsage::shader_read;
    case ResourceState::copy_source:
        return MetalResourceUsage::blit_read;
    case ResourceState::copy_destination:
        return MetalResourceUsage::blit_write;
    case ResourceState::present:
        return MetalResourceUsage::drawable_present;
    case ResourceState::undefined:
        return MetalResourceUsage::none;
    }

    return MetalResourceUsage::none;
}

[[nodiscard]] bool state_supported_by_texture_usage(TextureUsage usage, ResourceState state, bool drawable) noexcept {
    switch (state) {
    case ResourceState::undefined:
        return true;
    case ResourceState::copy_source:
        return has_flag(usage, TextureUsage::copy_source);
    case ResourceState::copy_destination:
        return has_flag(usage, TextureUsage::copy_destination);
    case ResourceState::shader_read:
        return has_flag(usage, TextureUsage::shader_resource);
    case ResourceState::render_target:
        return has_flag(usage, TextureUsage::render_target);
    case ResourceState::depth_write:
        return has_flag(usage, TextureUsage::depth_stencil);
    case ResourceState::present:
        return drawable && has_flag(usage, TextureUsage::present);
    }

    return false;
}

[[nodiscard]] bool is_render_encoder_state(ResourceState state) noexcept {
    return state == ResourceState::render_target || state == ResourceState::depth_write;
}

[[nodiscard]] bool is_blit_encoder_state(ResourceState state) noexcept {
    return state == ResourceState::copy_source || state == ResourceState::copy_destination;
}

[[nodiscard]] bool is_write_state(ResourceState state) noexcept {
    return state == ResourceState::render_target || state == ResourceState::depth_write ||
           state == ResourceState::copy_destination;
}

[[nodiscard]] bool is_reviewed_visible_renderer_host_recipe(const std::string_view recipe_id) noexcept {
    return recipe_id == kRendererMetalAppleHostEvidenceRecipeId;
}

[[nodiscard]] std::string_view
visible_renderer_package_counter_id(const MetalVisibleRendererPackageEvidenceKind kind) noexcept {
    switch (kind) {
    case MetalVisibleRendererPackageEvidenceKind::visible_3d_scene:
        return "renderer_metal_visible_3d_package_ready";
    case MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload:
        return "renderer_metal_visible_ui_atlas_package_ready";
    case MetalVisibleRendererPackageEvidenceKind::environment_package:
        return "renderer_metal_visible_environment_package_ready";
    case MetalVisibleRendererPackageEvidenceKind::generated_game_package:
        return "renderer_metal_visible_generated_game_package_ready";
    }
    return {};
}

[[nodiscard]] bool visible_renderer_package_row_ready(const MetalVisibleRendererPackageEvidenceDesc& desc,
                                                      const MetalVisibleRendererPackageEvidenceKind kind) noexcept {
    switch (kind) {
    case MetalVisibleRendererPackageEvidenceKind::visible_3d_scene:
        return desc.visible_3d_scene_material_lighting_postprocess_ready && desc.visible_3d_readback_hash != 0U;
    case MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload:
        return desc.ui_atlas_upload_readback_ready && desc.ui_atlas_readback_hash != 0U;
    case MetalVisibleRendererPackageEvidenceKind::environment_package:
        return desc.environment_renderer_package_row_consumed && desc.environment_package_replay_hash != 0U;
    case MetalVisibleRendererPackageEvidenceKind::generated_game_package:
        return desc.generated_game_package_output_row_ready && desc.generated_game_package_replay_hash != 0U;
    }
    return false;
}

[[nodiscard]] std::uint64_t
visible_renderer_package_row_hash(const MetalVisibleRendererPackageEvidenceDesc& desc,
                                  const MetalVisibleRendererPackageEvidenceKind kind) noexcept {
    switch (kind) {
    case MetalVisibleRendererPackageEvidenceKind::visible_3d_scene:
        return desc.visible_3d_readback_hash;
    case MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload:
        return desc.ui_atlas_readback_hash;
    case MetalVisibleRendererPackageEvidenceKind::environment_package:
        return desc.environment_package_replay_hash;
    case MetalVisibleRendererPackageEvidenceKind::generated_game_package:
        return desc.generated_game_package_replay_hash;
    }
    return 0U;
}

void hash_mix(std::uint64_t& hash, const std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void hash_mix_string(std::uint64_t& hash, const std::string& value) noexcept {
    for (const auto c : value) {
        hash_mix(hash, static_cast<unsigned char>(c));
    }
}

[[nodiscard]] MetalVisibleRendererPackageEvidenceRow
make_visible_renderer_package_evidence_row(const MetalVisibleRendererPackageEvidenceDesc& desc,
                                           const MetalVisibleRendererPackageEvidenceKind kind) {
    MetalVisibleRendererPackageEvidenceRow row;
    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    row.kind = kind;
    row.host_supported = supports_host(host);
    row.host_evidence_required = !row.host_supported || !desc.apple_host_validation_available;
    row.host_evidence_available = desc.apple_host_validation_available;
    row.runtime_ready = desc.runtime_ready;
    row.command_queue_ready = desc.command_queue_ready;
    row.metallib_valid = desc.metallib_valid;
    row.package_counter_ready = visible_renderer_package_row_ready(desc, kind);
    row.native_handle_access = desc.native_handles_exposed;
    row.broad_claim = desc.broad_backend_parity_claimed || desc.broad_metal_readiness_claimed;
    row.evidence_hash = visible_renderer_package_row_hash(desc, kind);
    row.host_validation_recipe_id = desc.host_validation_recipe_id;
    row.package_counter_id = std::string{visible_renderer_package_counter_id(kind)};

    if (row.host_evidence_required) {
        row.status = MetalVisibleRendererPackageEvidenceStatus::host_evidence_required;
        row.diagnostic = "Metal visible renderer package evidence requires Apple host validation";
        return row;
    }
    if (!is_reviewed_visible_renderer_host_recipe(desc.host_validation_recipe_id)) {
        row.status = MetalVisibleRendererPackageEvidenceStatus::blocked;
        row.diagnostic = "Metal visible renderer package evidence requires renderer-metal-apple-host-evidence";
        return row;
    }
    if (row.native_handle_access) {
        row.status = MetalVisibleRendererPackageEvidenceStatus::blocked;
        row.diagnostic = "Metal visible renderer package evidence must not expose native handles";
        return row;
    }
    if (row.broad_claim) {
        row.status = MetalVisibleRendererPackageEvidenceStatus::blocked;
        row.diagnostic = "Metal visible renderer package evidence must not claim broad backend or Metal readiness";
        return row;
    }
    if (!row.runtime_ready || !row.command_queue_ready || !row.metallib_valid) {
        row.status = MetalVisibleRendererPackageEvidenceStatus::blocked;
        row.diagnostic = "Metal visible renderer package evidence requires runtime, command queue, and metallib";
        return row;
    }
    if (!row.package_counter_ready) {
        row.status = MetalVisibleRendererPackageEvidenceStatus::blocked;
        row.diagnostic = std::string{"Metal visible renderer package evidence missing selected row: "} +
                         std::string{metal_visible_renderer_package_evidence_kind_label(kind)};
        return row;
    }

    row.status = MetalVisibleRendererPackageEvidenceStatus::ready;
    row.diagnostic = "Metal visible renderer package evidence row ready";
    return row;
}

void compute_visible_renderer_package_replay_hash(MetalVisibleRendererPackageEvidencePlan& plan) noexcept {
    if (plan.status != MetalVisibleRendererPackageEvidenceStatus::ready) {
        return;
    }

    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto& row : plan.rows) {
        hash_mix(hash, static_cast<std::uint8_t>(row.kind));
        hash_mix(hash, row.evidence_hash);
        hash_mix_string(hash, row.host_validation_recipe_id);
        hash_mix_string(hash, row.package_counter_id);
    }
    plan.replay_hash = hash;
}

} // namespace

MetalShaderLibraryArtifactValidation validate_shader_library_artifact(const MetalShaderLibraryArtifactDesc& desc) {
    MetalShaderLibraryArtifactValidation result;
    if (desc.bytecode == nullptr || desc.bytecode_size == 0) {
        result.diagnostic = "Metal shader library bytecode is required";
        return result;
    }

    result.valid = true;
    result.bytecode_size = desc.bytecode_size;
    result.diagnostic = "Metal shader library artifact ready";
    return result;
}

MetalRuntimeReadinessPlan build_runtime_readiness_plan(const MetalRuntimeReadinessDesc& desc) {
    MetalRuntimeReadinessPlan plan;
    plan.host_supported = supports_host(desc.host);
    if (!plan.host_supported) {
        plan.diagnostic = "Metal is only available on Apple platforms";
        plan.probe = make_probe_result(desc.host, BackendProbeStatus::unsupported_host, plan.diagnostic);
        return plan;
    }

    plan.runtime_loaded = desc.runtime_available;
    if (!plan.runtime_loaded) {
        plan.diagnostic = "Metal runtime is unavailable";
        plan.probe = make_probe_result(desc.host, BackendProbeStatus::missing_runtime, plan.diagnostic);
        return plan;
    }

    plan.default_device_ready = desc.default_device_created;
    if (!plan.default_device_ready) {
        plan.diagnostic = "Metal default device is required";
        plan.probe = make_probe_result(desc.host, BackendProbeStatus::no_suitable_device, plan.diagnostic);
        return plan;
    }

    plan.command_queue_ready = desc.command_queue_created;
    if (!plan.command_queue_ready) {
        plan.diagnostic = "Metal command queue is required";
        plan.probe = make_probe_result(desc.host, BackendProbeStatus::unavailable, plan.diagnostic);
        return plan;
    }

    plan.shader_library_ready = desc.shader_library.valid && desc.shader_library.bytecode_size > 0;
    if (!plan.shader_library_ready) {
        plan.diagnostic = "Metal non-empty shader library artifact is required";
        plan.probe = make_probe_result(desc.host, BackendProbeStatus::missing_shader_artifacts, plan.diagnostic);
        return plan;
    }

    plan.diagnostic = "Metal runtime readiness plan ready";
    plan.probe = make_probe_result(desc.host, BackendProbeStatus::available, plan.diagnostic);
    return plan;
}

MetalTextureSynchronizationPlan build_texture_synchronization_plan(const MetalTextureSynchronizationDesc& desc) {
    MetalTextureSynchronizationPlan plan;
    plan.before_usage = resource_usage_for_state(desc.before);
    plan.after_usage = resource_usage_for_state(desc.after);

    if (desc.after == ResourceState::undefined) {
        plan.diagnostic = "Metal texture synchronization destination state is required";
        return plan;
    }

    if (!state_supported_by_texture_usage(desc.usage, desc.before, desc.drawable)) {
        if (desc.before == ResourceState::present && !desc.drawable) {
            plan.diagnostic = "Metal present state requires a drawable texture";
        } else {
            plan.diagnostic = "Metal texture usage does not support source state";
        }
        return plan;
    }

    if (!state_supported_by_texture_usage(desc.usage, desc.after, desc.drawable)) {
        if (desc.after == ResourceState::present && !desc.drawable) {
            plan.diagnostic = "Metal present state requires a drawable texture";
        } else {
            plan.diagnostic = "Metal texture usage does not support destination state";
        }
        return plan;
    }

    plan.supported = true;
    if (desc.before == desc.after) {
        plan.steps.push_back(MetalSynchronizationStep::no_op);
        plan.diagnostic = "Metal texture synchronization no-op";
        return plan;
    }

    if (is_render_encoder_state(desc.before) && !is_render_encoder_state(desc.after)) {
        plan.requires_encoder_boundary = true;
        plan.steps.push_back(MetalSynchronizationStep::end_render_encoder);
    }

    if (is_write_state(desc.before) && desc.after == ResourceState::shader_read) {
        plan.requires_encoder_boundary = true;
    }

    if (is_render_encoder_state(desc.after)) {
        plan.requires_render_encoder = true;
        plan.steps.push_back(MetalSynchronizationStep::begin_render_encoder);
    }

    if (is_blit_encoder_state(desc.after)) {
        plan.requires_blit_encoder = true;
        plan.steps.push_back(MetalSynchronizationStep::begin_blit_encoder);
    }

    if (desc.after == ResourceState::present) {
        plan.requires_drawable_present = true;
        plan.steps.push_back(MetalSynchronizationStep::present_drawable);
    }

    plan.diagnostic = "Metal texture synchronization plan ready";
    return plan;
}

MetalPlatformAvailabilityDiagnostics diagnose_platform_availability(RhiHostPlatform host, bool objcxx_enabled) {
    MetalPlatformAvailabilityDiagnostics diagnostics;
    diagnostics.host = host;
    diagnostics.host_supported = supports_host(host);
    diagnostics.objcxx_enabled = objcxx_enabled;
    diagnostics.runtime_probe_required = diagnostics.host_supported;
    diagnostics.can_compile_native_sources = diagnostics.host_supported && objcxx_enabled;

    if (!diagnostics.host_supported) {
        diagnostics.diagnostic = "Metal native backend is unavailable on this host";
        return diagnostics;
    }

    if (!diagnostics.objcxx_enabled) {
        diagnostics.diagnostic = "Metal native backend requires Objective-C++ SDK linkage";
        return diagnostics;
    }

    diagnostics.diagnostic = "Metal native backend can compile and must probe runtime availability";
    return diagnostics;
}

std::vector<MetalEnvironmentFeatureEvidenceRequirement> default_environment_feature_evidence_requirements() {
    return {
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::physical_sky,
            .selected = true,
            .render_pass_required = true,
            .render_pipeline_required = true,
            .synchronization_evidence_required = true,
        },
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::height_fog,
            .selected = true,
            .render_pass_required = true,
            .render_pipeline_required = true,
            .depth_texture_required = true,
            .synchronization_evidence_required = true,
        },
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::cloud_layer,
            .selected = true,
            .render_pass_required = true,
            .render_pipeline_required = true,
            .synchronization_evidence_required = true,
        },
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::precipitation,
            .selected = true,
            .render_pass_required = true,
            .render_pipeline_required = true,
            .depth_texture_required = true,
            .particle_buffer_required = true,
            .synchronization_evidence_required = true,
        },
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::volumetric_fog,
            .selected = true,
            .compute_pipeline_required = true,
            .depth_texture_required = true,
            .synchronization_evidence_required = true,
        },
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::volumetric_cloud,
            .selected = true,
            .compute_pipeline_required = true,
            .synchronization_evidence_required = true,
        },
        MetalEnvironmentFeatureEvidenceRequirement{
            .feature = MetalEnvironmentFeatureKind::environment_lighting_ibl,
            .selected = true,
            .render_pass_required = true,
            .render_pipeline_required = true,
            .cube_map_texture_required = true,
            .hdr_texture_required = true,
            .synchronization_evidence_required = true,
        },
    };
}

std::string_view metal_environment_feature_kind_label(MetalEnvironmentFeatureKind feature) noexcept {
    switch (feature) {
    case MetalEnvironmentFeatureKind::physical_sky:
        return "physical_sky";
    case MetalEnvironmentFeatureKind::height_fog:
        return "height_fog";
    case MetalEnvironmentFeatureKind::cloud_layer:
        return "cloud_layer";
    case MetalEnvironmentFeatureKind::precipitation:
        return "precipitation";
    case MetalEnvironmentFeatureKind::volumetric_fog:
        return "volumetric_fog";
    case MetalEnvironmentFeatureKind::volumetric_cloud:
        return "volumetric_cloud";
    case MetalEnvironmentFeatureKind::environment_lighting_ibl:
        return "environment_lighting_ibl";
    }

    return "unknown";
}

std::string_view
metal_environment_feature_evidence_status_label(MetalEnvironmentFeatureEvidenceStatus status) noexcept {
    switch (status) {
    case MetalEnvironmentFeatureEvidenceStatus::blocked:
        return "blocked";
    case MetalEnvironmentFeatureEvidenceStatus::host_evidence_required:
        return "host_evidence_required";
    case MetalEnvironmentFeatureEvidenceStatus::ready:
        return "ready";
    }

    return "unknown";
}

std::string_view
metal_visible_renderer_package_evidence_kind_label(MetalVisibleRendererPackageEvidenceKind kind) noexcept {
    switch (kind) {
    case MetalVisibleRendererPackageEvidenceKind::visible_3d_scene:
        return "visible_3d_scene";
    case MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload:
        return "ui_atlas_upload";
    case MetalVisibleRendererPackageEvidenceKind::environment_package:
        return "environment_package";
    case MetalVisibleRendererPackageEvidenceKind::generated_game_package:
        return "generated_game_package";
    }

    return "unknown";
}

std::string_view
metal_visible_renderer_package_evidence_status_label(MetalVisibleRendererPackageEvidenceStatus status) noexcept {
    switch (status) {
    case MetalVisibleRendererPackageEvidenceStatus::blocked:
        return "blocked";
    case MetalVisibleRendererPackageEvidenceStatus::host_evidence_required:
        return "host_evidence_required";
    case MetalVisibleRendererPackageEvidenceStatus::ready:
        return "ready";
    }

    return "unknown";
}

MetalVisibleRendererPackageEvidencePlan
plan_metal_visible_renderer_package_evidence(const MetalVisibleRendererPackageEvidenceDesc& desc) {
    MetalVisibleRendererPackageEvidencePlan plan;
    constexpr MetalVisibleRendererPackageEvidenceKind kRows[] = {
        MetalVisibleRendererPackageEvidenceKind::visible_3d_scene,
        MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload,
        MetalVisibleRendererPackageEvidenceKind::environment_package,
        MetalVisibleRendererPackageEvidenceKind::generated_game_package,
    };

    plan.rows.reserve(std::size(kRows));
    for (const auto kind : kRows) {
        auto row = make_visible_renderer_package_evidence_row(desc, kind);
        switch (row.status) {
        case MetalVisibleRendererPackageEvidenceStatus::ready:
            ++plan.ready_row_count;
            break;
        case MetalVisibleRendererPackageEvidenceStatus::host_evidence_required:
            ++plan.host_gated_row_count;
            break;
        case MetalVisibleRendererPackageEvidenceStatus::blocked:
            ++plan.blocked_row_count;
            break;
        }
        if (row.native_handle_access) {
            ++plan.native_handle_access_count;
        }
        if (row.broad_claim) {
            ++plan.broad_claim_count;
        }
        plan.rows.push_back(std::move(row));
    }

    plan.required_row_count = plan.rows.size();
    if (plan.blocked_row_count > 0U) {
        plan.status = MetalVisibleRendererPackageEvidenceStatus::blocked;
        plan.diagnostic = "Metal visible renderer package evidence is incomplete";
    } else if (plan.host_gated_row_count > 0U) {
        plan.status = MetalVisibleRendererPackageEvidenceStatus::host_evidence_required;
        plan.diagnostic = "Metal visible renderer package evidence requires Apple host validation";
    } else {
        plan.status = MetalVisibleRendererPackageEvidenceStatus::ready;
        plan.diagnostic = "Metal visible renderer package evidence ready";
    }

    compute_visible_renderer_package_replay_hash(plan);
    return plan;
}

std::string metal_visible_renderer_package_evidence_status_line(const MetalVisibleRendererPackageEvidencePlan& plan) {
    auto row_status = [&plan](MetalVisibleRendererPackageEvidenceKind kind) {
        for (const auto& row : plan.rows) {
            if (row.kind == kind) {
                return metal_visible_renderer_package_evidence_status_label(row.status);
            }
        }
        return std::string_view{"unknown"};
    };
    auto row_ready = [&plan](MetalVisibleRendererPackageEvidenceKind kind) {
        for (const auto& row : plan.rows) {
            if (row.kind == kind) {
                return row.status == MetalVisibleRendererPackageEvidenceStatus::ready ? 1 : 0;
            }
        }
        return 0;
    };

    std::string line{"renderer_metal_visible_package_evidence_status="};
    line += metal_visible_renderer_package_evidence_status_label(plan.status);
    line += " renderer_metal_visible_package_evidence_ready=";
    line += plan.status == MetalVisibleRendererPackageEvidenceStatus::ready ? "1" : "0";
    line += " renderer_metal_visible_package_evidence_rows=" + std::to_string(plan.required_row_count);
    line += " renderer_metal_visible_package_evidence_ready_rows=" + std::to_string(plan.ready_row_count);
    line += " renderer_metal_visible_package_evidence_host_gated_rows=" + std::to_string(plan.host_gated_row_count);
    line += " renderer_metal_visible_package_evidence_blocked_rows=" + std::to_string(plan.blocked_row_count);
    line += " renderer_metal_visible_3d_scene_status=";
    line += row_status(MetalVisibleRendererPackageEvidenceKind::visible_3d_scene);
    line += " renderer_metal_visible_3d_package_ready=";
    line += std::to_string(row_ready(MetalVisibleRendererPackageEvidenceKind::visible_3d_scene));
    line += " renderer_metal_visible_ui_atlas_status=";
    line += row_status(MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload);
    line += " renderer_metal_visible_ui_atlas_package_ready=";
    line += std::to_string(row_ready(MetalVisibleRendererPackageEvidenceKind::ui_atlas_upload));
    line += " renderer_metal_visible_environment_package_status=";
    line += row_status(MetalVisibleRendererPackageEvidenceKind::environment_package);
    line += " renderer_metal_visible_environment_package_ready=";
    line += std::to_string(row_ready(MetalVisibleRendererPackageEvidenceKind::environment_package));
    line += " renderer_metal_visible_generated_game_package_status=";
    line += row_status(MetalVisibleRendererPackageEvidenceKind::generated_game_package);
    line += " renderer_metal_visible_generated_game_package_ready=";
    line += std::to_string(row_ready(MetalVisibleRendererPackageEvidenceKind::generated_game_package));
    line += " renderer_metal_visible_package_native_handle_access=" + std::to_string(plan.native_handle_access_count);
    line += " renderer_metal_visible_package_broad_claims=" + std::to_string(plan.broad_claim_count);
    line += " renderer_metal_visible_package_replay_hash=" + std::to_string(plan.replay_hash);
    line += " renderer_backend_parity_ready=0 renderer_metal_broad_readiness=0 renderer_broad_quality_ready=0";
    line += " renderer_commercial_readiness=0";
    return line;
}

namespace {

[[nodiscard]] bool is_reviewed_metal_environment_host_recipe(std::string_view recipe_id) noexcept {
    return recipe_id == "renderer-metal-apple-host-evidence";
}

[[nodiscard]] bool has_required_feature_evidence(const MetalEnvironmentFeatureHostEvidenceDesc& desc,
                                                 const MetalEnvironmentFeatureEvidenceRequirement& requirement) {
    return (!requirement.render_pass_required || desc.render_pass_ready) &&
           (!requirement.render_pipeline_required || desc.render_pipeline_ready) &&
           (!requirement.compute_pipeline_required || desc.compute_pipeline_ready) &&
           (!requirement.depth_texture_required || desc.depth_texture_ready) &&
           (!requirement.particle_buffer_required || desc.particle_buffer_ready) &&
           (!requirement.cube_map_texture_required || desc.cube_map_texture_feature_available) &&
           (!requirement.hdr_texture_required || desc.hdr_texture_feature_available) &&
           (!requirement.synchronization_evidence_required || desc.synchronization_evidence_ready);
}

[[nodiscard]] MetalEnvironmentFeatureEvidenceRow
make_environment_feature_evidence_row(const MetalEnvironmentFeatureHostEvidenceDesc& desc,
                                      const MetalEnvironmentFeatureEvidenceRequirement& requirement) {
    MetalEnvironmentFeatureEvidenceRow row;
    row.feature = requirement.feature;
    row.selected = requirement.selected;
    row.host_supported = supports_host(desc.host);
    row.host_evidence_available = desc.apple_host_validation_available;
    row.runtime_ready = desc.runtime_ready;
    row.command_queue_ready = desc.command_queue_ready;
    row.shader_library_ready = desc.shader_library_ready;
    row.render_pass_ready = desc.render_pass_ready;
    row.render_pipeline_ready = desc.render_pipeline_ready;
    row.compute_pipeline_ready = desc.compute_pipeline_ready;
    row.depth_texture_ready = desc.depth_texture_ready;
    row.particle_buffer_ready = desc.particle_buffer_ready;
    row.cube_map_texture_feature_available = desc.cube_map_texture_feature_available;
    row.hdr_texture_feature_available = desc.hdr_texture_feature_available;
    row.synchronization_evidence_ready = desc.synchronization_evidence_ready;
    row.native_handle_access = desc.native_handles_exposed;
    row.host_validation_recipe_id = desc.host_validation_recipe_id;

    const auto feature_label = metal_environment_feature_kind_label(requirement.feature);
    if (!requirement.selected) {
        row.status = MetalEnvironmentFeatureEvidenceStatus::blocked;
        row.diagnostic = std::string{"Metal environment feature is not selected: "} + std::string{feature_label};
        return row;
    }

    if (desc.native_handles_exposed) {
        row.status = MetalEnvironmentFeatureEvidenceStatus::blocked;
        row.diagnostic = std::string{"Metal environment feature evidence must not expose native handles: "} +
                         std::string{feature_label};
        return row;
    }

    if (!is_reviewed_metal_environment_host_recipe(desc.host_validation_recipe_id)) {
        row.status = MetalEnvironmentFeatureEvidenceStatus::blocked;
        row.diagnostic = std::string{"Metal environment feature evidence requires reviewed Apple host recipe: "} +
                         std::string{feature_label};
        return row;
    }

    const auto host_ready = row.host_supported && desc.apple_host_validation_available && desc.runtime_ready &&
                            desc.command_queue_ready && desc.shader_library_ready;
    if (!host_ready) {
        row.status = MetalEnvironmentFeatureEvidenceStatus::host_evidence_required;
        row.host_evidence_required = true;
        row.diagnostic =
            std::string{"Metal environment feature requires Apple host validation: "} + std::string{feature_label};
        return row;
    }

    if (!has_required_feature_evidence(desc, requirement)) {
        row.status = MetalEnvironmentFeatureEvidenceStatus::blocked;
        row.diagnostic =
            std::string{"Metal environment feature is missing feature-local pipeline, texture, or synchronization "
                        "proof: "} +
            std::string{feature_label};
        return row;
    }

    row.status = MetalEnvironmentFeatureEvidenceStatus::ready;
    row.diagnostic = std::string{"Metal environment feature evidence ready: "} + std::string{feature_label};
    return row;
}

} // namespace

MetalEnvironmentFeatureHostEvidencePlan
build_environment_feature_host_evidence_plan(const MetalEnvironmentFeatureHostEvidenceDesc& desc) {
    MetalEnvironmentFeatureHostEvidencePlan plan;
    const auto requirements =
        desc.requirements.empty() ? default_environment_feature_evidence_requirements() : desc.requirements;
    plan.rows.reserve(requirements.size());

    if (requirements.empty()) {
        plan.diagnostic = "Metal environment feature evidence requires at least one selected row";
        return plan;
    }

    for (const auto& requirement : requirements) {
        auto row = make_environment_feature_evidence_row(desc, requirement);
        switch (row.status) {
        case MetalEnvironmentFeatureEvidenceStatus::ready:
            ++plan.ready_row_count;
            break;
        case MetalEnvironmentFeatureEvidenceStatus::host_evidence_required:
            ++plan.host_gated_row_count;
            break;
        case MetalEnvironmentFeatureEvidenceStatus::blocked:
            ++plan.blocked_row_count;
            break;
        }
        if (row.native_handle_access) {
            ++plan.native_handle_access_count;
        }
        plan.rows.push_back(std::move(row));
    }

    if (plan.blocked_row_count > 0U || plan.native_handle_access_count > 0U) {
        plan.status = MetalEnvironmentFeatureEvidenceStatus::blocked;
        plan.diagnostic = "Metal environment feature evidence blocked";
        return plan;
    }

    if (plan.host_gated_row_count > 0U) {
        plan.status = MetalEnvironmentFeatureEvidenceStatus::host_evidence_required;
        plan.diagnostic = "Metal environment feature evidence requires Apple host validation";
        return plan;
    }

    plan.status = MetalEnvironmentFeatureEvidenceStatus::ready;
    plan.diagnostic = "Metal environment feature evidence ready";
    return plan;
}

std::string metal_environment_feature_host_evidence_status_line(const MetalEnvironmentFeatureHostEvidencePlan& plan) {
    auto feature_status = [&plan](MetalEnvironmentFeatureKind feature) {
        for (const auto& row : plan.rows) {
            if (row.feature == feature) {
                return metal_environment_feature_evidence_status_label(row.status);
            }
        }
        return std::string_view{"blocked"};
    };

    std::string line;
    line += "metal_environment_status=";
    line += metal_environment_feature_evidence_status_label(plan.status);
    line += " metal_environment_selected_backend=metal";
    line += " metal_environment_host_validation_recipe_id=renderer-metal-apple-host-evidence";
    line += " metal_environment_ready_rows=" + std::to_string(plan.ready_row_count);
    line += " metal_environment_host_gated_rows=" + std::to_string(plan.host_gated_row_count);
    line += " metal_environment_blocked_rows=" + std::to_string(plan.blocked_row_count);
    line += " metal_environment_physical_sky_status=";
    line += feature_status(MetalEnvironmentFeatureKind::physical_sky);
    line += " metal_environment_height_fog_status=";
    line += feature_status(MetalEnvironmentFeatureKind::height_fog);
    line += " metal_environment_cloud_layer_status=";
    line += feature_status(MetalEnvironmentFeatureKind::cloud_layer);
    line += " metal_environment_precipitation_status=";
    line += feature_status(MetalEnvironmentFeatureKind::precipitation);
    line += " metal_environment_volumetric_fog_status=";
    line += feature_status(MetalEnvironmentFeatureKind::volumetric_fog);
    line += " metal_environment_volumetric_cloud_status=";
    line += feature_status(MetalEnvironmentFeatureKind::volumetric_cloud);
    line += " metal_environment_lighting_ibl_status=";
    line += feature_status(MetalEnvironmentFeatureKind::environment_lighting_ibl);
    line += " metal_environment_native_handle_access=" + std::to_string(plan.native_handle_access_count);
    line += " metal_environment_broad_environment_ready_claimed=0";
    return line;
}

#if !defined(__APPLE__)
MetalNativeEnvironmentFeatureHostEvidenceResult
create_native_environment_feature_host_evidence(const MetalNativeEnvironmentFeatureHostEvidenceDesc& desc) {
    MetalNativeEnvironmentFeatureHostEvidenceResult result;
    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    if (!supports_host(host)) {
        result.diagnostic = "Metal environment native feature evidence requires an Apple host";
        return result;
    }

    result.diagnostic = "Metal environment native feature evidence is unavailable without Objective-C++";
    return result;
}

MetalNativeVisibleRendererPackageEvidenceResult
create_native_visible_renderer_package_evidence(const MetalNativeVisibleRendererPackageEvidenceDesc& desc) {
    MetalNativeVisibleRendererPackageEvidenceResult result;
    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    if (!supports_host(host)) {
        result.diagnostic = "Metal visible renderer package native evidence requires an Apple host";
        return result;
    }

    result.environment_package_replay_hash = desc.environment_package_replay_hash;
    result.generated_game_package_replay_hash = desc.generated_game_package_replay_hash;
    result.diagnostic = "Metal visible renderer package native evidence is unavailable without Objective-C++";
    return result;
}
#endif

MetalRuntimeDevice::MetalRuntimeDevice() noexcept = default;

MetalRuntimeDevice::~MetalRuntimeDevice() = default;

MetalRuntimeDevice::MetalRuntimeDevice(MetalRuntimeDevice&& other) noexcept = default;

MetalRuntimeDevice& MetalRuntimeDevice::operator=(MetalRuntimeDevice&& other) noexcept = default;

MetalRuntimeDevice::MetalRuntimeDevice(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool MetalRuntimeDevice::owns_device() const noexcept {
    return impl_ != nullptr && impl_->device != nullptr;
}

bool MetalRuntimeDevice::owns_command_queue() const noexcept {
    return impl_ != nullptr && impl_->command_queue != nullptr;
}

bool MetalRuntimeDevice::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void MetalRuntimeDevice::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
    impl_.reset();
}

MetalRuntimeCommandBuffer::MetalRuntimeCommandBuffer() noexcept = default;

MetalRuntimeCommandBuffer::~MetalRuntimeCommandBuffer() = default;

MetalRuntimeCommandBuffer::MetalRuntimeCommandBuffer(MetalRuntimeCommandBuffer&& other) noexcept = default;

MetalRuntimeCommandBuffer& MetalRuntimeCommandBuffer::operator=(MetalRuntimeCommandBuffer&& other) noexcept = default;

MetalRuntimeCommandBuffer::MetalRuntimeCommandBuffer(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool MetalRuntimeCommandBuffer::owns_command_buffer() const noexcept {
    return impl_ != nullptr && impl_->command_buffer != nullptr;
}

bool MetalRuntimeCommandBuffer::committed() const noexcept {
    return impl_ != nullptr && impl_->committed;
}

bool MetalRuntimeCommandBuffer::completed() const noexcept {
    return impl_ != nullptr && impl_->completed;
}

bool MetalRuntimeCommandBuffer::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void MetalRuntimeCommandBuffer::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
    impl_.reset();
}

MetalRuntimeTexture::MetalRuntimeTexture() noexcept = default;

MetalRuntimeTexture::~MetalRuntimeTexture() = default;

MetalRuntimeTexture::MetalRuntimeTexture(MetalRuntimeTexture&& other) noexcept = default;

MetalRuntimeTexture& MetalRuntimeTexture::operator=(MetalRuntimeTexture&& other) noexcept = default;

MetalRuntimeTexture::MetalRuntimeTexture(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool MetalRuntimeTexture::owns_texture() const noexcept {
    return impl_ != nullptr && impl_->texture != nullptr;
}

Extent2D MetalRuntimeTexture::extent() const noexcept {
    return impl_ != nullptr ? impl_->extent : Extent2D{};
}

Format MetalRuntimeTexture::format() const noexcept {
    return impl_ != nullptr ? impl_->format : Format::unknown;
}

bool MetalRuntimeTexture::drawable() const noexcept {
    return impl_ != nullptr && impl_->drawable;
}

bool MetalRuntimeTexture::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void MetalRuntimeTexture::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
    impl_.reset();
}

MetalRuntimeDrawable::MetalRuntimeDrawable() noexcept = default;

MetalRuntimeDrawable::~MetalRuntimeDrawable() = default;

MetalRuntimeDrawable::MetalRuntimeDrawable(MetalRuntimeDrawable&& other) noexcept = default;

MetalRuntimeDrawable& MetalRuntimeDrawable::operator=(MetalRuntimeDrawable&& other) noexcept = default;

MetalRuntimeDrawable::MetalRuntimeDrawable(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool MetalRuntimeDrawable::owns_drawable() const noexcept {
    return impl_ != nullptr && impl_->drawable != nullptr;
}

bool MetalRuntimeDrawable::owns_texture() const noexcept {
    return impl_ != nullptr && impl_->texture != nullptr;
}

Extent2D MetalRuntimeDrawable::extent() const noexcept {
    return impl_ != nullptr ? impl_->extent : Extent2D{};
}

Format MetalRuntimeDrawable::format() const noexcept {
    return impl_ != nullptr ? impl_->format : Format::unknown;
}

bool MetalRuntimeDrawable::presented() const noexcept {
    return impl_ != nullptr && impl_->presented;
}

bool MetalRuntimeDrawable::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void MetalRuntimeDrawable::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
    impl_.reset();
}

MetalRuntimeRenderEncoder::MetalRuntimeRenderEncoder() noexcept = default;

MetalRuntimeRenderEncoder::~MetalRuntimeRenderEncoder() = default;

MetalRuntimeRenderEncoder::MetalRuntimeRenderEncoder(MetalRuntimeRenderEncoder&& other) noexcept = default;

MetalRuntimeRenderEncoder& MetalRuntimeRenderEncoder::operator=(MetalRuntimeRenderEncoder&& other) noexcept = default;

MetalRuntimeRenderEncoder::MetalRuntimeRenderEncoder(std::shared_ptr<Impl> impl) noexcept : impl_(std::move(impl)) {}

bool MetalRuntimeRenderEncoder::owns_render_encoder() const noexcept {
    return impl_ != nullptr && impl_->render_encoder != nullptr;
}

bool MetalRuntimeRenderEncoder::ended() const noexcept {
    return impl_ != nullptr && impl_->ended;
}

bool MetalRuntimeRenderEncoder::destroyed() const noexcept {
    return impl_ != nullptr && impl_->destroyed;
}

void MetalRuntimeRenderEncoder::end() noexcept {
    if (impl_ != nullptr) {
        impl_->end();
    }
}

void MetalRuntimeRenderEncoder::reset() noexcept {
    if (impl_ != nullptr) {
        impl_->reset();
    }
    impl_.reset();
}

#if !defined(__APPLE__)
MetalRuntimeDeviceQueueCreateResult create_native_device_and_command_queue(const MetalNativeDeviceQueueDesc& desc) {
    MetalRuntimeDeviceQueueCreateResult result;
    const auto host = desc.host == RhiHostPlatform::unknown ? current_rhi_host_platform() : desc.host;
    result.diagnostic = "Metal native device and command queue require an Apple host";
    result.probe = make_probe_result(host, BackendProbeStatus::unsupported_host, result.diagnostic);
    return result;
}

MetalRuntimeCommandBufferCreateResult create_native_command_buffer(MetalRuntimeDevice& device) {
    MetalRuntimeCommandBufferCreateResult result;
    if (!device.owns_command_queue()) {
        result.diagnostic = "Metal command queue is required before creating a command buffer";
        return result;
    }

    result.diagnostic = "Metal native command buffers require an Apple host";
    return result;
}

MetalRuntimeTextureCreateResult create_native_texture_target(MetalRuntimeDevice& device,
                                                             const MetalTextureTargetDesc& desc) {
    MetalRuntimeTextureCreateResult result;
    if (!device.owns_device()) {
        result.diagnostic = "Metal device is required before creating a texture target";
        return result;
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        result.diagnostic = "Metal texture target extent is required";
        return result;
    }

    result.diagnostic = "Metal native texture targets require an Apple host";
    return result;
}

MetalRuntimeDrawableAcquireResult acquire_native_drawable(MetalRuntimeDevice& device,
                                                          const MetalDrawableAcquireDesc& desc) {
    MetalRuntimeDrawableAcquireResult result;
    if (!device.owns_device()) {
        result.diagnostic = "Metal device is required before acquiring a drawable";
        return result;
    }
    if (desc.surface.value == 0) {
        result.diagnostic = "Metal drawable surface handle is required";
        return result;
    }
    if (desc.extent.width == 0 || desc.extent.height == 0) {
        result.diagnostic = "Metal drawable extent is required";
        return result;
    }

    result.diagnostic = "Metal native drawable acquisition requires an Apple host";
    return result;
}

MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                   MetalRuntimeTexture& texture,
                                                                   const MetalRenderEncoderDesc& /*unused*/) {
    MetalRuntimeRenderEncoderCreateResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before creating a render encoder";
        return result;
    }
    if (!texture.owns_texture()) {
        result.diagnostic = "Metal texture target is required before creating a render encoder";
        return result;
    }

    result.diagnostic = "Metal native render encoders require an Apple host";
    return result;
}

MetalRuntimeRenderEncoderCreateResult create_native_render_encoder(MetalRuntimeCommandBuffer& command_buffer,
                                                                   MetalRuntimeDrawable& drawable,
                                                                   const MetalRenderEncoderDesc& /*unused*/) {
    MetalRuntimeRenderEncoderCreateResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before creating a render encoder";
        return result;
    }
    if (!drawable.owns_texture()) {
        result.diagnostic = "Metal drawable texture is required before creating a render encoder";
        return result;
    }

    result.diagnostic = "Metal native render encoders require an Apple host";
    return result;
}

MetalRuntimeCommandBufferSubmitResult commit_and_wait_native_command_buffer(MetalRuntimeCommandBuffer& command_buffer) {
    MetalRuntimeCommandBufferSubmitResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before commit";
        return result;
    }

    result.diagnostic = "Metal native command buffer submission requires an Apple host";
    return result;
}

MetalRuntimeTextureUploadResult upload_native_texture_bytes(MetalRuntimeTexture& texture,
                                                            const MetalRuntimeTextureUploadDesc& desc) {
    MetalRuntimeTextureUploadResult result;
    if (!texture.owns_texture()) {
        result.diagnostic = "Metal texture is required before texture upload";
        return result;
    }
    if (desc.bytes == nullptr || desc.byte_count == 0) {
        result.diagnostic = "Metal texture upload bytes are required";
        return result;
    }

    result.diagnostic = "Metal native texture upload requires an Apple host";
    return result;
}

MetalRuntimeTextureReadbackResult read_native_texture_bytes(MetalRuntimeDevice& device, MetalRuntimeTexture& texture) {
    MetalRuntimeTextureReadbackResult result;
    if (!device.owns_command_queue()) {
        result.diagnostic = "Metal command queue is required before texture readback";
        return result;
    }
    if (!texture.owns_texture()) {
        result.diagnostic = "Metal texture is required before readback";
        return result;
    }

    result.diagnostic = "Metal native texture readback requires an Apple host";
    return result;
}

MetalRuntimeDrawablePresentResult present_native_drawable(MetalRuntimeCommandBuffer& command_buffer,
                                                          MetalRuntimeDrawable& drawable) {
    MetalRuntimeDrawablePresentResult result;
    if (!command_buffer.owns_command_buffer()) {
        result.diagnostic = "Metal command buffer is required before presenting a drawable";
        return result;
    }
    if (!drawable.owns_drawable()) {
        result.diagnostic = "Metal drawable is required before present";
        return result;
    }

    result.diagnostic = "Metal native drawable present requires an Apple host";
    return result;
}
#endif

} // namespace mirakana::rhi::metal
