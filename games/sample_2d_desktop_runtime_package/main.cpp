// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/behavior_tree.hpp"
#include "mirakana/ai/perception.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"
#include "mirakana/physics/physics2d.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/runtime/addressable_content_streaming.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/entity_scale_culling.hpp"
#include "mirakana/runtime/gameplay_interaction.hpp"
#include "mirakana/runtime/gameplay_runtime_scheduler.hpp"
#include "mirakana/runtime/genre_rpg_systems.hpp"
#include "mirakana/runtime/genre_sandbox_world.hpp"
#include "mirakana/runtime/genre_simulation_management.hpp"
#include "mirakana/runtime/inventory_items.hpp"
#include "mirakana/runtime/network_production_security.hpp"
#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/procedural_generation.hpp"
#include "mirakana/runtime/production_network_replication.hpp"
#include "mirakana/runtime/quest_dialogue.hpp"
#include "mirakana/runtime/runtime_diagnostics.hpp"
#include "mirakana/runtime/scripting_sandbox.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/runtime/simulation_orchestration.hpp"
#include "mirakana/runtime/sprite_collision_hitbox.hpp"
#include "mirakana/runtime/sprite_effect_particles.hpp"
#include "mirakana/runtime/world_entity_model.hpp"
#include "mirakana/runtime/world_region_streaming.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"
#include "mirakana/scene/playable_2d.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/tools/gameplay_authoring_tool.hpp"
#include "mirakana/tools/production_authoring_workflows.hpp"
#include "mirakana/ui/runtime_ui_production_stack.hpp"
#include "mirakana/ui/runtime_ui_workbench.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct DesktopRuntimeOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    bool require_d3d12_shaders{false};
    bool require_d3d12_renderer{false};
    bool require_vulkan_shaders{false};
    bool require_vulkan_renderer{false};
    bool require_native_2d_sprites{false};
    bool require_sprite_animation{false};
    bool require_sprite_sorting_layer{false};
    bool require_sprite_9slice_tiled{false};
    bool require_sprite_collision_hitbox{false};
    bool require_sprite_effects_particles{false};
    bool require_tilemap_runtime_ux{false};
    bool require_gameplay_systems{false};
    bool require_procedural_generation{false};
    bool require_world_region_streaming{false};
    bool require_entity_scale_culling{false};
    bool require_scripting_sandbox_policy{false};
    bool require_networking_foundation_policy{false};
    bool require_simulation_orchestration{false};
    bool require_gameplay_authoring_review{false};
    bool require_production_authoring_workflows{false};
    bool require_runtime_profile_resume{false};
    bool require_runtime_menu_hud{false};
    bool require_runtime_ui_workbench{false};
    bool require_runtime_ui_production_stack{false};
    bool require_runtime_ui_renderer_atlas_handoff{false};
    bool require_audio_gameplay_mixer{false};
    std::uint32_t max_frames{0};
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.Sample2DDesktopRuntimePackage.Config.v1"};
constexpr std::string_view kRuntime2dVertexShaderPath{"shaders/sample_2d_desktop_runtime_package_sprite.vs.dxil"};
constexpr std::string_view kRuntime2dFragmentShaderPath{"shaders/sample_2d_desktop_runtime_package_sprite.ps.dxil"};
constexpr std::string_view kRuntime2dVulkanVertexShaderPath{"shaders/sample_2d_desktop_runtime_package_sprite.vs.spv"};
constexpr std::string_view kRuntime2dVulkanFragmentShaderPath{
    "shaders/sample_2d_desktop_runtime_package_sprite.ps.spv"};
constexpr std::string_view kRuntime2dNativeSpriteOverlayVertexShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil"};
constexpr std::string_view kRuntime2dNativeSpriteOverlayFragmentShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil"};
constexpr std::string_view kRuntime2dVulkanNativeSpriteOverlayVertexShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv"};
constexpr std::string_view kRuntime2dVulkanNativeSpriteOverlayFragmentShaderPath{
    "shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv"};
constexpr mirakana::SceneNodeId kPlayerNode{2};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dRootNode{1};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dHasTargetNode{2};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dNeedsMoveNode{3};
constexpr mirakana::BehaviorTreeNodeId kGameplay2dMoveActionNode{4};
constexpr const char* kGameplay2dBehaviorId{"sample_2d_move_to_target"};
constexpr const char* kGameplay2dMoveActionId{"move_to_target"};
constexpr const char* kGameplay2dHasTargetKey{"sample2d.has_target"};
constexpr const char* kGameplay2dNeedsMoveKey{"sample2d.needs_move"};
constexpr const char* kGameplay2dTargetIdKey{"sample2d.target_id"};
constexpr const char* kGameplay2dTargetDistanceKey{"sample2d.target_distance"};
constexpr const char* kGameplay2dVisibleTargetsKey{"sample2d.visible_targets"};
constexpr const char* kGameplay2dAudibleTargetsKey{"sample2d.audible_targets"};
constexpr const char* kGameplay2dTargetStateKey{"sample2d.target_state"};
constexpr std::uint64_t kSpriteBatchWorldMaxSprites{512U};
constexpr std::uint64_t kSpriteBatchWorldMaxDraws{64U};
constexpr std::uint64_t kSpriteBatchWorldMaxTextureBinds{64U};
constexpr std::uint64_t kSpriteBatchUiMaxSprites{128U};
constexpr std::uint64_t kSpriteBatchUiMaxDraws{32U};
constexpr std::uint64_t kSpriteBatchUiMaxTextureBinds{0U};
constexpr std::uint64_t kSpriteBatchEffectsMaxSprites{256U};
constexpr std::uint64_t kSpriteBatchEffectsMaxDraws{64U};
constexpr std::uint64_t kSpriteBatchEffectsMaxTextureBinds{64U};

enum class Gameplay2DSystemsStatus : std::uint8_t {
    not_started,
    ready,
    diagnostics,
};

struct WorldRegionStreamingProbeResult {
    mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus status{
        mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::invalid_plan};
    mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus large_scene_readiness_status{
        mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus::invalid_evidence};
    mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic large_scene_readiness_diagnostic{
        mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::none};
    std::size_t plan_rows{0U};
    std::size_t load_rows{0U};
    std::size_t keep_rows{0U};
    std::size_t unload_rows{0U};
    std::size_t safe_point_rows{0U};
    std::size_t committed_rows{0U};
    std::size_t reviewed_package_adoptions{0U};
    std::size_t projected_regions{0U};
    std::uint64_t projected_bytes{0U};
    std::uint64_t budget_bytes{0U};
    std::size_t missing_region_diagnostics{0U};
    std::size_t safe_point_diagnostics{0U};
    std::size_t large_scene_readiness_diagnostics{0U};
    std::size_t navigation_resident_regions{0U};
    std::size_t navigation_missing_resident_regions{0U};
    bool navigation_path_cache_ready{false};
    bool ready{false};
};

struct EntityScaleCullingProbeResult {
    mirakana::runtime::RuntimeEntityScaleCullingPlanStatus status{
        mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::invalid_request};
    std::size_t rows{0U};
    std::size_t visible_rows{0U};
    std::size_t culled_rows{0U};
    std::size_t lod_rows{0U};
    std::size_t priority_update_rows{0U};
    std::size_t normal_update_rows{0U};
    std::size_t background_update_rows{0U};
    std::uint64_t projected_draw_cost{0U};
    std::uint64_t projected_update_cost{0U};
    std::size_t budget_protected_rows{0U};
    std::size_t diagnostics{0U};
    std::size_t budget_diagnostics{0U};
    bool ready{false};
};

struct ScriptingSandboxProbeResult {
    mirakana::runtime::RuntimeScriptSandboxPlanStatus status{
        mirakana::runtime::RuntimeScriptSandboxPlanStatus::invalid_request};
    std::size_t entrypoint_rows{0U};
    std::size_t permission_rows{0U};
    std::size_t allowed_permission_rows{0U};
    std::size_t denied_permission_rows{0U};
    std::size_t rejected_unsafe_capability_rows{0U};
    std::size_t unsupported_host_api_diagnostics{0U};
    std::size_t budget_rows{0U};
    std::uint64_t projected_instruction_budget{0U};
    std::uint64_t projected_memory_budget_bytes{0U};
    std::size_t budget_diagnostics{0U};
    std::size_t replay_seed_rows{0U};
    std::uint64_t replay_seed_sum{0U};
    std::size_t diagnostics{0U};
    mirakana::runtime::RuntimeScriptExecutionStatus execution_status{
        mirakana::runtime::RuntimeScriptExecutionStatus::invalid_request};
    std::size_t execution_dispatches{0U};
    std::size_t execution_host_api_calls{0U};
    std::uint64_t execution_replay_signature{0U};
    std::size_t execution_diagnostics{0U};
    bool execution_ready{false};
    bool ready{false};
};

struct NetworkingFoundationProbeResult {
    mirakana::runtime::RuntimeNetworkFoundationPlanStatus status{
        mirakana::runtime::RuntimeNetworkFoundationPlanStatus::invalid_request};
    std::size_t session_rows{0U};
    std::size_t transport_rows{0U};
    std::size_t channel_rows{0U};
    std::size_t rejected_unsafe_transport_rows{0U};
    std::size_t replay_prerequisite_rows{0U};
    std::uint64_t replay_seed_sum{0U};
    std::size_t remote_session_rows{0U};
    std::size_t secure_remote_session_rows{0U};
    std::size_t security_diagnostics{0U};
    std::size_t diagnostics{0U};
    bool ready{false};
};

struct NetworkReplicationProbeResult {
    mirakana::runtime::RuntimeNetworkReplicationStatus status{
        mirakana::runtime::RuntimeNetworkReplicationStatus::invalid_request};
    std::size_t object_rows{0U};
    std::size_t input_rows{0U};
    std::size_t snapshot_rows{0U};
    std::size_t rollback_rows{0U};
    std::size_t rejected_unsafe_rows{0U};
    std::uint64_t replay_hash{0U};
    bool requires_transport_host_evidence{false};
    bool has_transport_host_evidence{false};
    bool invoked_network_io{false};
    bool invoked_rollback_execution{false};
    bool invoked_world_mutation{false};
    std::size_t diagnostics{0U};
    bool reviewed{false};
    bool ready{false};
};

struct NetworkProductionSecurityProbeResult {
    mirakana::runtime::RuntimeNetworkProductionSecurityStatus status{
        mirakana::runtime::RuntimeNetworkProductionSecurityStatus::invalid_request};
    std::size_t session_lifecycle_rows{0U};
    std::size_t connection_state_rows{0U};
    std::size_t channel_policy_rows{0U};
    std::size_t reliable_delivery_rows{0U};
    std::size_t unreliable_delivery_rows{0U};
    std::size_t sequence_replay_rejection_rows{0U};
    std::size_t input_command_validation_rows{0U};
    std::size_t snapshot_validation_rows{0U};
    std::size_t rollback_window_diagnostic_rows{0U};
    std::size_t unsupported_online_claim_rows{0U};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool threat_model_reviewed{false};
    bool loopback_host_evidence{false};
    bool replication_evidence_ready{false};
    bool general_online_ready{false};
    bool invoked_external_network_io{false};
    bool invoked_threads{false};
    bool invoked_save_io{false};
    bool invoked_world_mutation{false};
    bool reviewed{false};
    bool ready{false};
};

struct SimulationOrchestrationProbeResult {
    mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus status{
        mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus::invalid_request};
    mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus budget_limited_status{
        mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus::invalid_request};
    std::size_t available_steps{0U};
    std::size_t planned_steps{0U};
    std::size_t step_rows{0U};
    std::size_t command_rows{0U};
    std::size_t command_playback_rows{0U};
    std::uint64_t consumed_time_us{0U};
    std::uint64_t remaining_time_us{0U};
    std::size_t budget_limited_available_steps{0U};
    std::size_t budget_limited_planned_steps{0U};
    std::uint64_t budget_limited_remaining_time_us{0U};
    std::size_t invalid_command_diagnostics{0U};
    std::size_t diagnostics{0U};
    bool ready{false};
};

struct GameplayRuntimeSchedulerProbeResult {
    mirakana::runtime::RuntimeGameplaySchedulerStatus status{
        mirakana::runtime::RuntimeGameplaySchedulerStatus::invalid_request};
    std::size_t available_steps{0U};
    std::size_t planned_steps{0U};
    std::size_t step_rows{0U};
    std::size_t system_rows{0U};
    std::size_t command_rows{0U};
    std::uint64_t consumed_time_us{0U};
    std::uint64_t remaining_time_us{0U};
    bool budget_limited{false};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool ready{false};
};

struct WorldEntityModelProbeResult {
    mirakana::runtime::RuntimeWorldEntityLifecycleStatus status{
        mirakana::runtime::RuntimeWorldEntityLifecycleStatus::invalid_request};
    std::size_t entity_rows{0U};
    std::size_t component_rows{0U};
    std::size_t region_ownership_rows{0U};
    std::size_t lifecycle_rows{0U};
    std::size_t persistence_rows{0U};
    std::size_t streaming_region_rows{0U};
    std::size_t spawn_rows{0U};
    std::size_t move_rows{0U};
    std::size_t despawn_rows{0U};
    std::size_t duplicate_entity_diagnostics{0U};
    mirakana::runtime::RuntimeWorldEntityLifecycleStatus bridge_rejection_status{
        mirakana::runtime::RuntimeWorldEntityLifecycleStatus::invalid_request};
    std::size_t bridge_rejection_diagnostics{0U};
    std::size_t bridge_rejection_persistence_rows{0U};
    std::size_t bridge_rejection_streaming_region_rows{0U};
    std::size_t bridge_rejection_streaming_diagnostics_present{0U};
    bool bridge_rejection_fail_closed{false};
    std::size_t diagnostics{0U};
    bool ready{false};
};

struct AddressableContentStreamingProbeResult {
    mirakana::runtime::RuntimeAddressableContentStreamingStatus status{
        mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request};
    mirakana::runtime::RuntimeAddressableContentStreamingStatus budget_rejection_status{
        mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request};
    std::size_t address_rows{0U};
    std::size_t dependency_rows{0U};
    std::size_t load_rows{0U};
    std::size_t release_rows{0U};
    std::size_t refcount_rows{0U};
    std::uint64_t resident_bytes{0U};
    std::uint64_t resident_budget_bytes{0U};
    std::size_t budget_rejection_diagnostics{0U};
    std::size_t diagnostics{0U};
    bool package_io{false};
    bool async_execution{false};
    bool committed{false};
    bool ready{false};
};

struct RpgSystemsProbeResult {
    mirakana::runtime::RuntimeRpgSystemsStatus status{mirakana::runtime::RuntimeRpgSystemsStatus::invalid_request};
    std::size_t party_members{0U};
    std::size_t enemy_members{0U};
    std::size_t stat_rows{0U};
    std::size_t progression_rows{0U};
    std::size_t skill_rows{0U};
    std::size_t blocked_skill_rows{0U};
    std::size_t equipment_rows{0U};
    std::size_t blocked_equipment_rows{0U};
    std::size_t combat_turn_rows{0U};
    std::size_t combat_rounds{0U};
    std::size_t reward_rows{0U};
    std::size_t save_validation_rows{0U};
    std::size_t repairable_save_validation_rows{0U};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_combat_execution{false};
    bool invoked_reward_application{false};
    bool invoked_save_io{false};
    bool ready{false};
};

struct SandboxWorldProbeResult {
    mirakana::runtime::RuntimeSandboxWorldStatus status{mirakana::runtime::RuntimeSandboxWorldStatus::invalid_request};
    std::size_t chunk_rows{0U};
    std::size_t resident_chunk_rows{0U};
    std::size_t existing_cell_rows{0U};
    std::size_t placement_intent_rows{0U};
    std::size_t placement_accepted_rows{0U};
    std::size_t placement_rejected_rows{0U};
    std::size_t destruction_intent_rows{0U};
    std::size_t destruction_accepted_rows{0U};
    std::size_t destruction_rejected_rows{0U};
    std::size_t construction_cost_rows{0U};
    std::size_t mutation_rows{0U};
    std::size_t persistence_rows{0U};
    std::size_t persistence_repairable_rows{0U};
    std::size_t rejected_unsafe_mutation_rows{0U};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_world_mutation{false};
    bool invoked_persistence_io{false};
    bool invoked_package_io{false};
    bool ready{false};
};

struct SimulationManagementProbeResult {
    mirakana::runtime::RuntimeSimulationManagementStatus status{
        mirakana::runtime::RuntimeSimulationManagementStatus::invalid_request};
    std::uint64_t tick_count{0U};
    std::size_t resource_balance_rows{0U};
    std::size_t job_rows{0U};
    std::size_t job_assignment_rows{0U};
    std::size_t logistics_links{0U};
    std::size_t logistics_transfer_rows{0U};
    std::size_t logistics_scheduled_transfer_rows{0U};
    std::size_t economy_summary_rows{0U};
    std::size_t population_need_rows{0U};
    std::size_t need_deficit_rows{0U};
    std::size_t schedule_rows{0U};
    std::size_t save_review_rows{0U};
    std::size_t save_review_repairable_rows{0U};
    std::size_t dashboard_rows{0U};
    std::uint64_t replay_hash{0U};
    std::size_t diagnostics{0U};
    bool invoked_economy_execution{false};
    bool invoked_save_io{false};
    bool invoked_runtime_ui{false};
    bool invoked_package_io{false};
    bool ready{false};
};

struct GameplayAuthoringReviewProbeResult {
    std::size_t feature_rows{0U};
    std::size_t accepted_rows{0U};
    std::size_t mutation_ledger_rows{0U};
    std::size_t remediation_rows{0U};
    std::size_t missing_required_capability_diagnostics{0U};
    std::size_t missing_validation_recipe_diagnostics{0U};
    std::size_t missing_package_evidence_diagnostics{0U};
    std::size_t unsupported_claim_diagnostics{0U};
    std::size_t diagnostics{0U};
    bool ready{false};
};

struct ProductionAuthoringWorkflowProbeResult {
    std::size_t workflow_rows{0U};
    std::size_t accepted_rows{0U};
    std::size_t mutation_ledger_rows{0U};
    std::size_t validation_repair_rows{0U};
    std::size_t shared_surface_mutation_diagnostics{0U};
    std::size_t arbitrary_shell_diagnostics{0U};
    std::size_t cooked_package_mutation_diagnostics{0U};
    std::size_t native_backend_term_diagnostics{0U};
    std::size_t invalid_target_path_diagnostics{0U};
    std::size_t diagnostics{0U};
    bool invoked_file_mutation{false};
    bool invoked_package_io{false};
    bool invoked_command_execution{false};
    bool ready{false};
};

[[nodiscard]] std::string_view gameplay_2d_systems_status_name(Gameplay2DSystemsStatus status) noexcept {
    switch (status) {
    case Gameplay2DSystemsStatus::not_started:
        return "not_started";
    case Gameplay2DSystemsStatus::ready:
        return "ready";
    case Gameplay2DSystemsStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
sprite_batch_budget_profile_status_name(mirakana::SpriteBatchBudgetProfileStatus status) noexcept {
    switch (status) {
    case mirakana::SpriteBatchBudgetProfileStatus::ready:
        return "ready";
    case mirakana::SpriteBatchBudgetProfileStatus::invalid_request:
        return "invalid_request";
    case mirakana::SpriteBatchBudgetProfileStatus::diagnostics:
        return "diagnostics";
    case mirakana::SpriteBatchBudgetProfileStatus::budget_exceeded:
        return "budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
runtime_gameplay_session_state_name(mirakana::runtime::RuntimeGameplaySessionState state) noexcept {
    switch (state) {
    case mirakana::runtime::RuntimeGameplaySessionState::running:
        return "running";
    case mirakana::runtime::RuntimeGameplaySessionState::won:
        return "won";
    case mirakana::runtime::RuntimeGameplaySessionState::lost:
        return "lost";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
runtime_scene_gameplay_session_state_name(mirakana::runtime_scene::RuntimeSceneGameplaySessionState state) noexcept {
    switch (state) {
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running:
        return "running";
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won:
        return "won";
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::lost:
        return "lost";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
runtime_session_profile_resume_status_name(mirakana::runtime::RuntimeSessionProfileResumeStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSessionProfileResumeStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSessionProfileResumeStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

struct SceneGameplayBindingProbeResult {
    std::size_t source_rows{0U};
    std::size_t binding_rows{0U};
    std::size_t gameplay_systems{0U};
    std::size_t component_rows{0U};
    std::size_t binding_diagnostics{0U};
    std::size_t interaction_rows{0U};
    std::size_t interaction_diagnostics{0U};
    mirakana::runtime_scene::RuntimeSceneGameplaySessionState final_session_state{
        mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running};
    bool ready{false};
};

struct InputContextRebindingProbeResult {
    std::size_t requested_layers{0U};
    std::size_t active_contexts{0U};
    bool capture_context_active{false};
    bool gameplay_input_consumed{false};
    std::size_t profile_overlays_applied{0U};
    mirakana::runtime::RuntimeInputRebindingCaptureStatus action_capture_status{
        mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting};
    mirakana::runtime::RuntimeInputRebindingCaptureStatus axis_capture_status{
        mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting};
    bool focus_gameplay_consumed{false};
    bool focus_retained{false};
    std::size_t presentation_rows{0U};
    std::size_t glyph_lookup_keys{0U};
    std::size_t diagnostics{0U};
    bool ready{false};
};

[[nodiscard]] std::size_t count_scene_gameplay_binding_systems(
    std::span<const mirakana::runtime_scene::RuntimeSceneGameplayBindingRow> bindings) {
    std::vector<std::string> systems;
    systems.reserve(bindings.size());
    for (const auto& binding : bindings) {
        if (!std::ranges::contains(systems, binding.gameplay_system_id)) {
            systems.push_back(binding.gameplay_system_id);
        }
    }
    return systems.size();
}

[[nodiscard]] std::string_view
runtime_input_rebinding_capture_status_name(mirakana::runtime::RuntimeInputRebindingCaptureStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting:
        return "waiting";
    case mirakana::runtime::RuntimeInputRebindingCaptureStatus::captured:
        return "captured";
    case mirakana::runtime::RuntimeInputRebindingCaptureStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger
make_input_rebinding_gamepad_button_trigger(mirakana::GamepadId gamepad_id, mirakana::GamepadButton button) {
    return mirakana::runtime::RuntimeInputActionTrigger{
        .kind = mirakana::runtime::RuntimeInputActionTriggerKind::gamepad_button,
        .key = mirakana::Key::unknown,
        .pointer_id = mirakana::PointerId{0},
        .gamepad_id = gamepad_id,
        .gamepad_button = button,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeInputAxisSource
make_input_rebinding_gamepad_axis_source(mirakana::GamepadId gamepad_id, mirakana::GamepadAxis axis, float scale,
                                         float deadzone) {
    return mirakana::runtime::RuntimeInputAxisSource{
        .kind = mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis,
        .negative_key = mirakana::Key::unknown,
        .positive_key = mirakana::Key::unknown,
        .gamepad_id = gamepad_id,
        .gamepad_axis = axis,
        .scale = scale,
        .deadzone = deadzone,
    };
}

[[nodiscard]] std::size_t
count_input_rebinding_glyph_lookup_keys(const mirakana::runtime::RuntimeInputRebindingPresentationModel& model) {
    std::size_t count = 0U;
    const auto count_tokens = [&count](
                                  std::span<const mirakana::runtime::RuntimeInputRebindingPresentationToken> tokens) {
        count += static_cast<std::size_t>(std::count_if(
            tokens.begin(), tokens.end(), [](const mirakana::runtime::RuntimeInputRebindingPresentationToken& token) {
                return !token.glyph_lookup_key.empty();
            }));
    };
    for (const auto& row : model.rows) {
        count_tokens(row.base_tokens);
        count_tokens(row.profile_tokens);
    }
    return count;
}

[[nodiscard]] InputContextRebindingProbeResult validate_sample_input_context_rebinding() {
    mirakana::runtime::RuntimeInputContextStackRequest context_request;
    context_request.layers = {
        mirakana::runtime::RuntimeInputContextLayerDesc{
            .context = "rebinding_confirm",
            .kind = mirakana::runtime::RuntimeInputContextLayerKind::rebinding,
            .active = true,
            .blocks_lower_priority = true,
            .consumes_gameplay_input = true,
        },
        mirakana::runtime::RuntimeInputContextLayerDesc{
            .context = "hud",
            .kind = mirakana::runtime::RuntimeInputContextLayerKind::overlay,
            .active = true,
            .blocks_lower_priority = false,
            .consumes_gameplay_input = false,
        },
        mirakana::runtime::RuntimeInputContextLayerDesc{
            .context = "gameplay",
            .kind = mirakana::runtime::RuntimeInputContextLayerKind::gameplay,
            .active = true,
            .blocks_lower_priority = false,
            .consumes_gameplay_input = false,
        },
    };
    const auto context_plan = mirakana::runtime::plan_runtime_input_context_stack(context_request);

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_pointer_in_context("gameplay", "confirm", mirakana::primary_pointer_id);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {make_input_rebinding_gamepad_button_trigger(mirakana::GamepadId{1},
                                                                 mirakana::GamepadButton::south)},
    });
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {make_input_rebinding_gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x,
                                                             1.0F, 0.25F)},
    });

    const auto applied = mirakana::runtime::apply_runtime_input_rebinding_profile(base, profile);

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::up);
    mirakana::runtime::RuntimeInputRebindingCaptureRequest action_request;
    action_request.context = "gameplay";
    action_request.action = "confirm";
    action_request.state.keyboard = &keyboard;
    const auto action_capture =
        mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, action_request);

    mirakana::VirtualGamepadInput gamepad;
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, 0.9F);
    mirakana::runtime::RuntimeInputRebindingAxisCaptureRequest axis_request;
    axis_request.context = "gameplay";
    axis_request.action = "move_x";
    axis_request.state.gamepad = &gamepad;
    const auto axis_capture = mirakana::runtime::capture_runtime_input_rebinding_axis(base, profile, axis_request);

    mirakana::VirtualInput focus_keyboard;
    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest focus_request;
    focus_request.capture.context = "gameplay";
    focus_request.capture.action = "confirm";
    focus_request.capture.state.keyboard = &focus_keyboard;
    focus_request.capture_id = "rebinding_confirm";
    focus_request.focused_id = "rebinding_confirm";
    focus_request.modal_layer_id = "rebinding_confirm";
    focus_request.armed = true;
    focus_request.consume_gameplay_input = true;
    const auto focus_capture =
        mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, focus_request);

    const auto presentation = mirakana::runtime::make_runtime_input_rebinding_presentation(base, profile);

    InputContextRebindingProbeResult result{
        .requested_layers = context_request.layers.size(),
        .active_contexts = context_plan.stack.active_contexts.size(),
        .capture_context_active = context_plan.capture_context_active,
        .gameplay_input_consumed = context_plan.gameplay_input_consumed,
        .profile_overlays_applied = applied.action_overrides_applied + applied.axis_overrides_applied,
        .action_capture_status = action_capture.status,
        .axis_capture_status = axis_capture.status,
        .focus_gameplay_consumed = focus_capture.gameplay_input_consumed,
        .focus_retained = focus_capture.focus_retained,
        .presentation_rows = presentation.rows.size(),
        .glyph_lookup_keys = count_input_rebinding_glyph_lookup_keys(presentation),
        .diagnostics = context_plan.diagnostics.size() + applied.diagnostics.size() +
                       action_capture.diagnostics.size() + axis_capture.diagnostics.size() +
                       focus_capture.diagnostics.size() + presentation.diagnostics.size(),
    };
    result.ready = context_plan.succeeded() && applied.succeeded() && action_capture.captured() &&
                   axis_capture.captured() && focus_capture.waiting() && presentation.ready() &&
                   result.requested_layers == 3U && result.active_contexts == 1U && result.capture_context_active &&
                   result.gameplay_input_consumed && result.profile_overlays_applied == 2U &&
                   result.focus_gameplay_consumed && result.focus_retained && result.presentation_rows == 2U &&
                   result.glyph_lookup_keys == 5U && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] SceneGameplayBindingProbeResult validate_sample_2d_scene_gameplay_bindings(const mirakana::Scene& scene) {
    using namespace mirakana::runtime_scene;

    RuntimeSceneInstance instance{
        .scene_asset = mirakana::AssetId{},
        .handle = mirakana::runtime::RuntimeAssetHandle{},
        .scene = scene,
        .references = {},
    };
    const std::vector<RuntimeSceneGameplayBindingSourceRow> source_rows{
        {
            .binding_id = "player.actor",
            .gameplay_system_id = "sample_2d_controller",
            .slot_id = "actor",
            .node_name = "Player",
            .required_component = RuntimeSceneGameplayBindingComponentKind::sprite_renderer,
        },
        {
            .binding_id = "player.echo",
            .gameplay_system_id = "sample_2d_feedback",
            .slot_id = "pickup",
            .node_name = "Player Echo",
            .required_component = RuntimeSceneGameplayBindingComponentKind::sprite_renderer,
        },
        {
            .binding_id = "camera.primary",
            .gameplay_system_id = "sample_2d_camera",
            .slot_id = "camera",
            .node_name = "Main Camera",
            .required_component = RuntimeSceneGameplayBindingComponentKind::camera,
        },
    };
    const auto bindings = resolve_runtime_scene_gameplay_bindings(instance, source_rows);
    const std::vector<RuntimeSceneGameplayInteractionSourceRow> interactions{
        {
            .action_id = "player.echo.pickup",
            .kind = RuntimeSceneGameplayInteractionKind::pickup,
            .source_binding_id = "player.actor",
            .target_binding_id = "player.echo",
            .amount = 1,
        },
        {
            .action_id = "camera.objective",
            .kind = RuntimeSceneGameplayInteractionKind::objective_progress,
            .source_binding_id = "camera.primary",
            .objective_id = "frame_player",
            .amount = 1,
        },
        {
            .action_id = "camera.win",
            .kind = RuntimeSceneGameplayInteractionKind::win,
            .source_binding_id = "camera.primary",
        },
    };
    const auto plan = plan_runtime_scene_gameplay_interactions(
        bindings.bindings, interactions,
        RuntimeSceneGameplayInteractionPlanRequest{.session_state = RuntimeSceneGameplaySessionState::running});

    SceneGameplayBindingProbeResult result{
        .source_rows = source_rows.size(),
        .binding_rows = bindings.bindings.size(),
        .gameplay_systems = count_scene_gameplay_binding_systems(bindings.bindings),
        .component_rows = static_cast<std::size_t>(
            std::count_if(bindings.bindings.begin(), bindings.bindings.end(),
                          [](const RuntimeSceneGameplayBindingRow& binding) {
                              return binding.required_component != RuntimeSceneGameplayBindingComponentKind::none;
                          })),
        .binding_diagnostics = bindings.diagnostics.size(),
        .interaction_rows = plan.rows.size(),
        .interaction_diagnostics = plan.diagnostics.size(),
        .final_session_state = plan.final_session_state,
    };
    result.ready = bindings.succeeded() && plan.succeeded() && result.source_rows == 3U && result.binding_rows == 3U &&
                   result.gameplay_systems == 3U && result.component_rows == 3U && result.binding_diagnostics == 0U &&
                   result.interaction_rows == 3U && result.interaction_diagnostics == 0U &&
                   result.final_session_state == RuntimeSceneGameplaySessionState::won;
    return result;
}

[[nodiscard]] std::string_view
gameplay_authoring_review_status_name(const GameplayAuthoringReviewProbeResult& result) noexcept {
    return result.ready ? "ready" : "diagnostics";
}

[[nodiscard]] std::string_view
production_authoring_workflow_status_name(const ProductionAuthoringWorkflowProbeResult& result) noexcept {
    return result.ready ? "ready" : "diagnostics";
}

[[nodiscard]] std::string_view
runtime_ui_workbench_status_name(mirakana::ui::RuntimeUiWorkbenchStatus status) noexcept {
    switch (status) {
    case mirakana::ui::RuntimeUiWorkbenchStatus::ready:
        return "ready";
    case mirakana::ui::RuntimeUiWorkbenchStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
runtime_ui_renderer_atlas_handoff_status_name(mirakana::UiRendererAtlasHandoffStatus status) noexcept {
    switch (status) {
    case mirakana::UiRendererAtlasHandoffStatus::ready:
        return "ready";
    case mirakana::UiRendererAtlasHandoffStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
world_region_streaming_status_name(mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::invalid_plan:
        return "invalid_plan";
    case mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::no_changes:
        return "no_changes";
    case mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::failed:
        return "failed";
    case mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::completed:
        return "completed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view world_streaming_large_scene_readiness_status_name(
    mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus::diagnostics:
        return "diagnostics";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus::invalid_evidence:
        return "invalid_evidence";
    }
    return "unknown";
}

[[nodiscard]] std::string_view world_streaming_large_scene_readiness_diagnostic_name(
    mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::none:
        return "none";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::invalid_streaming_plan:
        return "invalid_streaming_plan";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::streaming_safe_point_failed:
        return "streaming_safe_point_failed";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_plan_rows:
        return "insufficient_plan_rows";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_load_rows:
        return "insufficient_load_rows";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_keep_rows:
        return "insufficient_keep_rows";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_unload_rows:
        return "insufficient_unload_rows";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_safe_point_rows:
        return "insufficient_safe_point_rows";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::insufficient_committed_rows:
        return "insufficient_committed_rows";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::missing_reviewed_package_adoption:
        return "missing_reviewed_package_adoption";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::missing_region_diagnostic_absent:
        return "missing_region_diagnostic_absent";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::safe_point_diagnostics_present:
        return "safe_point_diagnostics_present";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::projected_region_budget_exceeded:
        return "projected_region_budget_exceeded";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::projected_byte_budget_exceeded:
        return "projected_byte_budget_exceeded";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::navigation_refs_not_ready:
        return "navigation_refs_not_ready";
    case mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessDiagnostic::navigation_path_cache_not_ready:
        return "navigation_path_cache_not_ready";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
entity_scale_culling_status_name(mirakana::runtime::RuntimeEntityScaleCullingPlanStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::planned:
        return "planned";
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::no_entities:
        return "no_entities";
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::invalid_request:
        return "invalid_request";
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::budget_exceeded:
        return "budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] const char*
scripting_sandbox_status_name(mirakana::runtime::RuntimeScriptSandboxPlanStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeScriptSandboxPlanStatus::planned:
        return "planned";
    case mirakana::runtime::RuntimeScriptSandboxPlanStatus::no_modules:
        return "no_modules";
    case mirakana::runtime::RuntimeScriptSandboxPlanStatus::invalid_request:
        return "invalid_request";
    case mirakana::runtime::RuntimeScriptSandboxPlanStatus::budget_exceeded:
        return "budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] const char*
scripting_sandbox_execution_status_name(mirakana::runtime::RuntimeScriptExecutionStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeScriptExecutionStatus::completed:
        return "completed";
    case mirakana::runtime::RuntimeScriptExecutionStatus::invalid_request:
        return "invalid_request";
    case mirakana::runtime::RuntimeScriptExecutionStatus::budget_exceeded:
        return "budget_exceeded";
    case mirakana::runtime::RuntimeScriptExecutionStatus::adapter_failed:
        return "adapter_failed";
    }
    return "unknown";
}

[[nodiscard]] const char*
networking_foundation_status_name(mirakana::runtime::RuntimeNetworkFoundationPlanStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeNetworkFoundationPlanStatus::planned:
        return "planned";
    case mirakana::runtime::RuntimeNetworkFoundationPlanStatus::no_sessions:
        return "no_sessions";
    case mirakana::runtime::RuntimeNetworkFoundationPlanStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
network_replication_status_name(mirakana::runtime::RuntimeNetworkReplicationStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeNetworkReplicationStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeNetworkReplicationStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::runtime::RuntimeNetworkReplicationStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeNetworkReplicationStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
network_production_security_status_name(mirakana::runtime::RuntimeNetworkProductionSecurityStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeNetworkProductionSecurityStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeNetworkProductionSecurityStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::runtime::RuntimeNetworkProductionSecurityStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char* audio_production_status_name(mirakana::AudioProductionReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::AudioProductionReadinessStatus::ready:
        return "ready";
    case mirakana::AudioProductionReadinessStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::AudioProductionReadinessStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
simulation_orchestration_status_name(mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus::planned:
        return "planned";
    case mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus::no_steps:
        return "no_steps";
    case mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus::budget_limited:
        return "budget_limited";
    case mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
gameplay_runtime_scheduler_status_name(mirakana::runtime::RuntimeGameplaySchedulerStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::no_steps:
        return "no_steps";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::budget_limited:
        return "budget_limited";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::paused:
        return "paused";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
world_entity_model_status_name(mirakana::runtime::RuntimeWorldEntityLifecycleStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeWorldEntityLifecycleStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeWorldEntityLifecycleStatus::no_entities:
        return "no_entities";
    case mirakana::runtime::RuntimeWorldEntityLifecycleStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
addressable_content_status_name(mirakana::runtime::RuntimeAddressableContentStreamingStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::no_requests:
        return "no_requests";
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::budget_limited:
        return "budget_limited";
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char* rpg_systems_status_name(mirakana::runtime::RuntimeRpgSystemsStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeRpgSystemsStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeRpgSystemsStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeRpgSystemsStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char* sandbox_world_status_name(mirakana::runtime::RuntimeSandboxWorldStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSandboxWorldStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSandboxWorldStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeSandboxWorldStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
simulation_management_status_name(mirakana::runtime::RuntimeSimulationManagementStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSimulationManagementStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSimulationManagementStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeSimulationManagementStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
sprite_effect_particle_status_name(mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::no_spawns:
        return "no_spawns";
    case mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::invalid_request:
        return "invalid_request";
    case mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::budget_exceeded:
        return "budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] bool gameplay_2d_near(float value, float expected, float epsilon = 0.001F) noexcept {
    return std::abs(value - expected) <= epsilon;
}

[[nodiscard]] std::string_view
navigation_grid_agent_path_status_name(mirakana::NavigationGridAgentPathStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationGridAgentPathStatus::ready:
        return "ready";
    case mirakana::NavigationGridAgentPathStatus::invalid_request:
        return "invalid_request";
    case mirakana::NavigationGridAgentPathStatus::invalid_mapping:
        return "invalid_mapping";
    case mirakana::NavigationGridAgentPathStatus::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridAgentPathStatus::invalid_endpoint:
        return "invalid_endpoint";
    case mirakana::NavigationGridAgentPathStatus::blocked_endpoint:
        return "blocked_endpoint";
    case mirakana::NavigationGridAgentPathStatus::no_path:
        return "no_path";
    case mirakana::NavigationGridAgentPathStatus::invalid_source_path:
        return "invalid_source_path";
    case mirakana::NavigationGridAgentPathStatus::agent_path_invalid:
        return "agent_path_invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_grid_agent_path_diagnostic_name(mirakana::NavigationGridAgentPathDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationGridAgentPathDiagnostic::none:
        return "none";
    case mirakana::NavigationGridAgentPathDiagnostic::invalid_mapping:
        return "invalid_mapping";
    case mirakana::NavigationGridAgentPathDiagnostic::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridAgentPathDiagnostic::path_invalid_endpoint:
        return "path_invalid_endpoint";
    case mirakana::NavigationGridAgentPathDiagnostic::path_blocked_endpoint:
        return "path_blocked_endpoint";
    case mirakana::NavigationGridAgentPathDiagnostic::path_not_found:
        return "path_not_found";
    case mirakana::NavigationGridAgentPathDiagnostic::smoothing_invalid_source_path:
        return "smoothing_invalid_source_path";
    case mirakana::NavigationGridAgentPathDiagnostic::smoothing_unsupported_adjacency:
        return "smoothing_unsupported_adjacency";
    case mirakana::NavigationGridAgentPathDiagnostic::point_mapping_failed:
        return "point_mapping_failed";
    case mirakana::NavigationGridAgentPathDiagnostic::agent_path_rejected:
        return "agent_path_rejected";
    }
    return "unknown";
}

[[nodiscard]] std::string_view navigation_agent_status_name(mirakana::NavigationAgentStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationAgentStatus::idle:
        return "idle";
    case mirakana::NavigationAgentStatus::moving:
        return "moving";
    case mirakana::NavigationAgentStatus::reached_destination:
        return "reached_destination";
    case mirakana::NavigationAgentStatus::cancelled:
        return "cancelled";
    case mirakana::NavigationAgentStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view ai_perception_status_name(mirakana::AiPerceptionStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionStatus::ready:
        return "ready";
    case mirakana::AiPerceptionStatus::invalid_agent:
        return "invalid_agent";
    case mirakana::AiPerceptionStatus::invalid_target:
        return "invalid_target";
    case mirakana::AiPerceptionStatus::duplicate_target_id:
        return "duplicate_target_id";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
ai_perception_blackboard_status_name(mirakana::AiPerceptionBlackboardStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionBlackboardStatus::ready:
        return "ready";
    case mirakana::AiPerceptionBlackboardStatus::invalid_snapshot:
        return "invalid_snapshot";
    case mirakana::AiPerceptionBlackboardStatus::invalid_key:
        return "invalid_key";
    case mirakana::AiPerceptionBlackboardStatus::blackboard_write_failed:
        return "blackboard_write_failed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
ai_perception_readiness_status_name(mirakana::AiPerceptionReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionReadinessStatus::ready:
        return "ready";
    case mirakana::AiPerceptionReadinessStatus::diagnostics:
        return "diagnostics";
    case mirakana::AiPerceptionReadinessStatus::invalid_snapshot:
        return "invalid_snapshot";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
ai_perception_readiness_diagnostic_name(mirakana::AiPerceptionReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::AiPerceptionReadinessDiagnostic::none:
        return "none";
    case mirakana::AiPerceptionReadinessDiagnostic::invalid_snapshot:
        return "invalid_snapshot";
    case mirakana::AiPerceptionReadinessDiagnostic::blackboard_projection_failed:
        return "blackboard_projection_failed";
    case mirakana::AiPerceptionReadinessDiagnostic::unstable_primary_target:
        return "unstable_primary_target";
    case mirakana::AiPerceptionReadinessDiagnostic::insufficient_targets:
        return "insufficient_targets";
    case mirakana::AiPerceptionReadinessDiagnostic::missing_primary_target:
        return "missing_primary_target";
    case mirakana::AiPerceptionReadinessDiagnostic::missing_visible_target:
        return "missing_visible_target";
    case mirakana::AiPerceptionReadinessDiagnostic::missing_audible_target:
        return "missing_audible_target";
    case mirakana::AiPerceptionReadinessDiagnostic::target_budget_exceeded:
        return "target_budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] std::string_view behavior_tree_status_name(mirakana::BehaviorTreeStatus status) noexcept {
    switch (status) {
    case mirakana::BehaviorTreeStatus::success:
        return "success";
    case mirakana::BehaviorTreeStatus::failure:
        return "failure";
    case mirakana::BehaviorTreeStatus::running:
        return "running";
    case mirakana::BehaviorTreeStatus::invalid_tree:
        return "invalid_tree";
    case mirakana::BehaviorTreeStatus::missing_leaf_result:
        return "missing_leaf_result";
    case mirakana::BehaviorTreeStatus::invalid_leaf_result:
        return "invalid_leaf_result";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
behavior_authoring_readiness_status_name(mirakana::BehaviorAuthoringReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::BehaviorAuthoringReadinessStatus::ready:
        return "ready";
    case mirakana::BehaviorAuthoringReadinessStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
behavior_authoring_readiness_diagnostic_name(mirakana::BehaviorAuthoringReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::BehaviorAuthoringReadinessDiagnostic::none:
        return "none";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::validation_diagnostics:
        return "validation_diagnostics";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::nondeterministic_validation:
        return "nondeterministic_validation";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::insufficient_behaviors:
        return "insufficient_behaviors";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::insufficient_trace_nodes:
        return "insufficient_trace_nodes";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::insufficient_action_bindings:
        return "insufficient_action_bindings";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::insufficient_blackboard_conditions:
        return "insufficient_blackboard_conditions";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::behavior_budget_exceeded:
        return "behavior_budget_exceeded";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::diagnostic_budget_exceeded:
        return "diagnostic_budget_exceeded";
    case mirakana::BehaviorAuthoringReadinessDiagnostic::trace_budget_exceeded:
        return "trace_budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] mirakana::BehaviorTreeDesc gameplay_2d_behavior_tree_desc() {
    return mirakana::BehaviorTreeDesc{
        .root_id = kGameplay2dRootNode,
        .nodes =
            {
                mirakana::BehaviorTreeNodeDesc{
                    .id = kGameplay2dRootNode,
                    .kind = mirakana::BehaviorTreeNodeKind::sequence,
                    .children = {kGameplay2dHasTargetNode, kGameplay2dNeedsMoveNode, kGameplay2dMoveActionNode}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = kGameplay2dHasTargetNode, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = kGameplay2dNeedsMoveNode, .kind = mirakana::BehaviorTreeNodeKind::condition, .children = {}},
                mirakana::BehaviorTreeNodeDesc{
                    .id = kGameplay2dMoveActionNode, .kind = mirakana::BehaviorTreeNodeKind::action, .children = {}},
            },
    };
}

[[nodiscard]] std::vector<mirakana::BehaviorTreeBlackboardCondition> gameplay_2d_behavior_conditions() {
    return std::vector<mirakana::BehaviorTreeBlackboardCondition>{
        mirakana::BehaviorTreeBlackboardCondition{.node_id = kGameplay2dHasTargetNode,
                                                  .key = kGameplay2dHasTargetKey,
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        mirakana::BehaviorTreeBlackboardCondition{.node_id = kGameplay2dNeedsMoveNode,
                                                  .key = kGameplay2dNeedsMoveKey,
                                                  .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                  .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
    };
}

[[nodiscard]] mirakana::BehaviorAuthoringValidationResult validate_gameplay_2d_behavior_authoring() {
    const std::vector<std::string> blackboard_keys{kGameplay2dHasTargetKey, kGameplay2dNeedsMoveKey};
    const std::vector<std::string> supported_actions{kGameplay2dMoveActionId};
    const auto conditions = gameplay_2d_behavior_conditions();
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                mirakana::BehaviorAuthoringBehaviorDesc{
                    .id = kGameplay2dBehaviorId,
                    .tree = gameplay_2d_behavior_tree_desc(),
                    .blackboard_conditions = conditions,
                    .actions =
                        std::vector<mirakana::BehaviorAuthoringActionBinding>{
                            mirakana::BehaviorAuthoringActionBinding{.node_id = kGameplay2dMoveActionNode,
                                                                     .action_id = kGameplay2dMoveActionId},
                        },
                },
            },
    };
    return mirakana::validate_behavior_authoring_document(
        document, mirakana::BehaviorAuthoringValidationContext{
                      .blackboard_keys = std::span<const std::string>{blackboard_keys},
                      .supported_actions = std::span<const std::string>{supported_actions},
                  });
}

[[nodiscard]] mirakana::BehaviorAuthoringReadinessReport evaluate_gameplay_2d_behavior_authoring_readiness() {
    const std::vector<std::string> blackboard_keys{kGameplay2dHasTargetKey, kGameplay2dNeedsMoveKey};
    const std::vector<std::string> supported_actions{kGameplay2dMoveActionId};
    const auto conditions = gameplay_2d_behavior_conditions();
    const mirakana::BehaviorAuthoringDocument document{
        .behaviors =
            std::vector<mirakana::BehaviorAuthoringBehaviorDesc>{
                mirakana::BehaviorAuthoringBehaviorDesc{
                    .id = kGameplay2dBehaviorId,
                    .tree = gameplay_2d_behavior_tree_desc(),
                    .blackboard_conditions = conditions,
                    .actions =
                        std::vector<mirakana::BehaviorAuthoringActionBinding>{
                            mirakana::BehaviorAuthoringActionBinding{.node_id = kGameplay2dMoveActionNode,
                                                                     .action_id = kGameplay2dMoveActionId},
                        },
                },
            },
    };

    return mirakana::evaluate_behavior_authoring_readiness(
        document,
        mirakana::BehaviorAuthoringValidationContext{
            .blackboard_keys = std::span<const std::string>{blackboard_keys},
            .supported_actions = std::span<const std::string>{supported_actions},
        },
        mirakana::BehaviorAuthoringReadinessConfig{
            .min_behaviors = 1,
            .min_trace_nodes = 4,
            .min_action_bindings = 1,
            .min_blackboard_conditions = 2,
            .max_validation_diagnostics = 0,
            .max_behaviors = 1,
            .max_trace_nodes = 8,
        });
}

struct Gameplay2DQuestDialogueProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t transition_rows{0U};
    std::size_t completed_objectives{0U};
    std::size_t flags{0U};
    std::size_t dialogue_nodes{0U};
    std::size_t action_ids{0U};
    std::size_t reward_ids{0U};
    std::size_t state_rows{0U};
};

struct Gameplay2DInventoryProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t catalog_rows{0U};
    std::size_t state_rows{0U};
    std::size_t transition_rows{0U};
    std::size_t accepted_rows{0U};
    std::size_t completed_rows{0U};
    std::size_t final_stacks{0U};
    std::uint32_t final_workbench_quantity{0U};
};

struct Gameplay2DInteractionProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t rows{0U};
    std::size_t feedback_rows{0U};
    mirakana::runtime::RuntimeGameplaySessionState final_session_state{
        mirakana::runtime::RuntimeGameplaySessionState::running};
};

struct Gameplay2DSpriteCollisionHitboxProbeResult {
    bool ready{false};
    std::size_t hits{0U};
    std::size_t gameplay_events{0U};
    std::size_t interaction_rows{0U};
    std::size_t feedback_rows{0U};
    std::size_t diagnostics{0U};
};

struct RuntimeProfileResumeProbeResult {
    bool ready{false};
    mirakana::runtime::RuntimeSessionProfileResumeStatus status{
        mirakana::runtime::RuntimeSessionProfileResumeStatus::blocked};
    std::size_t diagnostics{0U};
    std::size_t loaded_documents{0U};
    std::size_t defaulted_documents{0U};
    std::uint32_t save_schema_version{0U};
    std::uint32_t settings_schema_version{0U};
};

struct RuntimeMenuHudProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t display_rows{0U};
    std::size_t command_rows{0U};
    std::size_t dialogue_rows{0U};
    std::size_t input_binding_prompt_rows{0U};
};

struct RuntimeUiWorkbenchProbeResult {
    bool ready{false};
    mirakana::ui::RuntimeUiWorkbenchStatus status{mirakana::ui::RuntimeUiWorkbenchStatus::invalid_request};
    std::size_t panels{0U};
    std::size_t table_columns{0U};
    std::size_t table_rows{0U};
    std::size_t graph_series{0U};
    std::size_t item_rows{0U};
    std::size_t inventory_rows{0U};
    std::size_t equipment_rows{0U};
    std::size_t shop_rows{0U};
    std::size_t text_inputs{0U};
    std::size_t platform_text_input_requests{0U};
    std::size_t focus_edges{0U};
    std::size_t localization_refs{0U};
    bool localization_identity_ready{false};
    std::size_t accessibility_refs{0U};
    bool accessibility_identity_ready{false};
    std::size_t diagnostics{0U};
    bool renderer_submission{false};
    bool text_shaping{false};
    bool font_rasterization{false};
    bool ime_sessions{false};
    bool accessibility_bridge{false};
    bool image_decoding{false};
    bool native_platform{false};
};

struct RuntimeUiProductionStackProbeResult {
    bool package_evidence_ready{false};
    bool reviewed{false};
    mirakana::ui::RuntimeUiProductionStackStatus status{mirakana::ui::RuntimeUiProductionStackStatus::invalid_request};
    std::size_t rows{0U};
    std::size_t ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t dependency_gated_rows{0U};
    std::size_t skipped_rows{0U};
    std::size_t adapter_invoked_rows{0U};
    std::size_t unsupported_rows{0U};
    bool text_contract_ready{false};
    bool selected_package_evidence_ready{false};
    bool production_ready{false};
    bool requires_ime_host_evidence{false};
    bool ime_host_evidence{false};
    std::size_t ime_session_rows{0U};
    std::size_t ime_composition_rows{0U};
    std::size_t ime_candidate_rows{0U};
    std::size_t ime_text_area_cursor_rows{0U};
    std::size_t ime_committed_text_rows{0U};
    std::size_t ime_clipboard_rows{0U};
    std::size_t ime_platform_adapter_proof_rows{0U};
    std::size_t ime_platform_host_gate_rows{0U};
    bool ime_platform_parity_ready{false};
    bool requires_accessibility_host_evidence{false};
    bool accessibility_host_evidence{false};
    std::size_t accessibility_role_rows{0U};
    std::size_t accessibility_name_rows{0U};
    std::size_t accessibility_description_rows{0U};
    std::size_t accessibility_state_rows{0U};
    std::size_t accessibility_focus_rows{0U};
    std::size_t accessibility_action_rows{0U};
    std::size_t accessibility_relationship_rows{0U};
    std::size_t accessibility_live_region_rows{0U};
    std::size_t accessibility_keyboard_pattern_rows{0U};
    std::size_t accessibility_publication_status_rows{0U};
    std::size_t accessibility_uia_host_gate_rows{0U};
    std::size_t accessibility_platform_host_gate_rows{0U};
    bool accessibility_platform_parity_ready{false};
    bool invoked_text_shaping{false};
    bool invoked_font_rasterization{false};
    bool invoked_ime{false};
    bool invoked_accessibility_bridge{false};
    bool invoked_native_platform{false};
    bool invoked_renderer_upload{false};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
};

struct RuntimeUiRendererAtlasHandoffProbeResult {
    bool ready{false};
    bool selected_package_evidence_ready{false};
    bool reviewed{false};
    mirakana::UiRendererAtlasHandoffStatus status{mirakana::UiRendererAtlasHandoffStatus::invalid_request};
    std::size_t image_atlas_pages{0U};
    std::size_t image_atlas_bindings{0U};
    std::size_t glyph_atlas_pages{0U};
    std::size_t glyph_atlas_bindings{0U};
    std::size_t atlas_placement_rows{0U};
    std::size_t atlas_budget_rows{0U};
    std::size_t atlas_eviction_diagnostic_rows{0U};
    std::size_t texture_upload_handoff_rows{0U};
    std::size_t renderer_submission_counter_rows{0U};
    std::size_t text_glyphs_available{0U};
    std::size_t text_glyphs_resolved{0U};
    std::size_t text_glyphs_missing{0U};
    std::size_t text_glyph_sprites_submitted{0U};
    std::size_t image_placeholders_available{0U};
    std::size_t image_resources_resolved{0U};
    std::size_t image_resources_missing{0U};
    std::size_t image_sprites_submitted{0U};
    std::size_t renderer_sprites_submitted{0U};
    std::size_t unsupported_claim_rows{0U};
    std::size_t side_effect_rows{0U};
    bool requested_renderer_texture_upload_api{false};
    bool requested_public_native_handle{false};
    bool invoked_source_image_decode{false};
    bool invoked_live_glyph_atlas_generation{false};
    bool invoked_renderer_upload{false};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
};

struct AudioGameplayMixerProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t buses{0U};
    std::size_t cues{0U};
    std::size_t triggers{0U};
    std::size_t commands{0U};
    std::size_t paused_buses{0U};
    std::size_t faded_buses{0U};
    std::size_t looping_commands{0U};
    std::size_t spatial_commands{0U};
    std::size_t render_commands{0U};
    std::uint32_t render_frames{0U};
    std::size_t render_samples{0U};
    float sample_abs_sum{0.0F};
    std::size_t payload_diagnostics{0U};
};

struct AudioProductionProbeResult {
    mirakana::AudioProductionReadinessStatus status{mirakana::AudioProductionReadinessStatus::invalid_request};
    bool reviewed{false};
    bool production_audio_ready{false};
    bool selected_package_evidence_ready{false};
    std::size_t decoded_source_rows{0U};
    std::size_t streaming_chunk_rows{0U};
    std::size_t format_conversion_policy_rows{0U};
    std::size_t bus_budget_rows{0U};
    std::size_t voice_budget_rows{0U};
    std::size_t dsp_graph_rows{0U};
    std::size_t listener_rows{0U};
    std::size_t spatial_source_rows{0U};
    std::size_t hrtf_host_gate_rows{0U};
    std::size_t device_lifecycle_rows{0U};
    bool device_host_evidence_available{false};
    bool hrtf_host_evidence_available{false};
    std::size_t unsupported_claim_rows{0U};
    bool invoked_codec_decode{false};
    bool invoked_background_streaming{false};
    bool invoked_middleware{false};
    bool invoked_hrtf{false};
    bool invoked_device_callback{false};
    bool invoked_device_io{false};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool package_evidence_ready{false};
};

struct Gameplay2DConstructionPlacementProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t validation_rows{0U};
    std::size_t intent_rows{0U};
    std::size_t intent_accepted_rows{0U};
    std::size_t intent_occupied_cells{0U};
};

struct Gameplay2DProceduralGenerationProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t rows{0U};
    std::size_t object_rows{0U};
    std::size_t encounter_rows{0U};
    std::size_t loot_rows{0U};
    std::uint64_t replay_hash{0ULL};
    std::size_t package_visible_rows{0U};
    std::size_t placement_intent_rows{0U};
    std::size_t placement_intent_accepted_rows{0U};
};

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id();
[[nodiscard]] mirakana::AssetId packaged_sprite_texture_asset_id();
[[nodiscard]] mirakana::AssetId packaged_material_asset_id();
[[nodiscard]] mirakana::AssetId packaged_sprite_animation_asset_id();
[[nodiscard]] mirakana::AssetId packaged_tilemap_asset_id();
[[nodiscard]] mirakana::AssetId packaged_ui_atlas_asset_id();

[[nodiscard]] mirakana::runtime::RuntimeQuestDialogueDocument gameplay_2d_quest_dialogue_document() {
    using namespace mirakana::runtime;

    return RuntimeQuestDialogueDocument{
        .flags = {"story.met_elder"},
        .quests =
            std::vector<RuntimeQuestDesc>{
                RuntimeQuestDesc{
                    .id = "quest.intro",
                    .title_localization_key = "quest.intro.title",
                    .prerequisites = {},
                    .objectives =
                        std::vector<RuntimeQuestObjectiveDesc>{
                            RuntimeQuestObjectiveDesc{
                                .id = "talk_elder",
                                .localization_key = "quest.intro.objective.talk_elder",
                                .prerequisites = {},
                                .reward_ids = {"xp_small"},
                            },
                        },
                    .reward_ids = {"story_unlock"},
                },
            },
        .dialogues =
            std::vector<RuntimeDialogueGraphDesc>{
                RuntimeDialogueGraphDesc{
                    .id = "dialogue.elder",
                    .root_node_id = "start",
                    .nodes =
                        std::vector<RuntimeDialogueNodeDesc>{
                            RuntimeDialogueNodeDesc{
                                .id = "start",
                                .localization_key = "dialogue.elder.start",
                                .choices =
                                    std::vector<RuntimeDialogueChoiceDesc>{
                                        RuntimeDialogueChoiceDesc{
                                            .id = "accept",
                                            .localization_key = "dialogue.elder.accept",
                                            .next_node_id = "accepted",
                                            .prerequisites =
                                                std::vector<RuntimeQuestPrerequisite>{
                                                    RuntimeQuestPrerequisite{
                                                        .kind = RuntimeQuestPrerequisiteKind::flag_set,
                                                        .quest_id = {},
                                                        .objective_id = {},
                                                        .flag_id = "story.met_elder",
                                                    },
                                                },
                                            .action_ids = {"open_quest_intro"},
                                        },
                                    },
                                .action_ids = {},
                            },
                            RuntimeDialogueNodeDesc{
                                .id = "accepted",
                                .localization_key = "dialogue.elder.accepted",
                                .choices = {},
                                .action_ids = {"close_dialogue"},
                            },
                        },
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeQuestDialogueValidationContext gameplay_2d_quest_dialogue_context() {
    static const std::vector<std::string> localization_keys{
        "quest.intro.title",     "quest.intro.objective.talk_elder", "dialogue.elder.start",
        "dialogue.elder.accept", "dialogue.elder.accepted",
    };
    static const std::vector<std::string> reward_ids{"xp_small", "story_unlock"};
    static const std::vector<std::string> action_ids{"open_quest_intro", "close_dialogue"};

    return mirakana::runtime::RuntimeQuestDialogueValidationContext{
        .localization_keys = std::span<const std::string>{localization_keys},
        .supported_reward_ids = std::span<const std::string>{reward_ids},
        .supported_action_ids = std::span<const std::string>{action_ids},
    };
}

[[nodiscard]] Gameplay2DQuestDialogueProbeResult validate_gameplay_2d_quest_dialogue() {
    using Kind = mirakana::runtime::RuntimeQuestDialogueTransitionKind;
    using Status = mirakana::runtime::RuntimeQuestDialogueTransitionStatus;

    Gameplay2DQuestDialogueProbeResult result;
    const auto document = gameplay_2d_quest_dialogue_document();
    const auto context = gameplay_2d_quest_dialogue_context();
    const auto validation = mirakana::runtime::validate_runtime_quest_dialogue_document(document, context);
    result.diagnostics += validation.diagnostics.size();
    if (!validation.succeeded) {
        return result;
    }

    mirakana::runtime::RuntimeQuestDialogueState state;
    const auto completed_objective = mirakana::runtime::advance_runtime_quest_dialogue_state(
        document, state,
        mirakana::runtime::RuntimeQuestDialogueTransitionRequest{
            .kind = Kind::complete_objective,
            .flag_id = {},
            .quest_id = "quest.intro",
            .objective_id = "talk_elder",
            .dialogue_id = {},
            .dialogue_choice_id = {},
        },
        context);
    result.transition_rows += completed_objective.rows.size();
    result.diagnostics += completed_objective.diagnostics.size();
    result.reward_ids += completed_objective.reward_ids.size();
    if (!completed_objective.succeeded || completed_objective.rows.empty() ||
        completed_objective.rows.front().status != Status::completed) {
        return result;
    }
    state = completed_objective.state;

    const auto accepted_flag = mirakana::runtime::advance_runtime_quest_dialogue_state(
        document, state,
        mirakana::runtime::RuntimeQuestDialogueTransitionRequest{
            .kind = Kind::set_flag,
            .flag_id = "story.met_elder",
            .quest_id = {},
            .objective_id = {},
            .dialogue_id = {},
            .dialogue_choice_id = {},
        },
        context);
    result.transition_rows += accepted_flag.rows.size();
    result.diagnostics += accepted_flag.diagnostics.size();
    if (!accepted_flag.succeeded || accepted_flag.rows.empty() ||
        accepted_flag.rows.front().status != Status::accepted) {
        return result;
    }
    state = accepted_flag.state;

    const auto accepted_choice = mirakana::runtime::advance_runtime_quest_dialogue_state(
        document, state,
        mirakana::runtime::RuntimeQuestDialogueTransitionRequest{
            .kind = Kind::choose_dialogue,
            .flag_id = {},
            .quest_id = {},
            .objective_id = {},
            .dialogue_id = "dialogue.elder",
            .dialogue_choice_id = "accept",
        },
        context);
    result.transition_rows += accepted_choice.rows.size();
    result.diagnostics += accepted_choice.diagnostics.size();
    result.reward_ids += accepted_choice.reward_ids.size();
    if (!accepted_choice.succeeded || accepted_choice.rows.empty() ||
        accepted_choice.rows.front().status != Status::accepted) {
        return result;
    }
    state = accepted_choice.state;

    result.completed_objectives = state.completed_objectives.size();
    result.flags = state.flags_set.size();
    result.dialogue_nodes = state.dialogue_nodes.size();
    result.action_ids = accepted_choice.action_ids.size();
    const auto state_validation = mirakana::runtime::validate_runtime_quest_dialogue_state(document, state, context);
    result.diagnostics += state_validation.diagnostics.size();
    result.state_rows = state_validation.rows.size();
    result.ready = result.diagnostics == 0U && result.transition_rows == 3U && result.completed_objectives == 1U &&
                   result.flags == 1U && result.dialogue_nodes == 1U && result.action_ids == 2U &&
                   result.reward_ids == 2U && result.state_rows == 3U && state_validation.succeeded;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeItemCatalogDocument gameplay_2d_inventory_catalog_document() {
    using namespace mirakana::runtime;

    return RuntimeItemCatalogDocument{
        .items =
            std::vector<RuntimeItemDesc>{
                RuntimeItemDesc{
                    .id = "wood",
                    .localization_key = "item.wood",
                    .category_id = "material",
                    .tag_ids = {"crafting", "pickup"},
                    .max_stack = 99U,
                    .placement_id = {},
                    .placement_costs = {},
                },
                RuntimeItemDesc{
                    .id = "workbench",
                    .localization_key = "item.workbench",
                    .category_id = "station",
                    .tag_ids = {"crafting", "placeable"},
                    .max_stack = 1U,
                    .placement_id = "grid_2d",
                    .placement_costs =
                        std::vector<RuntimeItemCostDesc>{
                            RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                        },
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeItemCatalogValidationContext gameplay_2d_inventory_catalog_context() {
    static const std::vector<std::string> localization_keys{"item.wood", "item.workbench"};
    static const std::vector<std::string> category_ids{"material", "station"};
    static const std::vector<std::string> tag_ids{"crafting", "pickup", "placeable"};
    static const std::vector<std::string> placement_ids{"grid_2d"};

    return mirakana::runtime::RuntimeItemCatalogValidationContext{
        .localization_keys = std::span<const std::string>{localization_keys},
        .supported_category_ids = std::span<const std::string>{category_ids},
        .supported_tag_ids = std::span<const std::string>{tag_ids},
        .supported_placement_ids = std::span<const std::string>{placement_ids},
    };
}

[[nodiscard]] mirakana::runtime::RuntimeCraftingRecipeDocument gameplay_2d_crafting_recipes() {
    using namespace mirakana::runtime;

    return RuntimeCraftingRecipeDocument{
        .recipes =
            std::vector<RuntimeCraftingRecipeDesc>{
                RuntimeCraftingRecipeDesc{
                    .id = "recipe.workbench",
                    .inputs =
                        std::vector<RuntimeItemCostDesc>{
                            RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                        },
                    .outputs =
                        std::vector<RuntimeItemCostDesc>{
                            RuntimeItemCostDesc{.item_id = "workbench", .quantity = 1U},
                        },
                },
            },
    };
}

[[nodiscard]] Gameplay2DInventoryProbeResult validate_gameplay_2d_inventory_items() {
    using Kind = mirakana::runtime::RuntimeInventoryTransitionKind;
    using Status = mirakana::runtime::RuntimeInventoryTransitionStatus;

    Gameplay2DInventoryProbeResult result;
    const auto catalog = gameplay_2d_inventory_catalog_document();
    const auto catalog_validation =
        mirakana::runtime::validate_runtime_item_catalog_document(catalog, gameplay_2d_inventory_catalog_context());
    result.diagnostics += catalog_validation.diagnostics.size();
    result.catalog_rows = catalog_validation.rows.size();
    if (!catalog_validation.succeeded) {
        return result;
    }

    mirakana::runtime::RuntimeInventoryState state{
        .stacks =
            std::vector<mirakana::runtime::RuntimeInventoryStackDesc>{
                mirakana::runtime::RuntimeInventoryStackDesc{.item_id = "wood", .quantity = 2U},
            },
    };
    const auto initial_state_validation = mirakana::runtime::validate_runtime_inventory_state(catalog, state);
    result.diagnostics += initial_state_validation.diagnostics.size();
    result.state_rows += initial_state_validation.rows.size();
    if (!initial_state_validation.succeeded) {
        return result;
    }

    const auto recipes = gameplay_2d_crafting_recipes();
    const auto pickup =
        mirakana::runtime::advance_runtime_inventory_state(catalog, recipes, state,
                                                           mirakana::runtime::RuntimeInventoryTransitionRequest{
                                                               .kind = Kind::add_item,
                                                               .item_id = "wood",
                                                               .quantity = 1U,
                                                               .recipe_id = {},
                                                           });
    result.diagnostics += pickup.diagnostics.size();
    result.transition_rows += pickup.rows.size();
    if (!pickup.rows.empty() && pickup.rows.front().status == Status::accepted) {
        ++result.accepted_rows;
    }
    if (!pickup.succeeded || pickup.rows.empty() || pickup.rows.front().status != Status::accepted) {
        return result;
    }
    state = pickup.state;

    const auto craft =
        mirakana::runtime::advance_runtime_inventory_state(catalog, recipes, state,
                                                           mirakana::runtime::RuntimeInventoryTransitionRequest{
                                                               .kind = Kind::craft_recipe,
                                                               .item_id = {},
                                                               .quantity = 0U,
                                                               .recipe_id = "recipe.workbench",
                                                           });
    result.diagnostics += craft.diagnostics.size();
    result.transition_rows += craft.rows.size();
    if (!craft.rows.empty() && craft.rows.front().status == Status::completed) {
        ++result.completed_rows;
    }
    if (!craft.succeeded || craft.rows.empty() || craft.rows.front().status != Status::completed) {
        return result;
    }
    state = craft.state;

    const auto final_state_validation = mirakana::runtime::validate_runtime_inventory_state(catalog, state);
    result.diagnostics += final_state_validation.diagnostics.size();
    result.state_rows += final_state_validation.rows.size();
    result.final_stacks = state.stacks.size();
    for (const auto& stack : state.stacks) {
        if (stack.item_id == "workbench") {
            result.final_workbench_quantity += stack.quantity;
        }
    }
    result.ready = final_state_validation.succeeded && result.diagnostics == 0U && result.catalog_rows == 2U &&
                   result.state_rows == 2U && result.transition_rows == 2U && result.accepted_rows == 1U &&
                   result.completed_rows == 1U && result.final_stacks == 1U && result.final_workbench_quantity == 1U;
    return result;
}

[[nodiscard]] Gameplay2DInteractionProbeResult validate_gameplay_2d_interactions() {
    using namespace mirakana::runtime;

    const RuntimeGameplayInteractionState state{
        .session_state = RuntimeGameplaySessionState::running,
        .entities =
            std::vector<RuntimeGameplayEntityState>{
                RuntimeGameplayEntityState{.id = "player", .health = 8, .max_health = 10, .active = true},
                RuntimeGameplayEntityState{.id = "enemy", .health = 5, .max_health = 5, .active = true},
            },
        .pickups =
            std::vector<RuntimeGameplayPickupState>{
                RuntimeGameplayPickupState{.id = "pickup.gem", .item_id = "gem", .quantity = 1U, .available = true},
            },
        .objectives =
            std::vector<RuntimeGameplayObjectiveState>{
                RuntimeGameplayObjectiveState{
                    .id = "objective.escape", .progress = 1U, .target = 3U, .completed = false},
            },
    };
    const std::vector<RuntimeGameplayInteractionEvent> events{
        RuntimeGameplayInteractionEvent{
            .id = "event.trigger",
            .kind = RuntimeGameplayInteractionKind::trigger,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.trigger",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.damage",
            .kind = RuntimeGameplayInteractionKind::damage,
            .source_entity_id = "player",
            .target_entity_id = "enemy",
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.damage",
            .amount = 3,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.heal",
            .kind = RuntimeGameplayInteractionKind::heal,
            .source_entity_id = "player",
            .target_entity_id = "player",
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.heal",
            .amount = 2,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.pickup",
            .kind = RuntimeGameplayInteractionKind::pickup,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = "pickup.gem",
            .objective_id = {},
            .feedback_id = "feedback.pickup",
            .amount = 1,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.objective",
            .kind = RuntimeGameplayInteractionKind::objective_progress,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = "objective.escape",
            .feedback_id = "feedback.objective",
            .amount = 2,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.feedback",
            .kind = RuntimeGameplayInteractionKind::feedback,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.prompt",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.win",
            .kind = RuntimeGameplayInteractionKind::win,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.win",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.restart",
            .kind = RuntimeGameplayInteractionKind::restart,
            .source_entity_id = {},
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.restart",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.loss",
            .kind = RuntimeGameplayInteractionKind::loss,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.loss",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.restart_after_loss",
            .kind = RuntimeGameplayInteractionKind::restart,
            .source_entity_id = {},
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.restart_after_loss",
            .amount = 0,
        },
    };

    const auto plan =
        plan_runtime_gameplay_interactions(state, std::span<const RuntimeGameplayInteractionEvent>{events});
    Gameplay2DInteractionProbeResult result{
        .ready = false,
        .diagnostics = plan.diagnostics.size(),
        .rows = plan.rows.size(),
        .feedback_rows = plan.feedback_rows.size(),
        .final_session_state = plan.state.session_state,
    };
    result.ready =
        plan.succeeded && result.diagnostics == 0U && result.rows == 10U && result.feedback_rows == 10U &&
        result.final_session_state == RuntimeGameplaySessionState::running && plan.state.entities.size() == 2U &&
        plan.state.entities[0].health == 10 && plan.state.entities[1].health == 2 && plan.state.pickups.size() == 1U &&
        !plan.state.pickups[0].available && plan.state.objectives.size() == 1U && plan.state.objectives[0].completed;
    return result;
}

[[nodiscard]] Gameplay2DSpriteCollisionHitboxProbeResult validate_gameplay_2d_sprite_collision_hitboxes() {
    using namespace mirakana::runtime;

    const RuntimeGameplayInteractionState state{
        .session_state = RuntimeGameplaySessionState::running,
        .entities =
            std::vector<RuntimeGameplayEntityState>{
                RuntimeGameplayEntityState{.id = "player", .health = 8, .max_health = 10, .active = true},
                RuntimeGameplayEntityState{.id = "enemy", .health = 5, .max_health = 5, .active = true},
            },
        .pickups = {},
        .objectives = {},
    };
    const RuntimeSpriteCollisionHitboxRequest request{
        .boxes =
            std::vector<RuntimeSpriteCollisionBoxDesc>{
                RuntimeSpriteCollisionBoxDesc{
                    .id = "sample_2d.player_slash",
                    .frame_id = "sample_2d.player.attack.1",
                    .entity_id = "player",
                    .kind = RuntimeSpriteCollisionBoxKind::hitbox,
                    .center_x = 0.35F,
                    .center_y = 0.0F,
                    .half_width = 0.35F,
                    .half_height = 0.25F,
                    .layer = 0x1U,
                    .mask = 0x2U,
                    .gameplay_kind = RuntimeGameplayInteractionKind::damage,
                    .gameplay_amount = 2,
                    .gameplay_feedback_id = "feedback.sprite_hit",
                    .source_index = 0U,
                },
                RuntimeSpriteCollisionBoxDesc{
                    .id = "sample_2d.enemy_body",
                    .frame_id = "sample_2d.enemy.idle.0",
                    .entity_id = "enemy",
                    .kind = RuntimeSpriteCollisionBoxKind::hurtbox,
                    .center_x = 0.0F,
                    .center_y = 0.0F,
                    .half_width = 0.45F,
                    .half_height = 0.5F,
                    .layer = 0x2U,
                    .mask = 0x1U,
                    .gameplay_kind = RuntimeGameplayInteractionKind::damage,
                    .gameplay_amount = 0,
                    .gameplay_feedback_id = {},
                    .source_index = 1U,
                },
            },
        .frame_poses =
            std::vector<RuntimeSpriteCollisionFramePoseDesc>{
                RuntimeSpriteCollisionFramePoseDesc{
                    .frame_id = "sample_2d.player.attack.1",
                    .entity_id = "player",
                    .world_x = 0.0F,
                    .world_y = 0.0F,
                    .active = true,
                    .source_index = 0U,
                },
                RuntimeSpriteCollisionFramePoseDesc{
                    .frame_id = "sample_2d.enemy.idle.0",
                    .entity_id = "enemy",
                    .world_x = 0.6F,
                    .world_y = 0.0F,
                    .active = true,
                    .source_index = 1U,
                },
            },
        .max_hit_rows = 4U,
    };

    Gameplay2DSpriteCollisionHitboxProbeResult result;
    const auto collision = plan_runtime_sprite_collision_hitboxes(request);
    result.hits = collision.rows.size();
    result.gameplay_events = collision.gameplay_events.size();
    result.diagnostics += collision.diagnostics.size();
    if (!collision.succeeded) {
        return result;
    }

    const auto interaction = plan_runtime_gameplay_interactions(
        state, std::span<const RuntimeGameplayInteractionEvent>{collision.gameplay_events});
    result.interaction_rows = interaction.rows.size();
    result.feedback_rows = interaction.feedback_rows.size();
    result.diagnostics += interaction.diagnostics.size();
    result.ready = interaction.succeeded && result.diagnostics == 0U && result.hits == 1U &&
                   result.gameplay_events == 1U && result.interaction_rows == 1U && result.feedback_rows == 1U &&
                   interaction.state.entities.size() == 2U && interaction.state.entities[1].id == "enemy" &&
                   interaction.state.entities[1].health == 3;
    return result;
}

[[nodiscard]] RuntimeProfileResumeProbeResult validate_gameplay_2d_runtime_profile_resume() {
    mirakana::MemoryFileSystem fs;
    mirakana::runtime::RuntimeSessionProfileDocuments documents;
    documents.save_data.schema_version = 3;
    documents.save_data.set_value("save.slot", "slot_1");
    documents.save_data.set_value("progression.checkpoint", "quest.intro.completed");
    documents.save_data.set_value("package.id", "runtime/sample_2d_desktop_runtime_package.geindex");
    documents.settings.schema_version = 2;
    documents.settings.set_value("settings.profile", "desktop_2d");
    documents.input_rebinding_profile.profile_id = "slot_1";

    const auto profile = mirakana::runtime::RuntimeSessionProfilePathRequest{
        .game_id = "sample_2d_desktop_runtime_package", .profile_id = "slot_1", .root_path = "profiles"};
    const auto write = mirakana::runtime::write_runtime_session_profile_documents(
        fs, mirakana::runtime::RuntimeSessionProfileDocumentWriteRequest{.profile = profile, .documents = documents});
    const auto loaded = mirakana::runtime::load_runtime_session_profile_documents(
        fs, mirakana::runtime::RuntimeSessionProfileDocumentLoadRequest{.profile = profile, .defaults = documents});
    auto plan =
        mirakana::runtime::plan_runtime_session_profile_resume(mirakana::runtime::RuntimeSessionProfileResumeRequest{
            .documents = loaded,
            .expected_save_slot = "slot_1",
            .expected_progression_checkpoint = "quest.intro.completed",
            .expected_package_id = "runtime/sample_2d_desktop_runtime_package.geindex",
            .expected_profile_id = "slot_1"});

    if (!write.succeeded()) {
        plan.status = mirakana::runtime::RuntimeSessionProfileResumeStatus::blocked;
    }
    return RuntimeProfileResumeProbeResult{.ready = write.succeeded() && plan.ready(),
                                           .status = plan.status,
                                           .diagnostics = plan.diagnostics.size(),
                                           .loaded_documents = plan.loaded_document_rows,
                                           .defaulted_documents = plan.defaulted_document_rows,
                                           .save_schema_version = plan.save_schema_version,
                                           .settings_schema_version = plan.settings_schema_version};
}

[[nodiscard]] RuntimeMenuHudProbeResult validate_gameplay_2d_runtime_menu_hud() {
    const auto plan = mirakana::ui::plan_runtime_menu_hud({
        mirakana::ui::RuntimeMenuHudRowDesc{
            .id = "hud.prompt",
            .kind = mirakana::ui::RuntimeMenuHudRowKind::prompt,
            .label = "Pause",
            .value = "Esc",
        },
        mirakana::ui::RuntimeMenuHudRowDesc{
            .id = "dialogue.intro",
            .kind = mirakana::ui::RuntimeMenuHudRowKind::dialogue_box,
            .label = "Guide",
            .value = "Reach the beacon.",
        },
        mirakana::ui::RuntimeMenuHudRowDesc{
            .id = "bindings.jump",
            .kind = mirakana::ui::RuntimeMenuHudRowKind::input_binding_prompt,
            .label = "Jump",
            .value = "Space",
        },
        mirakana::ui::RuntimeMenuHudRowDesc{
            .id = "menu.pause",
            .kind = mirakana::ui::RuntimeMenuHudRowKind::command,
            .label = "Pause",
            .command_id = "game.pause",
            .command_intent = mirakana::ui::RuntimeMenuHudCommandIntent::pause_game,
            .command_target = mirakana::ui::RuntimeMenuHudCommandTarget::game_session,
        },
        mirakana::ui::RuntimeMenuHudRowDesc{
            .id = "menu.restart",
            .kind = mirakana::ui::RuntimeMenuHudRowKind::command,
            .label = "Restart",
            .command_id = "game.restart",
            .command_intent = mirakana::ui::RuntimeMenuHudCommandIntent::restart_session,
            .command_target = mirakana::ui::RuntimeMenuHudCommandTarget::game_session,
        },
        mirakana::ui::RuntimeMenuHudRowDesc{
            .id = "hud.score",
            .kind = mirakana::ui::RuntimeMenuHudRowKind::counter,
            .label = "Score",
            .value = "0",
        },
    });

    RuntimeMenuHudProbeResult result{.ready = plan.succeeded(),
                                     .diagnostics = plan.diagnostics.size(),
                                     .display_rows = plan.display_rows.size(),
                                     .command_rows = plan.command_rows.size()};
    for (const auto& row : plan.display_rows) {
        if (row.kind == mirakana::ui::RuntimeMenuHudRowKind::dialogue_box) {
            ++result.dialogue_rows;
        }
        if (row.kind == mirakana::ui::RuntimeMenuHudRowKind::input_binding_prompt) {
            ++result.input_binding_prompt_rows;
        }
    }
    result.ready = result.ready && result.display_rows == 6U && result.command_rows == 2U &&
                   result.dialogue_rows == 1U && result.input_binding_prompt_rows == 1U;
    return result;
}

[[nodiscard]] bool
has_runtime_ui_workbench_localization_ref(const std::vector<mirakana::ui::RuntimeUiWorkbenchLocalizationRef>& refs,
                                          std::string_view owner_id, std::string_view key) {
    return std::ranges::any_of(refs,
                               [owner_id, key](const auto& ref) { return ref.owner_id == owner_id && ref.key == key; });
}

[[nodiscard]] bool
has_runtime_ui_workbench_accessibility_ref(const std::vector<mirakana::ui::RuntimeUiWorkbenchAccessibilityRef>& refs,
                                           std::string_view owner_id, std::string_view label) {
    return std::ranges::any_of(
        refs, [owner_id, label](const auto& ref) { return ref.owner_id == owner_id && ref.label == label; });
}

[[nodiscard]] RuntimeUiWorkbenchProbeResult validate_gameplay_2d_runtime_ui_workbench() {
    mirakana::ui::RuntimeUiWorkbenchDocument document;
    document.id = "sample.production.runtime_ui_workbench";
    document.title_localization_key = "ui.workbench.title";
    document.panels = {
        mirakana::ui::RuntimeUiWorkbenchPanelRow{
            .id = "menu.pause",
            .kind = mirakana::ui::RuntimeUiWorkbenchPanelKind::menu,
            .title_localization_key = "ui.menu.pause.title",
            .accessibility_label = "Pause menu",
            .enabled = true,
        },
        mirakana::ui::RuntimeUiWorkbenchPanelRow{
            .id = "inventory.pack",
            .kind = mirakana::ui::RuntimeUiWorkbenchPanelKind::inventory,
            .title_localization_key = "ui.inventory.title",
            .accessibility_label = "Inventory",
            .enabled = true,
        },
        mirakana::ui::RuntimeUiWorkbenchPanelRow{
            .id = "equipment.paperdoll",
            .kind = mirakana::ui::RuntimeUiWorkbenchPanelKind::equipment,
            .title_localization_key = "ui.equipment.title",
            .accessibility_label = "Equipment",
            .enabled = true,
        },
        mirakana::ui::RuntimeUiWorkbenchPanelRow{
            .id = "shop.vendor",
            .kind = mirakana::ui::RuntimeUiWorkbenchPanelKind::shop,
            .title_localization_key = "ui.shop.title",
            .accessibility_label = "Shop",
            .enabled = true,
        },
        mirakana::ui::RuntimeUiWorkbenchPanelRow{
            .id = "dashboard.colony",
            .kind = mirakana::ui::RuntimeUiWorkbenchPanelKind::simulation_dashboard,
            .title_localization_key = "ui.dashboard.title",
            .accessibility_label = "Simulation dashboard",
            .enabled = true,
        },
    };
    document.table_columns = {
        mirakana::ui::RuntimeUiWorkbenchTableColumn{
            .panel_id = "dashboard.colony",
            .id = "resource",
            .localization_key = "ui.table.resource",
        },
        mirakana::ui::RuntimeUiWorkbenchTableColumn{
            .panel_id = "dashboard.colony",
            .id = "stored",
            .localization_key = "ui.table.stored",
        },
        mirakana::ui::RuntimeUiWorkbenchTableColumn{
            .panel_id = "dashboard.colony",
            .id = "delta",
            .localization_key = "ui.table.delta",
        },
    };
    document.table_rows = {
        mirakana::ui::RuntimeUiWorkbenchTableRow{
            .id = "dashboard.food",
            .panel_id = "dashboard.colony",
            .cells = {mirakana::ui::RuntimeUiWorkbenchTableCell{
                          .column_id = "resource",
                          .text = {},
                          .localization_key = "ui.resource.food",
                      },
                      mirakana::ui::RuntimeUiWorkbenchTableCell{
                          .column_id = "stored",
                          .text = "124",
                          .localization_key = {},
                      },
                      mirakana::ui::RuntimeUiWorkbenchTableCell{
                          .column_id = "delta",
                          .text = "+8",
                          .localization_key = {},
                      }},
        },
        mirakana::ui::RuntimeUiWorkbenchTableRow{
            .id = "dashboard.power",
            .panel_id = "dashboard.colony",
            .cells = {mirakana::ui::RuntimeUiWorkbenchTableCell{
                          .column_id = "resource",
                          .text = {},
                          .localization_key = "ui.resource.power",
                      },
                      mirakana::ui::RuntimeUiWorkbenchTableCell{
                          .column_id = "stored",
                          .text = "72",
                          .localization_key = {},
                      },
                      mirakana::ui::RuntimeUiWorkbenchTableCell{
                          .column_id = "delta",
                          .text = "-3",
                          .localization_key = {},
                      }},
        },
    };
    document.graph_series = {
        mirakana::ui::RuntimeUiWorkbenchGraphSeries{
            .id = "graph.food",
            .panel_id = "dashboard.colony",
            .localization_key = "ui.graph.food",
            .accessibility_label = "Food trend",
            .points = {mirakana::ui::RuntimeUiWorkbenchGraphPoint{.x = 0.0, .y = 100.0},
                       mirakana::ui::RuntimeUiWorkbenchGraphPoint{.x = 1.0, .y = 108.0}},
        },
        mirakana::ui::RuntimeUiWorkbenchGraphSeries{
            .id = "graph.power",
            .panel_id = "dashboard.colony",
            .localization_key = "ui.graph.power",
            .accessibility_label = "Power trend",
            .points = {mirakana::ui::RuntimeUiWorkbenchGraphPoint{.x = 0.0, .y = 75.0},
                       mirakana::ui::RuntimeUiWorkbenchGraphPoint{.x = 1.0, .y = 72.0}},
        },
    };
    document.item_rows = {
        mirakana::ui::RuntimeUiWorkbenchItemRow{
            .id = "inventory.potion",
            .panel_id = "inventory.pack",
            .kind = mirakana::ui::RuntimeUiWorkbenchItemRowKind::inventory,
            .item_id = "item.potion",
            .slot_id = {},
            .quantity = 3,
            .price = 0,
            .localization_key = "ui.item.potion",
            .accessibility_label = "Potion",
        },
        mirakana::ui::RuntimeUiWorkbenchItemRow{
            .id = "equipment.weapon",
            .panel_id = "equipment.paperdoll",
            .kind = mirakana::ui::RuntimeUiWorkbenchItemRowKind::equipment,
            .item_id = "item.sword",
            .slot_id = "slot.weapon",
            .quantity = 1,
            .price = 0,
            .localization_key = "ui.item.sword",
            .accessibility_label = "Equipped weapon",
        },
        mirakana::ui::RuntimeUiWorkbenchItemRow{
            .id = "shop.elixir",
            .panel_id = "shop.vendor",
            .kind = mirakana::ui::RuntimeUiWorkbenchItemRowKind::shop,
            .item_id = "item.elixir",
            .slot_id = {},
            .quantity = 4,
            .price = 25,
            .localization_key = "ui.item.elixir",
            .accessibility_label = "Elixir for sale",
        },
    };
    document.text_inputs = {
        mirakana::ui::RuntimeUiWorkbenchTextInputFieldRow{
            .id = "input.search",
            .panel_id = "inventory.pack",
            .target = mirakana::ui::ElementId{"inventory.search"},
            .text_bounds = mirakana::ui::Rect{.x = 8.0F, .y = 8.0F, .width = 220.0F, .height = 32.0F},
            .placeholder_localization_key = "ui.inventory.search.placeholder",
            .accessibility_label = "Search inventory",
            .max_code_units = 64U,
        },
    };
    document.initial_focus_id = "inventory.potion";
    document.focus_edges = {
        mirakana::ui::RuntimeUiWorkbenchFocusEdge{
            .id = "inventory.potion",
            .next = "equipment.weapon",
            .previous = {},
            .up = {},
            .down = "input.search",
            .left = {},
            .right = "equipment.weapon",
        },
        mirakana::ui::RuntimeUiWorkbenchFocusEdge{
            .id = "equipment.weapon",
            .next = "shop.elixir",
            .previous = "inventory.potion",
            .up = {},
            .down = {},
            .left = {},
            .right = "shop.elixir",
        },
        mirakana::ui::RuntimeUiWorkbenchFocusEdge{
            .id = "shop.elixir",
            .next = "input.search",
            .previous = "equipment.weapon",
            .up = {},
            .down = {},
            .left = "equipment.weapon",
            .right = {},
        },
        mirakana::ui::RuntimeUiWorkbenchFocusEdge{
            .id = "input.search",
            .next = "inventory.potion",
            .previous = "shop.elixir",
            .up = "inventory.potion",
            .down = {},
            .left = {},
            .right = {},
        },
    };

    const auto plan = mirakana::ui::plan_runtime_ui_workbench(document);
    RuntimeUiWorkbenchProbeResult result{
        .ready = plan.succeeded(),
        .status = plan.status,
        .panels = plan.panels.size(),
        .table_columns = plan.table_columns.size(),
        .table_rows = plan.table_rows.size(),
        .graph_series = plan.graph_series.size(),
        .item_rows = plan.item_rows.size(),
        .inventory_rows = 0U,
        .equipment_rows = 0U,
        .shop_rows = 0U,
        .text_inputs = plan.text_inputs.size(),
        .platform_text_input_requests = plan.platform_text_input_requests.size(),
        .focus_edges = plan.focus_plan.edges.size(),
        .localization_refs = plan.localization_references.size(),
        .localization_identity_ready =
            has_runtime_ui_workbench_localization_ref(plan.localization_references, "dashboard.colony:resource",
                                                      "ui.table.resource") &&
            has_runtime_ui_workbench_localization_ref(plan.localization_references, "dashboard.food:resource",
                                                      "ui.resource.food"),
        .accessibility_refs = plan.accessibility_references.size(),
        .accessibility_identity_ready =
            has_runtime_ui_workbench_accessibility_ref(plan.accessibility_references, "inventory.potion", "Potion") &&
            has_runtime_ui_workbench_accessibility_ref(plan.accessibility_references, "input.search",
                                                       "Search inventory"),
        .diagnostics = plan.diagnostics.size(),
        .renderer_submission = plan.invoked_renderer_submission,
        .text_shaping = plan.invoked_text_shaping,
        .font_rasterization = plan.invoked_font_rasterization,
        .ime_sessions = plan.invoked_ime,
        .accessibility_bridge = plan.invoked_accessibility_bridge,
        .image_decoding = plan.invoked_image_decoding,
        .native_platform = plan.invoked_native_platform};
    for (const auto& row : plan.item_rows) {
        if (row.kind == mirakana::ui::RuntimeUiWorkbenchItemRowKind::inventory) {
            ++result.inventory_rows;
        }
        if (row.kind == mirakana::ui::RuntimeUiWorkbenchItemRowKind::equipment) {
            ++result.equipment_rows;
        }
        if (row.kind == mirakana::ui::RuntimeUiWorkbenchItemRowKind::shop) {
            ++result.shop_rows;
        }
    }
    result.ready =
        result.ready && result.panels == 5U && result.table_columns == 3U && result.table_rows == 2U &&
        result.graph_series == 2U && result.item_rows == 3U && result.inventory_rows == 1U &&
        result.equipment_rows == 1U && result.shop_rows == 1U && result.text_inputs == 1U &&
        result.platform_text_input_requests == 1U && result.focus_edges == 4U && result.localization_refs == 17U &&
        result.localization_identity_ready && result.accessibility_refs == 11U && result.accessibility_identity_ready &&
        result.diagnostics == 0U && !result.renderer_submission && !result.text_shaping && !result.font_rasterization &&
        !result.ime_sessions && !result.accessibility_bridge && !result.image_decoding && !result.native_platform;
    return result;
}

[[nodiscard]] mirakana::ui::RuntimeUiProductionEvidenceRow make_runtime_ui_text_shaping_evidence_row() {
    mirakana::ui::RuntimeUiProductionEvidenceRow row;
    row.id = "runtime-ui.text-shaping";
    row.feature = mirakana::ui::RuntimeUiProductionFeatureKind::text_shaping;
    row.proof = mirakana::ui::RuntimeUiProductionProofKind::first_party_contract;
    row.request_validation = true;
    row.shaping_segments = true;
    row.shaping_direction_script_language = true;
    row.glyph_clusters = true;
    row.glyph_advances_offsets = true;
    row.fallback_font_rows = true;
    row.bidi_boundaries = true;
    row.line_break_boundaries = true;
    return row;
}

[[nodiscard]] mirakana::ui::RuntimeUiProductionEvidenceRow make_runtime_ui_font_rasterization_evidence_row() {
    mirakana::ui::RuntimeUiProductionEvidenceRow row;
    row.id = "runtime-ui.font-rasterization";
    row.feature = mirakana::ui::RuntimeUiProductionFeatureKind::font_rasterization;
    row.proof = mirakana::ui::RuntimeUiProductionProofKind::adapter_handoff;
    row.request_validation = true;
    row.glyph_bitmap_rows = true;
    row.glyph_pixel_format_rows = true;
    row.glyph_metric_rows = true;
    return row;
}

[[nodiscard]] mirakana::ui::RuntimeUiProductionEvidenceRow make_runtime_ui_glyph_atlas_evidence_row() {
    mirakana::ui::RuntimeUiProductionEvidenceRow row;
    row.id = "runtime-ui.glyph-atlas";
    row.feature = mirakana::ui::RuntimeUiProductionFeatureKind::glyph_atlas;
    row.proof = mirakana::ui::RuntimeUiProductionProofKind::adapter_handoff;
    row.atlas_placement_rows = true;
    row.atlas_budget_rows = true;
    row.atlas_eviction_diagnostics = true;
    row.renderer_texture_upload_handoff = true;
    return row;
}

[[nodiscard]] mirakana::ui::RuntimeUiProductionEvidenceRow make_runtime_ui_renderer_submission_evidence_row() {
    mirakana::ui::RuntimeUiProductionEvidenceRow row;
    row.id = "runtime-ui.renderer-submission";
    row.feature = mirakana::ui::RuntimeUiProductionFeatureKind::renderer_submission;
    row.proof = mirakana::ui::RuntimeUiProductionProofKind::selected_package;
    row.renderer_texture_upload_handoff = true;
    row.selected_package_counter_evidence = true;
    return row;
}

[[nodiscard]] mirakana::ui::RuntimeUiProductionEvidenceRow make_runtime_ui_ime_evidence_row() {
    mirakana::ui::RuntimeUiProductionEvidenceRow row;
    row.id = "runtime-ui.ime";
    row.feature = mirakana::ui::RuntimeUiProductionFeatureKind::ime;
    row.proof = mirakana::ui::RuntimeUiProductionProofKind::host_gate;
    row.host_evidence_required = true;
    row.ime_session_begin_end_rows = true;
    row.ime_composition_update_rows = true;
    row.ime_candidate_rows = true;
    row.ime_text_area_cursor_rows = true;
    row.ime_committed_text_rows = true;
    row.ime_clipboard_rows = true;
    row.ime_platform_adapter_proof_rows = true;
    row.ime_platform_host_gate_rows = true;
    row.platform_adapter_dispatch_boundary = true;
    return row;
}

[[nodiscard]] mirakana::ui::RuntimeUiProductionEvidenceRow make_runtime_ui_accessibility_evidence_row() {
    mirakana::ui::RuntimeUiProductionEvidenceRow row;
    row.id = "runtime-ui.accessibility";
    row.feature = mirakana::ui::RuntimeUiProductionFeatureKind::accessibility;
    row.proof = mirakana::ui::RuntimeUiProductionProofKind::host_gate;
    row.host_evidence_required = true;
    row.accessibility_role_rows = true;
    row.accessibility_name_rows = true;
    row.accessibility_description_rows = true;
    row.accessibility_state_rows = true;
    row.accessibility_focus_rows = true;
    row.accessibility_action_rows = true;
    row.accessibility_relationship_rows = true;
    row.accessibility_live_region_rows = true;
    row.accessibility_keyboard_pattern_rows = true;
    row.accessibility_publication_status_rows = true;
    row.accessibility_uia_host_gate_rows = true;
    row.accessibility_platform_host_gate_rows = true;
    return row;
}

[[nodiscard]] RuntimeUiProductionStackProbeResult validate_runtime_ui_production_stack_package_evidence() {
    mirakana::ui::RuntimeUiProductionStackRequest request;
    request.id = "sample-2d-runtime-ui-production-stack";
    request.rows = {
        make_runtime_ui_text_shaping_evidence_row(), make_runtime_ui_font_rasterization_evidence_row(),
        make_runtime_ui_glyph_atlas_evidence_row(),  make_runtime_ui_renderer_submission_evidence_row(),
        make_runtime_ui_ime_evidence_row(),          make_runtime_ui_accessibility_evidence_row(),
    };

    const auto plan = mirakana::ui::plan_runtime_ui_production_stack(request);
    const auto& ime_row = request.rows[4];
    const auto& accessibility_row = request.rows[5];
    const auto bool_to_count = [](bool value) -> std::size_t { return value ? 1U : 0U; };
    RuntimeUiProductionStackProbeResult result{
        .package_evidence_ready =
            plan.reviewed && plan.status == mirakana::ui::RuntimeUiProductionStackStatus::host_evidence_required &&
            plan.rows.size() == 6U && plan.ready_rows == 4U && plan.host_gated_rows == 2U &&
            plan.dependency_gated_rows == 0U && plan.skipped_rows == 0U && plan.adapter_invoked_rows == 0U &&
            plan.unsupported_rows == 0U && plan.text_stack_contract_ready &&
            plan.selected_package_counter_evidence_ready && !plan.production_runtime_ui_ready &&
            plan.requires_ime_host_evidence && !plan.ime_host_evidence_available &&
            ime_row.ime_session_begin_end_rows && ime_row.ime_composition_update_rows && ime_row.ime_candidate_rows &&
            ime_row.ime_text_area_cursor_rows && ime_row.ime_committed_text_rows && ime_row.ime_clipboard_rows &&
            ime_row.ime_platform_adapter_proof_rows && ime_row.ime_platform_host_gate_rows &&
            plan.requires_accessibility_host_evidence && !plan.accessibility_host_evidence_available &&
            accessibility_row.accessibility_role_rows && accessibility_row.accessibility_name_rows &&
            accessibility_row.accessibility_description_rows && accessibility_row.accessibility_state_rows &&
            accessibility_row.accessibility_focus_rows && accessibility_row.accessibility_action_rows &&
            accessibility_row.accessibility_relationship_rows && accessibility_row.accessibility_live_region_rows &&
            accessibility_row.accessibility_keyboard_pattern_rows &&
            accessibility_row.accessibility_publication_status_rows &&
            accessibility_row.accessibility_uia_host_gate_rows &&
            accessibility_row.accessibility_platform_host_gate_rows && plan.diagnostics.empty(),
        .reviewed = plan.reviewed,
        .status = plan.status,
        .rows = plan.rows.size(),
        .ready_rows = plan.ready_rows,
        .host_gated_rows = plan.host_gated_rows,
        .dependency_gated_rows = plan.dependency_gated_rows,
        .skipped_rows = plan.skipped_rows,
        .adapter_invoked_rows = plan.adapter_invoked_rows,
        .unsupported_rows = plan.unsupported_rows,
        .text_contract_ready = plan.text_stack_contract_ready,
        .selected_package_evidence_ready = plan.selected_package_counter_evidence_ready,
        .production_ready = plan.production_runtime_ui_ready,
        .requires_ime_host_evidence = plan.requires_ime_host_evidence,
        .ime_host_evidence = plan.ime_host_evidence_available,
        .ime_session_rows = bool_to_count(ime_row.ime_session_begin_end_rows),
        .ime_composition_rows = bool_to_count(ime_row.ime_composition_update_rows),
        .ime_candidate_rows = bool_to_count(ime_row.ime_candidate_rows),
        .ime_text_area_cursor_rows = bool_to_count(ime_row.ime_text_area_cursor_rows),
        .ime_committed_text_rows = bool_to_count(ime_row.ime_committed_text_rows),
        .ime_clipboard_rows = bool_to_count(ime_row.ime_clipboard_rows),
        .ime_platform_adapter_proof_rows = bool_to_count(ime_row.ime_platform_adapter_proof_rows),
        .ime_platform_host_gate_rows = bool_to_count(ime_row.ime_platform_host_gate_rows),
        .ime_platform_parity_ready = !plan.requires_ime_host_evidence && plan.ime_host_evidence_available,
        .requires_accessibility_host_evidence = plan.requires_accessibility_host_evidence,
        .accessibility_host_evidence = plan.accessibility_host_evidence_available,
        .accessibility_role_rows = bool_to_count(accessibility_row.accessibility_role_rows),
        .accessibility_name_rows = bool_to_count(accessibility_row.accessibility_name_rows),
        .accessibility_description_rows = bool_to_count(accessibility_row.accessibility_description_rows),
        .accessibility_state_rows = bool_to_count(accessibility_row.accessibility_state_rows),
        .accessibility_focus_rows = bool_to_count(accessibility_row.accessibility_focus_rows),
        .accessibility_action_rows = bool_to_count(accessibility_row.accessibility_action_rows),
        .accessibility_relationship_rows = bool_to_count(accessibility_row.accessibility_relationship_rows),
        .accessibility_live_region_rows = bool_to_count(accessibility_row.accessibility_live_region_rows),
        .accessibility_keyboard_pattern_rows = bool_to_count(accessibility_row.accessibility_keyboard_pattern_rows),
        .accessibility_publication_status_rows = bool_to_count(accessibility_row.accessibility_publication_status_rows),
        .accessibility_uia_host_gate_rows = bool_to_count(accessibility_row.accessibility_uia_host_gate_rows),
        .accessibility_platform_host_gate_rows = bool_to_count(accessibility_row.accessibility_platform_host_gate_rows),
        .accessibility_platform_parity_ready =
            !plan.requires_accessibility_host_evidence && plan.accessibility_host_evidence_available,
        .invoked_text_shaping = plan.invoked_text_shaping,
        .invoked_font_rasterization = plan.invoked_font_rasterization,
        .invoked_ime = plan.invoked_ime_adapter,
        .invoked_accessibility_bridge = plan.invoked_accessibility_bridge,
        .invoked_native_platform = plan.invoked_native_platform,
        .invoked_renderer_upload = plan.invoked_renderer_upload,
        .diagnostics = plan.diagnostics.size(),
        .replay_hash = plan.replay_hash,
    };
    result.package_evidence_ready =
        result.package_evidence_ready && !result.invoked_text_shaping && !result.invoked_font_rasterization &&
        !result.invoked_ime && !result.invoked_accessibility_bridge && !result.invoked_native_platform &&
        !result.invoked_renderer_upload && result.ime_session_rows == 1U && result.ime_composition_rows == 1U &&
        result.ime_candidate_rows == 1U && result.ime_text_area_cursor_rows == 1U &&
        result.ime_committed_text_rows == 1U && result.ime_clipboard_rows == 1U &&
        result.ime_platform_adapter_proof_rows == 1U && result.ime_platform_host_gate_rows == 1U &&
        !result.ime_platform_parity_ready && result.accessibility_role_rows == 1U &&
        result.accessibility_name_rows == 1U && result.accessibility_description_rows == 1U &&
        result.accessibility_state_rows == 1U && result.accessibility_focus_rows == 1U &&
        result.accessibility_action_rows == 1U && result.accessibility_relationship_rows == 1U &&
        result.accessibility_live_region_rows == 1U && result.accessibility_keyboard_pattern_rows == 1U &&
        result.accessibility_publication_status_rows == 1U && result.accessibility_uia_host_gate_rows == 1U &&
        result.accessibility_platform_host_gate_rows == 1U && !result.accessibility_platform_parity_ready &&
        result.replay_hash != 0U;
    return result;
}

[[nodiscard]] RuntimeUiRendererAtlasHandoffProbeResult validate_runtime_ui_renderer_atlas_handoff_package_evidence(
    const mirakana::runtime::RuntimeAssetPackage& runtime_package) {
    RuntimeUiRendererAtlasHandoffProbeResult result;
    const auto image_palette =
        mirakana::build_ui_renderer_image_palette_from_runtime_ui_atlas(runtime_package, packaged_ui_atlas_asset_id());
    const auto glyph_palette = mirakana::build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(
        runtime_package, packaged_ui_atlas_asset_id());
    if (!image_palette.succeeded() || !glyph_palette.succeeded()) {
        result.diagnostics = image_palette.failures.size() + glyph_palette.failures.size();
        return result;
    }

    mirakana::ui::RendererSubmission submission;
    mirakana::ui::RendererTextRun text;
    text.id = mirakana::ui::ElementId{"hud-text"};
    text.bounds = mirakana::ui::Rect{.x = 4.0F, .y = 4.0F, .width = 16.0F, .height = 16.0F};
    text.text.label = "A";
    text.text.font_family = "ui/body";
    submission.text_runs.push_back(text);

    mirakana::ui::RendererImagePlaceholder image;
    image.id = mirakana::ui::ElementId{"hud-icon"};
    image.bounds = mirakana::ui::Rect{.x = 24.0F, .y = 4.0F, .width = 16.0F, .height = 16.0F};
    image.image.resource_id = "hud.icon";
    image.image.asset_uri = "runtime/assets/2d/player.texture.geasset";
    submission.image_placeholders.push_back(image);

    mirakana::UiRenderSubmitDesc desc;
    desc.image_palette = &image_palette.palette;
    desc.glyph_atlas = &glyph_palette.palette;
    desc.text_layout_policy = mirakana::ui::MonospaceTextLayoutPolicy{
        .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F};

    mirakana::NullRenderer renderer(mirakana::Extent2D{.width = 96, .height = 64});
    renderer.begin_frame();
    const auto submit = mirakana::submit_ui_renderer_submission(renderer, submission, desc);
    renderer.end_frame();

    const auto plan = mirakana::review_ui_renderer_atlas_handoff(mirakana::UiRendererAtlasHandoffRequest{
        .id = "sample-2d-runtime-ui-renderer-atlas-handoff",
        .image_atlas_page_count = image_palette.atlas_page_assets.size(),
        .image_atlas_binding_count = image_palette.palette.count(),
        .glyph_atlas_page_count = glyph_palette.atlas_page_assets.size(),
        .glyph_atlas_binding_count = glyph_palette.palette.count(),
        .submit_result = submit,
        .renderer_sprites_submitted = renderer.stats().sprites_submitted,
        .max_image_bindings = 4U,
        .max_glyph_bindings = 8U,
        .image_atlas_metadata_built = image_palette.succeeded(),
        .glyph_atlas_metadata_built = glyph_palette.succeeded(),
        .atlas_eviction_diagnostics_reviewed = true,
        .texture_upload_handoff_reviewed = true,
        .renderer_submission_counters_reviewed = true,
        .selected_package_counter_evidence = true,
        .requested_renderer_texture_upload_api = false,
        .requested_public_native_handle = false,
        .claims_broad_text_rendering = false,
        .claims_broad_image_rendering = false,
        .invoked_source_image_decode = false,
        .invoked_live_glyph_atlas_generation = false,
        .invoked_renderer_upload = false,
        .seed = 57U,
    });

    result.selected_package_evidence_ready = plan.selected_package_evidence_ready;
    result.reviewed = plan.reviewed;
    result.status = plan.status;
    result.image_atlas_pages = plan.image_atlas_pages;
    result.image_atlas_bindings = plan.image_atlas_bindings;
    result.glyph_atlas_pages = plan.glyph_atlas_pages;
    result.glyph_atlas_bindings = plan.glyph_atlas_bindings;
    result.atlas_placement_rows = plan.atlas_placement_rows;
    result.atlas_budget_rows = plan.atlas_budget_rows;
    result.atlas_eviction_diagnostic_rows = plan.atlas_eviction_diagnostic_rows;
    result.texture_upload_handoff_rows = plan.texture_upload_handoff_rows;
    result.renderer_submission_counter_rows = plan.renderer_submission_counter_rows;
    result.text_glyphs_available = plan.text_glyphs_available;
    result.text_glyphs_resolved = plan.text_glyphs_resolved;
    result.text_glyphs_missing = plan.text_glyphs_missing;
    result.text_glyph_sprites_submitted = plan.text_glyph_sprites_submitted;
    result.image_placeholders_available = plan.image_placeholders_available;
    result.image_resources_resolved = plan.image_resources_resolved;
    result.image_resources_missing = plan.image_resources_missing;
    result.image_sprites_submitted = plan.image_sprites_submitted;
    result.renderer_sprites_submitted = plan.renderer_sprites_submitted;
    result.unsupported_claim_rows = plan.unsupported_claim_rows;
    result.side_effect_rows = plan.side_effect_rows;
    result.requested_renderer_texture_upload_api = plan.requested_renderer_texture_upload_api;
    result.requested_public_native_handle = plan.requested_public_native_handle;
    result.invoked_source_image_decode = plan.invoked_source_image_decode;
    result.invoked_live_glyph_atlas_generation = plan.invoked_live_glyph_atlas_generation;
    result.invoked_renderer_upload = plan.invoked_renderer_upload;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.ready = plan.ready() && result.image_atlas_pages == 1U && result.image_atlas_bindings == 1U &&
                   result.glyph_atlas_pages == 1U && result.glyph_atlas_bindings == 1U &&
                   result.atlas_placement_rows == 2U && result.atlas_budget_rows == 2U &&
                   result.atlas_eviction_diagnostic_rows == 1U && result.texture_upload_handoff_rows == 1U &&
                   result.renderer_submission_counter_rows == 1U && result.text_glyphs_available == 1U &&
                   result.text_glyphs_resolved == 1U && result.text_glyphs_missing == 0U &&
                   result.text_glyph_sprites_submitted == 1U && result.image_placeholders_available == 1U &&
                   result.image_resources_resolved == 1U && result.image_resources_missing == 0U &&
                   result.image_sprites_submitted == 1U && result.renderer_sprites_submitted == 2U &&
                   result.unsupported_claim_rows == 0U && result.side_effect_rows == 0U &&
                   !result.requested_renderer_texture_upload_api && !result.requested_public_native_handle &&
                   !result.invoked_source_image_decode && !result.invoked_live_glyph_atlas_generation &&
                   !result.invoked_renderer_upload && result.replay_hash != 0U;
    return result;
}

[[nodiscard]] AudioGameplayMixerProbeResult
validate_audio_gameplay_mixer_package_evidence(const mirakana::AudioClipSampleData& samples) {
    AudioGameplayMixerProbeResult result;
    result.payload_diagnostics = mirakana::is_valid_audio_clip_sample_data(samples) ? 0U : 1U;

    const auto request = mirakana::AudioGameplayMixRequest{
        .buses =
            {
                mirakana::AudioGameplayBusMixDesc{.name = "music",
                                                  .gain = 0.8F,
                                                  .paused = false,
                                                  .fade_from_gain = 0.5F,
                                                  .fade_to_gain = 1.0F,
                                                  .fade_elapsed_seconds = 0.5F,
                                                  .fade_duration_seconds = 1.0F},
                mirakana::AudioGameplayBusMixDesc{.name = "sfx", .gain = 1.0F, .paused = true},
            },
        .cues =
            {
                mirakana::AudioGameplayCueDesc{.id = "music.theme",
                                               .kind = mirakana::AudioGameplayCueKind::music,
                                               .clip = samples.clip,
                                               .bus = "music",
                                               .gain = 0.5F,
                                               .looping = true},
                mirakana::AudioGameplayCueDesc{.id = "sfx.jump",
                                               .kind = mirakana::AudioGameplayCueKind::sfx,
                                               .clip = samples.clip,
                                               .bus = "sfx",
                                               .gain = 1.0F,
                                               .looping = false,
                                               .spatialized = true,
                                               .position = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                               .min_distance = 1.0F,
                                               .max_distance = 8.0F},
            },
        .triggers =
            {
                mirakana::AudioGameplayCueTrigger{.cue_id = "music.theme", .start_frame = 0U, .gain_scale = 1.0F},
                mirakana::AudioGameplayCueTrigger{.cue_id = "sfx.jump", .start_frame = 0U, .gain_scale = 0.75F},
            },
    };

    result.buses = request.buses.size();
    result.cues = request.cues.size();
    result.triggers = request.triggers.size();
    for (const auto& bus : request.buses) {
        if (bus.paused) {
            ++result.paused_buses;
        }
        if (bus.fade_duration_seconds > 0.0F) {
            ++result.faded_buses;
        }
    }

    const auto plan = mirakana::plan_gameplay_audio_mix(request);
    result.diagnostics = plan.diagnostics.size();
    result.commands = plan.commands.size();
    for (const auto& command : plan.commands) {
        if (command.voice.looping) {
            ++result.looping_commands;
        }
        if (command.spatialized) {
            ++result.spatial_commands;
        }
    }
    if (!plan.succeeded() || result.payload_diagnostics != 0U) {
        return result;
    }

    try {
        mirakana::AudioMixer mixer;
        for (const auto& bus : plan.buses) {
            mixer.add_bus(bus);
        }
        if (!mixer.register_clip(mirakana::AudioClipDesc{.clip = samples.clip,
                                                         .sample_rate = samples.format.sample_rate,
                                                         .channel_count = samples.format.channel_count,
                                                         .frame_count = samples.frame_count,
                                                         .sample_format = samples.format.sample_format,
                                                         .streaming = false,
                                                         .buffered_frame_count = samples.frame_count})) {
            ++result.payload_diagnostics;
            return result;
        }
        for (const auto& command : plan.commands) {
            (void)mixer.play(command.voice);
        }

        const auto output = mirakana::render_audio_device_stream_interleaved_float(
            mixer,
            mirakana::AudioDeviceStreamRequest{
                .format = samples.format,
                .device_frame = 0U,
                .queued_frames = 0U,
                .target_queued_frames = 2U,
                .max_render_frames = 2U,
            },
            std::span<const mirakana::AudioClipSampleData>{&samples, 1});
        result.render_commands = output.buffer.plan.commands.size();
        result.render_frames = output.buffer.frame_count;
        result.render_samples = output.buffer.interleaved_float_samples.size();
        for (const auto sample : output.buffer.interleaved_float_samples) {
            result.sample_abs_sum += std::abs(sample);
        }
    } catch (const std::exception&) {
        ++result.diagnostics;
        return result;
    }

    result.ready = result.diagnostics == 0U && result.payload_diagnostics == 0U && result.buses == 2U &&
                   result.cues == 2U && result.triggers == 2U && result.commands == 2U && result.paused_buses == 1U &&
                   result.faded_buses == 1U && result.looping_commands == 1U && result.spatial_commands == 1U &&
                   result.render_commands == 2U && result.render_frames == 2U && result.render_samples == 2U &&
                   result.sample_abs_sum > 0.0F;
    return result;
}

[[nodiscard]] AudioProductionProbeResult
validate_audio_production_package_evidence(const mirakana::AudioClipSampleData& samples) {
    AudioProductionProbeResult result;
    const auto reviewed_frames = std::min<std::uint64_t>(samples.frame_count, 4U);
    const auto clip = samples.clip;
    const auto request = mirakana::AudioProductionReviewRequest{
        .decoded_sources =
            {
                mirakana::AudioProductionDecodedSourceEvidenceRow{
                    .clip = clip,
                    .format = samples.format,
                    .frame_count = reviewed_frames,
                    .decoded_byte_count = reviewed_frames * samples.format.channel_count * sizeof(float),
                    .reviewed = true,
                    .source_index = 1U,
                },
            },
        .streaming_chunks =
            {
                mirakana::AudioProductionStreamingChunkEvidenceRow{
                    .chunk =
                        mirakana::AudioStreamingChunkDesc{
                            .clip = clip,
                            .format = samples.format,
                            .start_frame = 0U,
                            .frame_count = reviewed_frames,
                        },
                    .queued_frame_count = reviewed_frames,
                    .reviewed = true,
                    .source_index = 2U,
                },
            },
        .format_conversion_policies =
            {
                mirakana::AudioProductionFormatConversionPolicyRow{
                    .clip = clip,
                    .source_format = samples.format,
                    .device_format = samples.format,
                    .resampling_quality = mirakana::AudioResamplingQuality::linear,
                    .reviewed = true,
                    .source_index = 3U,
                },
            },
        .dsp_graph_rows =
            {
                mirakana::AudioProductionDspGraphRow{
                    .node_id = "sample2d.audio.limiter",
                    .kind = mirakana::AudioProductionDspNodeKind::limiter,
                    .input_count = 1U,
                    .output_count = 1U,
                    .deterministic = true,
                    .reviewed = true,
                    .source_index = 4U,
                },
            },
        .listener =
            mirakana::AudioSpatialListenerDesc{
                .position = mirakana::AudioPoint3{},
                .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            },
        .spatial_voices =
            {
                mirakana::AudioSpatialVoiceDesc{
                    .voice = mirakana::AudioVoiceId{1U},
                    .position = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                    .min_distance = 1.0F,
                    .max_distance = 8.0F,
                    .spatialized = true,
                },
            },
        .device_lifecycle_rows =
            {
                mirakana::AudioProductionDeviceLifecycleRow{
                    .backend_id = "wasapi",
                    .uses_logical_device = true,
                    .uses_audio_stream = true,
                    .uses_queueing = true,
                    .uses_callback = false,
                    .can_pause_resume = true,
                    .can_clear = true,
                    .host_evidence_available = false,
                    .native_handle_exposed = false,
                    .source_index = 5U,
                },
            },
        .unsupported_claim_rows = {},
        .max_voice_budget = 8U,
        .active_voice_count = 1U,
        .max_bus_budget = 4U,
        .active_bus_count = 2U,
        .row_budget = 32U,
        .official_sources_reviewed = true,
        .hrtf_host_evidence_available = false,
        .request_native_device_handles = false,
        .invoked_codec_decode = false,
        .invoked_background_streaming = false,
        .invoked_middleware = false,
        .invoked_hrtf = false,
        .invoked_device_callback = false,
        .invoked_device_io = false,
        .seed = 20260527U,
    };

    const auto plan = mirakana::review_audio_production_readiness(request);
    result.status = plan.status;
    result.reviewed = plan.reviewed;
    result.production_audio_ready = plan.production_audio_ready;
    result.selected_package_evidence_ready = plan.selected_package_evidence_ready;
    result.decoded_source_rows = plan.decoded_source_rows;
    result.streaming_chunk_rows = plan.streaming_chunk_rows;
    result.format_conversion_policy_rows = plan.format_conversion_policy_rows;
    result.bus_budget_rows = plan.bus_budget_rows;
    result.voice_budget_rows = plan.voice_budget_rows;
    result.dsp_graph_rows = plan.dsp_graph_rows;
    result.listener_rows = plan.listener_rows;
    result.spatial_source_rows = plan.spatial_source_rows;
    result.hrtf_host_gate_rows = plan.hrtf_host_gate_rows;
    result.device_lifecycle_rows = plan.device_lifecycle_rows;
    result.device_host_evidence_available = plan.device_host_evidence_available;
    result.hrtf_host_evidence_available = plan.hrtf_host_evidence_available;
    result.unsupported_claim_rows = plan.unsupported_claim_rows;
    result.invoked_codec_decode = plan.invoked_codec_decode;
    result.invoked_background_streaming = plan.invoked_background_streaming;
    result.invoked_middleware = plan.invoked_middleware;
    result.invoked_hrtf = plan.invoked_hrtf;
    result.invoked_device_callback = plan.invoked_device_callback;
    result.invoked_device_io = plan.invoked_device_io;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.package_evidence_ready =
        result.status == mirakana::AudioProductionReadinessStatus::host_evidence_required && result.reviewed &&
        !result.production_audio_ready && result.selected_package_evidence_ready && result.decoded_source_rows == 1U &&
        result.streaming_chunk_rows == 1U && result.format_conversion_policy_rows == 1U &&
        result.bus_budget_rows == 1U && result.voice_budget_rows == 1U && result.dsp_graph_rows == 1U &&
        result.listener_rows == 1U && result.spatial_source_rows == 1U && result.hrtf_host_gate_rows == 1U &&
        result.device_lifecycle_rows == 1U && !result.device_host_evidence_available &&
        !result.hrtf_host_evidence_available && result.unsupported_claim_rows == 0U && !result.invoked_codec_decode &&
        !result.invoked_background_streaming && !result.invoked_middleware && !result.invoked_hrtf &&
        !result.invoked_device_callback && !result.invoked_device_io && result.diagnostics == 2U &&
        result.replay_hash != 0U;
    return result;
}

[[nodiscard]] Gameplay2DConstructionPlacementProbeResult validate_gameplay_2d_construction_placement() {
    using namespace mirakana::runtime;
    using IntentStatus = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    Gameplay2DConstructionPlacementProbeResult result;
    const auto catalog = gameplay_2d_inventory_catalog_document();
    const std::vector<std::string> placement_ids{"grid_2d"};
    const std::vector<RuntimeConstructionPlacementSurfaceDesc> surfaces{
        RuntimeConstructionPlacementSurfaceDesc{.id = "floor", .placement_id = "grid_2d"},
    };
    const std::vector<RuntimeConstructionPlacementCandidateDesc> candidates{
        RuntimeConstructionPlacementCandidateDesc{
            .item_id = "workbench",
            .surface_id = "floor",
            .grid_x = 4.0F,
            .grid_y = 7.0F,
            .grid_z = 0.0F,
            .world_x = 4.5F,
            .world_y = 7.5F,
            .world_z = 0.0F,
            .footprint_width = 2U,
            .footprint_height = 1U,
            .footprint_depth = 1U,
            .occupied_cells =
                std::vector<RuntimeConstructionPlacementCellDesc>{
                    RuntimeConstructionPlacementCellDesc{.x = 4, .y = 7, .z = 0},
                    RuntimeConstructionPlacementCellDesc{.x = 5, .y = 7, .z = 0},
                },
            .provided_costs =
                std::vector<RuntimeItemCostDesc>{
                    RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                },
        },
    };

    const auto placement = validate_runtime_construction_placement(
        catalog, candidates,
        RuntimeConstructionPlacementValidationContext{
            .supported_placement_ids = std::span<const std::string>{placement_ids},
            .supported_surfaces = std::span<const RuntimeConstructionPlacementSurfaceDesc>{surfaces},
        });
    result.diagnostics += placement.diagnostics.size();
    result.validation_rows = placement.rows.size();
    if (!placement.succeeded) {
        return result;
    }

    mirakana::SceneNodeComponents components;
    components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = packaged_sprite_texture_asset_id(),
        .material = mirakana::AssetId{11460315010553722633ULL},
        .size = mirakana::Vec2{.x = 2.0F, .y = 1.0F},
        .tint = {0.8F, 0.6F, 0.35F, 1.0F},
        .visible = true,
    };
    const std::vector<mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentDesc{
            .candidate_index = 0U,
            .node_name = "Workbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 4.5F, .y = 7.5F, .z = 0.0F}},
            .components = components,
            .reviewed = true,
        },
    };
    const auto intent_plan = mirakana::runtime_scene::plan_runtime_scene_construction_placement_intents(
        placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});
    result.diagnostics += intent_plan.diagnostics.size();
    result.intent_rows = intent_plan.rows.size();
    for (const auto& row : intent_plan.rows) {
        if (row.status == IntentStatus::accepted) {
            ++result.intent_accepted_rows;
            result.intent_occupied_cells += row.occupied_cells.size();
        }
    }
    result.ready = intent_plan.succeeded() && result.diagnostics == 0U && result.validation_rows == 3U &&
                   result.intent_rows == 1U && result.intent_accepted_rows == 1U && result.intent_occupied_cells == 2U;
    return result;
}

[[nodiscard]] Gameplay2DProceduralGenerationProbeResult validate_gameplay_2d_procedural_generation() {
    using Kind = mirakana::runtime::RuntimeProceduralGenerationContentKind;
    using IntentStatus = mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentStatus;

    Gameplay2DProceduralGenerationProbeResult result;
    const std::vector<std::string> supported_content_ids{
        "sample_2d.object.workbench",
        "sample_2d.encounter.tutorial",
        "sample_2d.loot.cache",
    };
    const auto generation = mirakana::runtime::plan_runtime_procedural_generation(
        mirakana::runtime::RuntimeProceduralGenerationRequest{
            .generator_id = "sample_2d.package.procedural.v1",
            .seed = 0x2D20260521ULL,
            .map_width = 8U,
            .map_height = 6U,
            .output_budget = 3U,
            .content =
                std::vector<mirakana::runtime::RuntimeProceduralGenerationContentRequest>{
                    mirakana::runtime::RuntimeProceduralGenerationContentRequest{
                        .content_id = "sample_2d.object.workbench",
                        .kind = Kind::object,
                        .count = 1U,
                    },
                    mirakana::runtime::RuntimeProceduralGenerationContentRequest{
                        .content_id = "sample_2d.encounter.tutorial",
                        .kind = Kind::encounter,
                        .count = 1U,
                    },
                    mirakana::runtime::RuntimeProceduralGenerationContentRequest{
                        .content_id = "sample_2d.loot.cache",
                        .kind = Kind::loot,
                        .count = 1U,
                    },
                },
        },
        mirakana::runtime::RuntimeProceduralGenerationContext{
            .supported_content_ids = std::span<const std::string>{supported_content_ids},
            .max_output_rows = 3U,
        });

    result.diagnostics += generation.diagnostics.size();
    result.rows = generation.rows.size();
    result.replay_hash = generation.replay_hash;
    std::string object_output_id;
    for (const auto& row : generation.rows) {
        switch (row.kind) {
        case Kind::map_tile:
            break;
        case Kind::encounter:
            ++result.encounter_rows;
            break;
        case Kind::loot:
            ++result.loot_rows;
            break;
        case Kind::object:
            ++result.object_rows;
            if (object_output_id.empty()) {
                object_output_id = row.id;
            }
            break;
        }
    }
    if (!generation.succeeded || object_output_id.empty() || result.rows != 3U || result.object_rows != 1U ||
        result.encounter_rows != 1U || result.loot_rows != 1U || result.replay_hash == 0ULL) {
        return result;
    }

    const auto catalog = gameplay_2d_inventory_catalog_document();
    const std::vector<std::string> placement_ids{"grid_2d"};
    const std::vector<mirakana::runtime::RuntimeConstructionPlacementSurfaceDesc> surfaces{
        mirakana::runtime::RuntimeConstructionPlacementSurfaceDesc{.id = "procedural_floor", .placement_id = "grid_2d"},
    };
    const std::vector<mirakana::runtime::RuntimeConstructionPlacementCandidateDesc> candidates{
        mirakana::runtime::RuntimeConstructionPlacementCandidateDesc{
            .item_id = "workbench",
            .surface_id = "procedural_floor",
            .grid_x = 2.0F,
            .grid_y = 3.0F,
            .grid_z = 0.0F,
            .world_x = 2.5F,
            .world_y = 3.5F,
            .world_z = 0.0F,
            .footprint_width = 2U,
            .footprint_height = 1U,
            .footprint_depth = 1U,
            .occupied_cells =
                std::vector<mirakana::runtime::RuntimeConstructionPlacementCellDesc>{
                    mirakana::runtime::RuntimeConstructionPlacementCellDesc{.x = 2, .y = 3, .z = 0},
                    mirakana::runtime::RuntimeConstructionPlacementCellDesc{.x = 3, .y = 3, .z = 0},
                },
            .provided_costs =
                std::vector<mirakana::runtime::RuntimeItemCostDesc>{
                    mirakana::runtime::RuntimeItemCostDesc{.item_id = "wood", .quantity = 3U},
                },
        },
    };
    const auto placement = mirakana::runtime::validate_runtime_construction_placement(
        catalog, candidates,
        mirakana::runtime::RuntimeConstructionPlacementValidationContext{
            .supported_placement_ids = std::span<const std::string>{placement_ids},
            .supported_surfaces = std::span<const mirakana::runtime::RuntimeConstructionPlacementSurfaceDesc>{surfaces},
        });
    result.diagnostics += placement.diagnostics.size();
    if (!placement.succeeded) {
        return result;
    }

    mirakana::SceneNodeComponents components;
    components.sprite_renderer = mirakana::SpriteRendererComponent{
        .sprite = packaged_sprite_texture_asset_id(),
        .material = mirakana::AssetId{11460315010553722633ULL},
        .size = mirakana::Vec2{.x = 2.0F, .y = 1.0F},
        .tint = {0.7F, 0.78F, 0.42F, 1.0F},
        .visible = true,
    };
    const std::vector<mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc> intents{
        mirakana::runtime_scene::RuntimeSceneProceduralConstructionPlacementIntentDesc{
            .procedural_output_id = object_output_id,
            .anchor_id = "sample_2d.package.procedural.workbench_anchor",
            .candidate_index = 0U,
            .node_name = "ProceduralWorkbench",
            .transform = mirakana::Transform3D{.position = mirakana::Vec3{.x = 2.5F, .y = 3.5F, .z = 0.0F}},
            .components = components,
            .reviewed = true,
            .package_visible = true,
        },
    };
    const auto intent_plan = mirakana::runtime_scene::plan_runtime_scene_procedural_construction_placement_intents(
        generation, placement, intents, mirakana::runtime_scene::RuntimeSceneConstructionPlacementIntentContext{});
    result.diagnostics += intent_plan.diagnostics.size();
    result.placement_intent_rows = intent_plan.rows.size();
    for (const auto& row : intent_plan.rows) {
        if (row.package_visible) {
            ++result.package_visible_rows;
        }
        if (row.status == IntentStatus::accepted) {
            ++result.placement_intent_accepted_rows;
        }
    }
    result.ready = intent_plan.succeeded() && result.diagnostics == 0U && result.rows == 3U &&
                   result.object_rows == 1U && result.encounter_rows == 1U && result.loot_rows == 1U &&
                   result.replay_hash != 0ULL && result.package_visible_rows == 1U &&
                   result.placement_intent_rows == 1U && result.placement_intent_accepted_rows == 1U;
    return result;
}

[[nodiscard]] EntityScaleCullingProbeResult validate_entity_scale_culling_package_evidence() {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Code = mirakana::runtime::RuntimeEntityScaleCullingDiagnosticCode;
    using DrawIntent = mirakana::runtime::RuntimeEntityScaleCullingDrawIntentKind;
    using Entity = mirakana::runtime::RuntimeEntityScaleCullingEntityDesc;
    using LodBand = mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_2d = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{};
    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};

    auto hero = Entity{
        .entity_id = "hero",
        .bounds_kind = BoundsKind::aabb_2d,
        .bounds_2d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                .min_x = -1.0F,
                .min_y = -1.0F,
                .max_x = 1.0F,
                .max_y = 1.0F,
            },
        .bounds_3d = empty_3d,
        .layer_mask = 0x1U,
        .update_bucket = UpdateBucket::priority,
        .enabled = true,
        .source_index = 2U,
        .lod_bands =
            std::vector<LodBand>{
                LodBand{.lod_index = 3U,
                        .max_view_distance = 4.0F,
                        .draw_cost = 99U,
                        .update_cost = 99U,
                        .update_interval_frames = 9U,
                        .draw_intent = DrawIntent::custom},
                LodBand{.lod_index = 0U,
                        .max_view_distance = 4.0F,
                        .draw_cost = 2U,
                        .update_cost = 1U,
                        .update_interval_frames = 1U,
                        .draw_intent = DrawIntent::sprite_2d},
                LodBand{.lod_index = 1U,
                        .max_view_distance = 12.0F,
                        .draw_cost = 4U,
                        .update_cost = 2U,
                        .update_interval_frames = 3U,
                        .draw_intent = DrawIntent::sprite_2d},
            },
        .budget_protected = true,
    };
    auto mesh = Entity{
        .entity_id = "mesh",
        .bounds_kind = BoundsKind::aabb_3d,
        .bounds_2d = empty_2d,
        .bounds_3d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                .min_x = 9.0F,
                .min_y = -1.0F,
                .min_z = -1.0F,
                .max_x = 11.0F,
                .max_y = 1.0F,
                .max_z = 1.0F,
            },
        .layer_mask = 0x1U,
        .update_bucket = UpdateBucket::normal,
        .enabled = true,
        .source_index = 1U,
        .lod_bands =
            std::vector<LodBand>{
                LodBand{.lod_index = 1U,
                        .max_view_distance = 20.0F,
                        .draw_cost = 5U,
                        .update_cost = 2U,
                        .update_interval_frames = 4U,
                        .draw_intent = DrawIntent::mesh_3d},
                LodBand{.lod_index = 0U,
                        .max_view_distance = 5.0F,
                        .draw_cost = 9U,
                        .update_cost = 4U,
                        .update_interval_frames = 1U,
                        .draw_intent = DrawIntent::mesh_3d},
            },
        .budget_protected = false,
    };
    auto disabled = Entity{
        .entity_id = "sleeping",
        .bounds_kind = BoundsKind::aabb_2d,
        .bounds_2d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                .min_x = 0.0F,
                .min_y = 0.0F,
                .max_x = 1.0F,
                .max_y = 1.0F,
            },
        .bounds_3d = empty_3d,
        .layer_mask = 0x1U,
        .update_bucket = UpdateBucket::background,
        .enabled = false,
        .source_index = 3U,
        .lod_bands =
            std::vector<LodBand>{
                LodBand{.lod_index = 0U,
                        .max_view_distance = 10.0F,
                        .draw_cost = 20U,
                        .update_cost = 20U,
                        .update_interval_frames = 1U,
                        .draw_intent = DrawIntent::sprite_2d},
            },
        .budget_protected = false,
    };
    const auto hidden = Entity{
        .entity_id = "hidden",
        .bounds_kind = BoundsKind::aabb_2d,
        .bounds_2d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                .min_x = 2.0F,
                .min_y = 2.0F,
                .max_x = 3.0F,
                .max_y = 3.0F,
            },
        .bounds_3d = empty_3d,
        .layer_mask = 0x4U,
        .update_bucket = UpdateBucket::normal,
        .enabled = true,
        .source_index = 4U,
        .lod_bands = {},
        .budget_protected = false,
    };
    const auto view = mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
        .bounds_kind = BoundsKind::aabb_3d,
        .bounds_2d = empty_2d,
        .bounds_3d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                .min_x = -16.0F,
                .min_y = -16.0F,
                .min_z = -8.0F,
                .max_x = 16.0F,
                .max_y = 16.0F,
                .max_z = 8.0F,
            },
        .layer_mask = 0x1U,
        .max_visible_entities = 8U,
        .max_projected_draw_cost = 32U,
        .max_projected_update_cost = 16U,
    };

    EntityScaleCullingProbeResult result;
    const auto plan = mirakana::runtime::plan_runtime_entity_scale_culling(
        mirakana::runtime::RuntimeEntityScaleCullingRequest{.entities = {mesh, disabled, hero, hidden}, .view = view});
    result.status = plan.status;
    result.rows = plan.rows.size();
    result.projected_draw_cost = plan.projected_draw_cost;
    result.projected_update_cost = plan.projected_update_cost;
    result.diagnostics = plan.diagnostics.size();
    for (const auto& row : plan.rows) {
        if (row.visible) {
            ++result.visible_rows;
        } else {
            ++result.culled_rows;
        }
        if (row.projected_draw_cost > 0U || row.projected_update_cost > 0U) {
            ++result.lod_rows;
        }
        if (row.budget_protected) {
            ++result.budget_protected_rows;
        }
        switch (row.update_bucket) {
        case UpdateBucket::background:
            ++result.background_update_rows;
            break;
        case UpdateBucket::normal:
            ++result.normal_update_rows;
            break;
        case UpdateBucket::priority:
            ++result.priority_update_rows;
            break;
        }
    }

    hero.lod_bands = {
        LodBand{.lod_index = 0U,
                .max_view_distance = 4.0F,
                .draw_cost = 9U,
                .update_cost = 5U,
                .update_interval_frames = 1U,
                .draw_intent = DrawIntent::sprite_2d},
    };
    mesh.lod_bands = {
        LodBand{.lod_index = 0U,
                .max_view_distance = 20.0F,
                .draw_cost = 4U,
                .update_cost = 2U,
                .update_interval_frames = 2U,
                .draw_intent = DrawIntent::mesh_3d},
    };
    auto budget_view = view;
    budget_view.max_projected_draw_cost = 10U;
    budget_view.max_projected_update_cost = 6U;
    const auto budget_plan = mirakana::runtime::plan_runtime_entity_scale_culling(
        mirakana::runtime::RuntimeEntityScaleCullingRequest{.entities = {mesh, hero}, .view = budget_view});
    for (const auto& diagnostic : budget_plan.diagnostics) {
        if (diagnostic.code == Code::draw_budget_exceeded || diagnostic.code == Code::update_budget_exceeded) {
            ++result.budget_diagnostics;
        }
    }

    result.ready =
        plan.succeeded() && result.status == mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::planned &&
        result.rows == 4U && result.visible_rows == 2U && result.culled_rows == 2U && result.lod_rows == 2U &&
        result.priority_update_rows == 1U && result.normal_update_rows == 2U && result.background_update_rows == 1U &&
        result.projected_draw_cost == 7U && result.projected_update_cost == 3U && result.budget_protected_rows == 1U &&
        result.diagnostics == 0U && result.budget_diagnostics == 2U;
    return result;
}

[[nodiscard]] std::size_t
count_scripting_sandbox_diagnostics(const mirakana::runtime::RuntimeScriptSandboxPlan& plan,
                                    mirakana::runtime::RuntimeScriptSandboxDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

class SampleScriptingSandboxAdapter final : public mirakana::runtime::IRuntimeScriptExecutionAdapter {
  public:
    [[nodiscard]] mirakana::runtime::RuntimeScriptAdapterResult
    execute(const mirakana::runtime::RuntimeScriptExecutionRequest& request,
            const mirakana::runtime::RuntimeScriptSandboxEntrypointPlanRow& entrypoint,
            std::span<const mirakana::runtime::RuntimeScriptSandboxPermissionPlanRow> permissions) override {
        const auto quest_api = std::ranges::find_if(permissions, [](const auto& row) {
            return row.allowed && row.kind == mirakana::runtime::RuntimeScriptSandboxPermissionKind::host_api &&
                   row.host_api_id == "sample2d.quest";
        });
        if (quest_api == permissions.end()) {
            return mirakana::runtime::RuntimeScriptAdapterResult{
                .completed = false,
                .stats =
                    mirakana::runtime::RuntimeScriptExecutionStats{
                        .instructions_consumed = 0U,
                        .memory_bytes_touched = 0U,
                        .host_api_call_count = 0U,
                    },
                .diagnostic_message = "sample script adapter needs the reviewed sample2d.quest host API",
            };
        }

        return mirakana::runtime::RuntimeScriptAdapterResult{
            .completed = true,
            .stats =
                mirakana::runtime::RuntimeScriptExecutionStats{
                    .instructions_consumed = 96U,
                    .memory_bytes_touched = 1536U,
                    .host_api_call_count = 1U,
                },
            .host_api_calls =
                {
                    mirakana::runtime::RuntimeScriptHostApiCall{
                        .host_api_id = quest_api->host_api_id,
                        .payload = request.input_event_id,
                        .source_index = quest_api->source_index,
                    },
                },
            .output_rows =
                {
                    entrypoint.module_id + "." + entrypoint.entrypoint_id + ".completed",
                },
        };
    }
};

[[nodiscard]] ScriptingSandboxProbeResult validate_scripting_sandbox_package_evidence() {
    using Code = mirakana::runtime::RuntimeScriptSandboxDiagnosticCode;
    using EntryKind = mirakana::runtime::RuntimeScriptSandboxEntrypointKind;
    using PermissionKind = mirakana::runtime::RuntimeScriptSandboxPermissionKind;
    using Status = mirakana::runtime::RuntimeScriptSandboxPlanStatus;

    const auto reviewed_policy =
        mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
            .modules =
                std::vector<mirakana::runtime::RuntimeScriptSandboxModuleDesc>{
                    mirakana::runtime::RuntimeScriptSandboxModuleDesc{
                        .module_id = "gameplay",
                        .source_uri = "scripts/gameplay.reviewed",
                        .source_index = 1U,
                        .entrypoints =
                            std::vector<mirakana::runtime::RuntimeScriptSandboxEntrypointDesc>{
                                mirakana::runtime::RuntimeScriptSandboxEntrypointDesc{
                                    .entrypoint_id = "tick",
                                    .kind = EntryKind::update,
                                    .instruction_budget = 1200U,
                                    .memory_budget_bytes = 4096U,
                                    .replay_seed = 1001U,
                                    .permissions =
                                        std::vector<mirakana::runtime::RuntimeScriptSandboxPermissionDesc>{
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::read_runtime_state, .source_index = 1U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::write_runtime_state, .source_index = 2U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::host_api,
                                                .host_api_id = "sample2d.quest",
                                                .source_index = 3U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::emit_diagnostic, .source_index = 4U},
                                        },
                                    .source_index = 10U,
                                },
                            },
                    },
                    mirakana::runtime::RuntimeScriptSandboxModuleDesc{
                        .module_id = "ui",
                        .source_uri = "scripts/ui.reviewed",
                        .source_index = 2U,
                        .entrypoints =
                            std::vector<mirakana::runtime::RuntimeScriptSandboxEntrypointDesc>{
                                mirakana::runtime::RuntimeScriptSandboxEntrypointDesc{
                                    .entrypoint_id = "render_hud",
                                    .kind = EntryKind::event,
                                    .instruction_budget = 600U,
                                    .memory_budget_bytes = 2048U,
                                    .replay_seed = 2002U,
                                    .permissions =
                                        std::vector<mirakana::runtime::RuntimeScriptSandboxPermissionDesc>{
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::read_runtime_state, .source_index = 1U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::host_api,
                                                .host_api_id = "sample2d.hud",
                                                .source_index = 2U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::emit_diagnostic, .source_index = 3U},
                                        },
                                    .source_index = 20U,
                                },
                            },
                    },
                },
            .allowed_host_apis = {"sample2d.hud", "sample2d.quest"},
            .max_instruction_budget_per_entrypoint = 2000U,
            .max_memory_budget_bytes_per_entrypoint = 8192U,
        };

    ScriptingSandboxProbeResult result;
    const auto reviewed_plan = mirakana::runtime::plan_runtime_script_sandbox(reviewed_policy);
    result.status = reviewed_plan.status;
    result.entrypoint_rows = reviewed_plan.entrypoints.size();
    result.permission_rows = reviewed_plan.permissions.size();
    result.projected_instruction_budget = reviewed_plan.projected_instruction_budget;
    result.projected_memory_budget_bytes = reviewed_plan.projected_memory_budget_bytes;
    result.diagnostics = reviewed_plan.diagnostics.size();
    for (const auto& row : reviewed_plan.entrypoints) {
        result.allowed_permission_rows += row.allowed_permission_count;
        if (row.instruction_budget > 0U && row.memory_budget_bytes > 0U) {
            ++result.budget_rows;
        }
        if (row.replay_seed != 0U) {
            ++result.replay_seed_rows;
            result.replay_seed_sum += row.replay_seed;
        }
    }

    SampleScriptingSandboxAdapter adapter;
    const auto execution_result = mirakana::runtime::execute_runtime_script_entrypoint(
        mirakana::runtime::RuntimeScriptExecutionRequest{
            .plan = &reviewed_plan,
            .module_id = "gameplay",
            .entrypoint_id = "tick",
            .input_event_id = "frame.0001",
            .instruction_budget = 120U,
            .memory_budget_bytes = 2048U,
            .replay_seed = 1001U,
        },
        adapter);
    result.execution_status = execution_result.status;
    result.execution_dispatches = execution_result.dispatched ? 1U : 0U;
    result.execution_host_api_calls = execution_result.stats.host_api_call_count;
    result.execution_replay_signature = execution_result.replay_signature;
    result.execution_diagnostics = execution_result.diagnostics.size();
    result.execution_ready = execution_result.succeeded() && execution_result.dispatched &&
                             result.execution_host_api_calls == 1U && result.execution_replay_signature != 0U &&
                             result.execution_diagnostics == 0U;

    const auto unsafe_policy =
        mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
            .modules =
                std::vector<mirakana::runtime::RuntimeScriptSandboxModuleDesc>{
                    mirakana::runtime::RuntimeScriptSandboxModuleDesc{
                        .module_id = "unsafe",
                        .source_uri = "scripts/unsafe.reviewed",
                        .source_index = 3U,
                        .entrypoints =
                            std::vector<mirakana::runtime::RuntimeScriptSandboxEntrypointDesc>{
                                mirakana::runtime::RuntimeScriptSandboxEntrypointDesc{
                                    .entrypoint_id = "tick",
                                    .kind = EntryKind::update,
                                    .instruction_budget = 100U,
                                    .memory_budget_bytes = 512U,
                                    .replay_seed = 3003U,
                                    .permissions =
                                        std::vector<mirakana::runtime::RuntimeScriptSandboxPermissionDesc>{
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::filesystem_read, .source_index = 1U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::filesystem_write, .source_index = 2U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::network, .source_index = 3U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::process, .source_index = 4U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::native_plugin, .source_index = 5U},
                                            mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                                .kind = PermissionKind::host_api,
                                                .host_api_id = "sample2d.unreviewed",
                                                .source_index = 6U},
                                        },
                                    .source_index = 30U,
                                },
                            },
                    },
                },
            .allowed_host_apis = {"sample2d.quest"},
            .max_instruction_budget_per_entrypoint = 2000U,
            .max_memory_budget_bytes_per_entrypoint = 8192U,
        };
    const auto unsafe_plan = mirakana::runtime::plan_runtime_script_sandbox(unsafe_policy);
    result.denied_permission_rows = count_scripting_sandbox_diagnostics(unsafe_plan, Code::default_denied_permission);
    result.unsupported_host_api_diagnostics =
        count_scripting_sandbox_diagnostics(unsafe_plan, Code::unsupported_host_api_id);
    result.rejected_unsafe_capability_rows = result.denied_permission_rows + result.unsupported_host_api_diagnostics;

    const auto budget_policy = mirakana::runtime::RuntimeScriptSandboxPolicyDesc{
        .modules =
            std::vector<mirakana::runtime::RuntimeScriptSandboxModuleDesc>{
                mirakana::runtime::RuntimeScriptSandboxModuleDesc{
                    .module_id = "budget",
                    .source_uri = "scripts/budget.reviewed",
                    .source_index = 4U,
                    .entrypoints =
                        std::vector<mirakana::runtime::RuntimeScriptSandboxEntrypointDesc>{
                            mirakana::runtime::RuntimeScriptSandboxEntrypointDesc{
                                .entrypoint_id = "tick",
                                .kind = EntryKind::update,
                                .instruction_budget = 2001U,
                                .memory_budget_bytes = 8193U,
                                .replay_seed = 4004U,
                                .permissions =
                                    std::vector<mirakana::runtime::RuntimeScriptSandboxPermissionDesc>{
                                        mirakana::runtime::RuntimeScriptSandboxPermissionDesc{
                                            .kind = PermissionKind::emit_diagnostic, .source_index = 1U},
                                    },
                                .source_index = 40U,
                            },
                        },
                },
            },
        .allowed_host_apis = {},
        .max_instruction_budget_per_entrypoint = 2000U,
        .max_memory_budget_bytes_per_entrypoint = 8192U,
    };
    const auto budget_plan = mirakana::runtime::plan_runtime_script_sandbox(budget_policy);
    result.budget_diagnostics = count_scripting_sandbox_diagnostics(budget_plan, Code::instruction_budget_exceeded) +
                                count_scripting_sandbox_diagnostics(budget_plan, Code::memory_budget_exceeded);

    result.ready = reviewed_plan.status == Status::planned && reviewed_plan.succeeded() &&
                   result.entrypoint_rows == 2U && result.permission_rows == 7U &&
                   result.allowed_permission_rows == 7U && result.denied_permission_rows == 5U &&
                   result.rejected_unsafe_capability_rows == 6U && result.unsupported_host_api_diagnostics == 1U &&
                   result.budget_rows == 2U && result.projected_instruction_budget == 1800U &&
                   result.projected_memory_budget_bytes == 6144U && result.budget_diagnostics == 2U &&
                   result.replay_seed_rows == 2U && result.replay_seed_sum == 3003U && result.diagnostics == 0U &&
                   result.execution_ready && unsafe_plan.status == Status::invalid_request &&
                   budget_plan.status == Status::budget_exceeded;
    return result;
}

[[nodiscard]] std::size_t
count_networking_foundation_diagnostics(const mirakana::runtime::RuntimeNetworkFoundationPlan& plan,
                                        mirakana::runtime::RuntimeNetworkDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] NetworkingFoundationProbeResult validate_networking_foundation_package_evidence() {
    using Authority = mirakana::runtime::RuntimeNetworkReplicationAuthority;
    using Capability = mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
    using Code = mirakana::runtime::RuntimeNetworkDiagnosticCode;
    using Delivery = mirakana::runtime::RuntimeNetworkReplicationDelivery;
    using Role = mirakana::runtime::RuntimeNetworkLocalRole;
    using Status = mirakana::runtime::RuntimeNetworkFoundationPlanStatus;
    using Topology = mirakana::runtime::RuntimeNetworkSessionTopology;
    using Trust = mirakana::runtime::RuntimeNetworkTrustBoundary;

    const auto reviewed_policy =
        mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
            .sessions =
                std::vector<mirakana::runtime::RuntimeNetworkSessionDesc>{
                    mirakana::runtime::RuntimeNetworkSessionDesc{
                        .session_id = "arena",
                        .topology = Topology::listen_server,
                        .local_role = Role::host,
                        .trust_boundary = Trust::untrusted_remote_peers,
                        .transports =
                            std::vector<mirakana::runtime::RuntimeNetworkTransportRequirementDesc>{
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::authenticated_peer, .source_index = 1U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::encrypted_transport, .source_index = 2U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::reliable_ordered, .source_index = 3U},
                            },
                        .channels =
                            std::vector<mirakana::runtime::RuntimeNetworkReplicationChannelDesc>{
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "input",
                                    .authority = Authority::client,
                                    .delivery = Delivery::unreliable_unordered,
                                    .tick_rate_hz = 60U,
                                    .source_index = 6U,
                                },
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "state",
                                    .authority = Authority::server,
                                    .delivery = Delivery::state_snapshot,
                                    .tick_rate_hz = 30U,
                                    .source_index = 7U,
                                },
                            },
                        .replay =
                            mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                .replay_seed = 42U,
                                .fixed_tick_rate_hz = 60U,
                                .deterministic_simulation = true,
                                .ordered_inputs = true,
                                .source_index = 9U,
                            },
                        .source_index = 4U,
                    },
                    mirakana::runtime::RuntimeNetworkSessionDesc{
                        .session_id = "local",
                        .topology = Topology::local_only,
                        .local_role = Role::offline,
                        .trust_boundary = Trust::trusted_local,
                        .transports =
                            std::vector<mirakana::runtime::RuntimeNetworkTransportRequirementDesc>{
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::reliable_ordered, .source_index = 12U},
                            },
                        .channels =
                            std::vector<mirakana::runtime::RuntimeNetworkReplicationChannelDesc>{
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "solo_state",
                                    .authority = Authority::host,
                                    .delivery = Delivery::reliable_ordered,
                                    .tick_rate_hz = 30U,
                                    .source_index = 13U,
                                },
                            },
                        .replay =
                            mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                .replay_seed = 7U,
                                .fixed_tick_rate_hz = 30U,
                                .deterministic_simulation = true,
                                .ordered_inputs = true,
                                .source_index = 14U,
                            },
                        .source_index = 10U,
                    },
                },
            .reviewed_transport_capabilities =
                {
                    Capability::authenticated_peer,
                    Capability::encrypted_transport,
                    Capability::reliable_ordered,
                    Capability::unreliable_unordered,
                },
        };

    NetworkingFoundationProbeResult result;
    const auto reviewed_plan = mirakana::runtime::plan_runtime_network_foundation(reviewed_policy);
    result.status = reviewed_plan.status;
    result.session_rows = reviewed_plan.sessions.size();
    result.transport_rows = reviewed_plan.transports.size();
    result.channel_rows = reviewed_plan.channels.size();
    result.replay_prerequisite_rows = reviewed_plan.replay_prerequisites.size();
    result.remote_session_rows = reviewed_plan.remote_session_count;
    result.secure_remote_session_rows = reviewed_plan.secure_remote_session_count;
    result.diagnostics = reviewed_plan.diagnostics.size();
    for (const auto& replay : reviewed_plan.replay_prerequisites) {
        result.replay_seed_sum += replay.replay_seed;
    }

    const auto unsafe_policy =
        mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
            .sessions =
                std::vector<mirakana::runtime::RuntimeNetworkSessionDesc>{
                    mirakana::runtime::RuntimeNetworkSessionDesc{
                        .session_id = "unsafe_remote",
                        .topology = Topology::listen_server,
                        .local_role = Role::host,
                        .trust_boundary = Trust::untrusted_remote_peers,
                        .transports =
                            std::vector<mirakana::runtime::RuntimeNetworkTransportRequirementDesc>{
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::reliable_ordered, .source_index = 1U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::nat_traversal, .source_index = 2U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::platform_socket, .source_index = 3U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::native_transport_handle, .source_index = 4U},
                            },
                        .channels =
                            std::vector<mirakana::runtime::RuntimeNetworkReplicationChannelDesc>{
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "state",
                                    .authority = Authority::server,
                                    .delivery = Delivery::state_snapshot,
                                    .tick_rate_hz = 30U,
                                    .source_index = 5U,
                                },
                            },
                        .replay =
                            mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                .replay_seed = 90U,
                                .fixed_tick_rate_hz = 30U,
                                .deterministic_simulation = true,
                                .ordered_inputs = true,
                                .source_index = 6U,
                            },
                        .source_index = 7U,
                    },
                },
            .reviewed_transport_capabilities = {Capability::reliable_ordered},
        };
    const auto unsafe_plan = mirakana::runtime::plan_runtime_network_foundation(unsafe_policy);
    result.rejected_unsafe_transport_rows =
        count_networking_foundation_diagnostics(unsafe_plan, Code::unsupported_transport_capability) +
        count_networking_foundation_diagnostics(unsafe_plan, Code::native_transport_capability_requested);
    result.security_diagnostics = count_networking_foundation_diagnostics(unsafe_plan, Code::insecure_remote_transport);

    result.ready = reviewed_plan.status == Status::planned && reviewed_plan.succeeded() && result.session_rows == 2U &&
                   result.transport_rows == 4U && result.channel_rows == 3U &&
                   result.rejected_unsafe_transport_rows == 3U && result.replay_prerequisite_rows == 2U &&
                   result.replay_seed_sum == 49U && result.remote_session_rows == 1U &&
                   result.secure_remote_session_rows == 1U && result.security_diagnostics == 2U &&
                   result.diagnostics == 0U && unsafe_plan.status == Status::invalid_request;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkFoundationPlan make_network_replication_foundation_plan() {
    using Authority = mirakana::runtime::RuntimeNetworkReplicationAuthority;
    using Capability = mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
    using Delivery = mirakana::runtime::RuntimeNetworkReplicationDelivery;
    using Role = mirakana::runtime::RuntimeNetworkLocalRole;
    using Topology = mirakana::runtime::RuntimeNetworkSessionTopology;
    using Trust = mirakana::runtime::RuntimeNetworkTrustBoundary;

    return mirakana::runtime::plan_runtime_network_foundation(
        mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
            .sessions =
                std::vector<mirakana::runtime::RuntimeNetworkSessionDesc>{
                    mirakana::runtime::RuntimeNetworkSessionDesc{
                        .session_id = "arena",
                        .topology = Topology::listen_server,
                        .local_role = Role::host,
                        .trust_boundary = Trust::untrusted_remote_peers,
                        .transports =
                            std::vector<mirakana::runtime::RuntimeNetworkTransportRequirementDesc>{
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::reliable_ordered, .source_index = 1U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::unreliable_unordered, .source_index = 2U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::encrypted_transport, .source_index = 3U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::authenticated_peer, .source_index = 4U},
                            },
                        .channels =
                            std::vector<mirakana::runtime::RuntimeNetworkReplicationChannelDesc>{
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "state",
                                    .authority = Authority::server,
                                    .delivery = Delivery::state_snapshot,
                                    .tick_rate_hz = 60U,
                                    .source_index = 5U,
                                },
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "input",
                                    .authority = Authority::client,
                                    .delivery = Delivery::unreliable_unordered,
                                    .tick_rate_hz = 60U,
                                    .source_index = 6U,
                                },
                            },
                        .replay =
                            mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                .replay_seed = 42U,
                                .fixed_tick_rate_hz = 60U,
                                .deterministic_simulation = true,
                                .ordered_inputs = true,
                                .source_index = 7U,
                            },
                        .source_index = 8U,
                    },
                },
            .reviewed_transport_capabilities =
                {
                    Capability::reliable_ordered,
                    Capability::unreliable_unordered,
                    Capability::encrypted_transport,
                    Capability::authenticated_peer,
                },
        });
}

[[nodiscard]] NetworkReplicationProbeResult validate_network_replication_package_evidence(std::string_view sample_id) {
    using Mode = mirakana::runtime::RuntimeNetworkReplicationMode;
    using Ownership = mirakana::runtime::RuntimeReplicationOwnership;
    using Request = mirakana::runtime::RuntimeNetworkReplicationRequest;
    using RollbackMode = mirakana::runtime::RuntimeRollbackMode;
    using SnapshotKind = mirakana::runtime::RuntimeReplicationSnapshotKind;
    using Status = mirakana::runtime::RuntimeNetworkReplicationStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan =
        mirakana::runtime::plan_runtime_network_replication(
            Request{.foundation_plan = make_network_replication_foundation_plan(),
                    .session =
                        mirakana::runtime::RuntimeNetworkReplicationSessionDesc{
                            .session_id = "arena",
                            .world_id = sample_prefix + ".network.world",
                            .mode = Mode::authoritative_snapshot,
                            .fixed_tick_rate_hz = 60U,
                            .max_players = 4U,
                            .max_objects = 16U,
                            .source_index = 1U,
                        },
                    .object_rows =
                        std::vector<mirakana::runtime::RuntimeReplicatedObjectRow>{
                            mirakana::runtime::RuntimeReplicatedObjectRow{
                                .object_id = "player.0",
                                .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "entity.player0"},
                                .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "region.arena"},
                                .schema_id =
                                    mirakana::runtime::RuntimeWorldComponentSchemaId{.value = "schema.transform"},
                                .channel_id = "state",
                                .ownership = Ownership::server_owned,
                                .priority = 10U,
                                .source_index = 1U,
                            },
                            mirakana::runtime::RuntimeReplicatedObjectRow{
                                .object_id = "crate.0",
                                .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "entity.crate0"},
                                .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "region.arena"},
                                .schema_id =
                                    mirakana::runtime::RuntimeWorldComponentSchemaId{.value = "schema.transform"},
                                .channel_id = "state",
                                .ownership = Ownership::server_owned,
                                .priority = 8U,
                                .source_index = 2U,
                            },
                        },
                    .input_rows =
                        std::vector<mirakana::runtime::RuntimeReplicationInputCommandRow>{
                            mirakana::runtime::RuntimeReplicationInputCommandRow{
                                .player_id = "player.0",
                                .command_id = "move.left",
                                .channel_id = "input",
                                .target_tick = 100U,
                                .sequence = 1U,
                                .payload_hash = 1001U,
                                .source_index = 1U,
                            },
                            mirakana::runtime::RuntimeReplicationInputCommandRow{
                                .player_id = "player.0",
                                .command_id = "move.right",
                                .channel_id = "input",
                                .target_tick = 101U,
                                .sequence = 2U,
                                .payload_hash = 1002U,
                                .source_index = 2U,
                            },
                        },
                    .snapshot_rows =
                        std::vector<mirakana::runtime::RuntimeReplicationSnapshotRow>{
                            mirakana::runtime::RuntimeReplicationSnapshotRow{
                                .snapshot_id = "snapshot.100",
                                .channel_id = "state",
                                .tick = 100U,
                                .kind = SnapshotKind::full_state,
                                .object_ids = {"player.0", "crate.0"},
                                .state_hash = 9001U,
                                .byte_count = 256U,
                                .source_index = 1U,
                            },
                            mirakana::runtime::RuntimeReplicationSnapshotRow{
                                .snapshot_id = "snapshot.101",
                                .channel_id = "state",
                                .tick = 101U,
                                .kind = SnapshotKind::delta_state,
                                .object_ids = {"player.0", "crate.0"},
                                .state_hash = 9002U,
                                .byte_count = 128U,
                                .source_index = 2U,
                            },
                        },
                    .rollback_policy =
                        mirakana::runtime::RuntimeRollbackPolicyRow{
                            .mode = RollbackMode::input_resimulation,
                            .max_rollback_ticks = 8U,
                            .input_delay_ticks = 2U,
                            .snapshot_history_limit = 16U,
                            .requires_deterministic_simulation = true,
                            .requires_ordered_inputs = true,
                            .requires_transport_host_evidence = true,
                            .source_index = 20U,
                        },
                    .row_budget = 32U,
                    .snapshot_byte_budget = 1024U,
                    .seed = 99U});

    NetworkReplicationProbeResult result;
    result.status = plan.status;
    result.object_rows = plan.replicated_object_count;
    result.input_rows = plan.input_row_count;
    result.snapshot_rows = plan.snapshot_row_count;
    result.rollback_rows = plan.rollback_row_count;
    result.rejected_unsafe_rows = plan.rejected_unsafe_row_count;
    result.replay_hash = plan.replay_hash;
    result.requires_transport_host_evidence = plan.requires_transport_host_evidence;
    result.has_transport_host_evidence = plan.has_transport_host_evidence;
    result.invoked_network_io = plan.invoked_network_io;
    result.invoked_rollback_execution = plan.invoked_rollback_execution;
    result.invoked_world_mutation = plan.invoked_world_mutation;
    result.diagnostics = plan.diagnostics.size();
    result.reviewed = plan.status == Status::host_evidence_required && result.object_rows == 2U &&
                      result.input_rows == 2U && result.snapshot_rows == 2U && result.rollback_rows == 1U &&
                      result.rejected_unsafe_rows == 0U && result.replay_hash != 0U &&
                      result.requires_transport_host_evidence && !result.has_transport_host_evidence &&
                      !result.invoked_network_io && !result.invoked_rollback_execution &&
                      !result.invoked_world_mutation && result.diagnostics == 0U;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.object_rows == 2U &&
                   result.input_rows == 2U && result.snapshot_rows == 2U && result.rollback_rows == 1U &&
                   result.rejected_unsafe_rows == 0U && result.replay_hash != 0U &&
                   result.requires_transport_host_evidence && result.has_transport_host_evidence &&
                   !result.invoked_network_io && !result.invoked_rollback_execution && !result.invoked_world_mutation &&
                   result.diagnostics == 0U;
    return result;
}

[[nodiscard]] NetworkProductionSecurityProbeResult validate_network_production_security_package_evidence() {
    using Kind = mirakana::runtime::RuntimeNetworkProductionValidationKind;
    using SecurityRequest = mirakana::runtime::RuntimeNetworkProductionSecurityRequest;
    using SecurityStatus = mirakana::runtime::RuntimeNetworkProductionSecurityStatus;

    const auto plan =
        mirakana::runtime::plan_runtime_network_production_security_gate(
            SecurityRequest{
                .threat_model =
                    mirakana::runtime::RuntimeNetworkProductionThreatModelEvidenceRow{
                        .evidence_id = "docs/specs/2026-05-26-networking-production-security-threat-model.md",
                        .attacker_capabilities_reviewed = true,
                        .trust_boundaries_reviewed = true,
                        .packet_tampering_reviewed = true,
                        .packet_replay_reviewed = true,
                        .authentication_gap_reviewed = true,
                        .denial_of_service_reviewed = true,
                        .nat_matchmaking_exclusions_reviewed = true,
                        .save_rollback_abuse_reviewed = true,
                        .official_sources_reviewed = true,
                        .source_index = 1U,
                    },
                .foundation_plan = make_network_replication_foundation_plan(),
                .replication_plan =
                    mirakana::runtime::RuntimeNetworkReplicationPlan{
                        .status = mirakana::runtime::RuntimeNetworkReplicationStatus::host_evidence_required,
                        .diagnostics = {},
                        .session_rows =
                            {
                                mirakana::runtime::RuntimeNetworkReplicationSessionDesc{
                                    .session_id = "arena",
                                    .world_id = "sample2d.network.world",
                                    .mode = mirakana::runtime::RuntimeNetworkReplicationMode::authoritative_snapshot,
                                    .fixed_tick_rate_hz = 60U,
                                    .max_players = 4U,
                                    .max_objects = 16U,
                                    .source_index = 2U,
                                },
                            },
                        .object_rows = {},
                        .input_rows = {},
                        .snapshot_rows = {},
                        .rollback_rows = {},
                        .transport_evidence_rows = {},
                        .replicated_object_count = 2U,
                        .input_row_count = 2U,
                        .snapshot_row_count = 2U,
                        .rollback_row_count = 1U,
                        .rejected_unsafe_row_count = 0U,
                        .replay_hash = 99U,
                        .requires_transport_host_evidence = true,
                        .has_transport_host_evidence = false,
                        .invoked_network_io = false,
                        .invoked_rollback_execution = false,
                        .invoked_world_mutation = false,
                    },
                .loopback_exchange = mirakana::runtime::RuntimeNetworkLoopbackExchangeResult{},
                .validation_evidence_rows =
                    {
                        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                            .kind = Kind::sequence_replay_rejection,
                            .evidence_id = "sequence-replay",
                            .reviewed = true,
                            .source_index = 3U,
                        },
                        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                            .kind = Kind::input_command_validation,
                            .evidence_id = "input-command",
                            .reviewed = true,
                            .source_index = 4U,
                        },
                        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                            .kind = Kind::snapshot_validation,
                            .evidence_id = "snapshot-validation",
                            .reviewed = true,
                            .source_index = 5U,
                        },
                        mirakana::runtime::RuntimeNetworkProductionValidationEvidenceRow{
                            .kind = Kind::rollback_window_diagnostic,
                            .evidence_id = "rollback-window",
                            .reviewed = true,
                            .source_index = 6U,
                        },
                    },
                .unsupported_online_claim_rows = {},
                .row_budget = 64U,
                .request_native_handles = false,
                .invoked_external_network_io = false,
                .invoked_threads = false,
                .invoked_save_io = false,
                .invoked_world_mutation = false,
                .seed = 123U});

    NetworkProductionSecurityProbeResult result;
    result.status = plan.status;
    result.session_lifecycle_rows = plan.session_lifecycle_rows;
    result.connection_state_rows = plan.connection_state_rows;
    result.channel_policy_rows = plan.channel_policy_rows;
    result.reliable_delivery_rows = plan.reliable_delivery_rows;
    result.unreliable_delivery_rows = plan.unreliable_delivery_rows;
    result.sequence_replay_rejection_rows = plan.sequence_replay_rejection_rows;
    result.input_command_validation_rows = plan.input_command_validation_rows;
    result.snapshot_validation_rows = plan.snapshot_validation_rows;
    result.rollback_window_diagnostic_rows = plan.rollback_window_diagnostic_rows;
    result.unsupported_online_claim_rows = plan.unsupported_online_claim_rows;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.threat_model_reviewed = plan.threat_model_reviewed;
    result.loopback_host_evidence = plan.loopback_host_evidence;
    result.replication_evidence_ready = plan.replication_evidence_ready;
    result.general_online_ready = plan.general_online_ready;
    result.invoked_external_network_io = plan.invoked_external_network_io;
    result.invoked_threads = plan.invoked_threads;
    result.invoked_save_io = plan.invoked_save_io;
    result.invoked_world_mutation = plan.invoked_world_mutation;
    result.reviewed = plan.status == SecurityStatus::host_evidence_required && result.threat_model_reviewed &&
                      !result.loopback_host_evidence && !result.replication_evidence_ready &&
                      !result.general_online_ready && result.sequence_replay_rejection_rows == 1U &&
                      result.input_command_validation_rows == 1U && result.snapshot_validation_rows == 1U &&
                      result.rollback_window_diagnostic_rows == 1U && result.unsupported_online_claim_rows == 0U &&
                      result.diagnostics == 2U && result.replay_hash != 0U && !result.invoked_external_network_io &&
                      !result.invoked_threads && !result.invoked_save_io && !result.invoked_world_mutation;
    result.ready = plan.status == SecurityStatus::ready && plan.succeeded() && result.loopback_host_evidence &&
                   result.replication_evidence_ready && !result.general_online_ready && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::size_t
count_simulation_orchestration_diagnostics(const mirakana::runtime::RuntimeSimulationOrchestrationPlan& plan,
                                           mirakana::runtime::RuntimeSimulationOrchestrationDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] GameplayRuntimeSchedulerProbeResult validate_gameplay_runtime_scheduler_package_evidence() {
    using Command = mirakana::runtime::RuntimeGameplaySchedulerInputCommandDesc;
    using Mode = mirakana::runtime::RuntimeGameplaySchedulerMode;
    using Request = mirakana::runtime::RuntimeGameplaySchedulerRequest;
    using Status = mirakana::runtime::RuntimeGameplaySchedulerStatus;
    using System = mirakana::runtime::RuntimeGameplaySchedulerSystemDesc;

    const auto plan = mirakana::runtime::plan_runtime_gameplay_schedule(Request{
        .scheduler_id = "sample2d.gameplay",
        .next_tick = 42U,
        .tick_delta_us = 16'666U,
        .accumulated_time_us = 49'998U,
        .max_steps_per_frame = 2U,
        .mode = Mode::run,
        .systems =
            std::vector<System>{
                System{.system_id = "physics", .order = 20, .enabled = true, .budget_us = 400U, .source_index = 2U},
                System{.system_id = "input", .order = 10, .enabled = true, .budget_us = 100U, .source_index = 1U},
                System{.system_id = "ai", .order = 30, .enabled = true, .budget_us = 300U, .source_index = 3U},
            },
        .input_commands =
            std::vector<Command>{
                Command{.command_id = "cmd.move", .target_tick = 42U, .payload_hash = 1001U, .source_index = 1U},
                Command{.command_id = "cmd.interact", .target_tick = 43U, .payload_hash = 1002U, .source_index = 2U},
            },
    });

    GameplayRuntimeSchedulerProbeResult result;
    result.status = plan.status;
    result.available_steps = plan.available_step_count;
    result.planned_steps = plan.planned_step_count;
    result.step_rows = plan.steps.size();
    result.system_rows = plan.total_system_rows;
    result.command_rows = plan.total_command_rows;
    result.consumed_time_us = plan.consumed_time_us;
    result.remaining_time_us = plan.remaining_time_us;
    result.budget_limited = plan.status == Status::budget_limited;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.ready = plan.succeeded() && result.budget_limited && result.available_steps == 3U &&
                   result.planned_steps == 2U && result.step_rows == 2U && result.system_rows == 6U &&
                   result.command_rows == 2U && result.consumed_time_us == 33'332U &&
                   result.remaining_time_us == 16'666U && result.diagnostics == 0U && result.replay_hash != 0U;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeWorldEntityId make_rpg_entity(std::string value) {
    return mirakana::runtime::RuntimeWorldEntityId{.value = std::move(value)};
}

[[nodiscard]] RpgSystemsProbeResult validate_rpg_systems_package_evidence(std::string_view sample_id) {
    using Combat = mirakana::runtime::RuntimeRpgCombatLoopRequest;
    using Equipment = mirakana::runtime::RuntimeRpgEquipmentRow;
    using EquipmentStatus = mirakana::runtime::RuntimeRpgEquipmentStatus;
    using Progression = mirakana::runtime::RuntimeRpgProgressionRow;
    using Request = mirakana::runtime::RuntimeRpgSystemsRequest;
    using Reward = mirakana::runtime::RuntimeRpgRewardRow;
    using RewardKind = mirakana::runtime::RuntimeRpgRewardKind;
    using Save = mirakana::runtime::RuntimeRpgSaveValidationRow;
    using SaveStatus = mirakana::runtime::RuntimeRpgSaveValidationStatus;
    using Skill = mirakana::runtime::RuntimeRpgSkillRow;
    using SkillStatus = mirakana::runtime::RuntimeRpgSkillStatus;
    using Stat = mirakana::runtime::RuntimeRpgStatRow;
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan =
        mirakana::runtime::plan_runtime_rpg_systems(
            Request{
                .system_id = sample_prefix + ".rpg",
                .world_id = sample_prefix + ".world",
                .world_tick = 42U,
                .party_entity_ids = {make_rpg_entity("party.entity.0"), make_rpg_entity("party.entity.1")},
                .enemy_entity_ids = {make_rpg_entity("opponent.entity.0")},
                .stat_rows =
                    std::vector<Stat>{
                        Stat{.entity_id = make_rpg_entity("party.entity.0"),
                             .stat_id = "health",
                             .current_value = 30,
                             .max_value = 30,
                             .source_index = 1U},
                        Stat{.entity_id = make_rpg_entity("party.entity.0"),
                             .stat_id = "focus",
                             .current_value = 7,
                             .max_value = 10,
                             .source_index = 2U},
                        Stat{.entity_id = make_rpg_entity("party.entity.0"),
                             .stat_id = "initiative",
                             .current_value = 12,
                             .max_value = 12,
                             .source_index = 3U},
                        Stat{.entity_id = make_rpg_entity("party.entity.1"),
                             .stat_id = "health",
                             .current_value = 24,
                             .max_value = 24,
                             .source_index = 4U},
                        Stat{.entity_id = make_rpg_entity("party.entity.1"),
                             .stat_id = "focus",
                             .current_value = 2,
                             .max_value = 10,
                             .source_index = 5U},
                        Stat{.entity_id = make_rpg_entity("party.entity.1"),
                             .stat_id = "initiative",
                             .current_value = 8,
                             .max_value = 8,
                             .source_index = 6U},
                        Stat{.entity_id = make_rpg_entity("opponent.entity.0"),
                             .stat_id = "health",
                             .current_value = 18,
                             .max_value = 18,
                             .source_index = 7U},
                        Stat{.entity_id = make_rpg_entity("opponent.entity.0"),
                             .stat_id = "initiative",
                             .current_value = 10,
                             .max_value = 10,
                             .source_index = 8U},
                    },
                .progression_rows =
                    std::vector<Progression>{
                        Progression{.entity_id = make_rpg_entity("party.entity.0"),
                                    .track_id = "track.core",
                                    .level = 3U,
                                    .experience = 120U,
                                    .skill_points = 1U,
                                    .source_index = 1U},
                        Progression{.entity_id = make_rpg_entity("party.entity.1"),
                                    .track_id = "track.core",
                                    .level = 1U,
                                    .experience = 25U,
                                    .skill_points = 0U,
                                    .source_index = 2U},
                    },
                .skill_rows =
                    std::vector<Skill>{
                        Skill{.entity_id = make_rpg_entity("party.entity.0"),
                              .skill_id = "skill.primary",
                              .required_level = 2U,
                              .required_stat_id = "focus",
                              .required_stat_value = 6,
                              .status = SkillStatus::invalid,
                              .source_index = 1U},
                        Skill{.entity_id = make_rpg_entity("party.entity.1"),
                              .skill_id = "skill.support",
                              .required_level = 1U,
                              .required_stat_id = "focus",
                              .required_stat_value = 6,
                              .status = SkillStatus::invalid,
                              .source_index = 2U},
                    },
                .equipment_rows =
                    std::vector<Equipment>{
                        Equipment{.entity_id = make_rpg_entity("party.entity.0"),
                                  .slot_id = "slot.primary",
                                  .item_id = "item.primary",
                                  .required_level = 2U,
                                  .required_stat_id = "focus",
                                  .required_stat_value = 5,
                                  .status = EquipmentStatus::invalid,
                                  .source_index = 1U},
                        Equipment{.entity_id = make_rpg_entity("party.entity.1"),
                                  .slot_id = "slot.support",
                                  .item_id = "item.support",
                                  .required_level = 1U,
                                  .required_stat_id = "focus",
                                  .required_stat_value = 5,
                                  .status = EquipmentStatus::invalid,
                                  .source_index = 2U},
                    },
                .combat_request =
                    Combat{.encounter_id = "encounter.package", .initiative_stat_id = "initiative", .max_rounds = 2U},
                .reward_rows =
                    std::vector<Reward>{
                        Reward{.reward_id = "reward.progress",
                               .entity_id = make_rpg_entity("party.entity.0"),
                               .kind = RewardKind::experience,
                               .item_id = "",
                               .quantity = 50U,
                               .source_index = 1U},
                        Reward{.reward_id = "reward.item",
                               .entity_id = make_rpg_entity("party.entity.1"),
                               .kind = RewardKind::item,
                               .item_id = "item.resource",
                               .quantity = 1U,
                               .source_index = 2U},
                    },
                .save_validation_rows =
                    std::vector<Save>{
                        Save{.entity_id = make_rpg_entity("party.entity.0"),
                             .domain = "profile",
                             .key = "state.party.0",
                             .expected_schema_version = 1U,
                             .observed_schema_version = 1U,
                             .status = SaveStatus::rejected,
                             .source_index = 1U},
                        Save{.entity_id = make_rpg_entity("party.entity.1"),
                             .domain = "profile",
                             .key = "state.party.1",
                             .expected_schema_version = 2U,
                             .observed_schema_version = 1U,
                             .status = SaveStatus::rejected,
                             .source_index = 2U},
                    },
            });

    RpgSystemsProbeResult result;
    result.status = plan.status;
    result.party_members = plan.party_member_count;
    result.enemy_members = plan.enemy_member_count;
    result.stat_rows = plan.stat_count;
    result.progression_rows = plan.progression_count;
    result.skill_rows = plan.skill_rows.size();
    result.blocked_skill_rows = plan.blocked_skill_count;
    result.equipment_rows = plan.equipment_rows.size();
    result.blocked_equipment_rows = plan.blocked_equipment_count;
    result.combat_turn_rows = plan.combat_turn_count;
    result.combat_rounds = plan.combat_round_count;
    result.reward_rows = plan.reward_count;
    result.save_validation_rows = plan.save_validation_count;
    result.repairable_save_validation_rows = plan.repairable_save_validation_count;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.invoked_combat_execution = plan.invoked_combat_execution;
    result.invoked_reward_application = plan.invoked_reward_application;
    result.invoked_save_io = plan.invoked_save_io;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.party_members == 2U &&
                   result.enemy_members == 1U && result.stat_rows == 8U && result.progression_rows == 2U &&
                   result.skill_rows == 2U && result.blocked_skill_rows == 1U && result.equipment_rows == 2U &&
                   result.blocked_equipment_rows == 1U && result.combat_turn_rows == 6U && result.combat_rounds == 2U &&
                   result.reward_rows == 2U && result.save_validation_rows == 2U &&
                   result.repairable_save_validation_rows == 1U && result.diagnostics == 0U &&
                   result.replay_hash != 0U && !result.invoked_combat_execution && !result.invoked_reward_application &&
                   !result.invoked_save_io;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxCellCoord make_sandbox_cell(std::int32_t x, std::int32_t y,
                                                                           std::int32_t z) {
    return mirakana::runtime::RuntimeSandboxCellCoord{.x = x, .y = y, .z = z};
}

[[nodiscard]] SandboxWorldProbeResult validate_sandbox_world_package_evidence(std::string_view sample_id) {
    using Chunk = mirakana::runtime::RuntimeSandboxChunkRow;
    using Cost = mirakana::runtime::RuntimeSandboxConstructionCostRow;
    using Destruction = mirakana::runtime::RuntimeSandboxDestructionIntent;
    using ExistingCell = mirakana::runtime::RuntimeSandboxExistingCellRow;
    using Persistence = mirakana::runtime::RuntimeSandboxPersistenceRow;
    using PersistenceStatus = mirakana::runtime::RuntimeSandboxPersistenceStatus;
    using Placement = mirakana::runtime::RuntimeSandboxPlacementIntent;
    using Request = mirakana::runtime::RuntimeSandboxWorldMutationRequest;
    using Status = mirakana::runtime::RuntimeSandboxWorldStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_mutation(
            Request{.world_id = sample_prefix + ".sandbox.world",
                    .world_tick = 64U,
                    .chunk_rows =
                        std::vector<Chunk>{
                            Chunk{.chunk_id = "chunk.0",
                                  .region_id = "region.spawn",
                                  .origin_x = 0,
                                  .origin_y = 0,
                                  .origin_z = 0,
                                  .size_x = 8U,
                                  .size_y = 8U,
                                  .size_z = 8U,
                                  .resident = true,
                                  .persistent = true,
                                  .source_index = 1U},
                            Chunk{.chunk_id = "chunk.1",
                                  .region_id = "region.spawn",
                                  .origin_x = 8,
                                  .origin_y = 0,
                                  .origin_z = 0,
                                  .size_x = 8U,
                                  .size_y = 8U,
                                  .size_z = 8U,
                                  .resident = true,
                                  .persistent = true,
                                  .source_index = 2U},
                        },
                    .existing_cell_rows =
                        std::vector<ExistingCell>{
                            ExistingCell{.chunk_id = "chunk.0",
                                         .coord = make_sandbox_cell(1, 0, 1),
                                         .block_id = "block.soil",
                                         .destructible = true,
                                         .protected_cell = false,
                                         .source_index = 1U},
                            ExistingCell{.chunk_id = "chunk.0",
                                         .coord = make_sandbox_cell(2, 0, 1),
                                         .block_id = "block.stone",
                                         .destructible = false,
                                         .protected_cell = true,
                                         .source_index = 2U},
                        },
                    .placement_intents =
                        std::vector<Placement>{
                            Placement{.intent_id = "place.wall",
                                      .chunk_id = "chunk.0",
                                      .coord = make_sandbox_cell(3, 0, 1),
                                      .block_id = "block.wall",
                                      .provided_costs = {Cost{.block_id = "block.wall",
                                                              .item_id = "item.wood",
                                                              .quantity = 3U,
                                                              .source_index = 1U}},
                                      .source_index = 1U},
                            Placement{.intent_id = "place.occupied",
                                      .chunk_id = "chunk.0",
                                      .coord = make_sandbox_cell(1, 0, 1),
                                      .block_id = "block.wall",
                                      .provided_costs = {Cost{.block_id = "block.wall",
                                                              .item_id = "item.wood",
                                                              .quantity = 3U,
                                                              .source_index = 2U}},
                                      .source_index = 2U},
                            Placement{.intent_id = "place.missing_cost",
                                      .chunk_id = "chunk.1",
                                      .coord = make_sandbox_cell(9, 0, 1),
                                      .block_id = "block.door",
                                      .source_index = 3U},
                        },
                    .destruction_intents =
                        std::vector<Destruction>{
                            Destruction{.intent_id = "destroy.soil",
                                        .chunk_id = "chunk.0",
                                        .coord = make_sandbox_cell(1, 0, 1),
                                        .source_index = 1U},
                            Destruction{.intent_id = "destroy.protected",
                                        .chunk_id = "chunk.0",
                                        .coord = make_sandbox_cell(2, 0, 1),
                                        .source_index = 2U},
                        },
                    .construction_cost_rows =
                        std::vector<Cost>{
                            Cost{.block_id = "block.wall", .item_id = "item.wood", .quantity = 3U, .source_index = 1U},
                            Cost{.block_id = "block.door", .item_id = "item.wood", .quantity = 4U, .source_index = 2U},
                        },
                    .persistence_rows = std::vector<Persistence>{
                        Persistence{.chunk_id = "chunk.0",
                                    .key = "persist.chunk.0",
                                    .expected_schema_version = 2U,
                                    .observed_schema_version = 2U,
                                    .status = PersistenceStatus::rejected,
                                    .source_index = 1U},
                        Persistence{.chunk_id = "chunk.1",
                                    .key = "persist.chunk.1",
                                    .expected_schema_version = 3U,
                                    .observed_schema_version = 2U,
                                    .status = PersistenceStatus::rejected,
                                    .source_index = 2U},
                    }});

    SandboxWorldProbeResult result;
    result.status = plan.status;
    result.chunk_rows = plan.chunk_count;
    result.resident_chunk_rows = plan.resident_chunk_count;
    result.existing_cell_rows = plan.existing_cell_rows.size();
    result.placement_intent_rows = plan.placement_intent_count;
    result.placement_accepted_rows = plan.accepted_placement_count;
    result.placement_rejected_rows = plan.rejected_placement_count;
    result.destruction_intent_rows = plan.destruction_intent_count;
    result.destruction_accepted_rows = plan.accepted_destruction_count;
    result.destruction_rejected_rows = plan.rejected_destruction_count;
    result.construction_cost_rows = plan.construction_cost_count;
    result.mutation_rows = plan.mutation_rows.size();
    result.persistence_rows = plan.persistence_row_count;
    result.persistence_repairable_rows = plan.repairable_persistence_row_count;
    result.rejected_unsafe_mutation_rows = plan.rejected_unsafe_mutation_count;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.invoked_world_mutation = plan.invoked_world_mutation;
    result.invoked_persistence_io = plan.invoked_persistence_io;
    result.invoked_package_io = plan.invoked_package_io;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.chunk_rows == 2U &&
                   result.resident_chunk_rows == 2U && result.existing_cell_rows == 2U &&
                   result.placement_intent_rows == 3U && result.placement_accepted_rows == 1U &&
                   result.placement_rejected_rows == 2U && result.destruction_intent_rows == 2U &&
                   result.destruction_accepted_rows == 1U && result.destruction_rejected_rows == 1U &&
                   result.construction_cost_rows == 2U && result.mutation_rows == 5U && result.persistence_rows == 2U &&
                   result.persistence_repairable_rows == 1U && result.rejected_unsafe_mutation_rows == 3U &&
                   result.diagnostics == 0U && result.replay_hash != 0U && !result.invoked_world_mutation &&
                   !result.invoked_persistence_io && !result.invoked_package_io;
    return result;
}

[[nodiscard]] SimulationManagementProbeResult
validate_simulation_management_package_evidence(std::string_view sample_id) {
    using Economy = mirakana::runtime::RuntimeSimulationEconomySummary;
    using Job = mirakana::runtime::RuntimeSimulationJobRow;
    using JobStatus = mirakana::runtime::RuntimeSimulationJobStatus;
    using Link = mirakana::runtime::RuntimeSimulationLogisticsLink;
    using Need = mirakana::runtime::RuntimeSimulationPopulationNeedRow;
    using NeedStatus = mirakana::runtime::RuntimeSimulationNeedStatus;
    using Request = mirakana::runtime::RuntimeSimulationManagementRequest;
    using Resource = mirakana::runtime::RuntimeSimulationResourceRow;
    using Save = mirakana::runtime::RuntimeSimulationSaveReviewRow;
    using SaveStatus = mirakana::runtime::RuntimeSimulationSaveReviewStatus;
    using Schedule = mirakana::runtime::RuntimeSimulationScheduleRow;
    using Status = mirakana::runtime::RuntimeSimulationManagementStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan = mirakana::runtime::plan_runtime_simulation_management(Request{
        .simulation_id = sample_prefix + ".simulation.management",
        .world_tick = 100U,
        .long_run_tick_count = 240U,
        .resource_rows =
            std::vector<Resource>{
                Resource{
                    .resource_id = "food", .storage_id = "colony", .quantity = 25, .capacity = 100, .source_index = 1U},
                Resource{
                    .resource_id = "meal", .storage_id = "colony", .quantity = 2, .capacity = 30, .source_index = 2U},
                Resource{
                    .resource_id = "ore", .storage_id = "mine", .quantity = 12, .capacity = 40, .source_index = 3U},
                Resource{
                    .resource_id = "ore", .storage_id = "colony", .quantity = 0, .capacity = 40, .source_index = 4U},
            },
        .job_rows =
            std::vector<Job>{
                Job{.job_id = "job.cook",
                    .worker_id = "worker.0",
                    .input_resource_id = "food",
                    .input_storage_id = "colony",
                    .output_resource_id = "meal",
                    .output_storage_id = "colony",
                    .input_quantity = 5,
                    .output_quantity = 3,
                    .duration_ticks = 4U,
                    .status = JobStatus::invalid,
                    .source_index = 1U},
                Job{.job_id = "job.shortage",
                    .worker_id = "worker.1",
                    .input_resource_id = "ore",
                    .input_storage_id = "mine",
                    .output_resource_id = "meal",
                    .output_storage_id = "colony",
                    .input_quantity = 99,
                    .output_quantity = 1,
                    .duration_ticks = 4U,
                    .status = JobStatus::invalid,
                    .source_index = 2U},
            },
        .logistics_links =
            std::vector<Link>{
                Link{.link_id = "route.ore",
                     .resource_id = "ore",
                     .source_storage_id = "mine",
                     .destination_storage_id = "colony",
                     .transfer_quantity = 4,
                     .travel_ticks = 6U,
                     .enabled = true,
                     .source_index = 1U},
                Link{.link_id = "route.ore.back",
                     .resource_id = "ore",
                     .source_storage_id = "colony",
                     .destination_storage_id = "mine",
                     .transfer_quantity = 99,
                     .travel_ticks = 6U,
                     .enabled = true,
                     .source_index = 2U},
            },
        .economy_summaries = std::vector<Economy>{Economy{.summary_id = "economy.food",
                                                          .resource_id = "food",
                                                          .produced_quantity = 6,
                                                          .consumed_quantity = 5,
                                                          .traded_quantity = 0,
                                                          .source_index = 1U}},
        .population_need_rows =
            std::vector<Need>{
                Need{.population_id = "population.colony",
                     .need_id = "need.food",
                     .resource_id = "food",
                     .storage_id = "colony",
                     .required_quantity = 20,
                     .available_quantity = 0,
                     .status = NeedStatus::invalid,
                     .source_index = 1U},
                Need{.population_id = "population.colony",
                     .need_id = "need.comfort",
                     .resource_id = "meal",
                     .storage_id = "colony",
                     .required_quantity = 5,
                     .available_quantity = 0,
                     .status = NeedStatus::invalid,
                     .source_index = 2U},
            },
        .schedule_rows =
            std::vector<Schedule>{
                Schedule{.schedule_id = "schedule.job.cook",
                         .target_id = "job.cook",
                         .start_tick = 90U,
                         .end_tick = 200U,
                         .enabled = true,
                         .source_index = 1U},
                Schedule{.schedule_id = "schedule.route.ore",
                         .target_id = "route.ore",
                         .start_tick = 90U,
                         .end_tick = 200U,
                         .enabled = true,
                         .source_index = 2U},
            },
        .save_review_rows =
            std::vector<Save>{
                Save{.domain = "simulation",
                     .key = "state",
                     .expected_schema_version = 2U,
                     .observed_schema_version = 2U,
                     .status = SaveStatus::rejected,
                     .source_index = 1U},
                Save{.domain = "simulation",
                     .key = "balances",
                     .expected_schema_version = 3U,
                     .observed_schema_version = 2U,
                     .status = SaveStatus::rejected,
                     .source_index = 2U},
            },
        .seed = 42U,
    });

    SimulationManagementProbeResult result;
    result.status = plan.status;
    result.tick_count = plan.tick_count;
    result.resource_balance_rows = plan.resource_balance_rows.size();
    result.job_rows = plan.job_rows.size();
    result.job_assignment_rows = plan.job_assignment_count;
    result.logistics_links = plan.logistics_links.size();
    result.logistics_transfer_rows = plan.logistics_transfer_rows.size();
    result.logistics_scheduled_transfer_rows = plan.scheduled_logistics_transfer_count;
    result.economy_summary_rows = plan.economy_summaries.size();
    result.population_need_rows = plan.population_need_rows.size();
    result.need_deficit_rows = plan.need_deficit_count;
    result.schedule_rows = plan.schedule_rows.size();
    result.save_review_rows = plan.save_review_rows.size();
    result.save_review_repairable_rows = plan.repairable_save_review_count;
    result.dashboard_rows = plan.dashboard_rows.size();
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.invoked_economy_execution = plan.invoked_economy_execution;
    result.invoked_save_io = plan.invoked_save_io;
    result.invoked_runtime_ui = plan.invoked_runtime_ui;
    result.invoked_package_io = plan.invoked_package_io;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.tick_count == 240U &&
                   result.resource_balance_rows == 4U && result.job_rows == 2U && result.job_assignment_rows == 1U &&
                   result.logistics_links == 2U && result.logistics_transfer_rows == 2U &&
                   result.logistics_scheduled_transfer_rows == 1U && result.economy_summary_rows == 1U &&
                   result.population_need_rows == 2U && result.need_deficit_rows == 1U && result.schedule_rows == 2U &&
                   result.save_review_rows == 2U && result.save_review_repairable_rows == 1U &&
                   result.dashboard_rows == 7U && result.diagnostics == 0U && result.replay_hash != 0U &&
                   !result.invoked_economy_execution && !result.invoked_save_io && !result.invoked_runtime_ui &&
                   !result.invoked_package_io;
    return result;
}

[[nodiscard]] std::size_t count_world_entity_diagnostics(const mirakana::runtime::RuntimeWorldEntityLifecyclePlan& plan,
                                                         mirakana::runtime::RuntimeWorldEntityDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] WorldEntityModelProbeResult validate_world_entity_model_package_evidence() {
    using Action = mirakana::runtime::RuntimeWorldEntityLifecycleAction;
    using Code = mirakana::runtime::RuntimeWorldEntityDiagnosticCode;
    using Component = mirakana::runtime::RuntimeWorldComponentRow;
    using Entity = mirakana::runtime::RuntimeWorldEntityRow;
    using EntityId = mirakana::runtime::RuntimeWorldEntityId;
    using PersistEntity = mirakana::runtime::RuntimeSimulationPersistentEntityRow;
    using PersistPlan = mirakana::runtime::RuntimeSimulationPersistencePlan;
    using PersistStatus = mirakana::runtime::RuntimeSimulationPersistenceStatus;
    using Region = mirakana::runtime::RuntimeWorldRegionRow;
    using RegionAction = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using RegionCode = mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode;
    using RegionId = mirakana::runtime::RuntimeWorldRegionId;
    using StreamingDiagnostic = mirakana::runtime::RuntimeWorldRegionStreamingDiagnostic;
    using StreamingPlan = mirakana::runtime::RuntimeWorldRegionStreamingPlan;
    using StreamingRow = mirakana::runtime::RuntimeWorldRegionStreamingPlanRow;
    using StreamingStatus = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;
    using Request = mirakana::runtime::RuntimeWorldEntityLifecycleRequest;
    using Schema = mirakana::runtime::RuntimeWorldComponentSchemaRow;
    using SchemaId = mirakana::runtime::RuntimeWorldComponentSchemaId;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto
        plan =
            mirakana::runtime::plan_runtime_world_entity_lifecycle(
                Request{
                    .world_id = "sample2d.world",
                    .regions =
                        std::vector<Region>{
                            Region{.region_id = RegionId{.value = "field"}, .source_index = 1U},
                            Region{.region_id = RegionId{.value = "town"}, .source_index = 2U},
                        },
                    .entities =
                        std::vector<Entity>{
                            Entity{.entity_id = EntityId{.value = "player"},
                                   .region_id = RegionId{.value = "field"},
                                   .archetype_id = "hero",
                                   .active = true,
                                   .generation = 1U,
                                   .source_index = 1U},
                            Entity{.entity_id = EntityId{.value = "npc.vendor"},
                                   .region_id = RegionId{.value = "town"},
                                   .archetype_id = "vendor",
                                   .active = true,
                                   .generation = 1U,
                                   .source_index = 2U},
                            Entity{.entity_id = EntityId{.value = "chest"},
                                   .region_id = RegionId{.value = "town"},
                                   .archetype_id = "container",
                                   .active = true,
                                   .generation = 1U,
                                   .source_index = 3U},
                        },
                    .component_schemas =
                        std::vector<Schema>{
                            Schema{.schema_id = SchemaId{.value = "inventory"}, .version = 1U, .source_index = 2U},
                            Schema{.schema_id = SchemaId{.value = "transform"}, .version = 1U, .source_index = 1U},
                        },
                    .components =
                        std::vector<Component>{
                            Component{.entity_id = EntityId{.value = "player"},
                                      .schema_id = SchemaId{.value = "transform"},
                                      .state_hash = "hash.player.transform",
                                      .source_index = 1U},
                            Component{.entity_id = EntityId{.value = "player"},
                                      .schema_id = SchemaId{.value = "inventory"},
                                      .state_hash = "hash.player.inventory",
                                      .source_index = 2U},
                            Component{.entity_id = EntityId{.value = "npc.vendor"},
                                      .schema_id = SchemaId{.value = "transform"},
                                      .state_hash = "hash.vendor.transform",
                                      .source_index = 3U},
                            Component{.entity_id = EntityId{.value = "chest"},
                                      .schema_id = SchemaId{.value = "inventory"},
                                      .state_hash = "hash.chest.inventory",
                                      .source_index = 4U},
                        },
                    .lifecycle_intents =
                        {
                            mirakana::runtime::RuntimeWorldEntityLifecycleIntent{
                                .action = Action::spawn_entity,
                                .entity_id = EntityId{.value = "quest.item"},
                                .target_region_id = RegionId{.value = "town"},
                                .archetype_id = "pickup",
                                .source_index = 1U},
                            mirakana::runtime::RuntimeWorldEntityLifecycleIntent{
                                .action = Action::move_entity_region,
                                .entity_id = EntityId{.value = "player"},
                                .target_region_id = RegionId{.value = "town"},
                                .source_index = 2U},
                            mirakana::runtime::RuntimeWorldEntityLifecycleIntent{.action = Action::despawn_entity,
                                                                                 .entity_id =
                                                                                     EntityId{.value = "npc.vendor"},
                                                                                 .source_index = 3U},
                        },
                    .persistence_bridge =
                        mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                            .required = true,
                            .plan =
                                PersistPlan{
                                    .status = PersistStatus::ready,
                                    .world_id = "sample2d.world",
                                    .world_tick = 120U,
                                    .entity_rows =
                                        {
                                            PersistEntity{.entity_id = "player",
                                                          .entity_type = "hero",
                                                          .region_id = "field",
                                                          .state_hash = "hash.player"},
                                        },
                                },
                        },
                    .streaming_bridge =
                        mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                            .required = true,
                            .plan =
                                StreamingPlan{
                                    .status = StreamingStatus::planned,
                                    .rows =
                                        {
                                            StreamingRow{.action = RegionAction::load_region,
                                                         .region_id = "town",
                                                         .estimated_resident_bytes = 4096U,
                                                         .estimated_asset_records = 8U},
                                        },
                                },
                        },
                });

    const auto duplicate_plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(Request{
        .world_id = "sample2d.world",
        .regions = std::vector<Region>{Region{.region_id = RegionId{.value = "field"}, .source_index = 1U}},
        .entities =
            std::vector<Entity>{
                Entity{.entity_id = EntityId{.value = "player"},
                       .region_id = RegionId{.value = "field"},
                       .archetype_id = "hero",
                       .active = true,
                       .generation = 1U,
                       .source_index = 1U},
                Entity{.entity_id = EntityId{.value = "player"},
                       .region_id = RegionId{.value = "field"},
                       .archetype_id = "hero.copy",
                       .active = true,
                       .generation = 1U,
                       .source_index = 2U},
            },
    });
    const auto bridge_rejection_plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(Request{
        .world_id = "sample2d.world",
        .regions = std::vector<Region>{Region{.region_id = RegionId{.value = "field"}, .source_index = 1U}},
        .entities =
            std::vector<Entity>{
                Entity{.entity_id = EntityId{.value = "player"},
                       .region_id = RegionId{.value = "field"},
                       .archetype_id = "hero",
                       .active = true,
                       .generation = 1U,
                       .source_index = 1U},
            },
        .persistence_bridge =
            mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                .required = true,
                .plan =
                    PersistPlan{
                        .status = PersistStatus::ready,
                        .world_id = "other_world",
                        .world_tick = 120U,
                        .entity_rows =
                            {
                                PersistEntity{.entity_id = "ghost",
                                              .entity_type = "npc",
                                              .region_id = "missing_region",
                                              .state_hash = "hash.ghost"},
                            },
                    },
            },
        .streaming_bridge =
            mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                .required = true,
                .plan =
                    StreamingPlan{
                        .status = StreamingStatus::planned,
                        .diagnostics =
                            {
                                StreamingDiagnostic{.code = RegionCode::missing_desired_region,
                                                    .region_id = "missing_region",
                                                    .message = "synthetic bridge diagnostic"},
                            },
                        .rows =
                            {
                                StreamingRow{.action = RegionAction::load_region,
                                             .region_id = "missing_region",
                                             .estimated_resident_bytes = 4096U,
                                             .estimated_asset_records = 8U},
                            },
                    },
            },
    });

    WorldEntityModelProbeResult result;
    result.status = plan.status;
    result.entity_rows = plan.entity_rows.size();
    result.component_rows = plan.component_rows.size();
    result.region_ownership_rows = plan.region_ownership_rows.size();
    result.lifecycle_rows = plan.lifecycle_rows.size();
    result.persistence_rows = plan.persistence_rows.size();
    result.streaming_region_rows = plan.streaming_region_rows.size();
    result.spawn_rows = plan.spawn_count;
    result.move_rows = plan.move_count;
    result.despawn_rows = plan.despawn_count;
    result.duplicate_entity_diagnostics = count_world_entity_diagnostics(duplicate_plan, Code::duplicate_entity_id);
    result.bridge_rejection_status = bridge_rejection_plan.status;
    result.bridge_rejection_diagnostics = bridge_rejection_plan.diagnostics.size();
    result.bridge_rejection_persistence_rows = bridge_rejection_plan.persistence_rows.size();
    result.bridge_rejection_streaming_region_rows = bridge_rejection_plan.streaming_region_rows.size();
    result.bridge_rejection_streaming_diagnostics_present =
        count_world_entity_diagnostics(bridge_rejection_plan, Code::streaming_bridge_diagnostics_present);
    result.bridge_rejection_fail_closed =
        bridge_rejection_plan.status == Status::invalid_request && !bridge_rejection_plan.succeeded() &&
        result.bridge_rejection_persistence_rows == 0U && result.bridge_rejection_streaming_region_rows == 0U;
    result.diagnostics = plan.diagnostics.size();
    result.ready = plan.status == Status::ready && plan.succeeded() && result.entity_rows == 3U &&
                   result.component_rows == 4U && result.region_ownership_rows == 3U && result.lifecycle_rows == 3U &&
                   result.persistence_rows == 1U && result.streaming_region_rows == 1U && result.spawn_rows == 1U &&
                   result.move_rows == 1U && result.despawn_rows == 1U && result.duplicate_entity_diagnostics == 1U &&
                   result.bridge_rejection_status == Status::invalid_request &&
                   result.bridge_rejection_diagnostics == 5U &&
                   result.bridge_rejection_streaming_diagnostics_present == 1U && result.bridge_rejection_fail_closed &&
                   result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::size_t
count_runtime_addressable_diagnostics(const mirakana::runtime::RuntimeAddressableContentStreamingPlan& plan,
                                      mirakana::runtime::RuntimeAddressableContentDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] AddressableContentStreamingProbeResult
validate_addressable_content_streaming_package_evidence(const mirakana::runtime::RuntimeAssetPackage& runtime_package) {
    using AddressId = mirakana::runtime::RuntimeAddressableAssetId;
    using AddressRow = mirakana::runtime::RuntimeAddressableAssetRow;
    using Budget = mirakana::runtime::RuntimeAddressableResidentBudget;
    using Code = mirakana::runtime::RuntimeAddressableContentDiagnosticCode;
    using LoadRequest = mirakana::runtime::RuntimeAddressableLoadRequest;
    using ReleaseRequest = mirakana::runtime::RuntimeAddressableReleaseRequest;
    using Request = mirakana::runtime::RuntimeAddressableContentStreamingRequest;
    using ResidentRow = mirakana::runtime::RuntimeAddressableResidentAssetRow;
    using Status = mirakana::runtime::RuntimeAddressableContentStreamingStatus;

    const std::vector<AddressRow> address_rows{
        AddressRow{
            .address_id = AddressId{.value = "scene/playable"}, .asset = packaged_scene_asset_id(), .source_index = 1U},
        AddressRow{.address_id = AddressId{.value = "texture/player"},
                   .asset = packaged_sprite_texture_asset_id(),
                   .source_index = 2U},
        AddressRow{.address_id = AddressId{.value = "material/player"},
                   .asset = packaged_material_asset_id(),
                   .source_index = 3U},
        AddressRow{.address_id = AddressId{.value = "tilemap/level"},
                   .asset = packaged_tilemap_asset_id(),
                   .source_index = 4U},
        AddressRow{.address_id = AddressId{.value = "animation/player"},
                   .asset = packaged_sprite_animation_asset_id(),
                   .source_index = 5U},
        AddressRow{.address_id = AddressId{.value = "ui/hud-atlas"},
                   .asset = packaged_ui_atlas_asset_id(),
                   .source_index = 6U},
    };
    const Request request{
        .stream_id = "sample2d.addressable_content",
        .package = runtime_package,
        .addressable_assets = address_rows,
        .resident_assets =
            {
                ResidentRow{.address_id = AddressId{.value = "material/player"}, .ref_count = 1U, .source_index = 1U},
            },
        .load_requests =
            {
                LoadRequest{.address_id = AddressId{.value = "scene/playable"},
                            .include_dependencies = true,
                            .source_index = 1U},
            },
        .release_requests =
            {
                ReleaseRequest{.address_id = AddressId{.value = "material/player"},
                               .release_count = 1U,
                               .include_dependencies = false,
                               .source_index = 1U},
            },
        .budget = Budget{.max_resident_bytes = 16U * 1024U * 1024U},
    };
    auto budget_request = request;
    budget_request.budget = Budget{.max_resident_bytes = 1U};

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);
    const auto budget_plan = mirakana::runtime::plan_runtime_addressable_content_streaming(budget_request);

    AddressableContentStreamingProbeResult result;
    result.status = plan.status;
    result.budget_rejection_status = budget_plan.status;
    result.address_rows = plan.address_rows.size();
    result.dependency_rows = plan.dependency_rows.size();
    result.load_rows = plan.load_rows.size();
    result.release_rows = plan.release_rows.size();
    result.refcount_rows = plan.load_rows.size() + plan.release_rows.size();
    result.resident_bytes = plan.final_resident_bytes;
    result.resident_budget_bytes = plan.resident_budget_bytes;
    result.budget_rejection_diagnostics =
        count_runtime_addressable_diagnostics(budget_plan, Code::resident_budget_exceeded);
    result.diagnostics = plan.diagnostics.size();
    result.package_io = plan.invoked_package_io || budget_plan.invoked_package_io;
    result.async_execution = plan.invoked_async_execution || budget_plan.invoked_async_execution;
    result.committed = plan.committed || budget_plan.committed;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.address_rows == 6U &&
                   result.dependency_rows == 7U && result.load_rows == 3U && result.release_rows == 1U &&
                   result.refcount_rows == 4U && result.resident_bytes > 0U && result.resident_budget_bytes > 0U &&
                   budget_plan.status == Status::budget_limited && result.budget_rejection_diagnostics == 1U &&
                   !result.package_io && !result.async_execution && !result.committed && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] SimulationOrchestrationProbeResult validate_simulation_orchestration_package_evidence() {
    using Code = mirakana::runtime::RuntimeSimulationOrchestrationDiagnosticCode;
    using Command = mirakana::runtime::RuntimeSimulationInputCommandDesc;
    using Request = mirakana::runtime::RuntimeSimulationOrchestrationRequest;
    using Status = mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus;

    const auto reviewed_plan = mirakana::runtime::plan_runtime_simulation_orchestration(Request{
        .simulation_id = "sample2d.loop",
        .next_tick = 120U,
        .fixed_tick_rate_hz = 60U,
        .accumulated_time_us = 50'000U,
        .max_steps_per_frame = 4U,
        .input_commands =
            std::vector<Command>{
                Command{.command_id = "cmd.jump", .action_id = "jump", .target_tick = 120U, .source_index = 2U},
                Command{.command_id = "cmd.fire", .action_id = "fire", .target_tick = 121U, .source_index = 1U},
                Command{.command_id = "cmd.move", .action_id = "move_right", .target_tick = 121U, .source_index = 3U},
            },
    });

    const auto budget_plan = mirakana::runtime::plan_runtime_simulation_orchestration(Request{
        .simulation_id = "sample2d.budget",
        .next_tick = 200U,
        .fixed_tick_rate_hz = 30U,
        .accumulated_time_us = 200'000U,
        .max_steps_per_frame = 2U,
    });

    const auto invalid_plan = mirakana::runtime::plan_runtime_simulation_orchestration(Request{
        .simulation_id = "sample2d.invalid",
        .next_tick = 10U,
        .fixed_tick_rate_hz = 60U,
        .accumulated_time_us = 33'333U,
        .max_steps_per_frame = 2U,
        .input_commands =
            std::vector<Command>{
                Command{.command_id = "", .action_id = "move", .target_tick = 10U, .source_index = 1U},
                Command{.command_id = "cmd.dupe", .action_id = "jump", .target_tick = 10U, .source_index = 2U},
                Command{.command_id = "cmd.dupe", .action_id = "jump", .target_tick = 10U, .source_index = 3U},
                Command{.command_id = "cmd.past", .action_id = "dash", .target_tick = 9U, .source_index = 4U},
                Command{.command_id = "cmd.future", .action_id = "fire", .target_tick = 14U, .source_index = 5U},
            },
    });

    SimulationOrchestrationProbeResult result;
    result.status = reviewed_plan.status;
    result.budget_limited_status = budget_plan.status;
    result.available_steps = reviewed_plan.available_step_count;
    result.planned_steps = reviewed_plan.planned_step_count;
    result.step_rows = reviewed_plan.steps.size();
    result.command_rows = reviewed_plan.commands.size();
    for (const auto& step : reviewed_plan.steps) {
        result.command_playback_rows += step.command_count;
    }
    result.consumed_time_us = reviewed_plan.consumed_time_us;
    result.remaining_time_us = reviewed_plan.remaining_time_us;
    result.budget_limited_available_steps = budget_plan.available_step_count;
    result.budget_limited_planned_steps = budget_plan.planned_step_count;
    result.budget_limited_remaining_time_us = budget_plan.remaining_time_us;
    result.diagnostics = reviewed_plan.diagnostics.size();
    result.invalid_command_diagnostics =
        count_simulation_orchestration_diagnostics(invalid_plan, Code::missing_command_id) +
        count_simulation_orchestration_diagnostics(invalid_plan, Code::duplicate_command_id) +
        count_simulation_orchestration_diagnostics(invalid_plan, Code::command_before_next_tick) +
        count_simulation_orchestration_diagnostics(invalid_plan, Code::command_after_planned_window);

    result.ready = reviewed_plan.status == Status::planned && reviewed_plan.succeeded() &&
                   budget_plan.status == Status::budget_limited && budget_plan.succeeded() &&
                   invalid_plan.status == Status::invalid_request && result.available_steps == 3U &&
                   result.planned_steps == 3U && result.step_rows == 3U && result.command_rows == 3U &&
                   result.command_playback_rows == 3U && result.consumed_time_us == 49'998U &&
                   result.remaining_time_us == 2U && result.budget_limited_available_steps == 6U &&
                   result.budget_limited_planned_steps == 2U && result.budget_limited_remaining_time_us == 133'334U &&
                   result.invalid_command_diagnostics == 4U && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::size_t count_gameplay_authoring_diagnostics(const mirakana::GameplayAuthoringReviewResult& result,
                                                               std::string_view code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::size_t
count_production_authoring_diagnostics(const mirakana::ProductionAuthoringWorkflowReviewResult& result,
                                       std::string_view code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] GameplayAuthoringReviewProbeResult validate_gameplay_authoring_review_package_evidence() {
    const mirakana::GameplayAuthoringCapabilityProfile profile{
        .supported_capability_ids = {"gameplay-authoring-foundation-v1", "engine-quest-dialogue-state-v1",
                                     "engine-inventory-items-crafting-v1", "engine-construction-placement-v1",
                                     "engine-procedural-generation-v1", "engine-world-region-streaming-v1",
                                     "engine-entity-scale-and-culling-v1", "engine-scripting-sandbox-v1",
                                     "engine-networking-foundation-v1", "gameplay-simulation-orchestration-v1"},
        .validation_recipe_ids = {"headless-gameplay", "2d-desktop-runtime-package",
                                  "installed-2d-gameplay-systems-smoke", "installed-2d-world-region-streaming-smoke",
                                  "installed-2d-entity-scale-culling-smoke",
                                  "installed-2d-scripting-sandbox-policy-smoke",
                                  "installed-2d-networking-foundation-policy-smoke",
                                  "installed-2d-simulation-orchestration-smoke",
                                  "installed-2d-gameplay-authoring-review-smoke"},
        .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
    };

    const mirakana::GameplayAuthoringReviewRequest reviewed_request{
        .profile = profile,
        .features =
            {
                mirakana::GameplayAuthoringRequestedFeatureRow{
                    .feature_id = "intro_quest",
                    .gameplay_family = "narrative",
                    .required_capability_ids = {"gameplay-authoring-foundation-v1", "engine-quest-dialogue-state-v1"},
                    .validation_recipe_ids = {"headless-gameplay", "installed-2d-gameplay-systems-smoke"},
                    .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
                    .claimed_scope_ids = {},
                    .source_index = 1U,
                },
                mirakana::GameplayAuthoringRequestedFeatureRow{
                    .feature_id = "settlement_crafting_loop",
                    .gameplay_family = "progression",
                    .required_capability_ids = {"engine-inventory-items-crafting-v1",
                                                "engine-construction-placement-v1"},
                    .validation_recipe_ids = {"2d-desktop-runtime-package", "installed-2d-gameplay-systems-smoke"},
                    .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
                    .claimed_scope_ids = {},
                    .source_index = 2U,
                },
                mirakana::GameplayAuthoringRequestedFeatureRow{
                    .feature_id = "seeded_region_event",
                    .gameplay_family = "world",
                    .required_capability_ids = {"engine-procedural-generation-v1", "engine-world-region-streaming-v1",
                                                "engine-entity-scale-and-culling-v1"},
                    .validation_recipe_ids = {"installed-2d-world-region-streaming-smoke",
                                              "installed-2d-entity-scale-culling-smoke"},
                    .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
                    .claimed_scope_ids = {},
                    .source_index = 3U,
                },
                mirakana::GameplayAuthoringRequestedFeatureRow{
                    .feature_id = "networked_simulation_policy",
                    .gameplay_family = "systems",
                    .required_capability_ids = {"engine-scripting-sandbox-v1", "engine-networking-foundation-v1",
                                                "gameplay-simulation-orchestration-v1"},
                    .validation_recipe_ids = {"installed-2d-scripting-sandbox-policy-smoke",
                                              "installed-2d-networking-foundation-policy-smoke",
                                              "installed-2d-simulation-orchestration-smoke"},
                    .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
                    .claimed_scope_ids = {},
                    .source_index = 4U,
                },
            },
    };
    const auto reviewed_result = mirakana::review_gameplay_authoring_request(reviewed_request);

    const mirakana::GameplayAuthoringReviewRequest invalid_request{
        .profile = profile,
        .features =
            {
                mirakana::GameplayAuthoringRequestedFeatureRow{
                    .feature_id = "unreviewed_autonomy",
                    .gameplay_family = "automation",
                    .required_capability_ids = {"autonomous-commercial-game-design-v1"},
                    .validation_recipe_ids = {"unreviewed-freeform-generation"},
                    .package_evidence_ids = {"direct-cooked-package-mutation"},
                    .claimed_scope_ids = {"autonomous-commercial-game-design"},
                    .source_index = 99U,
                },
            },
    };
    const auto invalid_result = mirakana::review_gameplay_authoring_request(invalid_request);

    GameplayAuthoringReviewProbeResult result;
    result.feature_rows = reviewed_request.features.size();
    result.accepted_rows = reviewed_result.accepted_features.size();
    result.mutation_ledger_rows = reviewed_result.mutation_ledger_rows.size();
    result.remediation_rows = invalid_result.remediation_rows.size();
    result.missing_required_capability_diagnostics =
        count_gameplay_authoring_diagnostics(invalid_result, "missing_required_capability");
    result.missing_validation_recipe_diagnostics =
        count_gameplay_authoring_diagnostics(invalid_result, "missing_validation_recipe");
    result.missing_package_evidence_diagnostics =
        count_gameplay_authoring_diagnostics(invalid_result, "missing_package_evidence");
    result.unsupported_claim_diagnostics = count_gameplay_authoring_diagnostics(invalid_result, "unsupported_claim");
    result.diagnostics = reviewed_result.diagnostics.size();
    result.ready = reviewed_result.succeeded() && !invalid_result.succeeded() && result.feature_rows == 4U &&
                   result.accepted_rows == 4U && result.mutation_ledger_rows == 4U && result.remediation_rows == 4U &&
                   result.missing_required_capability_diagnostics == 1U &&
                   result.missing_validation_recipe_diagnostics == 1U &&
                   result.missing_package_evidence_diagnostics == 1U && result.unsupported_claim_diagnostics == 1U &&
                   result.diagnostics == 0U;
    return result;
}

[[nodiscard]] ProductionAuthoringWorkflowProbeResult validate_production_authoring_workflow_package_evidence() {
    mirakana::ProductionAuthoringWorkflowRequest request;
    request.supported_capability_ids = {"scene-placement-v1", "quest-dialogue-v1", "item-economy-v1",
                                        "ai-behavior-v1",     "world-region-v1",   "validation-repair-v1"};
    request.validation_recipe_ids = {"installed-2d-gameplay-systems-smoke", "installed-2d-world-region-streaming-smoke",
                                     "installed-2d-gameplay-authoring-review-smoke",
                                     "installed-2d-production-authoring-workflows-smoke"};
    request.package_evidence_ids = {"sample_2d_desktop_runtime_package"};
    request.reviewed_surface_ids = {"scene-prefab-authoring", "gameplay-authoring-review", "runtime-scene-validation"};
    request.workflow_rows = {
        mirakana::ProductionAuthoringWorkflowRow{
            .workflow_id = "scene_spawn_review",
            .kind = mirakana::ProductionAuthoringWorkflowKind::scene_placement,
            .target_path = "games/sample_2d_desktop_runtime_package/source/scenes/playable.scene",
            .required_capability_ids = {"scene-placement-v1"},
            .validation_recipe_ids = {"installed-2d-gameplay-systems-smoke"},
            .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
            .reviewed_surface_ids = {"scene-prefab-authoring"},
            .claimed_scope_ids = {},
            .requests_shared_surface_mutation = false,
            .requests_arbitrary_shell = false,
            .requests_cooked_package_mutation = false,
            .source_index = 1U,
        },
        mirakana::ProductionAuthoringWorkflowRow{
            .workflow_id = "quest_dialogue_review",
            .kind = mirakana::ProductionAuthoringWorkflowKind::quest_dialogue,
            .target_path = "games/sample_2d_desktop_runtime_package/source/gameplay/intro.quest",
            .required_capability_ids = {"quest-dialogue-v1"},
            .validation_recipe_ids = {"installed-2d-gameplay-systems-smoke"},
            .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
            .reviewed_surface_ids = {"gameplay-authoring-review"},
            .claimed_scope_ids = {},
            .requests_shared_surface_mutation = false,
            .requests_arbitrary_shell = false,
            .requests_cooked_package_mutation = false,
            .source_index = 2U,
        },
        mirakana::ProductionAuthoringWorkflowRow{
            .workflow_id = "item_economy_review",
            .kind = mirakana::ProductionAuthoringWorkflowKind::item_economy,
            .target_path = "games/sample_2d_desktop_runtime_package/source/gameplay/shop.items",
            .required_capability_ids = {"item-economy-v1"},
            .validation_recipe_ids = {"installed-2d-gameplay-systems-smoke"},
            .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
            .reviewed_surface_ids = {"gameplay-authoring-review"},
            .claimed_scope_ids = {},
            .requests_shared_surface_mutation = false,
            .requests_arbitrary_shell = false,
            .requests_cooked_package_mutation = false,
            .source_index = 3U,
        },
        mirakana::ProductionAuthoringWorkflowRow{
            .workflow_id = "ai_behavior_review",
            .kind = mirakana::ProductionAuthoringWorkflowKind::ai_behavior,
            .target_path = "games/sample_2d_desktop_runtime_package/source/ai/patrol.behavior",
            .required_capability_ids = {"ai-behavior-v1"},
            .validation_recipe_ids = {"installed-2d-gameplay-systems-smoke"},
            .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
            .reviewed_surface_ids = {"gameplay-authoring-review"},
            .claimed_scope_ids = {},
            .requests_shared_surface_mutation = false,
            .requests_arbitrary_shell = false,
            .requests_cooked_package_mutation = false,
            .source_index = 4U,
        },
        mirakana::ProductionAuthoringWorkflowRow{
            .workflow_id = "world_region_review",
            .kind = mirakana::ProductionAuthoringWorkflowKind::world_region,
            .target_path = "games/sample_2d_desktop_runtime_package/source/world/regions.world",
            .required_capability_ids = {"world-region-v1"},
            .validation_recipe_ids = {"installed-2d-world-region-streaming-smoke"},
            .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
            .reviewed_surface_ids = {"runtime-scene-validation"},
            .claimed_scope_ids = {},
            .requests_shared_surface_mutation = false,
            .requests_arbitrary_shell = false,
            .requests_cooked_package_mutation = false,
            .source_index = 5U,
        },
        mirakana::ProductionAuthoringWorkflowRow{
            .workflow_id = "validation_repair_review",
            .kind = mirakana::ProductionAuthoringWorkflowKind::validation_repair,
            .target_path = "games/sample_2d_desktop_runtime_package/game.agent.json",
            .required_capability_ids = {"validation-repair-v1"},
            .validation_recipe_ids = {"installed-2d-production-authoring-workflows-smoke"},
            .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
            .reviewed_surface_ids = {"runtime-scene-validation"},
            .claimed_scope_ids = {},
            .requests_shared_surface_mutation = false,
            .requests_arbitrary_shell = false,
            .requests_cooked_package_mutation = false,
            .source_index = 6U,
        },
    };
    const auto reviewed_result = mirakana::review_production_authoring_workflow(request);

    mirakana::ProductionAuthoringWorkflowRequest invalid_request;
    invalid_request.supported_capability_ids = {"scene-placement-v1"};
    invalid_request.validation_recipe_ids = {"installed-2d-gameplay-systems-smoke"};
    invalid_request.package_evidence_ids = {"sample_2d_desktop_runtime_package"};
    invalid_request.reviewed_surface_ids = {"scene-prefab-authoring"};
    invalid_request.workflow_rows.push_back(mirakana::ProductionAuthoringWorkflowRow{
        .workflow_id = "unsafe_authoring",
        .kind = mirakana::ProductionAuthoringWorkflowKind::scene_placement,
        .target_path = "engine/runtime/src/unsafe.cpp",
        .required_capability_ids = {"scene-placement-v1"},
        .validation_recipe_ids = {"installed-2d-gameplay-systems-smoke"},
        .package_evidence_ids = {"sample_2d_desktop_runtime_package"},
        .reviewed_surface_ids = {"scene-prefab-authoring"},
        .claimed_scope_ids = {"native D3D12 handle", "arbitrary shell", "cooked package mutation"},
        .requests_shared_surface_mutation = true,
        .requests_arbitrary_shell = true,
        .requests_cooked_package_mutation = true,
        .source_index = 99U,
    });
    const auto invalid_result = mirakana::review_production_authoring_workflow(invalid_request);

    ProductionAuthoringWorkflowProbeResult result;
    result.workflow_rows = request.workflow_rows.size();
    result.accepted_rows = reviewed_result.accepted_rows.size();
    result.mutation_ledger_rows = reviewed_result.mutation_ledger_rows.size();
    result.validation_repair_rows = reviewed_result.validation_repair_rows.size();
    result.shared_surface_mutation_diagnostics =
        count_production_authoring_diagnostics(invalid_result, "shared_surface_mutation");
    result.arbitrary_shell_diagnostics = count_production_authoring_diagnostics(invalid_result, "arbitrary_shell");
    result.cooked_package_mutation_diagnostics =
        count_production_authoring_diagnostics(invalid_result, "cooked_package_mutation");
    result.native_backend_term_diagnostics =
        count_production_authoring_diagnostics(invalid_result, "native_backend_term");
    result.invalid_target_path_diagnostics =
        count_production_authoring_diagnostics(invalid_result, "invalid_target_path");
    result.diagnostics = reviewed_result.diagnostics.size();
    result.invoked_file_mutation = reviewed_result.invoked_file_mutation || invalid_result.invoked_file_mutation;
    result.invoked_package_io = reviewed_result.invoked_package_io || invalid_result.invoked_package_io;
    result.invoked_command_execution =
        reviewed_result.invoked_command_execution || invalid_result.invoked_command_execution;
    const auto repair_recipe_targets_production_smoke =
        reviewed_result.validation_repair_rows.size() == 1U &&
        std::ranges::find(reviewed_result.validation_repair_rows[0].validation_recipe_ids,
                          "installed-2d-production-authoring-workflows-smoke") !=
            reviewed_result.validation_repair_rows[0].validation_recipe_ids.end();
    result.ready = reviewed_result.succeeded() && !invalid_result.succeeded() && result.workflow_rows == 6U &&
                   result.accepted_rows == 6U && result.mutation_ledger_rows == 6U &&
                   result.validation_repair_rows == 1U && result.shared_surface_mutation_diagnostics == 1U &&
                   result.arbitrary_shell_diagnostics == 1U && result.cooked_package_mutation_diagnostics == 1U &&
                   result.native_backend_term_diagnostics == 1U && result.invalid_target_path_diagnostics == 1U &&
                   result.diagnostics == 0U && repair_recipe_targets_production_smoke &&
                   !result.invoked_file_mutation && !result.invoked_package_io && !result.invoked_command_execution;
    return result;
}

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/scene");
}

[[nodiscard]] mirakana::AssetId packaged_audio_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/jump-audio");
}

[[nodiscard]] mirakana::AssetId packaged_sprite_texture_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/player-sprite");
}

[[nodiscard]] mirakana::AssetId packaged_material_asset_id() {
    return mirakana::AssetId{11460315010553722633ULL};
}

[[nodiscard]] mirakana::AssetId packaged_sprite_animation_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/player-sprite-animation");
}

[[nodiscard]] mirakana::AssetId packaged_tilemap_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/tilemap");
}

[[nodiscard]] mirakana::AssetId packaged_ui_atlas_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/ui-atlas");
}

class Gameplay2DSystemsProbe final {
  public:
    Gameplay2DSystemsProbe() : physics_(mirakana::PhysicsWorld2DConfig{mirakana::Vec2{.x = 0.0F, .y = 0.0F}}) {}

    void start() {
        constexpr std::uint32_t character_layer = 1U << 0U;
        constexpr std::uint32_t solid_layer = 1U << 1U;
        constexpr std::uint32_t trigger_layer = 1U << 2U;

        actor_body_ = physics_.create_body(mirakana::PhysicsBody2DDesc{
            .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape2DKind::aabb,
            .radius = 0.5F,
            .collision_layer = character_layer,
            .collision_mask = solid_layer | trigger_layer,
        });
        solid_body_ = physics_.create_body(mirakana::PhysicsBody2DDesc{
            .position = mirakana::Vec2{.x = 0.75F, .y = 0.0F},
            .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec2{.x = 0.5F, .y = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape2DKind::aabb,
            .radius = 0.5F,
            .collision_layer = solid_layer,
            .collision_mask = character_layer,
        });
        trigger_body_ = physics_.create_body(mirakana::PhysicsBody2DDesc{
            .position = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .velocity = mirakana::Vec2{.x = 0.0F, .y = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec2{.x = 0.6F, .y = 0.6F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape2DKind::aabb,
            .radius = 0.5F,
            .collision_layer = trigger_layer,
            .collision_mask = character_layer,
            .trigger = true,
        });

        mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 2, .height = 1});
        const auto plan =
            mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                                .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                                .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                                                .mapping =
                                                                    mirakana::NavigationGridPointMapping{
                                                                        .origin = mirakana::NavigationPoint2{},
                                                                        .cell_size = 1.0F,
                                                                        .use_cell_centers = true,
                                                                    },
                                                                .adjacency = mirakana::NavigationAdjacency::cardinal4,
                                                                .smooth_path = true,
                                                            });
        navigation_plan_status_ = plan.status;
        navigation_plan_diagnostic_ = plan.diagnostic;
        navigation_path_point_count_ = plan.planned_grid_point_count;
        navigation_agent_ = plan.agent_state;
        if (!navigation_agent_.path.empty()) {
            navigation_goal_ = navigation_agent_.path.back();
        }

        const auto behavior_authoring = evaluate_gameplay_2d_behavior_authoring_readiness();
        behavior_authoring_readiness_status_ = behavior_authoring.status;
        behavior_authoring_readiness_diagnostic_ = behavior_authoring.primary_diagnostic;
        behavior_authoring_ready_ = behavior_authoring.status == mirakana::BehaviorAuthoringReadinessStatus::ready;
        behavior_authoring_diagnostics_ = behavior_authoring.validation_diagnostic_count;
        behavior_authoring_readiness_diagnostics_ = behavior_authoring.diagnostics.size();
        behavior_authoring_trace_nodes_ = behavior_authoring.trace_node_count;
        behavior_authoring_deterministic_trace_ready_ = behavior_authoring.deterministic_trace_ready;
        behavior_authoring_behavior_count_ = behavior_authoring.behavior_count;
        behavior_authoring_action_bindings_ = behavior_authoring.action_binding_count;
        behavior_authoring_blackboard_conditions_ = behavior_authoring.blackboard_condition_count;

        const auto quest_dialogue = validate_gameplay_2d_quest_dialogue();
        quest_dialogue_ready_ = quest_dialogue.ready;
        quest_dialogue_diagnostics_ = quest_dialogue.diagnostics;
        quest_dialogue_transition_rows_ = quest_dialogue.transition_rows;
        quest_dialogue_completed_objectives_ = quest_dialogue.completed_objectives;
        quest_dialogue_flags_ = quest_dialogue.flags;
        quest_dialogue_dialogue_nodes_ = quest_dialogue.dialogue_nodes;
        quest_dialogue_action_ids_ = quest_dialogue.action_ids;
        quest_dialogue_reward_ids_ = quest_dialogue.reward_ids;
        quest_dialogue_state_rows_ = quest_dialogue.state_rows;

        const auto inventory_items = validate_gameplay_2d_inventory_items();
        inventory_items_ready_ = inventory_items.ready;
        inventory_items_diagnostics_ = inventory_items.diagnostics;
        inventory_items_catalog_rows_ = inventory_items.catalog_rows;
        inventory_items_state_rows_ = inventory_items.state_rows;
        inventory_items_transition_rows_ = inventory_items.transition_rows;
        inventory_items_accepted_rows_ = inventory_items.accepted_rows;
        inventory_items_completed_rows_ = inventory_items.completed_rows;
        inventory_items_final_stacks_ = inventory_items.final_stacks;
        inventory_items_final_workbench_quantity_ = inventory_items.final_workbench_quantity;

        const auto interaction = validate_gameplay_2d_interactions();
        interaction_ready_ = interaction.ready;
        interaction_diagnostics_ = interaction.diagnostics;
        interaction_rows_ = interaction.rows;
        interaction_feedback_rows_ = interaction.feedback_rows;
        interaction_final_session_state_ = interaction.final_session_state;

        const auto sprite_collision_hitbox = validate_gameplay_2d_sprite_collision_hitboxes();
        sprite_collision_hitbox_ready_ = sprite_collision_hitbox.ready;
        sprite_collision_hitbox_hits_ = sprite_collision_hitbox.hits;
        sprite_collision_hitbox_gameplay_events_ = sprite_collision_hitbox.gameplay_events;
        sprite_collision_hitbox_interaction_rows_ = sprite_collision_hitbox.interaction_rows;
        sprite_collision_hitbox_feedback_rows_ = sprite_collision_hitbox.feedback_rows;
        sprite_collision_hitbox_diagnostics_ = sprite_collision_hitbox.diagnostics;

        const auto runtime_profile_resume = validate_gameplay_2d_runtime_profile_resume();
        runtime_profile_resume_ready_ = runtime_profile_resume.ready;
        runtime_profile_resume_status_ = runtime_profile_resume.status;
        runtime_profile_resume_diagnostics_ = runtime_profile_resume.diagnostics;
        runtime_profile_resume_loaded_documents_ = runtime_profile_resume.loaded_documents;
        runtime_profile_resume_defaulted_documents_ = runtime_profile_resume.defaulted_documents;
        runtime_profile_resume_save_schema_version_ = runtime_profile_resume.save_schema_version;
        runtime_profile_resume_settings_schema_version_ = runtime_profile_resume.settings_schema_version;

        const auto runtime_menu_hud = validate_gameplay_2d_runtime_menu_hud();
        runtime_menu_hud_ready_ = runtime_menu_hud.ready;
        runtime_menu_hud_diagnostics_ = runtime_menu_hud.diagnostics;
        runtime_menu_hud_display_rows_ = runtime_menu_hud.display_rows;
        runtime_menu_hud_command_rows_ = runtime_menu_hud.command_rows;
        runtime_menu_hud_dialogue_rows_ = runtime_menu_hud.dialogue_rows;
        runtime_menu_hud_input_binding_prompt_rows_ = runtime_menu_hud.input_binding_prompt_rows;

        const auto construction_placement = validate_gameplay_2d_construction_placement();
        construction_placement_ready_ = construction_placement.ready;
        construction_placement_diagnostics_ = construction_placement.diagnostics;
        construction_placement_validation_rows_ = construction_placement.validation_rows;
        construction_placement_intent_rows_ = construction_placement.intent_rows;
        construction_placement_intent_accepted_rows_ = construction_placement.intent_accepted_rows;
        construction_placement_intent_occupied_cells_ = construction_placement.intent_occupied_cells;

        const auto procedural_generation = validate_gameplay_2d_procedural_generation();
        procedural_generation_ready_ = procedural_generation.ready;
        procedural_generation_diagnostics_ = procedural_generation.diagnostics;
        procedural_generation_rows_ = procedural_generation.rows;
        procedural_generation_object_rows_ = procedural_generation.object_rows;
        procedural_generation_encounter_rows_ = procedural_generation.encounter_rows;
        procedural_generation_loot_rows_ = procedural_generation.loot_rows;
        procedural_generation_replay_hash_ = procedural_generation.replay_hash;
        procedural_generation_package_visible_rows_ = procedural_generation.package_visible_rows;
        procedural_generation_placement_intent_rows_ = procedural_generation.placement_intent_rows;
        procedural_generation_placement_intent_accepted_rows_ = procedural_generation.placement_intent_accepted_rows;

        started_ = true;
    }

    void tick() {
        if (!started_) {
            return;
        }

        physics_.apply_force(actor_body_, mirakana::Vec2{.x = 4.0F, .y = 0.0F});
        physics_.step(0.25F);
        const auto contacts = physics_.contacts();
        const auto overlaps = physics_.trigger_overlaps();
        physics_contact_count_ += contacts.size();
        physics_trigger_overlap_count_ += overlaps.size();
        physics_.resolve_contacts();
        ++physics_ticks_;

        update_ai_navigation_composition();
        ++ticks_;
    }

    [[nodiscard]] bool passed(std::uint32_t expected_ticks) const {
        return started_ && ticks_ == expected_ticks && physics_ticks_ == expected_ticks &&
               physics_.bodies().size() == 3U && actor_body_ != mirakana::null_physics_body_2d &&
               solid_body_ != mirakana::null_physics_body_2d && trigger_body_ != mirakana::null_physics_body_2d &&
               physics_contact_count_ > 0U && physics_trigger_overlap_count_ > 0U &&
               navigation_plan_status_ == mirakana::NavigationGridAgentPathStatus::ready &&
               navigation_plan_diagnostic_ == mirakana::NavigationGridAgentPathDiagnostic::none &&
               navigation_path_point_count_ == 2U &&
               navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination &&
               gameplay_2d_near(navigation_goal_.x, 1.5F) && gameplay_2d_near(navigation_goal_.y, 0.5F) &&
               gameplay_2d_near(navigation_agent_.position.x, 1.5F) &&
               gameplay_2d_near(navigation_agent_.position.y, 0.5F) &&
               last_perception_status_ == mirakana::AiPerceptionStatus::ready && last_perception_has_primary_target_ &&
               last_perception_target_count_ == 2U && last_perception_visible_count_ == 1U &&
               last_perception_audible_count_ == 1U &&
               perception_readiness_status_ == mirakana::AiPerceptionReadinessStatus::ready &&
               perception_readiness_diagnostic_ == mirakana::AiPerceptionReadinessDiagnostic::none &&
               perception_readiness_diagnostics_ == 0U && perception_stable_primary_target_ready_ &&
               perception_blackboard_projection_ready_ &&
               last_blackboard_status_ == mirakana::AiPerceptionBlackboardStatus::ready && blackboard_has_target_ &&
               blackboard_needs_move_ && last_tree_result_.status == mirakana::BehaviorTreeStatus::success &&
               last_tree_result_.visited_nodes.size() == 4U && behavior_authoring_ready_ &&
               behavior_authoring_readiness_status_ == mirakana::BehaviorAuthoringReadinessStatus::ready &&
               behavior_authoring_readiness_diagnostic_ == mirakana::BehaviorAuthoringReadinessDiagnostic::none &&
               behavior_authoring_diagnostics_ == 0U && behavior_authoring_readiness_diagnostics_ == 0U &&
               behavior_authoring_trace_nodes_ == 4U && behavior_authoring_deterministic_trace_ready_ &&
               behavior_authoring_behavior_count_ == 1U && behavior_authoring_action_bindings_ == 1U &&
               behavior_authoring_blackboard_conditions_ == 2U && quest_dialogue_ready_ &&
               quest_dialogue_diagnostics_ == 0U && quest_dialogue_transition_rows_ == 3U &&
               quest_dialogue_completed_objectives_ == 1U && quest_dialogue_flags_ == 1U &&
               quest_dialogue_dialogue_nodes_ == 1U && quest_dialogue_action_ids_ == 2U &&
               quest_dialogue_reward_ids_ == 2U && quest_dialogue_state_rows_ == 3U && inventory_items_ready_ &&
               inventory_items_diagnostics_ == 0U && inventory_items_catalog_rows_ == 2U &&
               inventory_items_state_rows_ == 2U && inventory_items_transition_rows_ == 2U &&
               inventory_items_accepted_rows_ == 1U && inventory_items_completed_rows_ == 1U &&
               inventory_items_final_stacks_ == 1U && inventory_items_final_workbench_quantity_ == 1U &&
               interaction_ready_ && interaction_diagnostics_ == 0U && interaction_rows_ == 10U &&
               interaction_feedback_rows_ == 10U &&
               interaction_final_session_state_ == mirakana::runtime::RuntimeGameplaySessionState::running &&
               sprite_collision_hitbox_ready_ && sprite_collision_hitbox_hits_ == 1U &&
               sprite_collision_hitbox_gameplay_events_ == 1U && sprite_collision_hitbox_interaction_rows_ == 1U &&
               sprite_collision_hitbox_feedback_rows_ == 1U && sprite_collision_hitbox_diagnostics_ == 0U &&
               runtime_profile_resume_ready_ && runtime_menu_hud_ready_ && runtime_menu_hud_diagnostics_ == 0U &&
               runtime_menu_hud_display_rows_ == 6U && runtime_menu_hud_command_rows_ == 2U &&
               runtime_menu_hud_dialogue_rows_ == 1U && runtime_menu_hud_input_binding_prompt_rows_ == 1U &&
               construction_placement_ready_ && construction_placement_diagnostics_ == 0U &&
               construction_placement_validation_rows_ == 3U && construction_placement_intent_rows_ == 1U &&
               construction_placement_intent_accepted_rows_ == 1U &&
               construction_placement_intent_occupied_cells_ == 2U && procedural_generation_ready_ &&
               procedural_generation_diagnostics_ == 0U && procedural_generation_rows_ == 3U &&
               procedural_generation_object_rows_ == 1U && procedural_generation_encounter_rows_ == 1U &&
               procedural_generation_loot_rows_ == 1U && procedural_generation_replay_hash_ != 0ULL &&
               procedural_generation_package_visible_rows_ == 1U &&
               procedural_generation_placement_intent_rows_ == 1U &&
               procedural_generation_placement_intent_accepted_rows_ == 1U;
    }

    [[nodiscard]] Gameplay2DSystemsStatus status(std::uint32_t expected_ticks) const {
        if (!started_) {
            return Gameplay2DSystemsStatus::not_started;
        }
        return passed(expected_ticks) ? Gameplay2DSystemsStatus::ready : Gameplay2DSystemsStatus::diagnostics;
    }

    [[nodiscard]] std::uint32_t ticks() const noexcept {
        return ticks_;
    }

    [[nodiscard]] std::uint32_t physics_ticks() const noexcept {
        return physics_ticks_;
    }

    [[nodiscard]] std::size_t physics_body_count() const noexcept {
        return physics_.bodies().size();
    }

    [[nodiscard]] std::size_t physics_contact_count() const noexcept {
        return physics_contact_count_;
    }

    [[nodiscard]] std::size_t physics_trigger_overlap_count() const noexcept {
        return physics_trigger_overlap_count_;
    }

    [[nodiscard]] std::size_t navigation_path_point_count() const noexcept {
        return navigation_path_point_count_;
    }

    [[nodiscard]] bool navigation_reached_destination() const noexcept {
        return navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination;
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathStatus navigation_plan_status() const noexcept {
        return navigation_plan_status_;
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathDiagnostic navigation_plan_diagnostic() const noexcept {
        return navigation_plan_diagnostic_;
    }

    [[nodiscard]] mirakana::NavigationAgentStatus navigation_agent_status() const noexcept {
        return navigation_agent_.status;
    }

    [[nodiscard]] mirakana::AiPerceptionStatus perception_status() const noexcept {
        return last_perception_status_;
    }

    [[nodiscard]] std::size_t perception_target_count() const noexcept {
        return last_perception_target_count_;
    }

    [[nodiscard]] bool perception_has_primary_target() const noexcept {
        return last_perception_has_primary_target_;
    }

    [[nodiscard]] std::size_t perception_visible_count() const noexcept {
        return last_perception_visible_count_;
    }

    [[nodiscard]] std::size_t perception_audible_count() const noexcept {
        return last_perception_audible_count_;
    }

    [[nodiscard]] mirakana::AiPerceptionReadinessStatus perception_readiness_status() const noexcept {
        return perception_readiness_status_;
    }

    [[nodiscard]] mirakana::AiPerceptionReadinessDiagnostic perception_readiness_diagnostic() const noexcept {
        return perception_readiness_diagnostic_;
    }

    [[nodiscard]] std::size_t perception_readiness_diagnostics() const noexcept {
        return perception_readiness_diagnostics_;
    }

    [[nodiscard]] bool perception_stable_primary_target_ready() const noexcept {
        return perception_stable_primary_target_ready_;
    }

    [[nodiscard]] bool perception_blackboard_projection_ready() const noexcept {
        return perception_blackboard_projection_ready_;
    }

    [[nodiscard]] mirakana::AiPerceptionBlackboardStatus blackboard_status() const noexcept {
        return last_blackboard_status_;
    }

    [[nodiscard]] bool blackboard_has_target() const noexcept {
        return blackboard_has_target_;
    }

    [[nodiscard]] bool blackboard_needs_move() const noexcept {
        return blackboard_needs_move_;
    }

    [[nodiscard]] mirakana::BehaviorTreeStatus behavior_status() const noexcept {
        return last_tree_result_.status;
    }

    [[nodiscard]] std::size_t behavior_visited_node_count() const noexcept {
        return last_tree_result_.visited_nodes.size();
    }

    [[nodiscard]] bool behavior_authoring_ready() const noexcept {
        return behavior_authoring_ready_;
    }

    [[nodiscard]] mirakana::BehaviorAuthoringReadinessStatus behavior_authoring_readiness_status() const noexcept {
        return behavior_authoring_readiness_status_;
    }

    [[nodiscard]] mirakana::BehaviorAuthoringReadinessDiagnostic
    behavior_authoring_readiness_diagnostic() const noexcept {
        return behavior_authoring_readiness_diagnostic_;
    }

    [[nodiscard]] std::size_t behavior_authoring_diagnostic_count() const noexcept {
        return behavior_authoring_diagnostics_;
    }

    [[nodiscard]] std::size_t behavior_authoring_readiness_diagnostic_count() const noexcept {
        return behavior_authoring_readiness_diagnostics_;
    }

    [[nodiscard]] std::size_t behavior_authoring_trace_node_count() const noexcept {
        return behavior_authoring_trace_nodes_;
    }

    [[nodiscard]] bool behavior_authoring_deterministic_trace_ready() const noexcept {
        return behavior_authoring_deterministic_trace_ready_;
    }

    [[nodiscard]] std::size_t behavior_authoring_behavior_count() const noexcept {
        return behavior_authoring_behavior_count_;
    }

    [[nodiscard]] std::size_t behavior_authoring_action_binding_count() const noexcept {
        return behavior_authoring_action_bindings_;
    }

    [[nodiscard]] std::size_t behavior_authoring_blackboard_condition_count() const noexcept {
        return behavior_authoring_blackboard_conditions_;
    }

    [[nodiscard]] bool quest_dialogue_ready() const noexcept {
        return quest_dialogue_ready_;
    }

    [[nodiscard]] std::size_t quest_dialogue_diagnostic_count() const noexcept {
        return quest_dialogue_diagnostics_;
    }

    [[nodiscard]] std::size_t quest_dialogue_transition_row_count() const noexcept {
        return quest_dialogue_transition_rows_;
    }

    [[nodiscard]] std::size_t quest_dialogue_completed_objective_count() const noexcept {
        return quest_dialogue_completed_objectives_;
    }

    [[nodiscard]] std::size_t quest_dialogue_flag_count() const noexcept {
        return quest_dialogue_flags_;
    }

    [[nodiscard]] std::size_t quest_dialogue_dialogue_node_count() const noexcept {
        return quest_dialogue_dialogue_nodes_;
    }

    [[nodiscard]] std::size_t quest_dialogue_action_id_count() const noexcept {
        return quest_dialogue_action_ids_;
    }

    [[nodiscard]] std::size_t quest_dialogue_reward_id_count() const noexcept {
        return quest_dialogue_reward_ids_;
    }

    [[nodiscard]] std::size_t quest_dialogue_state_row_count() const noexcept {
        return quest_dialogue_state_rows_;
    }

    [[nodiscard]] bool inventory_items_ready() const noexcept {
        return inventory_items_ready_;
    }

    [[nodiscard]] std::size_t inventory_items_diagnostic_count() const noexcept {
        return inventory_items_diagnostics_;
    }

    [[nodiscard]] std::size_t inventory_items_catalog_row_count() const noexcept {
        return inventory_items_catalog_rows_;
    }

    [[nodiscard]] std::size_t inventory_items_state_row_count() const noexcept {
        return inventory_items_state_rows_;
    }

    [[nodiscard]] std::size_t inventory_items_transition_row_count() const noexcept {
        return inventory_items_transition_rows_;
    }

    [[nodiscard]] std::size_t inventory_items_accepted_row_count() const noexcept {
        return inventory_items_accepted_rows_;
    }

    [[nodiscard]] std::size_t inventory_items_completed_row_count() const noexcept {
        return inventory_items_completed_rows_;
    }

    [[nodiscard]] std::size_t inventory_items_final_stack_count() const noexcept {
        return inventory_items_final_stacks_;
    }

    [[nodiscard]] std::uint32_t inventory_items_final_workbench_quantity() const noexcept {
        return inventory_items_final_workbench_quantity_;
    }

    [[nodiscard]] bool interaction_ready() const noexcept {
        return interaction_ready_;
    }

    [[nodiscard]] std::size_t interaction_diagnostic_count() const noexcept {
        return interaction_diagnostics_;
    }

    [[nodiscard]] std::size_t interaction_row_count() const noexcept {
        return interaction_rows_;
    }

    [[nodiscard]] std::size_t interaction_feedback_row_count() const noexcept {
        return interaction_feedback_rows_;
    }

    [[nodiscard]] mirakana::runtime::RuntimeGameplaySessionState interaction_final_session_state() const noexcept {
        return interaction_final_session_state_;
    }

    [[nodiscard]] bool sprite_collision_hitbox_ready() const noexcept {
        return sprite_collision_hitbox_ready_;
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_hit_count() const noexcept {
        return sprite_collision_hitbox_hits_;
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_gameplay_event_count() const noexcept {
        return sprite_collision_hitbox_gameplay_events_;
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_interaction_row_count() const noexcept {
        return sprite_collision_hitbox_interaction_rows_;
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_feedback_row_count() const noexcept {
        return sprite_collision_hitbox_feedback_rows_;
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_diagnostic_count() const noexcept {
        return sprite_collision_hitbox_diagnostics_;
    }

    [[nodiscard]] bool runtime_profile_resume_ready() const noexcept {
        return runtime_profile_resume_ready_;
    }

    [[nodiscard]] mirakana::runtime::RuntimeSessionProfileResumeStatus runtime_profile_resume_status() const noexcept {
        return runtime_profile_resume_status_;
    }

    [[nodiscard]] std::size_t runtime_profile_resume_diagnostic_count() const noexcept {
        return runtime_profile_resume_diagnostics_;
    }

    [[nodiscard]] std::size_t runtime_profile_resume_loaded_document_count() const noexcept {
        return runtime_profile_resume_loaded_documents_;
    }

    [[nodiscard]] std::size_t runtime_profile_resume_defaulted_document_count() const noexcept {
        return runtime_profile_resume_defaulted_documents_;
    }

    [[nodiscard]] std::uint32_t runtime_profile_resume_save_schema_version() const noexcept {
        return runtime_profile_resume_save_schema_version_;
    }

    [[nodiscard]] std::uint32_t runtime_profile_resume_settings_schema_version() const noexcept {
        return runtime_profile_resume_settings_schema_version_;
    }

    [[nodiscard]] bool runtime_menu_hud_ready() const noexcept {
        return runtime_menu_hud_ready_;
    }

    [[nodiscard]] std::size_t runtime_menu_hud_diagnostic_count() const noexcept {
        return runtime_menu_hud_diagnostics_;
    }

    [[nodiscard]] std::size_t runtime_menu_hud_display_row_count() const noexcept {
        return runtime_menu_hud_display_rows_;
    }

    [[nodiscard]] std::size_t runtime_menu_hud_command_row_count() const noexcept {
        return runtime_menu_hud_command_rows_;
    }

    [[nodiscard]] std::size_t runtime_menu_hud_dialogue_row_count() const noexcept {
        return runtime_menu_hud_dialogue_rows_;
    }

    [[nodiscard]] std::size_t runtime_menu_hud_input_binding_prompt_row_count() const noexcept {
        return runtime_menu_hud_input_binding_prompt_rows_;
    }

    [[nodiscard]] bool construction_placement_ready() const noexcept {
        return construction_placement_ready_;
    }

    [[nodiscard]] std::size_t construction_placement_diagnostic_count() const noexcept {
        return construction_placement_diagnostics_;
    }

    [[nodiscard]] std::size_t construction_placement_validation_row_count() const noexcept {
        return construction_placement_validation_rows_;
    }

    [[nodiscard]] std::size_t construction_placement_intent_row_count() const noexcept {
        return construction_placement_intent_rows_;
    }

    [[nodiscard]] std::size_t construction_placement_intent_accepted_row_count() const noexcept {
        return construction_placement_intent_accepted_rows_;
    }

    [[nodiscard]] std::size_t construction_placement_intent_occupied_cell_count() const noexcept {
        return construction_placement_intent_occupied_cells_;
    }

    [[nodiscard]] bool procedural_generation_ready() const noexcept {
        return procedural_generation_ready_;
    }

    [[nodiscard]] std::size_t procedural_generation_diagnostic_count() const noexcept {
        return procedural_generation_diagnostics_;
    }

    [[nodiscard]] std::size_t procedural_generation_row_count() const noexcept {
        return procedural_generation_rows_;
    }

    [[nodiscard]] std::size_t procedural_generation_object_row_count() const noexcept {
        return procedural_generation_object_rows_;
    }

    [[nodiscard]] std::size_t procedural_generation_encounter_row_count() const noexcept {
        return procedural_generation_encounter_rows_;
    }

    [[nodiscard]] std::size_t procedural_generation_loot_row_count() const noexcept {
        return procedural_generation_loot_rows_;
    }

    [[nodiscard]] std::uint64_t procedural_generation_replay_hash() const noexcept {
        return procedural_generation_replay_hash_;
    }

    [[nodiscard]] std::size_t procedural_generation_package_visible_row_count() const noexcept {
        return procedural_generation_package_visible_rows_;
    }

    [[nodiscard]] std::size_t procedural_generation_placement_intent_row_count() const noexcept {
        return procedural_generation_placement_intent_rows_;
    }

    [[nodiscard]] std::size_t procedural_generation_placement_intent_accepted_row_count() const noexcept {
        return procedural_generation_placement_intent_accepted_rows_;
    }

  private:
    void update_ai_navigation_composition() {
        if (navigation_plan_status_ != mirakana::NavigationGridAgentPathStatus::ready ||
            navigation_agent_.path.empty()) {
            return;
        }

        const std::vector<mirakana::AiPerceptionTarget2D> route_targets{
            mirakana::AiPerceptionTarget2D{
                .id = 1U,
                .position = mirakana::AiPerceptionPoint2{.x = navigation_agent_.path.back().x,
                                                         .y = navigation_agent_.path.back().y},
                .radius = 0.0F,
                .sight_enabled = true,
                .hearing_enabled = false,
                .sound_radius = 0.0F,
            },
            mirakana::AiPerceptionTarget2D{
                .id = 2U,
                .position = mirakana::AiPerceptionPoint2{.x = navigation_agent_.path.back().x + 3.0F,
                                                         .y = navigation_agent_.path.back().y},
                .radius = 0.0F,
                .sight_enabled = false,
                .hearing_enabled = true,
                .sound_radius = 1.0F,
            },
        };
        const auto perception_request = mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 100U,
                                                   .position =
                                                       mirakana::AiPerceptionPoint2{.x = navigation_agent_.position.x,
                                                                                    .y = navigation_agent_.position.y},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 4.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 4.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{route_targets},
        };
        const auto perception_keys = mirakana::AiPerceptionBlackboardKeys{
            .has_target_key = kGameplay2dHasTargetKey,
            .target_id_key = kGameplay2dTargetIdKey,
            .target_distance_key = kGameplay2dTargetDistanceKey,
            .visible_count_key = kGameplay2dVisibleTargetsKey,
            .audible_count_key = kGameplay2dAudibleTargetsKey,
            .target_state_key = kGameplay2dTargetStateKey,
        };
        const auto perception = mirakana::build_ai_perception_snapshot_2d(perception_request);
        const auto perception_readiness =
            mirakana::evaluate_ai_perception_readiness_2d(perception_request, perception_keys,
                                                          mirakana::AiPerceptionReadinessConfig{
                                                              .require_visible_target = true,
                                                              .require_audible_target = true,
                                                              .min_targets = 2U,
                                                              .max_targets = 2U,
                                                          });
        last_perception_status_ = perception.status;
        last_perception_target_count_ = perception.targets.size();
        last_perception_has_primary_target_ = perception.has_primary_target;
        last_perception_visible_count_ = perception.visible_count;
        last_perception_audible_count_ = perception.audible_count;
        perception_readiness_status_ = perception_readiness.status;
        perception_readiness_diagnostic_ = perception_readiness.diagnostic;
        perception_readiness_diagnostics_ = perception_readiness.diagnostics.size();
        perception_stable_primary_target_ready_ = perception_readiness.stable_primary_target_ready;
        perception_blackboard_projection_ready_ = perception_readiness.blackboard_projection_ready;

        mirakana::BehaviorTreeBlackboard blackboard;
        const auto blackboard_result =
            mirakana::write_ai_perception_blackboard(perception, perception_keys, blackboard);
        last_blackboard_status_ = blackboard_result.status;
        if (blackboard_result.status != mirakana::AiPerceptionBlackboardStatus::ready ||
            !blackboard.set(kGameplay2dNeedsMoveKey, mirakana::make_behavior_tree_blackboard_bool(true))) {
            return;
        }

        if (const auto* value = blackboard.find(kGameplay2dHasTargetKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::boolean) {
            blackboard_has_target_ = value->bool_value;
        }
        if (const auto* value = blackboard.find(kGameplay2dNeedsMoveKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::boolean) {
            blackboard_needs_move_ = value->bool_value;
        }

        const auto conditions = gameplay_2d_behavior_conditions();
        const auto supporting_systems_ready =
            physics_.bodies().size() == 3U && physics_contact_count_ > 0U && physics_trigger_overlap_count_ > 0U;
        const std::vector<mirakana::BehaviorTreeLeafResult> leaf_results{
            mirakana::BehaviorTreeLeafResult{.node_id = kGameplay2dMoveActionNode,
                                             .status = supporting_systems_ready
                                                           ? mirakana::BehaviorTreeStatus::success
                                                           : mirakana::BehaviorTreeStatus::failure},
        };

        last_tree_result_ = mirakana::evaluate_behavior_tree(
            gameplay_2d_behavior_tree_desc(),
            mirakana::BehaviorTreeEvaluationContext{
                .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{leaf_results},
                .blackboard_entries = blackboard.entries(),
                .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
            });

        if (last_tree_result_.status != mirakana::BehaviorTreeStatus::success ||
            navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination) {
            return;
        }

        const auto update = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
            .state = navigation_agent_,
            .config =
                mirakana::NavigationAgentConfig{.max_speed = 8.0F, .slowing_radius = 1.0F, .arrival_radius = 0.001F},
            .delta_seconds = 1.0F,
        });
        navigation_agent_ = update.state;
    }

    mirakana::PhysicsWorld2D physics_;
    mirakana::NavigationAgentState navigation_agent_;
    mirakana::BehaviorTreeTickResult last_tree_result_;
    mirakana::PhysicsBody2DId actor_body_{mirakana::null_physics_body_2d};
    mirakana::PhysicsBody2DId solid_body_{mirakana::null_physics_body_2d};
    mirakana::PhysicsBody2DId trigger_body_{mirakana::null_physics_body_2d};
    mirakana::NavigationGridAgentPathStatus navigation_plan_status_{
        mirakana::NavigationGridAgentPathStatus::invalid_request};
    mirakana::NavigationGridAgentPathDiagnostic navigation_plan_diagnostic_{
        mirakana::NavigationGridAgentPathDiagnostic::none};
    mirakana::NavigationPoint2 navigation_goal_{};
    mirakana::AiPerceptionStatus last_perception_status_{mirakana::AiPerceptionStatus::invalid_agent};
    mirakana::AiPerceptionBlackboardStatus last_blackboard_status_{
        mirakana::AiPerceptionBlackboardStatus::invalid_snapshot};
    mirakana::AiPerceptionReadinessStatus perception_readiness_status_{
        mirakana::AiPerceptionReadinessStatus::invalid_snapshot};
    mirakana::AiPerceptionReadinessDiagnostic perception_readiness_diagnostic_{
        mirakana::AiPerceptionReadinessDiagnostic::none};
    mirakana::BehaviorAuthoringReadinessStatus behavior_authoring_readiness_status_{
        mirakana::BehaviorAuthoringReadinessStatus::diagnostics};
    mirakana::BehaviorAuthoringReadinessDiagnostic behavior_authoring_readiness_diagnostic_{
        mirakana::BehaviorAuthoringReadinessDiagnostic::none};
    std::size_t physics_contact_count_{0U};
    std::size_t physics_trigger_overlap_count_{0U};
    std::size_t navigation_path_point_count_{0U};
    std::size_t last_perception_target_count_{0U};
    std::size_t last_perception_visible_count_{0U};
    std::size_t last_perception_audible_count_{0U};
    std::size_t perception_readiness_diagnostics_{0U};
    std::size_t behavior_authoring_diagnostics_{0U};
    std::size_t behavior_authoring_readiness_diagnostics_{0U};
    std::size_t behavior_authoring_trace_nodes_{0U};
    std::size_t behavior_authoring_behavior_count_{0U};
    std::size_t behavior_authoring_action_bindings_{0U};
    std::size_t behavior_authoring_blackboard_conditions_{0U};
    std::size_t quest_dialogue_diagnostics_{0U};
    std::size_t quest_dialogue_transition_rows_{0U};
    std::size_t quest_dialogue_completed_objectives_{0U};
    std::size_t quest_dialogue_flags_{0U};
    std::size_t quest_dialogue_dialogue_nodes_{0U};
    std::size_t quest_dialogue_action_ids_{0U};
    std::size_t quest_dialogue_reward_ids_{0U};
    std::size_t quest_dialogue_state_rows_{0U};
    std::size_t inventory_items_diagnostics_{0U};
    std::size_t inventory_items_catalog_rows_{0U};
    std::size_t inventory_items_state_rows_{0U};
    std::size_t inventory_items_transition_rows_{0U};
    std::size_t inventory_items_accepted_rows_{0U};
    std::size_t inventory_items_completed_rows_{0U};
    std::size_t inventory_items_final_stacks_{0U};
    std::size_t interaction_diagnostics_{0U};
    std::size_t interaction_rows_{0U};
    std::size_t interaction_feedback_rows_{0U};
    std::size_t sprite_collision_hitbox_hits_{0U};
    std::size_t sprite_collision_hitbox_gameplay_events_{0U};
    std::size_t sprite_collision_hitbox_interaction_rows_{0U};
    std::size_t sprite_collision_hitbox_feedback_rows_{0U};
    std::size_t sprite_collision_hitbox_diagnostics_{0U};
    std::size_t runtime_profile_resume_diagnostics_{0U};
    std::size_t runtime_profile_resume_loaded_documents_{0U};
    std::size_t runtime_profile_resume_defaulted_documents_{0U};
    std::size_t runtime_menu_hud_diagnostics_{0U};
    std::size_t runtime_menu_hud_display_rows_{0U};
    std::size_t runtime_menu_hud_command_rows_{0U};
    std::size_t runtime_menu_hud_dialogue_rows_{0U};
    std::size_t runtime_menu_hud_input_binding_prompt_rows_{0U};
    std::size_t construction_placement_diagnostics_{0U};
    std::size_t construction_placement_validation_rows_{0U};
    std::size_t construction_placement_intent_rows_{0U};
    std::size_t construction_placement_intent_accepted_rows_{0U};
    std::size_t construction_placement_intent_occupied_cells_{0U};
    std::size_t procedural_generation_diagnostics_{0U};
    std::size_t procedural_generation_rows_{0U};
    std::size_t procedural_generation_object_rows_{0U};
    std::size_t procedural_generation_encounter_rows_{0U};
    std::size_t procedural_generation_loot_rows_{0U};
    std::size_t procedural_generation_package_visible_rows_{0U};
    std::size_t procedural_generation_placement_intent_rows_{0U};
    std::size_t procedural_generation_placement_intent_accepted_rows_{0U};
    std::uint64_t procedural_generation_replay_hash_{0ULL};
    std::uint32_t inventory_items_final_workbench_quantity_{0U};
    std::uint32_t runtime_profile_resume_save_schema_version_{0U};
    std::uint32_t runtime_profile_resume_settings_schema_version_{0U};
    mirakana::runtime::RuntimeGameplaySessionState interaction_final_session_state_{
        mirakana::runtime::RuntimeGameplaySessionState::running};
    mirakana::runtime::RuntimeSessionProfileResumeStatus runtime_profile_resume_status_{
        mirakana::runtime::RuntimeSessionProfileResumeStatus::blocked};
    std::uint32_t ticks_{0U};
    std::uint32_t physics_ticks_{0U};
    bool last_perception_has_primary_target_{false};
    bool perception_stable_primary_target_ready_{false};
    bool perception_blackboard_projection_ready_{false};
    bool blackboard_has_target_{false};
    bool blackboard_needs_move_{false};
    bool behavior_authoring_ready_{false};
    bool behavior_authoring_deterministic_trace_ready_{false};
    bool quest_dialogue_ready_{false};
    bool inventory_items_ready_{false};
    bool interaction_ready_{false};
    bool sprite_collision_hitbox_ready_{false};
    bool runtime_profile_resume_ready_{false};
    bool runtime_menu_hud_ready_{false};
    bool construction_placement_ready_{false};
    bool procedural_generation_ready_{false};
    bool started_{false};
};

class Sample2DDesktopRuntimePackageGame final : public mirakana::GameApp {
  public:
    Sample2DDesktopRuntimePackageGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                      mirakana::RuntimeSceneRenderInstance scene,
                                      mirakana::AudioClipSampleData audio_samples,
                                      mirakana::runtime::RuntimeSpriteAnimationPayload sprite_animation,
                                      mirakana::runtime::RuntimeTilemapPayload tilemap)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          audio_samples_(std::move(audio_samples)), sprite_animation_(std::move(sprite_animation)),
          tilemap_(std::move(tilemap)) {
        sprite_flipbook_clips_.push_back(
            mirakana::RuntimeSpriteFlipbookClipDesc{.name = "idle.east",
                                                    .first_frame_index = 0,
                                                    .frame_count = sprite_animation_.frames.size(),
                                                    .loop = sprite_animation_.loop});
        sprite_flipbook_clips_.push_back(
            mirakana::RuntimeSpriteFlipbookClipDesc{.name = "jump.east",
                                                    .first_frame_index = 0,
                                                    .frame_count = sprite_animation_.frames.size(),
                                                    .loop = false});
        sprite_flipbook_direction_sets_.push_back(mirakana::RuntimeSpriteFlipbookDirectionSetRow{
            .gameplay_state = "idle",
            .direction = "east",
            .clip_name = "idle.east",
            .playback_mode = mirakana::RuntimeSpriteFlipbookPlaybackMode::loop,
        });
        sprite_flipbook_direction_sets_.push_back(mirakana::RuntimeSpriteFlipbookDirectionSetRow{
            .gameplay_state = "jump",
            .direction = "east",
            .clip_name = "jump.east",
            .playback_mode = mirakana::RuntimeSpriteFlipbookPlaybackMode::clamp_to_end,
        });
        sprite_flipbook_events_.push_back(mirakana::RuntimeSpriteFlipbookEventDesc{
            .clip_name = "idle.east",
            .frame_index = 1,
            .name = "footstep",
        });
        sprite_flipbook_events_.push_back(mirakana::RuntimeSpriteFlipbookEventDesc{
            .clip_name = "jump.east",
            .frame_index = 1,
            .name = "landing",
        });
        sprite_flipbook_state_.clip_name = "idle.east";
    }

    void on_start(mirakana::EngineContext&) override {
        actions_.bind_key_axis("move_x", mirakana::Key::left, mirakana::Key::right);
        actions_.bind_key("jump", mirakana::Key::space);
        input_.press(mirakana::Key::right);
        input_.press(mirakana::Key::space);
        gameplay_systems_.start();

        if (scene_.scene.has_value()) {
            const auto validation = mirakana::validate_playable_2d_scene(*scene_.scene);
            validation_ok_ = validation.succeeded();
            package_scene_sprites_ = validation.visible_sprite_count;
            scene_gameplay_bindings_ = validate_sample_2d_scene_gameplay_bindings(*scene_.scene);

            std::int32_t next_sorting_layer = 0;
            std::size_t slice_config_index = 0;
            for (const auto& node : scene_.scene->nodes()) {
                if (!node.components.sprite_renderer.has_value() || !node.components.sprite_renderer->visible) {
                    continue;
                }
                auto components = node.components;
                components.sprite_renderer->sorting_layer = next_sorting_layer;
                ++next_sorting_layer;
                if (slice_config_index == 0U) {
                    components.sprite_renderer->draw_mode = mirakana::SpriteDrawMode::nine_slice;
                    components.sprite_renderer->slice_border =
                        mirakana::SpriteSliceBorder{.left = 0.25F, .bottom = 0.25F, .right = 0.25F, .top = 0.25F};
                    components.sprite_renderer->size = mirakana::Vec2{.x = 4.0F, .y = 3.0F};
                } else if (slice_config_index == 1U) {
                    components.sprite_renderer->draw_mode = mirakana::SpriteDrawMode::tiled;
                    components.sprite_renderer->slice_border =
                        mirakana::SpriteSliceBorder{.left = 0.25F, .bottom = 0.25F, .right = 0.25F, .top = 0.25F};
                    components.sprite_renderer->tile_size = mirakana::Vec2{.x = 1.0F, .y = 1.0F};
                    components.sprite_renderer->size = mirakana::Vec2{.x = 6.0F, .y = 4.0F};
                }
                ++slice_config_index;
                scene_.scene->set_components(node.id, components);
            }
        }

        const auto tilemap_sample = mirakana::runtime::sample_runtime_tilemap_visible_cells(tilemap_);
        tilemap_runtime_ok_ = tilemap_sample.succeeded;
        tilemap_layers_ = tilemap_sample.layer_count;
        tilemap_visible_layers_ = tilemap_sample.visible_layer_count;
        tilemap_tiles_ = tilemap_sample.tile_count;
        tilemap_non_empty_cells_ = tilemap_sample.non_empty_cell_count;
        tilemap_sampled_cells_ = tilemap_sample.sampled_cell_count;
        tilemap_diagnostics_ = tilemap_sample.diagnostic_count;

        ui_ok_ = build_hud();
        theme_.add(mirakana::UiThemeColor{.token = "hud.panel",
                                          .color = mirakana::Color{.r = 0.06F, .g = 0.08F, .b = 0.09F, .a = 1.0F}});

        audio_clip_registered_ = mixer_.register_clip(mirakana::AudioClipDesc{
            .clip = audio_samples_.clip,
            .sample_rate = audio_samples_.format.sample_rate,
            .channel_count = audio_samples_.format.channel_count,
            .frame_count = audio_samples_.frame_count,
            .sample_format = mirakana::AudioSampleFormat::float32,
            .streaming = false,
            .buffered_frame_count = audio_samples_.frame_count,
        });
        audio_gameplay_mixer_ = validate_audio_gameplay_mixer_package_evidence(audio_samples_);

        renderer_.set_clear_color(mirakana::Color{.r = 0.02F, .g = 0.03F, .b = 0.04F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        if (!scene_.scene.has_value()) {
            return false;
        }
        gameplay_systems_.tick();

        const mirakana::runtime::RuntimeInputStateView input_state{
            .keyboard = &input_,
            .pointer = nullptr,
            .gamepad = nullptr,
        };

        auto* player = scene_.scene->find_node(kPlayerNode);
        if (player == nullptr) {
            return false;
        }

        const auto move_x = actions_.axis_value("move_x", input_state);
        const auto jump_requested = actions_.action_down("jump", input_state);
        player->transform.position.x += move_x;
        if (jump_voice_ == mirakana::null_audio_voice && jump_requested) {
            jump_voice_ = mixer_.play(
                mirakana::AudioVoiceDesc{.clip = audio_samples_.clip, .bus = "master", .gain = 1.0F, .looping = false});
        }

        const mirakana::RuntimeSpriteFlipbookDesc flipbook_desc{
            .frames = std::span<const mirakana::runtime::RuntimeSpriteAnimationFrame>{sprite_animation_.frames},
            .clips = std::span<const mirakana::RuntimeSpriteFlipbookClipDesc>{sprite_flipbook_clips_},
        };
        const std::string gameplay_state = jump_requested ? "jump" : "idle";
        const std::string direction = move_x >= 0.0F ? "east" : "west";
        const mirakana::RuntimeSpriteFlipbookPlaybackRequest playback_request{
            .gameplay_state = gameplay_state,
            .direction = direction,
            .delta_seconds = 0.25F,
            .direction_sets =
                std::span<const mirakana::RuntimeSpriteFlipbookDirectionSetRow>{sprite_flipbook_direction_sets_},
            .events = std::span<const mirakana::RuntimeSpriteFlipbookEventDesc>{sprite_flipbook_events_},
        };
        const auto playback_result =
            mirakana::advance_runtime_sprite_flipbook_playback(sprite_flipbook_state_, flipbook_desc, playback_request);
        const auto& flipbook_result = playback_result.sample;
        ++sprite_flipbook_ticks_;
        sprite_flipbook_ok_ = sprite_flipbook_ok_ && playback_result.succeeded && flipbook_result.succeeded;
        sprite_flipbook_frames_sampled_ += flipbook_result.sampled_frame_count;
        sprite_flipbook_selected_frame_sum_ += flipbook_result.selected_frame_index;
        sprite_flipbook_events_sampled_ += playback_result.events.size();
        if (!playback_result.succeeded || !flipbook_result.succeeded) {
            sprite_animation_ok_ = false;
            ++sprite_flipbook_diagnostics_;
            ++sprite_animation_diagnostics_;
        } else {
            mirakana::runtime::RuntimeSpriteAnimationPayload sampled_animation;
            sampled_animation.asset = sprite_animation_.asset;
            sampled_animation.handle = sprite_animation_.handle;
            sampled_animation.target_node = sprite_animation_.target_node;
            sampled_animation.loop = false;
            sampled_animation.frames.push_back(flipbook_result.frame);

            const auto animation_result =
                mirakana::sample_and_apply_runtime_scene_render_sprite_animation(scene_, sampled_animation, 0.0F);
            sprite_animation_ok_ = sprite_animation_ok_ && animation_result.succeeded;
            sprite_animation_frames_sampled_ += animation_result.sampled_frame_count;
            sprite_animation_frames_applied_ += animation_result.applied_frame_count;
            sprite_animation_selected_frame_sum_ += flipbook_result.selected_frame_index;
            sprite_flipbook_frames_applied_ += animation_result.applied_frame_count;
            if (!animation_result.succeeded) {
                sprite_flipbook_ok_ = false;
                ++sprite_flipbook_diagnostics_;
                ++sprite_animation_diagnostics_;
            }
        }

        const auto validation = mirakana::validate_playable_2d_scene(*scene_.scene);
        validation_ok_ = validation_ok_ && validation.succeeded();

        renderer_.begin_frame();
        const auto packet = mirakana::build_scene_render_packet(*scene_.scene);
        auto sorted_sprites = packet.sprites;
        const auto sort_stats = mirakana::sort_scene_render_sprites(sorted_sprites);
        sprite_sort_layers_applied_ = std::max(sort_stats.sorting_layers_applied, sprite_sort_layers_applied_);
        sprite_sorted_draws_ += sort_stats.sorted_draws;
        mirakana::SceneSpriteExpansionStats sprite_expansion_stats{};
        const auto batch_plan = mirakana::plan_scene_sprite_batches(packet, &sprite_expansion_stats);
        const auto scene_submit = mirakana::submit_scene_render_packet(
            renderer_, packet, mirakana::SceneRenderSubmitDesc{.material_palette = &scene_.material_palette});
        sprite_batch_plan_ok_ = sprite_batch_plan_ok_ && batch_plan.succeeded();
        sprite_batch_plan_sprites_ += batch_plan.sprite_count;
        sprite_batch_plan_textured_sprites_ += batch_plan.textured_sprite_count;
        sprite_batch_plan_draws_ += batch_plan.draw_count;
        sprite_batch_plan_texture_binds_ += batch_plan.texture_bind_count;
        sprite_batch_plan_diagnostics_ += batch_plan.diagnostics.size();
        scene_sprite_sources_submitted_ += sprite_expansion_stats.source_sprite_count;
        scene_sprites_submitted_ += scene_submit.sprites_submitted;
        const auto& sprite_expansion = scene_submit.sprite_expansion;
        sprite_9slice_tiled_source_sprites_ += sprite_expansion.source_sprite_count;
        const auto nine_slice_expanded_quads = sprite_expansion.nine_slice_count * 9U;
        const auto non_sliced_source_sprites =
            sprite_expansion.source_sprite_count > (sprite_expansion.nine_slice_count + sprite_expansion.tiled_count)
                ? sprite_expansion.source_sprite_count -
                      (sprite_expansion.nine_slice_count + sprite_expansion.tiled_count)
                : 0U;
        const auto tiled_expanded_quads =
            sprite_expansion.expanded_sprite_count > (non_sliced_source_sprites + nine_slice_expanded_quads)
                ? sprite_expansion.expanded_sprite_count - non_sliced_source_sprites - nine_slice_expanded_quads
                : 0U;
        sprite_9slice_expanded_quads_ += nine_slice_expanded_quads;
        sprite_tiled_expanded_quads_ += tiled_expanded_quads;
        sprite_batch_plan_atlas_backed_batches_ += batch_plan.atlas_backed_batch_count;
        sprite_batch_plan_repeated_atlas_batches_ += batch_plan.repeated_atlas_batch_count;
        sprite_batch_plan_repeated_atlas_sprites_ += batch_plan.repeated_atlas_sprite_count;
        primary_camera_seen_ = primary_camera_seen_ || scene_submit.has_primary_camera;

        const auto effect_batch_plan = update_sprite_effect_particles();

        update_hud_text();
        const auto layout =
            mirakana::ui::solve_layout(hud_, mirakana::ui::ElementId{"hud.root"},
                                       mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 180.0F});
        const auto submission = mirakana::ui::build_renderer_submission(hud_, layout);
        const auto ui_submit_desc = mirakana::UiRenderSubmitDesc{.theme = &theme_};
        std::vector<mirakana::SpriteCommand> ui_batch_sprites;
        ui_batch_sprites.reserve(submission.boxes.size());
        for (const auto& box : submission.boxes) {
            if (!box.background_token.empty()) {
                ui_batch_sprites.push_back(mirakana::make_ui_box_sprite_command(box, ui_submit_desc));
            }
        }
        const auto ui_batch_plan = mirakana::plan_sprite_batches(ui_batch_sprites);
        const auto hud_submit = mirakana::submit_ui_renderer_submission(renderer_, submission, ui_submit_desc);
        hud_boxes_submitted_ += hud_submit.boxes_submitted;
        record_sprite_batch_budget_profile(batch_plan, ui_batch_plan, effect_batch_plan);
        renderer_.end_frame();

        const auto audio = mixer_.render_interleaved_float(
            mirakana::AudioRenderRequest{
                .format = audio_samples_.format,
                .frame_count = 4,
                .device_frame = static_cast<std::uint64_t>(frames_) * 4U,
                .underrun_warning_threshold_frames = 4,
            },
            std::span<const mirakana::AudioClipSampleData>{&audio_samples_, 1});
        audio_commands_ += audio.plan.commands.size();
        audio_underruns_ += audio.plan.underruns.size();

        ++frames_;
        input_.begin_frame();

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    void on_stop(mirakana::EngineContext&) override {
        if (scene_.scene.has_value()) {
            if (const auto* player = scene_.scene->find_node(kPlayerNode); player != nullptr) {
                final_x_ = player->transform.position.x;
            }
        }
    }

    [[nodiscard]] bool passed(std::uint32_t expected_frames) const noexcept {
        const auto expected_sprite_submissions =
            static_cast<std::uint64_t>(expected_frames) * static_cast<std::uint64_t>(package_scene_sprites_);
        return ui_ok_ && ui_text_updates_ok_ && validation_ok_ && audio_clip_registered_ &&
               jump_voice_ != mirakana::null_audio_voice && frames_ == expected_frames &&
               final_x_ == static_cast<float>(expected_frames) && primary_camera_seen_ &&
               scene_sprite_sources_submitted_ == expected_sprite_submissions &&
               scene_sprites_submitted_ > expected_sprite_submissions && hud_boxes_submitted_ == expected_frames &&
               sprite_batch_plan_ok_ && sprite_batch_plan_draws_ == expected_frames &&
               sprite_batch_plan_texture_binds_ == expected_frames &&
               sprite_batch_plan_atlas_backed_batches_ == expected_frames &&
               sprite_batch_plan_repeated_atlas_batches_ == expected_frames && sprite_batch_plan_diagnostics_ == 0 &&
               sprite_batch_budget_ok_ && sprite_batch_budget_profiles_ready_ == expected_frames &&
               sprite_batch_budget_rows_ == (static_cast<std::uint64_t>(expected_frames) * 3U) &&
               sprite_batch_budget_diagnostics_ == 0 && sprite_batch_budget_world_ready_ == expected_frames &&
               sprite_batch_budget_ui_ready_ == expected_frames &&
               sprite_batch_budget_effects_ready_ == expected_frames && sprite_batch_budget_total_sprites_ > 0U &&
               sprite_batch_budget_total_draws_ > 0U && sprite_batch_budget_total_texture_binds_ > 0U &&
               sprite_animation_ok_ && sprite_animation_frames_sampled_ == expected_frames &&
               sprite_animation_frames_applied_ == expected_frames && sprite_animation_diagnostics_ == 0 &&
               sprite_animation_selected_frame_sum_ > 0 && sprite_flipbook_ok_ &&
               sprite_flipbook_ticks_ == expected_frames && sprite_flipbook_frames_sampled_ == expected_frames &&
               sprite_flipbook_frames_applied_ == expected_frames && sprite_flipbook_selected_frame_sum_ > 0 &&
               sprite_flipbook_events_sampled_ == expected_frames && sprite_flipbook_direction_sets() == 2U &&
               sprite_flipbook_event_rows() == 2U && sprite_flipbook_playback_modes() == 2U &&
               sprite_flipbook_gameplay_state_rows() == 2U && sprite_flipbook_diagnostics_ == 0 &&
               audio_commands_ == 1 && audio_underruns_ == 0 && package_scene_sprites_ == 2 && tilemap_runtime_ok_ &&
               tilemap_layers_ == 1 && tilemap_visible_layers_ == 1 && tilemap_tiles_ == 2 &&
               tilemap_non_empty_cells_ == 3 && tilemap_sampled_cells_ == 3 && tilemap_diagnostics_ == 0 &&
               sprite_effect_particles_ready() && scene_gameplay_bindings_.ready &&
               gameplay_systems_.passed(expected_frames);
    }

    [[nodiscard]] std::uint32_t frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] float final_x() const noexcept {
        return final_x_;
    }

    [[nodiscard]] std::size_t scene_sprites_submitted() const noexcept {
        return scene_sprites_submitted_;
    }

    [[nodiscard]] std::size_t hud_boxes_submitted() const noexcept {
        return hud_boxes_submitted_;
    }

    [[nodiscard]] std::size_t audio_commands() const noexcept {
        return audio_commands_;
    }

    [[nodiscard]] std::size_t audio_underruns() const noexcept {
        return audio_underruns_;
    }

    [[nodiscard]] const AudioGameplayMixerProbeResult& audio_gameplay_mixer_probe() const noexcept {
        return audio_gameplay_mixer_;
    }

    [[nodiscard]] std::size_t package_scene_sprites() const noexcept {
        return package_scene_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_sprites() const noexcept {
        return sprite_batch_plan_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_textured_sprites() const noexcept {
        return sprite_batch_plan_textured_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_draws() const noexcept {
        return sprite_batch_plan_draws_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_texture_binds() const noexcept {
        return sprite_batch_plan_texture_binds_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_atlas_backed_batches() const noexcept {
        return sprite_batch_plan_atlas_backed_batches_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_repeated_atlas_batches() const noexcept {
        return sprite_batch_plan_repeated_atlas_batches_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_plan_repeated_atlas_sprites() const noexcept {
        return sprite_batch_plan_repeated_atlas_sprites_;
    }

    [[nodiscard]] std::size_t sprite_batch_plan_diagnostics() const noexcept {
        return sprite_batch_plan_diagnostics_;
    }

    [[nodiscard]] mirakana::SpriteBatchBudgetProfileStatus sprite_batch_budget_status() const noexcept {
        return sprite_batch_budget_status_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_profiles_ready() const noexcept {
        return sprite_batch_budget_profiles_ready_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_rows() const noexcept {
        return sprite_batch_budget_rows_;
    }

    [[nodiscard]] std::size_t sprite_batch_budget_diagnostics() const noexcept {
        return sprite_batch_budget_diagnostics_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_total_sprites() const noexcept {
        return sprite_batch_budget_total_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_total_draws() const noexcept {
        return sprite_batch_budget_total_draws_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_total_texture_binds() const noexcept {
        return sprite_batch_budget_total_texture_binds_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_world_ready() const noexcept {
        return sprite_batch_budget_world_ready_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_ui_ready() const noexcept {
        return sprite_batch_budget_ui_ready_;
    }

    [[nodiscard]] std::uint64_t sprite_batch_budget_effects_ready() const noexcept {
        return sprite_batch_budget_effects_ready_;
    }

    [[nodiscard]] std::uint64_t sprite_sort_layers_applied() const noexcept {
        return sprite_sort_layers_applied_;
    }

    [[nodiscard]] std::uint64_t sprite_sorted_draws() const noexcept {
        return sprite_sorted_draws_;
    }

    [[nodiscard]] std::uint64_t sprite_animation_frames_sampled() const noexcept {
        return sprite_animation_frames_sampled_;
    }

    [[nodiscard]] std::uint64_t sprite_animation_frames_applied() const noexcept {
        return sprite_animation_frames_applied_;
    }

    [[nodiscard]] std::uint64_t sprite_animation_selected_frame_sum() const noexcept {
        return sprite_animation_selected_frame_sum_;
    }

    [[nodiscard]] std::size_t sprite_animation_diagnostics() const noexcept {
        return sprite_animation_diagnostics_;
    }

    [[nodiscard]] std::uint32_t sprite_flipbook_ticks() const noexcept {
        return sprite_flipbook_ticks_;
    }

    [[nodiscard]] std::uint64_t sprite_flipbook_frames_sampled() const noexcept {
        return sprite_flipbook_frames_sampled_;
    }

    [[nodiscard]] std::uint64_t sprite_flipbook_frames_applied() const noexcept {
        return sprite_flipbook_frames_applied_;
    }

    [[nodiscard]] std::uint64_t sprite_flipbook_selected_frame_sum() const noexcept {
        return sprite_flipbook_selected_frame_sum_;
    }

    [[nodiscard]] std::size_t sprite_flipbook_diagnostics() const noexcept {
        return sprite_flipbook_diagnostics_;
    }

    [[nodiscard]] std::size_t sprite_flipbook_direction_sets() const noexcept {
        return sprite_flipbook_direction_sets_.size();
    }

    [[nodiscard]] std::size_t sprite_flipbook_event_rows() const noexcept {
        return sprite_flipbook_events_.size();
    }

    [[nodiscard]] std::size_t sprite_flipbook_playback_modes() const noexcept {
        bool has_clip_loop = false;
        bool has_loop = false;
        bool has_clamp = false;
        for (const auto& row : sprite_flipbook_direction_sets_) {
            switch (row.playback_mode) {
            case mirakana::RuntimeSpriteFlipbookPlaybackMode::use_clip_loop:
                has_clip_loop = true;
                break;
            case mirakana::RuntimeSpriteFlipbookPlaybackMode::loop:
                has_loop = true;
                break;
            case mirakana::RuntimeSpriteFlipbookPlaybackMode::clamp_to_end:
                has_clamp = true;
                break;
            }
        }
        return static_cast<std::size_t>(has_clip_loop) + static_cast<std::size_t>(has_loop) +
               static_cast<std::size_t>(has_clamp);
    }

    [[nodiscard]] std::size_t sprite_flipbook_gameplay_state_rows() const noexcept {
        std::size_t count = 0;
        for (std::size_t index = 0; index < sprite_flipbook_direction_sets_.size(); ++index) {
            bool previously_seen = false;
            for (std::size_t previous = 0; previous < index; ++previous) {
                if (sprite_flipbook_direction_sets_[previous].gameplay_state ==
                    sprite_flipbook_direction_sets_[index].gameplay_state) {
                    previously_seen = true;
                    break;
                }
            }
            if (!previously_seen) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::uint64_t sprite_flipbook_events_sampled() const noexcept {
        return sprite_flipbook_events_sampled_;
    }

    [[nodiscard]] std::size_t sprite_flipbook_playback_diagnostics() const noexcept {
        return sprite_flipbook_diagnostics_;
    }

    [[nodiscard]] std::size_t tilemap_layers() const noexcept {
        return tilemap_layers_;
    }

    [[nodiscard]] std::size_t tilemap_visible_layers() const noexcept {
        return tilemap_visible_layers_;
    }

    [[nodiscard]] std::size_t tilemap_tiles() const noexcept {
        return tilemap_tiles_;
    }

    [[nodiscard]] std::size_t tilemap_non_empty_cells() const noexcept {
        return tilemap_non_empty_cells_;
    }

    [[nodiscard]] std::size_t tilemap_sampled_cells() const noexcept {
        return tilemap_sampled_cells_;
    }

    [[nodiscard]] std::size_t tilemap_diagnostics() const noexcept {
        return tilemap_diagnostics_;
    }

    [[nodiscard]] std::size_t scene_sprite_sources_submitted() const noexcept {
        return scene_sprite_sources_submitted_;
    }

    [[nodiscard]] std::uint64_t sprite_9slice_tiled_source_sprites() const noexcept {
        return sprite_9slice_tiled_source_sprites_;
    }

    [[nodiscard]] std::uint64_t sprite_9slice_expanded_quads() const noexcept {
        return sprite_9slice_expanded_quads_;
    }

    [[nodiscard]] std::uint64_t sprite_tiled_expanded_quads() const noexcept {
        return sprite_tiled_expanded_quads_;
    }

    [[nodiscard]] bool gameplay_systems_passed(std::uint32_t expected_frames) const {
        return gameplay_systems_.passed(expected_frames);
    }

    [[nodiscard]] Gameplay2DSystemsStatus gameplay_systems_status(std::uint32_t expected_frames) const {
        return gameplay_systems_.status(expected_frames);
    }

    [[nodiscard]] std::uint32_t gameplay_systems_ticks() const noexcept {
        return gameplay_systems_.ticks();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_physics_ticks() const noexcept {
        return gameplay_systems_.physics_ticks();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_bodies() const noexcept {
        return gameplay_systems_.physics_body_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_contacts() const noexcept {
        return gameplay_systems_.physics_contact_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_trigger_overlaps() const noexcept {
        return gameplay_systems_.physics_trigger_overlap_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_path_points() const noexcept {
        return gameplay_systems_.navigation_path_point_count();
    }

    [[nodiscard]] bool gameplay_systems_navigation_reached_destination() const noexcept {
        return gameplay_systems_.navigation_reached_destination();
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathStatus gameplay_systems_navigation_plan_status() const noexcept {
        return gameplay_systems_.navigation_plan_status();
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathDiagnostic
    gameplay_systems_navigation_plan_diagnostic() const noexcept {
        return gameplay_systems_.navigation_plan_diagnostic();
    }

    [[nodiscard]] mirakana::NavigationAgentStatus gameplay_systems_navigation_agent_status() const noexcept {
        return gameplay_systems_.navigation_agent_status();
    }

    [[nodiscard]] mirakana::AiPerceptionStatus gameplay_systems_perception_status() const noexcept {
        return gameplay_systems_.perception_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_targets() const noexcept {
        return gameplay_systems_.perception_target_count();
    }

    [[nodiscard]] bool gameplay_systems_perception_has_primary_target() const noexcept {
        return gameplay_systems_.perception_has_primary_target();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_visible_count() const noexcept {
        return gameplay_systems_.perception_visible_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_audible_count() const noexcept {
        return gameplay_systems_.perception_audible_count();
    }

    [[nodiscard]] mirakana::AiPerceptionReadinessStatus gameplay_systems_perception_readiness_status() const noexcept {
        return gameplay_systems_.perception_readiness_status();
    }

    [[nodiscard]] mirakana::AiPerceptionReadinessDiagnostic
    gameplay_systems_perception_readiness_diagnostic() const noexcept {
        return gameplay_systems_.perception_readiness_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_readiness_diagnostics() const noexcept {
        return gameplay_systems_.perception_readiness_diagnostics();
    }

    [[nodiscard]] bool gameplay_systems_perception_stable_primary_target_ready() const noexcept {
        return gameplay_systems_.perception_stable_primary_target_ready();
    }

    [[nodiscard]] bool gameplay_systems_perception_blackboard_projection_ready() const noexcept {
        return gameplay_systems_.perception_blackboard_projection_ready();
    }

    [[nodiscard]] mirakana::AiPerceptionBlackboardStatus gameplay_systems_blackboard_status() const noexcept {
        return gameplay_systems_.blackboard_status();
    }

    [[nodiscard]] bool gameplay_systems_blackboard_has_target() const noexcept {
        return gameplay_systems_.blackboard_has_target();
    }

    [[nodiscard]] bool gameplay_systems_blackboard_needs_move() const noexcept {
        return gameplay_systems_.blackboard_needs_move();
    }

    [[nodiscard]] mirakana::BehaviorTreeStatus gameplay_systems_behavior_status() const noexcept {
        return gameplay_systems_.behavior_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_nodes() const noexcept {
        return gameplay_systems_.behavior_visited_node_count();
    }

    [[nodiscard]] bool gameplay_systems_behavior_authoring_ready() const noexcept {
        return gameplay_systems_.behavior_authoring_ready();
    }

    [[nodiscard]] mirakana::BehaviorAuthoringReadinessStatus
    gameplay_systems_behavior_authoring_readiness_status() const noexcept {
        return gameplay_systems_.behavior_authoring_readiness_status();
    }

    [[nodiscard]] mirakana::BehaviorAuthoringReadinessDiagnostic
    gameplay_systems_behavior_authoring_readiness_diagnostic() const noexcept {
        return gameplay_systems_.behavior_authoring_readiness_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_diagnostics() const noexcept {
        return gameplay_systems_.behavior_authoring_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_readiness_diagnostics() const noexcept {
        return gameplay_systems_.behavior_authoring_readiness_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_trace_nodes() const noexcept {
        return gameplay_systems_.behavior_authoring_trace_node_count();
    }

    [[nodiscard]] bool gameplay_systems_behavior_authoring_deterministic_trace_ready() const noexcept {
        return gameplay_systems_.behavior_authoring_deterministic_trace_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_behaviors() const noexcept {
        return gameplay_systems_.behavior_authoring_behavior_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_action_bindings() const noexcept {
        return gameplay_systems_.behavior_authoring_action_binding_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_blackboard_conditions() const noexcept {
        return gameplay_systems_.behavior_authoring_blackboard_condition_count();
    }

    [[nodiscard]] bool gameplay_systems_quest_dialogue_ready() const noexcept {
        return gameplay_systems_.quest_dialogue_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_diagnostics() const noexcept {
        return gameplay_systems_.quest_dialogue_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_transition_rows() const noexcept {
        return gameplay_systems_.quest_dialogue_transition_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_completed_objectives() const noexcept {
        return gameplay_systems_.quest_dialogue_completed_objective_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_flags() const noexcept {
        return gameplay_systems_.quest_dialogue_flag_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_dialogue_nodes() const noexcept {
        return gameplay_systems_.quest_dialogue_dialogue_node_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_action_ids() const noexcept {
        return gameplay_systems_.quest_dialogue_action_id_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_reward_ids() const noexcept {
        return gameplay_systems_.quest_dialogue_reward_id_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_quest_dialogue_state_rows() const noexcept {
        return gameplay_systems_.quest_dialogue_state_row_count();
    }

    [[nodiscard]] bool gameplay_systems_inventory_items_ready() const noexcept {
        return gameplay_systems_.inventory_items_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_diagnostics() const noexcept {
        return gameplay_systems_.inventory_items_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_catalog_rows() const noexcept {
        return gameplay_systems_.inventory_items_catalog_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_state_rows() const noexcept {
        return gameplay_systems_.inventory_items_state_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_transition_rows() const noexcept {
        return gameplay_systems_.inventory_items_transition_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_accepted_rows() const noexcept {
        return gameplay_systems_.inventory_items_accepted_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_completed_rows() const noexcept {
        return gameplay_systems_.inventory_items_completed_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_inventory_items_final_stacks() const noexcept {
        return gameplay_systems_.inventory_items_final_stack_count();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_inventory_items_final_workbench_quantity() const noexcept {
        return gameplay_systems_.inventory_items_final_workbench_quantity();
    }

    [[nodiscard]] bool gameplay_systems_interaction_ready() const noexcept {
        return gameplay_systems_.interaction_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_interaction_diagnostics() const noexcept {
        return gameplay_systems_.interaction_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_interaction_rows() const noexcept {
        return gameplay_systems_.interaction_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_interaction_feedback_rows() const noexcept {
        return gameplay_systems_.interaction_feedback_row_count();
    }

    [[nodiscard]] mirakana::runtime::RuntimeGameplaySessionState
    gameplay_systems_interaction_final_session_state() const noexcept {
        return gameplay_systems_.interaction_final_session_state();
    }

    [[nodiscard]] bool gameplay_systems_scene_binding_ready() const noexcept {
        return scene_gameplay_bindings_.ready;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_source_rows() const noexcept {
        return scene_gameplay_bindings_.source_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_rows() const noexcept {
        return scene_gameplay_bindings_.binding_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_systems() const noexcept {
        return scene_gameplay_bindings_.gameplay_systems;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_component_rows() const noexcept {
        return scene_gameplay_bindings_.component_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_diagnostics() const noexcept {
        return scene_gameplay_bindings_.binding_diagnostics;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_interaction_rows() const noexcept {
        return scene_gameplay_bindings_.interaction_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_interaction_diagnostics() const noexcept {
        return scene_gameplay_bindings_.interaction_diagnostics;
    }

    [[nodiscard]] mirakana::runtime_scene::RuntimeSceneGameplaySessionState
    gameplay_systems_scene_interaction_final_session_state() const noexcept {
        return scene_gameplay_bindings_.final_session_state;
    }

    [[nodiscard]] bool sprite_collision_hitbox_ready() const noexcept {
        return gameplay_systems_.sprite_collision_hitbox_ready();
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_hits() const noexcept {
        return gameplay_systems_.sprite_collision_hitbox_hit_count();
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_gameplay_events() const noexcept {
        return gameplay_systems_.sprite_collision_hitbox_gameplay_event_count();
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_interaction_rows() const noexcept {
        return gameplay_systems_.sprite_collision_hitbox_interaction_row_count();
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_feedback_rows() const noexcept {
        return gameplay_systems_.sprite_collision_hitbox_feedback_row_count();
    }

    [[nodiscard]] std::size_t sprite_collision_hitbox_diagnostics() const noexcept {
        return gameplay_systems_.sprite_collision_hitbox_diagnostic_count();
    }

    [[nodiscard]] mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus
    sprite_effect_particles_status() const noexcept {
        return sprite_effect_particles_status_;
    }

    [[nodiscard]] bool sprite_effect_particles_ready() const noexcept {
        return sprite_effect_particles_ok_ && sprite_effect_particles_spawn_events_ == 1U &&
               sprite_effect_particles_spawned_particles_ == 3U && sprite_effect_particles_render_rows_ >= 3U &&
               sprite_effect_particles_submitted_ == sprite_effect_particles_render_rows_ &&
               sprite_effect_particles_draws_ > 0U && sprite_effect_particles_diagnostics_ == 0U &&
               sprite_effect_particles_budget_diagnostics_ == 0U;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_spawn_events() const noexcept {
        return sprite_effect_particles_spawn_events_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_spawned_particles() const noexcept {
        return sprite_effect_particles_spawned_particles_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_surviving_particles() const noexcept {
        return sprite_effect_particles_surviving_particles_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_render_rows() const noexcept {
        return sprite_effect_particles_render_rows_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_expired_particles() const noexcept {
        return sprite_effect_particles_expired_particles_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_submitted() const noexcept {
        return sprite_effect_particles_submitted_;
    }

    [[nodiscard]] std::uint64_t sprite_effect_particles_draws() const noexcept {
        return sprite_effect_particles_draws_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_diagnostics() const noexcept {
        return sprite_effect_particles_diagnostics_;
    }

    [[nodiscard]] std::size_t sprite_effect_particles_budget_diagnostics() const noexcept {
        return sprite_effect_particles_budget_diagnostics_;
    }

    [[nodiscard]] bool gameplay_systems_runtime_profile_resume_ready() const noexcept {
        return gameplay_systems_.runtime_profile_resume_ready();
    }

    [[nodiscard]] mirakana::runtime::RuntimeSessionProfileResumeStatus
    gameplay_systems_runtime_profile_resume_status() const noexcept {
        return gameplay_systems_.runtime_profile_resume_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_profile_resume_diagnostics() const noexcept {
        return gameplay_systems_.runtime_profile_resume_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_profile_resume_loaded_documents() const noexcept {
        return gameplay_systems_.runtime_profile_resume_loaded_document_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_profile_resume_defaulted_documents() const noexcept {
        return gameplay_systems_.runtime_profile_resume_defaulted_document_count();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_runtime_profile_resume_save_schema_version() const noexcept {
        return gameplay_systems_.runtime_profile_resume_save_schema_version();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_runtime_profile_resume_settings_schema_version() const noexcept {
        return gameplay_systems_.runtime_profile_resume_settings_schema_version();
    }

    [[nodiscard]] bool gameplay_systems_runtime_menu_hud_ready() const noexcept {
        return gameplay_systems_.runtime_menu_hud_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_diagnostics() const noexcept {
        return gameplay_systems_.runtime_menu_hud_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_display_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_display_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_command_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_command_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_dialogue_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_dialogue_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_input_binding_prompt_row_count();
    }

    [[nodiscard]] bool gameplay_systems_construction_placement_ready() const noexcept {
        return gameplay_systems_.construction_placement_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_construction_placement_diagnostics() const noexcept {
        return gameplay_systems_.construction_placement_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_construction_placement_validation_rows() const noexcept {
        return gameplay_systems_.construction_placement_validation_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_construction_placement_intent_rows() const noexcept {
        return gameplay_systems_.construction_placement_intent_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_construction_placement_intent_accepted_rows() const noexcept {
        return gameplay_systems_.construction_placement_intent_accepted_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_construction_placement_intent_occupied_cells() const noexcept {
        return gameplay_systems_.construction_placement_intent_occupied_cell_count();
    }

    [[nodiscard]] bool gameplay_systems_procedural_generation_ready() const noexcept {
        return gameplay_systems_.procedural_generation_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_diagnostics() const noexcept {
        return gameplay_systems_.procedural_generation_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_rows() const noexcept {
        return gameplay_systems_.procedural_generation_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_object_rows() const noexcept {
        return gameplay_systems_.procedural_generation_object_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_encounter_rows() const noexcept {
        return gameplay_systems_.procedural_generation_encounter_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_loot_rows() const noexcept {
        return gameplay_systems_.procedural_generation_loot_row_count();
    }

    [[nodiscard]] std::uint64_t gameplay_systems_procedural_generation_replay_hash() const noexcept {
        return gameplay_systems_.procedural_generation_replay_hash();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_package_visible_rows() const noexcept {
        return gameplay_systems_.procedural_generation_package_visible_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_placement_intent_rows() const noexcept {
        return gameplay_systems_.procedural_generation_placement_intent_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_procedural_generation_placement_intent_accepted_rows() const noexcept {
        return gameplay_systems_.procedural_generation_placement_intent_accepted_row_count();
    }

  private:
    [[nodiscard]] bool build_hud() {
        mirakana::ui::ElementDesc root;
        root.id = mirakana::ui::ElementId{"hud.root"};
        root.role = mirakana::ui::SemanticRole::root;
        root.style.layout = mirakana::ui::LayoutMode::column;
        root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
        if (!hud_.try_add_element(root)) {
            return false;
        }

        mirakana::ui::ElementDesc score;
        score.id = mirakana::ui::ElementId{"hud.score"};
        score.parent = root.id;
        score.role = mirakana::ui::SemanticRole::label;
        score.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 96.0F, .height = 24.0F};
        score.text = mirakana::ui::TextContent{
            .label = "Score 0", .localization_key = "hud.score", .font_family = "engine-default"};
        score.style.background_token = "hud.panel";
        score.accessibility_label = "Score";
        return hud_.try_add_element(score);
    }

    void update_hud_text() {
        const auto score = std::string{"Score "} + std::to_string((frames_ + 1U) * 100U);
        ui_text_updates_ok_ =
            ui_text_updates_ok_ &&
            hud_.set_text(mirakana::ui::ElementId{"hud.score"},
                          mirakana::ui::TextContent{
                              .label = score, .localization_key = "hud.score", .font_family = "engine-default"});
    }

    [[nodiscard]] std::vector<mirakana::runtime::RuntimeSpriteEffectParticleTemplateDesc>
    sprite_effect_particle_templates() const {
        return std::vector<mirakana::runtime::RuntimeSpriteEffectParticleTemplateDesc>{
            mirakana::runtime::RuntimeSpriteEffectParticleTemplateDesc{
                .id = "sample_2d.impact_dust",
                .kind = mirakana::runtime::RuntimeSpriteEffectParticleTemplateKind::radial_burst,
                .sprite = packaged_sprite_texture_asset_id(),
                .lifetime_seconds = 0.8F,
                .size_x = 0.22F,
                .size_y = 0.22F,
                .color_r = 0.85F,
                .color_g = 0.75F,
                .color_b = 0.45F,
                .color_a = 1.0F,
                .velocity_x = 0.0F,
                .velocity_y = 0.0F,
                .radial_speed = 0.8F,
                .layer = 10,
                .order = 0,
                .source_index = 0U,
            },
        };
    }

    [[nodiscard]] std::vector<mirakana::runtime::RuntimeSpriteEffectParticleSpawnEvent>
    sprite_effect_particle_spawn_events() const {
        if (frames_ != 0U || !gameplay_systems_.sprite_collision_hitbox_ready()) {
            return {};
        }
        return std::vector<mirakana::runtime::RuntimeSpriteEffectParticleSpawnEvent>{
            mirakana::runtime::RuntimeSpriteEffectParticleSpawnEvent{
                .id = "sample_2d.enemy_hit.0",
                .template_id = "sample_2d.impact_dust",
                .emitter_id = "enemy",
                .x = 1.75F,
                .y = 0.75F,
                .spawn_count = 3U,
                .source_index = 0U,
            },
        };
    }

    void record_sprite_batch_budget_profile(const mirakana::SpriteBatchPlan& world_plan,
                                            const mirakana::SpriteBatchPlan& ui_plan,
                                            const mirakana::SpriteBatchPlan& effects_plan) {
        const std::array<mirakana::SpriteBatchBudgetLanePlanDesc, 3> lanes{
            mirakana::SpriteBatchBudgetLanePlanDesc{
                .lane = mirakana::SpriteBatchBudgetLane::world,
                .plan = &world_plan,
                .budget =
                    mirakana::SpriteBatchBudgetDesc{
                        .max_sprites = kSpriteBatchWorldMaxSprites,
                        .max_draws = kSpriteBatchWorldMaxDraws,
                        .max_texture_binds = kSpriteBatchWorldMaxTextureBinds,
                    },
            },
            mirakana::SpriteBatchBudgetLanePlanDesc{
                .lane = mirakana::SpriteBatchBudgetLane::ui,
                .plan = &ui_plan,
                .budget =
                    mirakana::SpriteBatchBudgetDesc{
                        .max_sprites = kSpriteBatchUiMaxSprites,
                        .max_draws = kSpriteBatchUiMaxDraws,
                        .max_texture_binds = kSpriteBatchUiMaxTextureBinds,
                    },
            },
            mirakana::SpriteBatchBudgetLanePlanDesc{
                .lane = mirakana::SpriteBatchBudgetLane::effects,
                .plan = &effects_plan,
                .budget =
                    mirakana::SpriteBatchBudgetDesc{
                        .max_sprites = kSpriteBatchEffectsMaxSprites,
                        .max_draws = kSpriteBatchEffectsMaxDraws,
                        .max_texture_binds = kSpriteBatchEffectsMaxTextureBinds,
                    },
            },
        };
        const auto profile = mirakana::plan_sprite_batch_budget_profile(lanes);
        sprite_batch_budget_status_ = profile.status;
        sprite_batch_budget_ok_ = sprite_batch_budget_ok_ && profile.succeeded();
        if (profile.succeeded()) {
            ++sprite_batch_budget_profiles_ready_;
        }
        sprite_batch_budget_rows_ += profile.rows.size();
        sprite_batch_budget_diagnostics_ += profile.diagnostics.size();
        sprite_batch_budget_total_sprites_ += profile.total_sprites;
        sprite_batch_budget_total_draws_ += profile.total_draws;
        sprite_batch_budget_total_texture_binds_ += profile.total_texture_binds;
        for (const auto& row : profile.rows) {
            if (!row.within_budget) {
                continue;
            }
            switch (row.lane) {
            case mirakana::SpriteBatchBudgetLane::world:
                ++sprite_batch_budget_world_ready_;
                break;
            case mirakana::SpriteBatchBudgetLane::ui:
                ++sprite_batch_budget_ui_ready_;
                break;
            case mirakana::SpriteBatchBudgetLane::effects:
                ++sprite_batch_budget_effects_ready_;
                break;
            }
        }
    }

    [[nodiscard]] mirakana::SpriteBatchPlan update_sprite_effect_particles() {
        auto request = mirakana::runtime::RuntimeSpriteEffectParticleRequest{
            .templates = sprite_effect_particle_templates(),
            .active_particles = sprite_effect_particles_,
            .spawn_events = sprite_effect_particle_spawn_events(),
            .delta_seconds = 0.25F,
            .max_spawn_events = 4U,
            .max_active_particles = 16U,
            .max_render_rows = 16U,
        };
        const auto plan = mirakana::runtime::plan_runtime_sprite_effect_particles(request);
        sprite_effect_particles_status_ = plan.status;
        sprite_effect_particles_spawn_events_ += plan.counters.spawn_events;
        sprite_effect_particles_spawned_particles_ += plan.counters.spawned_particles;
        sprite_effect_particles_surviving_particles_ = plan.counters.surviving_particles;
        sprite_effect_particles_render_rows_ += plan.counters.render_rows;
        sprite_effect_particles_expired_particles_ += plan.counters.expired_particles;
        sprite_effect_particles_diagnostics_ += plan.diagnostics.size();
        sprite_effect_particles_budget_diagnostics_ += plan.counters.budget_diagnostics;
        sprite_effect_particles_ = plan.next_active_particles;
        if (!plan.succeeded()) {
            sprite_effect_particles_ok_ = false;
            return {};
        }

        std::vector<mirakana::SpriteCommand> commands;
        commands.reserve(plan.render_rows.size());
        for (const auto& row : plan.render_rows) {
            commands.push_back(mirakana::make_runtime_sprite_effect_particle_command(row));
        }
        const auto batch_plan = mirakana::plan_sprite_batches(commands);
        sprite_effect_particles_ok_ = sprite_effect_particles_ok_ && batch_plan.succeeded();
        sprite_effect_particles_draws_ += batch_plan.draw_count;
        sprite_effect_particles_submitted_ +=
            mirakana::submit_runtime_sprite_effect_particle_rows(renderer_, plan.render_rows);
        return batch_plan;
    }

    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::runtime::RuntimeInputActionMap actions_;
    mirakana::RuntimeSceneRenderInstance scene_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    mirakana::AudioMixer mixer_;
    Gameplay2DSystemsProbe gameplay_systems_;
    SceneGameplayBindingProbeResult scene_gameplay_bindings_;
    mirakana::AudioClipSampleData audio_samples_;
    AudioGameplayMixerProbeResult audio_gameplay_mixer_;
    mirakana::runtime::RuntimeSpriteAnimationPayload sprite_animation_;
    std::vector<mirakana::RuntimeSpriteFlipbookClipDesc> sprite_flipbook_clips_;
    std::vector<mirakana::RuntimeSpriteFlipbookDirectionSetRow> sprite_flipbook_direction_sets_;
    std::vector<mirakana::RuntimeSpriteFlipbookEventDesc> sprite_flipbook_events_;
    mirakana::RuntimeSpriteFlipbookState sprite_flipbook_state_;
    mirakana::runtime::RuntimeTilemapPayload tilemap_;
    std::vector<mirakana::runtime::RuntimeSpriteEffectParticleState> sprite_effect_particles_;
    mirakana::AudioVoiceId jump_voice_;
    std::size_t scene_sprites_submitted_{0};
    std::size_t scene_sprite_sources_submitted_{0};
    std::size_t hud_boxes_submitted_{0};
    std::size_t audio_commands_{0};
    std::size_t audio_underruns_{0};
    std::size_t package_scene_sprites_{0};
    std::uint64_t sprite_9slice_tiled_source_sprites_{0};
    std::uint64_t sprite_9slice_expanded_quads_{0};
    std::uint64_t sprite_tiled_expanded_quads_{0};
    std::uint64_t sprite_batch_plan_sprites_{0};
    std::uint64_t sprite_batch_plan_textured_sprites_{0};
    std::uint64_t sprite_batch_plan_draws_{0};
    std::uint64_t sprite_batch_plan_texture_binds_{0};
    std::uint64_t sprite_batch_plan_atlas_backed_batches_{0};
    std::uint64_t sprite_batch_plan_repeated_atlas_batches_{0};
    std::uint64_t sprite_batch_plan_repeated_atlas_sprites_{0};
    std::size_t sprite_batch_plan_diagnostics_{0};
    mirakana::SpriteBatchBudgetProfileStatus sprite_batch_budget_status_{
        mirakana::SpriteBatchBudgetProfileStatus::invalid_request};
    std::uint64_t sprite_batch_budget_profiles_ready_{0};
    std::uint64_t sprite_batch_budget_rows_{0};
    std::size_t sprite_batch_budget_diagnostics_{0};
    std::uint64_t sprite_batch_budget_total_sprites_{0};
    std::uint64_t sprite_batch_budget_total_draws_{0};
    std::uint64_t sprite_batch_budget_total_texture_binds_{0};
    std::uint64_t sprite_batch_budget_world_ready_{0};
    std::uint64_t sprite_batch_budget_ui_ready_{0};
    std::uint64_t sprite_batch_budget_effects_ready_{0};
    std::uint64_t sprite_sort_layers_applied_{0};
    std::uint64_t sprite_sorted_draws_{0};
    std::uint64_t sprite_animation_frames_sampled_{0};
    std::uint64_t sprite_animation_frames_applied_{0};
    std::uint64_t sprite_animation_selected_frame_sum_{0};
    std::size_t sprite_animation_diagnostics_{0};
    std::uint64_t sprite_flipbook_frames_sampled_{0};
    std::uint64_t sprite_flipbook_frames_applied_{0};
    std::uint64_t sprite_flipbook_selected_frame_sum_{0};
    std::uint64_t sprite_flipbook_events_sampled_{0};
    std::size_t sprite_flipbook_diagnostics_{0};
    std::size_t sprite_effect_particles_spawn_events_{0};
    std::size_t sprite_effect_particles_spawned_particles_{0};
    std::size_t sprite_effect_particles_surviving_particles_{0};
    std::size_t sprite_effect_particles_render_rows_{0};
    std::size_t sprite_effect_particles_expired_particles_{0};
    std::size_t sprite_effect_particles_submitted_{0};
    std::uint64_t sprite_effect_particles_draws_{0};
    std::size_t sprite_effect_particles_diagnostics_{0};
    std::size_t sprite_effect_particles_budget_diagnostics_{0};
    std::size_t tilemap_layers_{0};
    std::size_t tilemap_visible_layers_{0};
    std::size_t tilemap_tiles_{0};
    std::size_t tilemap_non_empty_cells_{0};
    std::size_t tilemap_sampled_cells_{0};
    std::size_t tilemap_diagnostics_{0};
    std::uint32_t frames_{0};
    std::uint32_t sprite_flipbook_ticks_{0};
    float final_x_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool validation_ok_{false};
    bool audio_clip_registered_{false};
    bool primary_camera_seen_{false};
    bool sprite_batch_plan_ok_{true};
    bool sprite_batch_budget_ok_{true};
    bool sprite_animation_ok_{true};
    bool sprite_flipbook_ok_{true};
    bool sprite_effect_particles_ok_{true};
    bool tilemap_runtime_ok_{false};
    bool throttle_{true};
    mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus sprite_effect_particles_status_{
        mirakana::runtime::RuntimeSpriteEffectParticlePlanStatus::no_spawns};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void print_usage() {
    std::cout << "sample_2d_desktop_runtime_package [--smoke] [--max-frames N] "
                 "[--require-config PATH] --require-scene-package PATH "
                 "[--require-d3d12-shaders] [--require-d3d12-renderer] "
                 "[--require-vulkan-shaders] [--require-vulkan-renderer] [--require-native-2d-sprites] "
                 "[--require-sprite-animation] [--require-sprite-sorting-layer] [--require-sprite-9slice-tiled] "
                 "[--require-sprite-collision-hitbox] [--require-sprite-effects-particles] "
                 "[--require-tilemap-runtime-ux] "
                 "[--require-gameplay-systems] "
                 "[--require-procedural-generation] [--require-world-region-streaming] "
                 "[--require-entity-scale-culling] [--require-scripting-sandbox-policy] "
                 "[--require-networking-foundation-policy] [--require-simulation-orchestration] "
                 "[--require-gameplay-authoring-review] [--require-production-authoring-workflows] "
                 "[--require-runtime-profile-resume] [--require-runtime-menu-hud] "
                 "[--require-runtime-ui-workbench] [--require-runtime-ui-production-stack] "
                 "[--require-runtime-ui-renderer-atlas-handoff] [--require-audio-gameplay-mixer]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeOptions& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return true;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--require-d3d12-shaders") {
            options.require_d3d12_shaders = true;
            continue;
        }
        if (arg == "--require-d3d12-renderer") {
            options.require_d3d12_renderer = true;
            options.require_d3d12_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-shaders") {
            options.require_vulkan_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-renderer") {
            options.require_vulkan_renderer = true;
            options.require_vulkan_shaders = true;
            continue;
        }
        if (arg == "--require-native-2d-sprites") {
            options.require_native_2d_sprites = true;
            continue;
        }
        if (arg == "--require-sprite-animation") {
            options.require_sprite_animation = true;
            continue;
        }
        if (arg == "--require-sprite-sorting-layer") {
            options.require_sprite_sorting_layer = true;
            continue;
        }
        if (arg == "--require-sprite-9slice-tiled") {
            options.require_sprite_9slice_tiled = true;
            continue;
        }
        if (arg == "--require-sprite-collision-hitbox") {
            options.require_sprite_collision_hitbox = true;
            continue;
        }
        if (arg == "--require-sprite-effects-particles") {
            options.require_sprite_effects_particles = true;
            continue;
        }
        if (arg == "--require-tilemap-runtime-ux") {
            options.require_tilemap_runtime_ux = true;
            continue;
        }
        if (arg == "--require-gameplay-systems") {
            options.require_gameplay_systems = true;
            continue;
        }
        if (arg == "--require-procedural-generation") {
            options.require_procedural_generation = true;
            continue;
        }
        if (arg == "--require-world-region-streaming") {
            options.require_world_region_streaming = true;
            continue;
        }
        if (arg == "--require-entity-scale-culling") {
            options.require_entity_scale_culling = true;
            continue;
        }
        if (arg == "--require-scripting-sandbox-policy") {
            options.require_scripting_sandbox_policy = true;
            continue;
        }
        if (arg == "--require-networking-foundation-policy") {
            options.require_networking_foundation_policy = true;
            continue;
        }
        if (arg == "--require-simulation-orchestration") {
            options.require_simulation_orchestration = true;
            continue;
        }
        if (arg == "--require-gameplay-authoring-review") {
            options.require_gameplay_authoring_review = true;
            continue;
        }
        if (arg == "--require-production-authoring-workflows") {
            options.require_production_authoring_workflows = true;
            continue;
        }
        if (arg == "--require-runtime-profile-resume") {
            options.require_runtime_profile_resume = true;
            continue;
        }
        if (arg == "--require-runtime-menu-hud") {
            options.require_runtime_menu_hud = true;
            continue;
        }
        if (arg == "--require-runtime-ui-workbench") {
            options.require_runtime_ui_workbench = true;
            continue;
        }
        if (arg == "--require-runtime-ui-production-stack") {
            options.require_runtime_ui_production_stack = true;
            options.require_runtime_ui_workbench = true;
            continue;
        }
        if (arg == "--require-runtime-ui-renderer-atlas-handoff") {
            options.require_runtime_ui_renderer_atlas_handoff = true;
            continue;
        }
        if (arg == "--require-audio-gameplay-mixer") {
            options.require_audio_gameplay_mixer = true;
            continue;
        }
        if (arg == "--max-frames") {
            if (index + 1 >= argc || !parse_positive_uint32(argv[index + 1], options.max_frames)) {
                std::cerr << "--max-frames requires a positive integer\n";
                return false;
            }
            ++index;
            continue;
        }
        if (arg == "--require-config") {
            if (index + 1 >= argc) {
                std::cerr << "--require-config requires a relative path\n";
                return false;
            }
            options.required_config_path = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-scene-package") {
            if (index + 1 >= argc) {
                std::cerr << "--require-scene-package requires a relative path\n";
                return false;
            }
            options.required_scene_package_path = argv[index + 1];
            ++index;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return false;
    }

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 3;
        }
        options.throttle = false;
    }
    if (options.require_native_2d_sprites) {
        if (options.require_vulkan_renderer) {
            options.require_vulkan_shaders = true;
        } else {
            options.require_d3d12_renderer = true;
            options.require_d3d12_shaders = true;
        }
    }
    return true;
}

[[nodiscard]] std::filesystem::path executable_directory(const char* executable_path) {
    try {
        if (executable_path != nullptr && !std::string_view{executable_path}.empty()) {
            const auto absolute_path = std::filesystem::absolute(std::filesystem::path{executable_path});
            if (absolute_path.has_parent_path()) {
                return absolute_path.parent_path();
            }
        }
        return std::filesystem::current_path();
    } catch (const std::exception&) {
        return std::filesystem::path{"."};
    }
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dVertexShaderPath},
        .fragment_path = std::string{kRuntime2dFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_native_sprite_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dNativeSpriteOverlayVertexShaderPath},
        .fragment_path = std::string{kRuntime2dNativeSpriteOverlayFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_vulkan_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntime2dVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_native_sprite_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntime2dVulkanNativeSpriteOverlayVertexShaderPath},
        .fragment_path = std::string{kRuntime2dVulkanNativeSpriteOverlayFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::Win32DesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::Win32DesktopPresentationShaderBytecode{
        .entry_point = bytecode.entry_point,
        .bytecode = std::span<const std::uint8_t>{bytecode.bytecode.data(), bytecode.bytecode.size()},
    };
}

[[nodiscard]] bool verify_required_config(const char* executable_path, std::string_view config_path) {
    if (config_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(config_path)) {
            std::cerr << "required config was not found: " << config_path << '\n';
            return false;
        }

        const auto config_text = filesystem.read_text(config_path);
        if (config_text.empty()) {
            std::cerr << "required config is empty: " << config_path << '\n';
            return false;
        }
        if (!config_text.starts_with(kExpectedConfigFormat)) {
            std::cerr << "required config has unexpected format: " << config_path << '\n';
            return false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required config '" << config_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime package failure asset=" << failure.asset.value << " path=" << failure.path << ": "
                  << failure.diagnostic << '\n';
    }
}

void print_scene_failures(const std::vector<mirakana::RuntimeSceneRenderLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime scene failure asset=" << failure.asset.value << ": " << failure.diagnostic << '\n';
    }
}

[[nodiscard]] std::optional<mirakana::AudioClipSampleData>
make_audio_samples(const mirakana::runtime::RuntimeAudioPayload& payload) {
    if (payload.sample_format != mirakana::AudioSourceSampleFormat::float32 || payload.channel_count == 0 ||
        payload.samples.size() != payload.source_bytes || payload.samples.size() % sizeof(float) != 0) {
        return std::nullopt;
    }

    std::vector<float> samples(payload.samples.size() / sizeof(float));
    if (!samples.empty()) {
        std::memcpy(samples.data(), payload.samples.data(), payload.samples.size());
    }

    return mirakana::AudioClipSampleData{
        .clip = payload.asset,
        .format = mirakana::AudioDeviceFormat{.sample_rate = payload.sample_rate,
                                              .channel_count = payload.channel_count,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .frame_count = payload.frame_count,
        .interleaved_float_samples = std::move(samples),
    };
}

[[nodiscard]] mirakana::runtime::RuntimeWorldRegionPackageDesc
make_world_region_desc(std::string region_id, std::uint32_t mount_id, std::string package_path,
                       std::uint64_t resident_bytes, std::size_t asset_records) {
    return mirakana::runtime::RuntimeWorldRegionPackageDesc{
        .region_id = std::move(region_id),
        .candidate =
            mirakana::runtime::RuntimePackageIndexDiscoveryCandidateV2{
                .package_index_path = std::move(package_path),
                .content_root = {},
                .label = "sample-2d-world-region",
            },
        .mount_id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = mount_id},
        .estimated_resident_bytes = resident_bytes,
        .estimated_asset_records = asset_records,
        .required_preload_assets = {packaged_scene_asset_id(), packaged_tilemap_asset_id(),
                                    packaged_sprite_animation_asset_id(), packaged_ui_atlas_asset_id()},
        .resident_resource_kinds =
            {
                mirakana::AssetKind::texture,
                mirakana::AssetKind::material,
                mirakana::AssetKind::scene,
                mirakana::AssetKind::audio,
                mirakana::AssetKind::tilemap,
                mirakana::AssetKind::sprite_animation,
                mirakana::AssetKind::ui_atlas,
            },
    };
}

[[nodiscard]] std::size_t
count_world_region_plan_diagnostics(const mirakana::runtime::RuntimeWorldRegionStreamingPlan& plan,
                                    mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode code) noexcept {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] WorldRegionStreamingProbeResult
run_world_region_streaming_probe(const char* executable_path, std::string_view package_path,
                                 const mirakana::runtime::RuntimeAssetPackage& runtime_package) {
    WorldRegionStreamingProbeResult probe;
    if (package_path.empty() || runtime_package.empty()) {
        return probe;
    }

    const auto package_bytes = mirakana::runtime::estimate_runtime_asset_package_resident_bytes(runtime_package);
    const auto package_records = runtime_package.records().size();
    if (package_bytes == 0U || package_records == 0U) {
        return probe;
    }

    const auto budget_bytes = (package_bytes * 2U) + 1U;
    const mirakana::runtime::RuntimeResourceResidencyBudgetV2 budget{
        .max_resident_content_bytes = budget_bytes,
        .max_resident_asset_records = (package_records * 2U) + 1U,
    };
    probe.budget_bytes = budget_bytes;

    std::vector<mirakana::runtime::RuntimeWorldRegionPackageDesc> regions;
    regions.push_back(make_world_region_desc("town", 1U, std::string{package_path}, package_bytes, package_records));
    regions.push_back(make_world_region_desc("field", 2U, std::string{package_path}, package_bytes, package_records));

    mirakana::runtime::RuntimeResidentPackageMountSetV2 mount_set;
    if (!mount_set
             .mount(mirakana::runtime::RuntimeResidentPackageMountRecordV2{
                 .id = mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U},
                 .label = "town",
                 .package = runtime_package,
             })
             .succeeded()) {
        return probe;
    }

    mirakana::runtime::RuntimeResidentCatalogCacheV2 catalog_cache;
    if (!catalog_cache.refresh(mount_set, mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins, budget)
             .succeeded()) {
        return probe;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        const mirakana::runtime::RuntimeWorldRegionStreamingPlanRequest load_request{
            .regions = regions,
            .active_region_ids = {"town"},
            .desired_region_ids = {"field", "town"},
            .protected_region_ids = {"town"},
            .budget = budget,
            .max_resident_regions = 2U,
        };
        const auto load_plan = mirakana::runtime::plan_runtime_world_region_streaming(load_request);

        auto missing_request = load_request;
        missing_request.desired_region_ids.push_back("missing");
        const auto missing_plan = mirakana::runtime::plan_runtime_world_region_streaming(missing_request);
        probe.missing_region_diagnostics = count_world_region_plan_diagnostics(
            missing_plan, mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode::missing_desired_region);

        const mirakana::runtime::RuntimeWorldRegionStreamingSafePointDesc load_desc{
            .plan = load_plan,
            .regions = regions,
            .target_id = "sample-2d-world-region-streaming",
            .runtime_scene_validation_target_id = "packaged-2d-scene",
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
            .budget = budget,
            .max_resident_packages = 2U,
            .safe_point_required = true,
            .runtime_scene_validation_succeeded = true,
            .eviction_candidate_unmount_order = {},
            .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 1U}},
        };
        const auto load_result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(
            filesystem, mount_set, catalog_cache, load_desc);

        auto navigation_regions = regions;
        navigation_regions[0].candidate.package_index_path = "runtime/regions/town.geindex";
        navigation_regions[1].candidate.package_index_path = "runtime/regions/field.geindex";
        const auto navigation_refs = mirakana::runtime::review_runtime_world_region_navigation_refs(
            mount_set, mirakana::runtime::RuntimeWorldRegionNavigationRefReviewRequest{
                           .regions = navigation_regions,
                           .route_region_ids = {"town", "field"},
                       });
        const mirakana::runtime::RuntimeWorldRegionNavigationPathCacheEntry navigation_cache{
            .region_path = {"town", "field"},
            .portal_path = {"town-field"},
            .mount_generation = mount_set.generation(),
            .catalog_generation = catalog_cache.catalog().generation(),
        };
        const auto navigation_path_cache = mirakana::runtime::review_runtime_world_region_navigation_path_cache(
            mount_set, catalog_cache,
            mirakana::runtime::RuntimeWorldRegionNavigationPathCacheReviewRequest{
                .regions = navigation_regions,
                .route_region_ids = {"town", "field"},
                .route_portal_ids = {"town-field"},
                .cache = navigation_cache,
            });

        const mirakana::runtime::RuntimeWorldRegionStreamingPlanRequest unload_request{
            .regions = regions,
            .active_region_ids = {"field", "town"},
            .desired_region_ids = {"field"},
            .protected_region_ids = {"field"},
            .budget = budget,
            .max_resident_regions = 2U,
        };
        const auto unload_plan = mirakana::runtime::plan_runtime_world_region_streaming(unload_request);
        const mirakana::runtime::RuntimeWorldRegionStreamingSafePointDesc unload_desc{
            .plan = unload_plan,
            .regions = regions,
            .target_id = "sample-2d-world-region-streaming",
            .runtime_scene_validation_target_id = "packaged-2d-scene",
            .overlay = mirakana::runtime::RuntimePackageMountOverlay::last_mount_wins,
            .budget = budget,
            .max_resident_packages = 2U,
            .safe_point_required = true,
            .runtime_scene_validation_succeeded = true,
            .eviction_candidate_unmount_order = {},
            .protected_mount_ids = {mirakana::runtime::RuntimeResidentPackageMountIdV2{.value = 2U}},
        };
        const auto unload_result = mirakana::runtime::execute_runtime_world_region_streaming_safe_point(
            filesystem, mount_set, catalog_cache, unload_desc);

        const mirakana::runtime::RuntimeWorldRegionStreamingPlan readiness_plans[]{load_plan, unload_plan};
        const mirakana::runtime::RuntimeWorldRegionStreamingSafePointResult readiness_safe_points[]{load_result,
                                                                                                    unload_result};
        const auto large_scene_readiness = mirakana::runtime::evaluate_runtime_world_streaming_large_scene_readiness(
            mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessRequest{
                .streaming_plans = std::span<const mirakana::runtime::RuntimeWorldRegionStreamingPlan>{readiness_plans},
                .safe_points =
                    std::span<const mirakana::runtime::RuntimeWorldRegionStreamingSafePointResult>{
                        readiness_safe_points},
                .missing_region_probe = &missing_plan,
                .navigation_refs = &navigation_refs,
                .navigation_path_cache = &navigation_path_cache,
            },
            mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessConfig{
                .require_missing_region_diagnostic = true,
                .require_navigation_refs_ready = true,
                .require_navigation_path_cache_ready = true,
                .min_keep_rows = 1U,
                .min_unload_rows = 1U,
                .max_projected_resident_regions = 2U,
                .max_projected_resident_bytes = budget_bytes,
            });

        probe.status = load_result.succeeded() && unload_result.succeeded()
                           ? mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::completed
                           : mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::failed;
        probe.large_scene_readiness_status = large_scene_readiness.status;
        probe.large_scene_readiness_diagnostic = large_scene_readiness.diagnostic;
        probe.plan_rows = load_plan.rows.size() + unload_plan.rows.size();
        probe.load_rows = load_plan.load_count + unload_plan.load_count;
        probe.keep_rows = load_plan.keep_count + unload_plan.keep_count;
        probe.unload_rows = load_plan.unload_count + unload_plan.unload_count;
        probe.safe_point_rows = load_result.rows.size() + unload_result.rows.size();
        probe.committed_rows = load_result.committed_count + unload_result.committed_count;
        probe.reviewed_package_adoptions = load_result.committed_count;
        probe.projected_regions = load_plan.projected_resident_region_count;
        probe.projected_bytes = load_plan.projected_resident_bytes;
        probe.safe_point_diagnostics = load_result.diagnostics.size() + unload_result.diagnostics.size();
        probe.large_scene_readiness_diagnostics = large_scene_readiness.diagnostics.size();
        probe.navigation_resident_regions = large_scene_readiness.navigation_resident_regions;
        probe.navigation_missing_resident_regions = large_scene_readiness.navigation_missing_resident_regions;
        probe.navigation_path_cache_ready = large_scene_readiness.navigation_path_cache_ready;
        probe.ready =
            load_plan.succeeded() && unload_plan.succeeded() && load_result.succeeded() && unload_result.succeeded() &&
            probe.load_rows == 1U && probe.unload_rows == 1U && probe.reviewed_package_adoptions > 0U &&
            probe.missing_region_diagnostics > 0U && probe.safe_point_diagnostics == 0U &&
            large_scene_readiness.status == mirakana::runtime::RuntimeWorldStreamingLargeSceneReadinessStatus::ready;
    } catch (const std::exception&) {
        probe.status = mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::failed;
        probe.ready = false;
    }

    return probe;
}

[[nodiscard]] bool
load_required_2d_package(const char* executable_path, std::string_view package_path,
                         std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                         std::optional<mirakana::RuntimeSceneRenderInstance>& scene,
                         std::optional<mirakana::AudioClipSampleData>& audio_samples,
                         std::optional<mirakana::runtime::RuntimeSpriteAnimationPayload>& sprite_animation,
                         std::optional<mirakana::runtime::RuntimeTilemapPayload>& tilemap) {
    if (package_path.empty()) {
        std::cerr << "--require-scene-package is required for the 2D desktop runtime package proof\n";
        return false;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(package_path)) {
            std::cerr << "required 2D package was not found: " << package_path << '\n';
            return false;
        }

        auto package_result =
            mirakana::runtime::load_runtime_asset_package(filesystem, mirakana::runtime::RuntimeAssetPackageDesc{
                                                                          .index_path = std::string{package_path},
                                                                          .content_root = {},
                                                                      });
        if (!package_result.succeeded()) {
            print_package_failures(package_result.failures);
            return false;
        }
        const auto diagnostics = mirakana::runtime::inspect_runtime_asset_package(package_result.package);
        if (!diagnostics.succeeded()) {
            for (const auto& diagnostic : diagnostics.diagnostics) {
                std::cerr << "runtime package diagnostic asset=" << diagnostic.asset.value
                          << " path=" << diagnostic.path << ": " << diagnostic.message << '\n';
            }
            return false;
        }

        auto instance =
            mirakana::instantiate_runtime_scene_render_data(package_result.package, packaged_scene_asset_id());
        if (!instance.succeeded()) {
            print_scene_failures(instance.failures);
            return false;
        }
        if (!instance.scene.has_value()) {
            std::cerr << "runtime 2D package did not produce a scene: " << package_path << '\n';
            return false;
        }
        const auto validation = mirakana::validate_playable_2d_scene(*instance.scene);
        if (!validation.succeeded() || instance.render_packet.sprites.empty()) {
            std::cerr << "runtime 2D package did not produce a playable orthographic sprite scene: " << package_path
                      << '\n';
            return false;
        }

        const auto* audio_record = package_result.package.find(packaged_audio_asset_id());
        if (audio_record == nullptr) {
            std::cerr << "runtime 2D package is missing the jump audio payload\n";
            return false;
        }
        const auto audio_payload = mirakana::runtime::runtime_audio_payload(*audio_record);
        if (!audio_payload.succeeded()) {
            std::cerr << "runtime 2D package audio payload failed: " << audio_payload.diagnostic << '\n';
            return false;
        }
        auto samples = make_audio_samples(audio_payload.payload);
        if (!samples.has_value()) {
            std::cerr << "runtime 2D package audio payload is not a supported float32 sample clip\n";
            return false;
        }

        const auto* animation_record = package_result.package.find(packaged_sprite_animation_asset_id());
        if (animation_record == nullptr) {
            std::cerr << "runtime 2D package is missing the player sprite animation payload\n";
            return false;
        }
        const auto animation_payload = mirakana::runtime::runtime_sprite_animation_payload(*animation_record);
        if (!animation_payload.succeeded()) {
            std::cerr << "runtime 2D package sprite animation payload failed: " << animation_payload.diagnostic << '\n';
            return false;
        }

        const auto* tilemap_record = package_result.package.find(packaged_tilemap_asset_id());
        if (tilemap_record == nullptr) {
            std::cerr << "runtime 2D package is missing the tilemap payload\n";
            return false;
        }
        const auto tilemap_payload = mirakana::runtime::runtime_tilemap_payload(*tilemap_record);
        if (!tilemap_payload.succeeded()) {
            std::cerr << "runtime 2D package tilemap payload failed: " << tilemap_payload.diagnostic << '\n';
            return false;
        }

        scene = std::move(instance);
        audio_samples = std::move(samples);
        sprite_animation = animation_payload.payload;
        tilemap = tilemap_payload.payload;
        runtime_package = std::move(package_result.package);
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required 2D package '" << package_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] std::string_view status_name(mirakana::DesktopRunStatus status) noexcept {
    switch (status) {
    case mirakana::DesktopRunStatus::completed:
        return "completed";
    case mirakana::DesktopRunStatus::stopped_by_app:
        return "stopped_by_app";
    case mirakana::DesktopRunStatus::window_closed:
        return "window_closed";
    case mirakana::DesktopRunStatus::lifecycle_quit:
        return "lifecycle_quit";
    }
    return "unknown";
}

void print_presentation_report(std::string_view prefix, const mirakana::Win32DesktopGameHost& host) {
    const auto report = host.presentation_report();
    std::cout << prefix << " presentation_report=requested="
              << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " native_2d_sprites_status="
              << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " native_2d_texture_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::win32_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::win32_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::win32_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.diagnostic << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    DesktopRuntimeOptions options;
    if (!parse_args(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help) {
        print_usage();
        return 0;
    }
    if (!verify_required_config(argc > 0 ? argv[0] : nullptr, options.required_config_path)) {
        return 4;
    }

    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    std::optional<mirakana::AudioClipSampleData> audio_samples;
    std::optional<mirakana::runtime::RuntimeSpriteAnimationPayload> sprite_animation;
    std::optional<mirakana::runtime::RuntimeTilemapPayload> tilemap;
    if (!load_required_2d_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                  packaged_scene, audio_samples, sprite_animation, tilemap)) {
        return 4;
    }
    const auto world_region_streaming_probe =
        runtime_package.has_value()
            ? run_world_region_streaming_probe(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path,
                                               *runtime_package)
            : WorldRegionStreamingProbeResult{};
    const auto entity_scale_culling_probe = options.require_entity_scale_culling
                                                ? validate_entity_scale_culling_package_evidence()
                                                : EntityScaleCullingProbeResult{};
    const auto scripting_sandbox_probe = options.require_scripting_sandbox_policy
                                             ? validate_scripting_sandbox_package_evidence()
                                             : ScriptingSandboxProbeResult{};
    const auto networking_foundation_probe = options.require_networking_foundation_policy
                                                 ? validate_networking_foundation_package_evidence()
                                                 : NetworkingFoundationProbeResult{};
    const auto simulation_orchestration_probe = options.require_simulation_orchestration
                                                    ? validate_simulation_orchestration_package_evidence()
                                                    : SimulationOrchestrationProbeResult{};
    const auto simulation_management_probe =
        (options.require_simulation_orchestration || options.require_gameplay_systems)
            ? validate_simulation_management_package_evidence("sample2d")
            : SimulationManagementProbeResult{};
    const auto network_replication_probe = options.require_gameplay_systems
                                               ? validate_network_replication_package_evidence("sample2d")
                                               : NetworkReplicationProbeResult{};
    const auto network_production_security_probe = options.require_gameplay_systems
                                                       ? validate_network_production_security_package_evidence()
                                                       : NetworkProductionSecurityProbeResult{};
    const auto gameplay_runtime_scheduler_probe = options.require_simulation_orchestration
                                                      ? validate_gameplay_runtime_scheduler_package_evidence()
                                                      : GameplayRuntimeSchedulerProbeResult{};
    const auto world_entity_model_probe = options.require_simulation_orchestration
                                              ? validate_world_entity_model_package_evidence()
                                              : WorldEntityModelProbeResult{};
    const auto addressable_content_probe =
        options.require_simulation_orchestration && runtime_package.has_value()
            ? validate_addressable_content_streaming_package_evidence(*runtime_package)
            : AddressableContentStreamingProbeResult{};
    const auto gameplay_authoring_review_probe = options.require_gameplay_authoring_review
                                                     ? validate_gameplay_authoring_review_package_evidence()
                                                     : GameplayAuthoringReviewProbeResult{};
    const auto production_authoring_workflow_probe = options.require_production_authoring_workflows
                                                         ? validate_production_authoring_workflow_package_evidence()
                                                         : ProductionAuthoringWorkflowProbeResult{};
    const auto runtime_ui_workbench_probe = options.require_runtime_ui_workbench
                                                ? validate_gameplay_2d_runtime_ui_workbench()
                                                : RuntimeUiWorkbenchProbeResult{};
    const auto runtime_ui_production_stack_probe = options.require_runtime_ui_production_stack
                                                       ? validate_runtime_ui_production_stack_package_evidence()
                                                       : RuntimeUiProductionStackProbeResult{};
    const auto runtime_ui_renderer_atlas_handoff_probe =
        options.require_runtime_ui_renderer_atlas_handoff && runtime_package.has_value()
            ? validate_runtime_ui_renderer_atlas_handoff_package_evidence(*runtime_package)
            : RuntimeUiRendererAtlasHandoffProbeResult{};
    const auto audio_production_probe = audio_samples.has_value()
                                            ? validate_audio_production_package_evidence(*audio_samples)
                                            : AudioProductionProbeResult{};

    auto shader_bytecode = load_packaged_d3d12_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shader_bytecode.ready()) {
        std::cout << "sample_2d_desktop_runtime_package shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shader_bytecode.status) << ": "
                  << shader_bytecode.diagnostic << '\n';
        if (options.require_d3d12_shaders) {
            return 5;
        }
    }
    auto native_sprite_overlay_shader_bytecode =
        load_packaged_d3d12_native_sprite_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!native_sprite_overlay_shader_bytecode.ready() && options.require_native_2d_sprites &&
        !options.require_vulkan_renderer) {
        std::cout << "sample_2d_desktop_runtime_package native_2d_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(native_sprite_overlay_shader_bytecode.status)
                  << ": " << native_sprite_overlay_shader_bytecode.diagnostic << '\n';
        return 9;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready() && options.require_vulkan_shaders) {
        std::cout << "sample_2d_desktop_runtime_package vulkan_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                  << vulkan_shader_bytecode.diagnostic << '\n';
        return 7;
    }
    auto vulkan_native_sprite_overlay_shader_bytecode =
        load_packaged_vulkan_native_sprite_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_native_sprite_overlay_shader_bytecode.ready() && options.require_native_2d_sprites &&
        options.require_vulkan_renderer) {
        std::cout << "sample_2d_desktop_runtime_package vulkan_native_2d_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         vulkan_native_sprite_overlay_shader_bytecode.status)
                  << ": " << vulkan_native_sprite_overlay_shader_bytecode.diagnostic << '\n';
        return 9;
    }

    std::optional<mirakana::Win32DesktopPresentationD3d12RendererDesc> d3d12_renderer;
    if (shader_bytecode.ready()) {
        d3d12_renderer.emplace(mirakana::Win32DesktopPresentationD3d12RendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(shader_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(shader_bytecode.fragment_shader),
            .native_sprite_overlay_vertex_shader =
                native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(native_sprite_overlay_shader_bytecode.vertex_shader)
                    : mirakana::Win32DesktopPresentationShaderBytecode{},
            .native_sprite_overlay_fragment_shader =
                native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(native_sprite_overlay_shader_bytecode.fragment_shader)
                    : mirakana::Win32DesktopPresentationShaderBytecode{},
            .native_sprite_overlay_package = runtime_package.has_value() ? &*runtime_package : nullptr,
            .native_sprite_overlay_atlas_asset = packaged_sprite_texture_asset_id(),
            .enable_native_sprite_overlay = options.require_native_2d_sprites && !options.require_vulkan_renderer,
            .enable_native_sprite_overlay_textures =
                options.require_native_2d_sprites && !options.require_vulkan_renderer,
        });
    }

    std::optional<mirakana::Win32DesktopPresentationVulkanRendererDesc> vulkan_renderer;
    if (vulkan_shader_bytecode.ready()) {
        vulkan_renderer.emplace(mirakana::Win32DesktopPresentationVulkanRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(vulkan_shader_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(vulkan_shader_bytecode.fragment_shader),
            .native_sprite_overlay_vertex_shader =
                vulkan_native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(vulkan_native_sprite_overlay_shader_bytecode.vertex_shader)
                    : mirakana::Win32DesktopPresentationShaderBytecode{},
            .native_sprite_overlay_fragment_shader =
                vulkan_native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(vulkan_native_sprite_overlay_shader_bytecode.fragment_shader)
                    : mirakana::Win32DesktopPresentationShaderBytecode{},
            .native_sprite_overlay_package = runtime_package.has_value() ? &*runtime_package : nullptr,
            .native_sprite_overlay_atlas_asset = packaged_sprite_texture_asset_id(),
            .enable_native_sprite_overlay = options.require_native_2d_sprites && options.require_vulkan_renderer,
            .enable_native_sprite_overlay_textures =
                options.require_native_2d_sprites && options.require_vulkan_renderer,
        });
    }

    mirakana::Win32DesktopGameHostDesc host_desc{
        .title = "Sample 2D Desktop Runtime Package",
        .extent = mirakana::WindowExtent{.width = 960, .height = 540},
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_renderer.has_value()) {
        host_desc.d3d12_renderer = &*d3d12_renderer;
    }
    if (vulkan_renderer.has_value()) {
        host_desc.vulkan_renderer = &*vulkan_renderer;
    }

    mirakana::Win32DesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::Win32DesktopPresentationBackend::d3d12) {
        std::cout << "sample_2d_desktop_runtime_package required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_2d_desktop_runtime_package", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 6;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::Win32DesktopPresentationBackend::vulkan) {
        std::cout << "sample_2d_desktop_runtime_package required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_2d_desktop_runtime_package", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }

    Sample2DDesktopRuntimePackageGame game(host.input(), host.renderer(), options.throttle, std::move(*packaged_scene),
                                           std::move(*audio_samples), std::move(*sprite_animation),
                                           std::move(*tilemap));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();

    const auto& audio_gameplay_mixer = game.audio_gameplay_mixer_probe();
    const auto input_context_rebinding = validate_sample_input_context_rebinding();
    const auto rpg_systems_probe =
        options.require_gameplay_systems ? validate_rpg_systems_package_evidence("sample2d") : RpgSystemsProbeResult{};
    const auto sandbox_world_probe = options.require_gameplay_systems
                                         ? validate_sandbox_world_package_evidence("sample2d")
                                         : SandboxWorldProbeResult{};
    const auto package_records = runtime_package.has_value() ? runtime_package->records().size() : 0U;
    std::cout
        << "sample_2d_desktop_runtime_package status=" << status_name(result.status)
        << " renderer=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_requested=" << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
        << " presentation_selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_fallback="
        << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
        << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
        << " presentation_backend_reports=" << report.backend_reports_count
        << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
        << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
        << " native_2d_sprites_requested=" << (report.native_ui_overlay_requested ? 1 : 0)
        << " native_2d_sprites_status="
        << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
        << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
        << " native_2d_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
        << " native_2d_textured_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
        << " native_2d_texture_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
        << " native_2d_texture_binds=" << report.native_ui_texture_overlay_texture_binds
        << " native_2d_draws=" << report.native_ui_overlay_draws
        << " native_2d_textured_draws=" << report.native_ui_texture_overlay_draws
        << " native_2d_sprite_batches_executed=" << report.renderer_stats.native_sprite_batches_executed
        << " native_2d_sprite_batch_sprites_executed=" << report.renderer_stats.native_sprite_batch_sprites_executed
        << " native_2d_sprite_batch_textured_sprites_executed="
        << report.renderer_stats.native_sprite_batch_textured_sprites_executed
        << " native_2d_sprite_batch_texture_binds=" << report.renderer_stats.native_sprite_batch_texture_binds
        << " frames=" << result.frames_run << " game_frames=" << game.frames() << " final_x=" << game.final_x()
        << " scene_sprites=" << game.scene_sprites_submitted()
        << " scene_sprite_sources=" << game.scene_sprite_sources_submitted()
        << " sprite_9slice_tiled_source_sprites=" << game.sprite_9slice_tiled_source_sprites()
        << " sprite_9slice_expanded_quads=" << game.sprite_9slice_expanded_quads()
        << " sprite_tiled_expanded_quads=" << game.sprite_tiled_expanded_quads()
        << " sprite_batch_plan_sprites=" << game.sprite_batch_plan_sprites()
        << " sprite_batch_plan_textured_sprites=" << game.sprite_batch_plan_textured_sprites()
        << " sprite_batch_plan_draws=" << game.sprite_batch_plan_draws()
        << " sprite_batch_plan_texture_binds=" << game.sprite_batch_plan_texture_binds()
        << " sprite_batch_plan_atlas_backed_batches=" << game.sprite_batch_plan_atlas_backed_batches()
        << " sprite_batch_plan_repeated_atlas_batches=" << game.sprite_batch_plan_repeated_atlas_batches()
        << " sprite_batch_plan_repeated_atlas_sprites=" << game.sprite_batch_plan_repeated_atlas_sprites()
        << " sprite_batch_plan_diagnostics=" << game.sprite_batch_plan_diagnostics()
        << " sprite_batch_budget_status=" << sprite_batch_budget_profile_status_name(game.sprite_batch_budget_status())
        << " sprite_batch_budget_profiles_ready=" << game.sprite_batch_budget_profiles_ready()
        << " sprite_batch_budget_rows=" << game.sprite_batch_budget_rows()
        << " sprite_batch_budget_diagnostics=" << game.sprite_batch_budget_diagnostics()
        << " sprite_batch_budget_total_sprites=" << game.sprite_batch_budget_total_sprites()
        << " sprite_batch_budget_total_draws=" << game.sprite_batch_budget_total_draws()
        << " sprite_batch_budget_total_texture_binds=" << game.sprite_batch_budget_total_texture_binds()
        << " sprite_batch_budget_world_ready=" << game.sprite_batch_budget_world_ready()
        << " sprite_batch_budget_world_max_sprites=" << kSpriteBatchWorldMaxSprites
        << " sprite_batch_budget_world_max_draws=" << kSpriteBatchWorldMaxDraws
        << " sprite_batch_budget_world_max_texture_binds=" << kSpriteBatchWorldMaxTextureBinds
        << " sprite_batch_budget_ui_ready=" << game.sprite_batch_budget_ui_ready()
        << " sprite_batch_budget_ui_max_sprites=" << kSpriteBatchUiMaxSprites
        << " sprite_batch_budget_ui_max_draws=" << kSpriteBatchUiMaxDraws
        << " sprite_batch_budget_ui_max_texture_binds=" << kSpriteBatchUiMaxTextureBinds
        << " sprite_batch_budget_effects_ready=" << game.sprite_batch_budget_effects_ready()
        << " sprite_batch_budget_effects_max_sprites=" << kSpriteBatchEffectsMaxSprites
        << " sprite_batch_budget_effects_max_draws=" << kSpriteBatchEffectsMaxDraws
        << " sprite_batch_budget_effects_max_texture_binds=" << kSpriteBatchEffectsMaxTextureBinds
        << " sprite_sort_layers_applied=" << game.sprite_sort_layers_applied()
        << " sprite_sorted_draws=" << game.sprite_sorted_draws()
        << " sprite_animation_frames_sampled=" << game.sprite_animation_frames_sampled()
        << " sprite_animation_frames_applied=" << game.sprite_animation_frames_applied()
        << " sprite_animation_selected_frame_sum=" << game.sprite_animation_selected_frame_sum()
        << " sprite_animation_diagnostics=" << game.sprite_animation_diagnostics()
        << " sprite_flipbook_ticks=" << game.sprite_flipbook_ticks()
        << " sprite_flipbook_frames_sampled=" << game.sprite_flipbook_frames_sampled()
        << " sprite_flipbook_frames_applied=" << game.sprite_flipbook_frames_applied()
        << " sprite_flipbook_selected_frame_sum=" << game.sprite_flipbook_selected_frame_sum()
        << " sprite_flipbook_diagnostics=" << game.sprite_flipbook_diagnostics()
        << " sprite_flipbook_direction_sets=" << game.sprite_flipbook_direction_sets()
        << " sprite_flipbook_event_rows=" << game.sprite_flipbook_event_rows()
        << " sprite_flipbook_playback_modes=" << game.sprite_flipbook_playback_modes()
        << " sprite_flipbook_gameplay_state_rows=" << game.sprite_flipbook_gameplay_state_rows()
        << " sprite_flipbook_events_sampled=" << game.sprite_flipbook_events_sampled()
        << " sprite_flipbook_playback_diagnostics=" << game.sprite_flipbook_playback_diagnostics()
        << " tilemap_layers=" << game.tilemap_layers() << " tilemap_visible_layers=" << game.tilemap_visible_layers()
        << " tilemap_tiles=" << game.tilemap_tiles() << " tilemap_non_empty_cells=" << game.tilemap_non_empty_cells()
        << " tilemap_cells_sampled=" << game.tilemap_sampled_cells()
        << " tilemap_diagnostics=" << game.tilemap_diagnostics() << " gameplay_systems_status="
        << gameplay_2d_systems_status_name(game.gameplay_systems_status(options.max_frames))
        << " gameplay_systems_ready="
        << ((game.gameplay_systems_passed(options.max_frames) && game.gameplay_systems_scene_binding_ready() &&
             (!options.require_gameplay_systems || (rpg_systems_probe.ready && sandbox_world_probe.ready)))
                ? 1
                : 0)
        << " gameplay_systems_ticks=" << game.gameplay_systems_ticks()
        << " gameplay_systems_physics_ticks=" << game.gameplay_systems_physics_ticks()
        << " gameplay_systems_physics_bodies=" << game.gameplay_systems_physics_bodies()
        << " gameplay_systems_physics_contacts=" << game.gameplay_systems_physics_contacts()
        << " gameplay_systems_physics_trigger_overlaps=" << game.gameplay_systems_physics_trigger_overlaps()
        << " gameplay_systems_navigation_path_points=" << game.gameplay_systems_navigation_path_points()
        << " gameplay_systems_navigation_reached=" << (game.gameplay_systems_navigation_reached_destination() ? 1 : 0)
        << " gameplay_systems_navigation_plan_status="
        << navigation_grid_agent_path_status_name(game.gameplay_systems_navigation_plan_status())
        << " gameplay_systems_navigation_plan_diagnostic="
        << navigation_grid_agent_path_diagnostic_name(game.gameplay_systems_navigation_plan_diagnostic())
        << " gameplay_systems_navigation_agent_status="
        << navigation_agent_status_name(game.gameplay_systems_navigation_agent_status())
        << " gameplay_systems_perception_status="
        << ai_perception_status_name(game.gameplay_systems_perception_status())
        << " gameplay_systems_perception_targets=" << game.gameplay_systems_perception_targets()
        << " gameplay_systems_perception_has_primary_target="
        << (game.gameplay_systems_perception_has_primary_target() ? 1 : 0)
        << " gameplay_systems_perception_visible_count=" << game.gameplay_systems_perception_visible_count()
        << " gameplay_systems_perception_audible_count=" << game.gameplay_systems_perception_audible_count()
        << " gameplay_systems_perception_readiness_status="
        << ai_perception_readiness_status_name(game.gameplay_systems_perception_readiness_status())
        << " gameplay_systems_perception_readiness_diagnostic="
        << ai_perception_readiness_diagnostic_name(game.gameplay_systems_perception_readiness_diagnostic())
        << " gameplay_systems_perception_readiness_diagnostics="
        << game.gameplay_systems_perception_readiness_diagnostics()
        << " gameplay_systems_perception_stable_primary_target_ready="
        << (game.gameplay_systems_perception_stable_primary_target_ready() ? 1 : 0)
        << " gameplay_systems_perception_blackboard_projection_ready="
        << (game.gameplay_systems_perception_blackboard_projection_ready() ? 1 : 0)
        << " gameplay_systems_blackboard_status="
        << ai_perception_blackboard_status_name(game.gameplay_systems_blackboard_status())
        << " gameplay_systems_blackboard_has_target=" << (game.gameplay_systems_blackboard_has_target() ? 1 : 0)
        << " gameplay_systems_blackboard_needs_move=" << (game.gameplay_systems_blackboard_needs_move() ? 1 : 0)
        << " gameplay_systems_behavior_status=" << behavior_tree_status_name(game.gameplay_systems_behavior_status())
        << " gameplay_systems_behavior_nodes=" << game.gameplay_systems_behavior_nodes()
        << " gameplay_systems_behavior_authoring_ready=" << (game.gameplay_systems_behavior_authoring_ready() ? 1 : 0)
        << " gameplay_systems_behavior_authoring_readiness_status="
        << behavior_authoring_readiness_status_name(game.gameplay_systems_behavior_authoring_readiness_status())
        << " gameplay_systems_behavior_authoring_readiness_diagnostic="
        << behavior_authoring_readiness_diagnostic_name(game.gameplay_systems_behavior_authoring_readiness_diagnostic())
        << " gameplay_systems_behavior_authoring_diagnostics=" << game.gameplay_systems_behavior_authoring_diagnostics()
        << " gameplay_systems_behavior_authoring_readiness_diagnostics="
        << game.gameplay_systems_behavior_authoring_readiness_diagnostics()
        << " gameplay_systems_behavior_authoring_trace_nodes=" << game.gameplay_systems_behavior_authoring_trace_nodes()
        << " gameplay_systems_behavior_authoring_deterministic_trace_ready="
        << (game.gameplay_systems_behavior_authoring_deterministic_trace_ready() ? 1 : 0)
        << " gameplay_systems_behavior_authoring_behaviors=" << game.gameplay_systems_behavior_authoring_behaviors()
        << " gameplay_systems_behavior_authoring_action_bindings="
        << game.gameplay_systems_behavior_authoring_action_bindings()
        << " gameplay_systems_behavior_authoring_blackboard_conditions="
        << game.gameplay_systems_behavior_authoring_blackboard_conditions()
        << " gameplay_systems_quest_dialogue_ready=" << (game.gameplay_systems_quest_dialogue_ready() ? 1 : 0)
        << " gameplay_systems_quest_dialogue_diagnostics=" << game.gameplay_systems_quest_dialogue_diagnostics()
        << " gameplay_systems_quest_dialogue_transition_rows=" << game.gameplay_systems_quest_dialogue_transition_rows()
        << " gameplay_systems_quest_dialogue_completed_objectives="
        << game.gameplay_systems_quest_dialogue_completed_objectives()
        << " gameplay_systems_quest_dialogue_flags=" << game.gameplay_systems_quest_dialogue_flags()
        << " gameplay_systems_quest_dialogue_dialogue_nodes=" << game.gameplay_systems_quest_dialogue_dialogue_nodes()
        << " gameplay_systems_quest_dialogue_action_ids=" << game.gameplay_systems_quest_dialogue_action_ids()
        << " gameplay_systems_quest_dialogue_reward_ids=" << game.gameplay_systems_quest_dialogue_reward_ids()
        << " gameplay_systems_quest_dialogue_state_rows=" << game.gameplay_systems_quest_dialogue_state_rows()
        << " gameplay_systems_inventory_items_ready=" << (game.gameplay_systems_inventory_items_ready() ? 1 : 0)
        << " gameplay_systems_inventory_items_diagnostics=" << game.gameplay_systems_inventory_items_diagnostics()
        << " gameplay_systems_inventory_items_catalog_rows=" << game.gameplay_systems_inventory_items_catalog_rows()
        << " gameplay_systems_inventory_items_state_rows=" << game.gameplay_systems_inventory_items_state_rows()
        << " gameplay_systems_inventory_items_transition_rows="
        << game.gameplay_systems_inventory_items_transition_rows()
        << " gameplay_systems_inventory_items_accepted_rows=" << game.gameplay_systems_inventory_items_accepted_rows()
        << " gameplay_systems_inventory_items_completed_rows=" << game.gameplay_systems_inventory_items_completed_rows()
        << " gameplay_systems_inventory_items_final_stacks=" << game.gameplay_systems_inventory_items_final_stacks()
        << " gameplay_systems_inventory_items_final_workbench_quantity="
        << game.gameplay_systems_inventory_items_final_workbench_quantity()
        << " gameplay_systems_interaction_ready=" << (game.gameplay_systems_interaction_ready() ? 1 : 0)
        << " gameplay_systems_interaction_diagnostics=" << game.gameplay_systems_interaction_diagnostics()
        << " gameplay_systems_interaction_rows=" << game.gameplay_systems_interaction_rows()
        << " gameplay_systems_interaction_feedback_rows=" << game.gameplay_systems_interaction_feedback_rows()
        << " gameplay_systems_interaction_final_session_state="
        << runtime_gameplay_session_state_name(game.gameplay_systems_interaction_final_session_state())
        << " gameplay_systems_scene_binding_ready=" << (game.gameplay_systems_scene_binding_ready() ? 1 : 0)
        << " gameplay_systems_scene_binding_source_rows=" << game.gameplay_systems_scene_binding_source_rows()
        << " gameplay_systems_scene_binding_rows=" << game.gameplay_systems_scene_binding_rows()
        << " gameplay_systems_scene_binding_systems=" << game.gameplay_systems_scene_binding_systems()
        << " gameplay_systems_scene_binding_component_rows=" << game.gameplay_systems_scene_binding_component_rows()
        << " gameplay_systems_scene_binding_diagnostics=" << game.gameplay_systems_scene_binding_diagnostics()
        << " gameplay_systems_scene_interaction_rows=" << game.gameplay_systems_scene_interaction_rows()
        << " gameplay_systems_scene_interaction_diagnostics=" << game.gameplay_systems_scene_interaction_diagnostics()
        << " gameplay_systems_scene_interaction_final_session_state="
        << runtime_scene_gameplay_session_state_name(game.gameplay_systems_scene_interaction_final_session_state())
        << " input_context_rebinding_ready=" << (input_context_rebinding.ready ? 1 : 0)
        << " input_context_rebinding_layers=" << input_context_rebinding.requested_layers
        << " input_context_rebinding_active_contexts=" << input_context_rebinding.active_contexts
        << " input_context_rebinding_capture_active=" << (input_context_rebinding.capture_context_active ? 1 : 0)
        << " input_context_rebinding_gameplay_consumed=" << (input_context_rebinding.gameplay_input_consumed ? 1 : 0)
        << " input_rebinding_profile_overlays_applied=" << input_context_rebinding.profile_overlays_applied
        << " input_rebinding_action_capture_status="
        << runtime_input_rebinding_capture_status_name(input_context_rebinding.action_capture_status)
        << " input_rebinding_axis_capture_status="
        << runtime_input_rebinding_capture_status_name(input_context_rebinding.axis_capture_status)
        << " input_rebinding_focus_consumed=" << (input_context_rebinding.focus_gameplay_consumed ? 1 : 0)
        << " input_rebinding_focus_retained=" << (input_context_rebinding.focus_retained ? 1 : 0)
        << " input_rebinding_presentation_rows=" << input_context_rebinding.presentation_rows
        << " input_rebinding_glyph_lookup_keys=" << input_context_rebinding.glyph_lookup_keys
        << " input_rebinding_diagnostics=" << input_context_rebinding.diagnostics
        << " sprite_collision_hitbox_ready=" << (game.sprite_collision_hitbox_ready() ? 1 : 0)
        << " sprite_collision_hitbox_hits=" << game.sprite_collision_hitbox_hits()
        << " sprite_collision_hitbox_gameplay_events=" << game.sprite_collision_hitbox_gameplay_events()
        << " sprite_collision_hitbox_interaction_rows=" << game.sprite_collision_hitbox_interaction_rows()
        << " sprite_collision_hitbox_feedback_rows=" << game.sprite_collision_hitbox_feedback_rows()
        << " sprite_collision_hitbox_diagnostics=" << game.sprite_collision_hitbox_diagnostics()
        << " sprite_effect_particles_status="
        << sprite_effect_particle_status_name(game.sprite_effect_particles_status())
        << " sprite_effect_particles_ready=" << (game.sprite_effect_particles_ready() ? 1 : 0)
        << " sprite_effect_particles_spawn_events=" << game.sprite_effect_particles_spawn_events()
        << " sprite_effect_particles_spawned_particles=" << game.sprite_effect_particles_spawned_particles()
        << " sprite_effect_particles_surviving_particles=" << game.sprite_effect_particles_surviving_particles()
        << " sprite_effect_particles_render_rows=" << game.sprite_effect_particles_render_rows()
        << " sprite_effect_particles_expired_particles=" << game.sprite_effect_particles_expired_particles()
        << " sprite_effect_particles_submitted=" << game.sprite_effect_particles_submitted()
        << " sprite_effect_particles_draws=" << game.sprite_effect_particles_draws()
        << " sprite_effect_particles_diagnostics=" << game.sprite_effect_particles_diagnostics()
        << " sprite_effect_particles_budget_diagnostics=" << game.sprite_effect_particles_budget_diagnostics()
        << " runtime_profile_resume_status="
        << runtime_session_profile_resume_status_name(game.gameplay_systems_runtime_profile_resume_status())
        << " runtime_profile_resume_ready=" << (game.gameplay_systems_runtime_profile_resume_ready() ? 1 : 0)
        << " runtime_profile_resume_diagnostics=" << game.gameplay_systems_runtime_profile_resume_diagnostics()
        << " runtime_profile_resume_loaded_documents="
        << game.gameplay_systems_runtime_profile_resume_loaded_documents()
        << " runtime_profile_resume_defaulted_documents="
        << game.gameplay_systems_runtime_profile_resume_defaulted_documents()
        << " runtime_profile_resume_save_schema_version="
        << game.gameplay_systems_runtime_profile_resume_save_schema_version()
        << " runtime_profile_resume_settings_schema_version="
        << game.gameplay_systems_runtime_profile_resume_settings_schema_version()
        << " runtime_menu_hud_ready=" << (game.gameplay_systems_runtime_menu_hud_ready() ? 1 : 0)
        << " runtime_menu_hud_diagnostics=" << game.gameplay_systems_runtime_menu_hud_diagnostics()
        << " runtime_menu_hud_display_rows=" << game.gameplay_systems_runtime_menu_hud_display_rows()
        << " runtime_menu_hud_command_rows=" << game.gameplay_systems_runtime_menu_hud_command_rows()
        << " runtime_menu_hud_dialogue_rows=" << game.gameplay_systems_runtime_menu_hud_dialogue_rows()
        << " runtime_menu_hud_input_binding_prompt_rows="
        << game.gameplay_systems_runtime_menu_hud_input_binding_prompt_rows()
        << " gameplay_systems_construction_placement_ready="
        << (game.gameplay_systems_construction_placement_ready() ? 1 : 0)
        << " gameplay_systems_construction_placement_diagnostics="
        << game.gameplay_systems_construction_placement_diagnostics()
        << " gameplay_systems_construction_placement_validation_rows="
        << game.gameplay_systems_construction_placement_validation_rows()
        << " gameplay_systems_construction_placement_intent_rows="
        << game.gameplay_systems_construction_placement_intent_rows()
        << " gameplay_systems_construction_placement_intent_accepted_rows="
        << game.gameplay_systems_construction_placement_intent_accepted_rows()
        << " gameplay_systems_construction_placement_intent_occupied_cells="
        << game.gameplay_systems_construction_placement_intent_occupied_cells()
        << " gameplay_systems_procedural_generation_ready="
        << (game.gameplay_systems_procedural_generation_ready() ? 1 : 0)
        << " gameplay_systems_procedural_generation_diagnostics="
        << game.gameplay_systems_procedural_generation_diagnostics()
        << " gameplay_systems_procedural_generation_rows=" << game.gameplay_systems_procedural_generation_rows()
        << " gameplay_systems_procedural_generation_object_rows="
        << game.gameplay_systems_procedural_generation_object_rows()
        << " gameplay_systems_procedural_generation_encounter_rows="
        << game.gameplay_systems_procedural_generation_encounter_rows()
        << " gameplay_systems_procedural_generation_loot_rows="
        << game.gameplay_systems_procedural_generation_loot_rows()
        << " gameplay_systems_procedural_generation_replay_hash="
        << game.gameplay_systems_procedural_generation_replay_hash()
        << " gameplay_systems_procedural_generation_package_visible_rows="
        << game.gameplay_systems_procedural_generation_package_visible_rows()
        << " gameplay_systems_procedural_generation_placement_intent_rows="
        << game.gameplay_systems_procedural_generation_placement_intent_rows()
        << " gameplay_systems_procedural_generation_placement_intent_accepted_rows="
        << game.gameplay_systems_procedural_generation_placement_intent_accepted_rows()
        << " rpg_systems_status=" << rpg_systems_status_name(rpg_systems_probe.status)
        << " rpg_systems_ready=" << (rpg_systems_probe.ready ? 1 : 0)
        << " rpg_systems_party_members=" << rpg_systems_probe.party_members
        << " rpg_systems_enemy_members=" << rpg_systems_probe.enemy_members
        << " rpg_systems_stat_rows=" << rpg_systems_probe.stat_rows
        << " rpg_systems_progression_rows=" << rpg_systems_probe.progression_rows
        << " rpg_systems_skill_rows=" << rpg_systems_probe.skill_rows
        << " rpg_systems_skill_blocked_rows=" << rpg_systems_probe.blocked_skill_rows
        << " rpg_systems_equipment_rows=" << rpg_systems_probe.equipment_rows
        << " rpg_systems_equipment_blocked_rows=" << rpg_systems_probe.blocked_equipment_rows
        << " rpg_systems_combat_turn_rows=" << rpg_systems_probe.combat_turn_rows
        << " rpg_systems_combat_rounds=" << rpg_systems_probe.combat_rounds
        << " rpg_systems_reward_rows=" << rpg_systems_probe.reward_rows
        << " rpg_systems_save_validation_rows=" << rpg_systems_probe.save_validation_rows
        << " rpg_systems_save_validation_repairable_rows=" << rpg_systems_probe.repairable_save_validation_rows
        << " rpg_systems_replay_hash=" << rpg_systems_probe.replay_hash
        << " rpg_systems_invoked_combat_execution=" << (rpg_systems_probe.invoked_combat_execution ? 1 : 0)
        << " rpg_systems_invoked_reward_application=" << (rpg_systems_probe.invoked_reward_application ? 1 : 0)
        << " rpg_systems_invoked_save_io=" << (rpg_systems_probe.invoked_save_io ? 1 : 0)
        << " rpg_systems_diagnostics=" << rpg_systems_probe.diagnostics
        << " sandbox_world_status=" << sandbox_world_status_name(sandbox_world_probe.status)
        << " sandbox_world_ready=" << (sandbox_world_probe.ready ? 1 : 0)
        << " sandbox_world_chunk_rows=" << sandbox_world_probe.chunk_rows
        << " sandbox_world_resident_chunk_rows=" << sandbox_world_probe.resident_chunk_rows
        << " sandbox_world_existing_cell_rows=" << sandbox_world_probe.existing_cell_rows
        << " sandbox_world_placement_intent_rows=" << sandbox_world_probe.placement_intent_rows
        << " sandbox_world_placement_accepted_rows=" << sandbox_world_probe.placement_accepted_rows
        << " sandbox_world_placement_rejected_rows=" << sandbox_world_probe.placement_rejected_rows
        << " sandbox_world_destruction_intent_rows=" << sandbox_world_probe.destruction_intent_rows
        << " sandbox_world_destruction_accepted_rows=" << sandbox_world_probe.destruction_accepted_rows
        << " sandbox_world_destruction_rejected_rows=" << sandbox_world_probe.destruction_rejected_rows
        << " sandbox_world_construction_cost_rows=" << sandbox_world_probe.construction_cost_rows
        << " sandbox_world_mutation_rows=" << sandbox_world_probe.mutation_rows
        << " sandbox_world_persistence_rows=" << sandbox_world_probe.persistence_rows
        << " sandbox_world_persistence_repairable_rows=" << sandbox_world_probe.persistence_repairable_rows
        << " sandbox_world_rejected_unsafe_mutation_rows=" << sandbox_world_probe.rejected_unsafe_mutation_rows
        << " sandbox_world_replay_hash=" << sandbox_world_probe.replay_hash
        << " sandbox_world_invoked_world_mutation=" << (sandbox_world_probe.invoked_world_mutation ? 1 : 0)
        << " sandbox_world_invoked_persistence_io=" << (sandbox_world_probe.invoked_persistence_io ? 1 : 0)
        << " sandbox_world_invoked_package_io=" << (sandbox_world_probe.invoked_package_io ? 1 : 0)
        << " sandbox_world_diagnostics=" << sandbox_world_probe.diagnostics
        << " world_region_streaming_status=" << world_region_streaming_status_name(world_region_streaming_probe.status)
        << " world_region_streaming_ready=" << (world_region_streaming_probe.ready ? 1 : 0)
        << " world_region_streaming_plan_rows=" << world_region_streaming_probe.plan_rows
        << " world_region_streaming_load_rows=" << world_region_streaming_probe.load_rows
        << " world_region_streaming_keep_rows=" << world_region_streaming_probe.keep_rows
        << " world_region_streaming_unload_rows=" << world_region_streaming_probe.unload_rows
        << " world_region_streaming_safe_point_rows=" << world_region_streaming_probe.safe_point_rows
        << " world_region_streaming_committed=" << (world_region_streaming_probe.committed_rows > 0U ? 1 : 0)
        << " world_region_streaming_committed_rows=" << world_region_streaming_probe.committed_rows
        << " world_region_streaming_reviewed_package_adoptions="
        << world_region_streaming_probe.reviewed_package_adoptions
        << " world_region_streaming_projected_regions=" << world_region_streaming_probe.projected_regions
        << " world_region_streaming_projected_bytes=" << world_region_streaming_probe.projected_bytes
        << " world_region_streaming_budget_bytes=" << world_region_streaming_probe.budget_bytes
        << " world_region_streaming_missing_region_diagnostics="
        << world_region_streaming_probe.missing_region_diagnostics
        << " world_region_streaming_safe_point_diagnostics=" << world_region_streaming_probe.safe_point_diagnostics
        << " world_region_streaming_large_scene_readiness_status="
        << world_streaming_large_scene_readiness_status_name(world_region_streaming_probe.large_scene_readiness_status)
        << " world_region_streaming_large_scene_readiness_diagnostic="
        << world_streaming_large_scene_readiness_diagnostic_name(
               world_region_streaming_probe.large_scene_readiness_diagnostic)
        << " world_region_streaming_large_scene_readiness_diagnostics="
        << world_region_streaming_probe.large_scene_readiness_diagnostics
        << " world_region_streaming_navigation_resident_regions="
        << world_region_streaming_probe.navigation_resident_regions
        << " world_region_streaming_navigation_missing_resident_regions="
        << world_region_streaming_probe.navigation_missing_resident_regions
        << " world_region_streaming_navigation_path_cache_ready="
        << (world_region_streaming_probe.navigation_path_cache_ready ? 1 : 0)
        << " entity_scale_culling_status=" << entity_scale_culling_status_name(entity_scale_culling_probe.status)
        << " entity_scale_culling_ready=" << (entity_scale_culling_probe.ready ? 1 : 0)
        << " entity_scale_culling_rows=" << entity_scale_culling_probe.rows
        << " entity_scale_culling_visible_rows=" << entity_scale_culling_probe.visible_rows
        << " entity_scale_culling_culled_rows=" << entity_scale_culling_probe.culled_rows
        << " entity_scale_culling_lod_rows=" << entity_scale_culling_probe.lod_rows
        << " entity_scale_culling_priority_update_rows=" << entity_scale_culling_probe.priority_update_rows
        << " entity_scale_culling_normal_update_rows=" << entity_scale_culling_probe.normal_update_rows
        << " entity_scale_culling_background_update_rows=" << entity_scale_culling_probe.background_update_rows
        << " entity_scale_culling_projected_draw_cost=" << entity_scale_culling_probe.projected_draw_cost
        << " entity_scale_culling_projected_update_cost=" << entity_scale_culling_probe.projected_update_cost
        << " entity_scale_culling_budget_protected_rows=" << entity_scale_culling_probe.budget_protected_rows
        << " entity_scale_culling_diagnostics=" << entity_scale_culling_probe.diagnostics
        << " entity_scale_culling_budget_diagnostics=" << entity_scale_culling_probe.budget_diagnostics
        << " scripting_sandbox_status=" << scripting_sandbox_status_name(scripting_sandbox_probe.status)
        << " scripting_sandbox_ready=" << (scripting_sandbox_probe.ready ? 1 : 0)
        << " scripting_sandbox_entrypoint_rows=" << scripting_sandbox_probe.entrypoint_rows
        << " scripting_sandbox_permission_rows=" << scripting_sandbox_probe.permission_rows
        << " scripting_sandbox_allowed_permission_rows=" << scripting_sandbox_probe.allowed_permission_rows
        << " scripting_sandbox_denied_permission_rows=" << scripting_sandbox_probe.denied_permission_rows
        << " scripting_sandbox_rejected_unsafe_capability_rows="
        << scripting_sandbox_probe.rejected_unsafe_capability_rows
        << " scripting_sandbox_unsupported_host_api_diagnostics="
        << scripting_sandbox_probe.unsupported_host_api_diagnostics
        << " scripting_sandbox_budget_rows=" << scripting_sandbox_probe.budget_rows
        << " scripting_sandbox_projected_instruction_budget=" << scripting_sandbox_probe.projected_instruction_budget
        << " scripting_sandbox_projected_memory_budget_bytes=" << scripting_sandbox_probe.projected_memory_budget_bytes
        << " scripting_sandbox_budget_diagnostics=" << scripting_sandbox_probe.budget_diagnostics
        << " scripting_sandbox_replay_seed_rows=" << scripting_sandbox_probe.replay_seed_rows
        << " scripting_sandbox_replay_seed_sum=" << scripting_sandbox_probe.replay_seed_sum
        << " scripting_sandbox_diagnostics=" << scripting_sandbox_probe.diagnostics
        << " scripting_sandbox_execution_status="
        << scripting_sandbox_execution_status_name(scripting_sandbox_probe.execution_status)
        << " scripting_sandbox_execution_ready=" << (scripting_sandbox_probe.execution_ready ? 1 : 0)
        << " scripting_sandbox_execution_dispatches=" << scripting_sandbox_probe.execution_dispatches
        << " scripting_sandbox_execution_host_api_calls=" << scripting_sandbox_probe.execution_host_api_calls
        << " scripting_sandbox_execution_replay_signature=" << scripting_sandbox_probe.execution_replay_signature
        << " scripting_sandbox_execution_diagnostics=" << scripting_sandbox_probe.execution_diagnostics
        << " networking_foundation_status=" << networking_foundation_status_name(networking_foundation_probe.status)
        << " networking_foundation_ready=" << (networking_foundation_probe.ready ? 1 : 0)
        << " networking_foundation_session_rows=" << networking_foundation_probe.session_rows
        << " networking_foundation_transport_rows=" << networking_foundation_probe.transport_rows
        << " networking_foundation_channel_rows=" << networking_foundation_probe.channel_rows
        << " networking_foundation_rejected_unsafe_transport_rows="
        << networking_foundation_probe.rejected_unsafe_transport_rows
        << " networking_foundation_replay_prerequisite_rows=" << networking_foundation_probe.replay_prerequisite_rows
        << " networking_foundation_replay_seed_sum=" << networking_foundation_probe.replay_seed_sum
        << " networking_foundation_remote_session_rows=" << networking_foundation_probe.remote_session_rows
        << " networking_foundation_secure_remote_session_rows="
        << networking_foundation_probe.secure_remote_session_rows
        << " networking_foundation_security_diagnostics=" << networking_foundation_probe.security_diagnostics
        << " networking_foundation_diagnostics=" << networking_foundation_probe.diagnostics
        << " simulation_orchestration_status="
        << simulation_orchestration_status_name(simulation_orchestration_probe.status)
        << " simulation_orchestration_ready=" << (simulation_orchestration_probe.ready ? 1 : 0)
        << " simulation_orchestration_available_steps=" << simulation_orchestration_probe.available_steps
        << " simulation_orchestration_planned_steps=" << simulation_orchestration_probe.planned_steps
        << " simulation_orchestration_step_rows=" << simulation_orchestration_probe.step_rows
        << " simulation_orchestration_command_rows=" << simulation_orchestration_probe.command_rows
        << " simulation_orchestration_command_playback_rows=" << simulation_orchestration_probe.command_playback_rows
        << " simulation_orchestration_consumed_time_us=" << simulation_orchestration_probe.consumed_time_us
        << " simulation_orchestration_remaining_time_us=" << simulation_orchestration_probe.remaining_time_us
        << " simulation_orchestration_budget_limited_status="
        << simulation_orchestration_status_name(simulation_orchestration_probe.budget_limited_status)
        << " simulation_orchestration_budget_limited_available_steps="
        << simulation_orchestration_probe.budget_limited_available_steps
        << " simulation_orchestration_budget_limited_planned_steps="
        << simulation_orchestration_probe.budget_limited_planned_steps
        << " simulation_orchestration_budget_limited_remaining_time_us="
        << simulation_orchestration_probe.budget_limited_remaining_time_us
        << " simulation_orchestration_invalid_command_diagnostics="
        << simulation_orchestration_probe.invalid_command_diagnostics
        << " simulation_orchestration_diagnostics=" << simulation_orchestration_probe.diagnostics
        << " simulation_management_status=" << simulation_management_status_name(simulation_management_probe.status)
        << " simulation_management_ready=" << (simulation_management_probe.ready ? 1 : 0)
        << " simulation_management_tick_count=" << simulation_management_probe.tick_count
        << " simulation_management_resource_balance_rows=" << simulation_management_probe.resource_balance_rows
        << " simulation_management_job_rows=" << simulation_management_probe.job_rows
        << " simulation_management_job_assignment_rows=" << simulation_management_probe.job_assignment_rows
        << " simulation_management_logistics_links=" << simulation_management_probe.logistics_links
        << " simulation_management_logistics_transfer_rows=" << simulation_management_probe.logistics_transfer_rows
        << " simulation_management_logistics_scheduled_transfer_rows="
        << simulation_management_probe.logistics_scheduled_transfer_rows
        << " simulation_management_economy_summary_rows=" << simulation_management_probe.economy_summary_rows
        << " simulation_management_population_need_rows=" << simulation_management_probe.population_need_rows
        << " simulation_management_need_deficit_rows=" << simulation_management_probe.need_deficit_rows
        << " simulation_management_schedule_rows=" << simulation_management_probe.schedule_rows
        << " simulation_management_save_review_rows=" << simulation_management_probe.save_review_rows
        << " simulation_management_save_review_repairable_rows="
        << simulation_management_probe.save_review_repairable_rows
        << " simulation_management_dashboard_rows=" << simulation_management_probe.dashboard_rows
        << " simulation_management_replay_hash=" << simulation_management_probe.replay_hash
        << " simulation_management_invoked_economy_execution="
        << (simulation_management_probe.invoked_economy_execution ? 1 : 0)
        << " simulation_management_invoked_save_io=" << (simulation_management_probe.invoked_save_io ? 1 : 0)
        << " simulation_management_invoked_runtime_ui=" << (simulation_management_probe.invoked_runtime_ui ? 1 : 0)
        << " simulation_management_invoked_package_io=" << (simulation_management_probe.invoked_package_io ? 1 : 0)
        << " simulation_management_diagnostics=" << simulation_management_probe.diagnostics
        << " network_replication_status=" << network_replication_status_name(network_replication_probe.status)
        << " network_replication_reviewed=" << (network_replication_probe.reviewed ? 1 : 0)
        << " network_replication_ready=" << (network_replication_probe.ready ? 1 : 0)
        << " network_replication_object_rows=" << network_replication_probe.object_rows
        << " network_replication_input_rows=" << network_replication_probe.input_rows
        << " network_replication_snapshot_rows=" << network_replication_probe.snapshot_rows
        << " network_replication_rollback_rows=" << network_replication_probe.rollback_rows
        << " network_replication_rejected_unsafe_rows=" << network_replication_probe.rejected_unsafe_rows
        << " network_replication_replay_hash=" << network_replication_probe.replay_hash
        << " network_replication_requires_transport_host_evidence="
        << (network_replication_probe.requires_transport_host_evidence ? 1 : 0)
        << " network_replication_transport_host_evidence="
        << (network_replication_probe.has_transport_host_evidence ? 1 : 0)
        << " network_replication_invoked_network_io=" << (network_replication_probe.invoked_network_io ? 1 : 0)
        << " network_replication_invoked_rollback_execution="
        << (network_replication_probe.invoked_rollback_execution ? 1 : 0)
        << " network_replication_invoked_world_mutation=" << (network_replication_probe.invoked_world_mutation ? 1 : 0)
        << " network_replication_diagnostics=" << network_replication_probe.diagnostics
        << " network_production_security_status="
        << network_production_security_status_name(network_production_security_probe.status)
        << " network_production_security_reviewed=" << (network_production_security_probe.reviewed ? 1 : 0)
        << " network_production_security_ready=" << (network_production_security_probe.ready ? 1 : 0)
        << " network_production_security_threat_model_reviewed="
        << (network_production_security_probe.threat_model_reviewed ? 1 : 0)
        << " network_production_security_loopback_host_evidence="
        << (network_production_security_probe.loopback_host_evidence ? 1 : 0)
        << " network_production_security_replication_evidence_ready="
        << (network_production_security_probe.replication_evidence_ready ? 1 : 0)
        << " network_production_security_general_online_ready="
        << (network_production_security_probe.general_online_ready ? 1 : 0)
        << " network_production_security_session_lifecycle_rows="
        << network_production_security_probe.session_lifecycle_rows
        << " network_production_security_connection_state_rows="
        << network_production_security_probe.connection_state_rows
        << " network_production_security_channel_policy_rows=" << network_production_security_probe.channel_policy_rows
        << " network_production_security_reliable_delivery_rows="
        << network_production_security_probe.reliable_delivery_rows
        << " network_production_security_unreliable_delivery_rows="
        << network_production_security_probe.unreliable_delivery_rows
        << " network_production_security_sequence_replay_rejection_rows="
        << network_production_security_probe.sequence_replay_rejection_rows
        << " network_production_security_input_command_validation_rows="
        << network_production_security_probe.input_command_validation_rows
        << " network_production_security_snapshot_validation_rows="
        << network_production_security_probe.snapshot_validation_rows
        << " network_production_security_rollback_window_diagnostic_rows="
        << network_production_security_probe.rollback_window_diagnostic_rows
        << " network_production_security_unsupported_online_claim_rows="
        << network_production_security_probe.unsupported_online_claim_rows
        << " network_production_security_replay_hash=" << network_production_security_probe.replay_hash
        << " network_production_security_invoked_external_network_io="
        << (network_production_security_probe.invoked_external_network_io ? 1 : 0)
        << " network_production_security_invoked_threads="
        << (network_production_security_probe.invoked_threads ? 1 : 0)
        << " network_production_security_invoked_save_io="
        << (network_production_security_probe.invoked_save_io ? 1 : 0)
        << " network_production_security_invoked_world_mutation="
        << (network_production_security_probe.invoked_world_mutation ? 1 : 0)
        << " network_production_security_diagnostics=" << network_production_security_probe.diagnostics
        << " gameplay_runtime_scheduler_status="
        << gameplay_runtime_scheduler_status_name(gameplay_runtime_scheduler_probe.status)
        << " gameplay_runtime_scheduler_ready=" << (gameplay_runtime_scheduler_probe.ready ? 1 : 0)
        << " gameplay_runtime_scheduler_available_steps=" << gameplay_runtime_scheduler_probe.available_steps
        << " gameplay_runtime_scheduler_steps=" << gameplay_runtime_scheduler_probe.step_rows
        << " gameplay_runtime_scheduler_system_rows=" << gameplay_runtime_scheduler_probe.system_rows
        << " gameplay_runtime_scheduler_command_rows=" << gameplay_runtime_scheduler_probe.command_rows
        << " gameplay_runtime_scheduler_consumed_time_us=" << gameplay_runtime_scheduler_probe.consumed_time_us
        << " gameplay_runtime_scheduler_remaining_time_us=" << gameplay_runtime_scheduler_probe.remaining_time_us
        << " gameplay_runtime_scheduler_budget_limited=" << (gameplay_runtime_scheduler_probe.budget_limited ? 1 : 0)
        << " gameplay_runtime_scheduler_replay_hash=" << gameplay_runtime_scheduler_probe.replay_hash
        << " gameplay_runtime_scheduler_diagnostics=" << gameplay_runtime_scheduler_probe.diagnostics
        << " world_entity_model_status=" << world_entity_model_status_name(world_entity_model_probe.status)
        << " world_entity_model_ready=" << (world_entity_model_probe.ready ? 1 : 0)
        << " world_entity_model_entities=" << world_entity_model_probe.entity_rows
        << " world_entity_model_components=" << world_entity_model_probe.component_rows
        << " world_entity_model_region_ownership_rows=" << world_entity_model_probe.region_ownership_rows
        << " world_entity_model_lifecycle_rows=" << world_entity_model_probe.lifecycle_rows
        << " world_entity_model_persistence_rows=" << world_entity_model_probe.persistence_rows
        << " world_entity_model_streaming_region_rows=" << world_entity_model_probe.streaming_region_rows
        << " world_entity_model_spawn_rows=" << world_entity_model_probe.spawn_rows
        << " world_entity_model_move_rows=" << world_entity_model_probe.move_rows
        << " world_entity_model_despawn_rows=" << world_entity_model_probe.despawn_rows
        << " world_entity_model_duplicate_entity_diagnostics=" << world_entity_model_probe.duplicate_entity_diagnostics
        << " world_entity_model_bridge_rejection_status="
        << world_entity_model_status_name(world_entity_model_probe.bridge_rejection_status)
        << " world_entity_model_bridge_rejection_diagnostics=" << world_entity_model_probe.bridge_rejection_diagnostics
        << " world_entity_model_bridge_rejection_persistence_rows="
        << world_entity_model_probe.bridge_rejection_persistence_rows
        << " world_entity_model_bridge_rejection_streaming_region_rows="
        << world_entity_model_probe.bridge_rejection_streaming_region_rows
        << " world_entity_model_bridge_rejection_streaming_diagnostics_present="
        << world_entity_model_probe.bridge_rejection_streaming_diagnostics_present
        << " world_entity_model_bridge_rejection_fail_closed="
        << (world_entity_model_probe.bridge_rejection_fail_closed ? 1 : 0)
        << " world_entity_model_diagnostics=" << world_entity_model_probe.diagnostics
        << " addressable_content_status=" << addressable_content_status_name(addressable_content_probe.status)
        << " addressable_content_ready=" << (addressable_content_probe.ready ? 1 : 0)
        << " addressable_content_address_rows=" << addressable_content_probe.address_rows
        << " addressable_content_dependency_rows=" << addressable_content_probe.dependency_rows
        << " addressable_content_load_rows=" << addressable_content_probe.load_rows
        << " addressable_content_release_rows=" << addressable_content_probe.release_rows
        << " addressable_content_refcount_rows=" << addressable_content_probe.refcount_rows
        << " addressable_content_resident_bytes=" << addressable_content_probe.resident_bytes
        << " addressable_content_resident_budget_bytes=" << addressable_content_probe.resident_budget_bytes
        << " addressable_content_budget_rejection_status="
        << addressable_content_status_name(addressable_content_probe.budget_rejection_status)
        << " addressable_content_budget_rejection_diagnostics="
        << addressable_content_probe.budget_rejection_diagnostics
        << " addressable_content_package_io=" << (addressable_content_probe.package_io ? 1 : 0)
        << " addressable_content_async_execution=" << (addressable_content_probe.async_execution ? 1 : 0)
        << " addressable_content_committed=" << (addressable_content_probe.committed ? 1 : 0)
        << " addressable_content_diagnostics=" << addressable_content_probe.diagnostics
        << " gameplay_authoring_review_status="
        << gameplay_authoring_review_status_name(gameplay_authoring_review_probe)
        << " gameplay_authoring_review_ready=" << (gameplay_authoring_review_probe.ready ? 1 : 0)
        << " gameplay_authoring_review_feature_rows=" << gameplay_authoring_review_probe.feature_rows
        << " gameplay_authoring_review_accepted_rows=" << gameplay_authoring_review_probe.accepted_rows
        << " gameplay_authoring_review_mutation_ledger_rows=" << gameplay_authoring_review_probe.mutation_ledger_rows
        << " gameplay_authoring_review_remediation_rows=" << gameplay_authoring_review_probe.remediation_rows
        << " gameplay_authoring_review_missing_required_capability_diagnostics="
        << gameplay_authoring_review_probe.missing_required_capability_diagnostics
        << " gameplay_authoring_review_missing_validation_recipe_diagnostics="
        << gameplay_authoring_review_probe.missing_validation_recipe_diagnostics
        << " gameplay_authoring_review_missing_package_evidence_diagnostics="
        << gameplay_authoring_review_probe.missing_package_evidence_diagnostics
        << " gameplay_authoring_review_unsupported_claim_diagnostics="
        << gameplay_authoring_review_probe.unsupported_claim_diagnostics
        << " gameplay_authoring_review_diagnostics=" << gameplay_authoring_review_probe.diagnostics
        << " production_authoring_workflow_status="
        << production_authoring_workflow_status_name(production_authoring_workflow_probe)
        << " production_authoring_workflow_ready=" << (production_authoring_workflow_probe.ready ? 1 : 0)
        << " production_authoring_workflow_rows=" << production_authoring_workflow_probe.workflow_rows
        << " production_authoring_workflow_accepted_rows=" << production_authoring_workflow_probe.accepted_rows
        << " production_authoring_workflow_mutation_ledger_rows="
        << production_authoring_workflow_probe.mutation_ledger_rows
        << " production_authoring_workflow_validation_repair_rows="
        << production_authoring_workflow_probe.validation_repair_rows
        << " production_authoring_workflow_shared_surface_mutation_diagnostics="
        << production_authoring_workflow_probe.shared_surface_mutation_diagnostics
        << " production_authoring_workflow_arbitrary_shell_diagnostics="
        << production_authoring_workflow_probe.arbitrary_shell_diagnostics
        << " production_authoring_workflow_cooked_package_mutation_diagnostics="
        << production_authoring_workflow_probe.cooked_package_mutation_diagnostics
        << " production_authoring_workflow_native_backend_term_diagnostics="
        << production_authoring_workflow_probe.native_backend_term_diagnostics
        << " production_authoring_workflow_invalid_target_path_diagnostics="
        << production_authoring_workflow_probe.invalid_target_path_diagnostics
        << " production_authoring_workflow_invoked_file_mutation="
        << (production_authoring_workflow_probe.invoked_file_mutation ? 1 : 0)
        << " production_authoring_workflow_invoked_package_io="
        << (production_authoring_workflow_probe.invoked_package_io ? 1 : 0)
        << " production_authoring_workflow_invoked_command_execution="
        << (production_authoring_workflow_probe.invoked_command_execution ? 1 : 0)
        << " production_authoring_workflow_diagnostics=" << production_authoring_workflow_probe.diagnostics
        << " runtime_ui_workbench_status=" << runtime_ui_workbench_status_name(runtime_ui_workbench_probe.status)
        << " runtime_ui_workbench_ready=" << (runtime_ui_workbench_probe.ready ? 1 : 0)
        << " runtime_ui_workbench_panels=" << runtime_ui_workbench_probe.panels
        << " runtime_ui_workbench_table_columns=" << runtime_ui_workbench_probe.table_columns
        << " runtime_ui_workbench_table_rows=" << runtime_ui_workbench_probe.table_rows
        << " runtime_ui_workbench_graph_series=" << runtime_ui_workbench_probe.graph_series
        << " runtime_ui_workbench_item_rows=" << runtime_ui_workbench_probe.item_rows
        << " runtime_ui_workbench_inventory_rows=" << runtime_ui_workbench_probe.inventory_rows
        << " runtime_ui_workbench_equipment_rows=" << runtime_ui_workbench_probe.equipment_rows
        << " runtime_ui_workbench_shop_rows=" << runtime_ui_workbench_probe.shop_rows
        << " runtime_ui_workbench_text_inputs=" << runtime_ui_workbench_probe.text_inputs
        << " runtime_ui_workbench_platform_text_input_requests="
        << runtime_ui_workbench_probe.platform_text_input_requests
        << " runtime_ui_workbench_focus_edges=" << runtime_ui_workbench_probe.focus_edges
        << " runtime_ui_workbench_localization_refs=" << runtime_ui_workbench_probe.localization_refs
        << " runtime_ui_workbench_localization_identity_ready="
        << (runtime_ui_workbench_probe.localization_identity_ready ? 1 : 0)
        << " runtime_ui_workbench_accessibility_refs=" << runtime_ui_workbench_probe.accessibility_refs
        << " runtime_ui_workbench_accessibility_identity_ready="
        << (runtime_ui_workbench_probe.accessibility_identity_ready ? 1 : 0)
        << " runtime_ui_workbench_renderer_submission=" << (runtime_ui_workbench_probe.renderer_submission ? 1 : 0)
        << " runtime_ui_workbench_text_shaping=" << (runtime_ui_workbench_probe.text_shaping ? 1 : 0)
        << " runtime_ui_workbench_font_rasterization=" << (runtime_ui_workbench_probe.font_rasterization ? 1 : 0)
        << " runtime_ui_workbench_ime_sessions=" << (runtime_ui_workbench_probe.ime_sessions ? 1 : 0)
        << " runtime_ui_workbench_accessibility_bridge=" << (runtime_ui_workbench_probe.accessibility_bridge ? 1 : 0)
        << " runtime_ui_workbench_image_decoding=" << (runtime_ui_workbench_probe.image_decoding ? 1 : 0)
        << " runtime_ui_workbench_native_platform=" << (runtime_ui_workbench_probe.native_platform ? 1 : 0)
        << " runtime_ui_workbench_diagnostics=" << runtime_ui_workbench_probe.diagnostics
        << " runtime_ui_production_stack_status="
        << mirakana::ui::runtime_ui_production_stack_status_name(runtime_ui_production_stack_probe.status)
        << " runtime_ui_production_stack_reviewed=" << (runtime_ui_production_stack_probe.reviewed ? 1 : 0)
        << " runtime_ui_production_stack_ready=" << (runtime_ui_production_stack_probe.package_evidence_ready ? 1 : 0)
        << " runtime_ui_production_stack_rows=" << runtime_ui_production_stack_probe.rows
        << " runtime_ui_production_stack_ready_rows=" << runtime_ui_production_stack_probe.ready_rows
        << " runtime_ui_production_stack_host_gated_rows=" << runtime_ui_production_stack_probe.host_gated_rows
        << " runtime_ui_production_stack_dependency_gated_rows="
        << runtime_ui_production_stack_probe.dependency_gated_rows
        << " runtime_ui_production_stack_skipped_rows=" << runtime_ui_production_stack_probe.skipped_rows
        << " runtime_ui_production_stack_adapter_invoked_rows="
        << runtime_ui_production_stack_probe.adapter_invoked_rows
        << " runtime_ui_production_stack_unsupported_rows=" << runtime_ui_production_stack_probe.unsupported_rows
        << " runtime_ui_production_stack_text_contract_ready="
        << (runtime_ui_production_stack_probe.text_contract_ready ? 1 : 0)
        << " runtime_ui_production_stack_selected_package_evidence_ready="
        << (runtime_ui_production_stack_probe.selected_package_evidence_ready ? 1 : 0)
        << " runtime_ui_production_stack_production_ready="
        << (runtime_ui_production_stack_probe.production_ready ? 1 : 0)
        << " runtime_ui_production_stack_requires_ime_host_evidence="
        << (runtime_ui_production_stack_probe.requires_ime_host_evidence ? 1 : 0)
        << " runtime_ui_production_stack_ime_host_evidence="
        << (runtime_ui_production_stack_probe.ime_host_evidence ? 1 : 0)
        << " runtime_ui_production_stack_ime_session_rows=" << runtime_ui_production_stack_probe.ime_session_rows
        << " runtime_ui_production_stack_ime_composition_rows="
        << runtime_ui_production_stack_probe.ime_composition_rows
        << " runtime_ui_production_stack_ime_candidate_rows=" << runtime_ui_production_stack_probe.ime_candidate_rows
        << " runtime_ui_production_stack_ime_text_area_cursor_rows="
        << runtime_ui_production_stack_probe.ime_text_area_cursor_rows
        << " runtime_ui_production_stack_ime_committed_text_rows="
        << runtime_ui_production_stack_probe.ime_committed_text_rows
        << " runtime_ui_production_stack_ime_clipboard_rows=" << runtime_ui_production_stack_probe.ime_clipboard_rows
        << " runtime_ui_production_stack_ime_platform_adapter_proof_rows="
        << runtime_ui_production_stack_probe.ime_platform_adapter_proof_rows
        << " runtime_ui_production_stack_ime_platform_host_gate_rows="
        << runtime_ui_production_stack_probe.ime_platform_host_gate_rows
        << " runtime_ui_production_stack_ime_platform_parity_ready="
        << (runtime_ui_production_stack_probe.ime_platform_parity_ready ? 1 : 0)
        << " runtime_ui_production_stack_requires_accessibility_host_evidence="
        << (runtime_ui_production_stack_probe.requires_accessibility_host_evidence ? 1 : 0)
        << " runtime_ui_production_stack_accessibility_host_evidence="
        << (runtime_ui_production_stack_probe.accessibility_host_evidence ? 1 : 0)
        << " runtime_ui_production_stack_accessibility_role_rows="
        << runtime_ui_production_stack_probe.accessibility_role_rows
        << " runtime_ui_production_stack_accessibility_name_rows="
        << runtime_ui_production_stack_probe.accessibility_name_rows
        << " runtime_ui_production_stack_accessibility_description_rows="
        << runtime_ui_production_stack_probe.accessibility_description_rows
        << " runtime_ui_production_stack_accessibility_state_rows="
        << runtime_ui_production_stack_probe.accessibility_state_rows
        << " runtime_ui_production_stack_accessibility_focus_rows="
        << runtime_ui_production_stack_probe.accessibility_focus_rows
        << " runtime_ui_production_stack_accessibility_action_rows="
        << runtime_ui_production_stack_probe.accessibility_action_rows
        << " runtime_ui_production_stack_accessibility_relationship_rows="
        << runtime_ui_production_stack_probe.accessibility_relationship_rows
        << " runtime_ui_production_stack_accessibility_live_region_rows="
        << runtime_ui_production_stack_probe.accessibility_live_region_rows
        << " runtime_ui_production_stack_accessibility_keyboard_pattern_rows="
        << runtime_ui_production_stack_probe.accessibility_keyboard_pattern_rows
        << " runtime_ui_production_stack_accessibility_publication_status_rows="
        << runtime_ui_production_stack_probe.accessibility_publication_status_rows
        << " runtime_ui_production_stack_accessibility_uia_host_gate_rows="
        << runtime_ui_production_stack_probe.accessibility_uia_host_gate_rows
        << " runtime_ui_production_stack_accessibility_platform_host_gate_rows="
        << runtime_ui_production_stack_probe.accessibility_platform_host_gate_rows
        << " runtime_ui_production_stack_accessibility_platform_parity_ready="
        << (runtime_ui_production_stack_probe.accessibility_platform_parity_ready ? 1 : 0)
        << " runtime_ui_production_stack_invoked_text_shaping="
        << (runtime_ui_production_stack_probe.invoked_text_shaping ? 1 : 0)
        << " runtime_ui_production_stack_invoked_font_rasterization="
        << (runtime_ui_production_stack_probe.invoked_font_rasterization ? 1 : 0)
        << " runtime_ui_production_stack_invoked_ime=" << (runtime_ui_production_stack_probe.invoked_ime ? 1 : 0)
        << " runtime_ui_production_stack_invoked_accessibility_bridge="
        << (runtime_ui_production_stack_probe.invoked_accessibility_bridge ? 1 : 0)
        << " runtime_ui_production_stack_invoked_native_platform="
        << (runtime_ui_production_stack_probe.invoked_native_platform ? 1 : 0)
        << " runtime_ui_production_stack_invoked_renderer_upload="
        << (runtime_ui_production_stack_probe.invoked_renderer_upload ? 1 : 0)
        << " runtime_ui_production_stack_diagnostics=" << runtime_ui_production_stack_probe.diagnostics
        << " runtime_ui_production_stack_replay_hash=" << runtime_ui_production_stack_probe.replay_hash
        << " runtime_ui_renderer_atlas_handoff_status="
        << runtime_ui_renderer_atlas_handoff_status_name(runtime_ui_renderer_atlas_handoff_probe.status)
        << " runtime_ui_renderer_atlas_handoff_ready=" << (runtime_ui_renderer_atlas_handoff_probe.ready ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_selected_package_evidence_ready="
        << (runtime_ui_renderer_atlas_handoff_probe.selected_package_evidence_ready ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_reviewed=" << (runtime_ui_renderer_atlas_handoff_probe.reviewed ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_image_atlas_pages="
        << runtime_ui_renderer_atlas_handoff_probe.image_atlas_pages
        << " runtime_ui_renderer_atlas_handoff_image_atlas_bindings="
        << runtime_ui_renderer_atlas_handoff_probe.image_atlas_bindings
        << " runtime_ui_renderer_atlas_handoff_glyph_atlas_pages="
        << runtime_ui_renderer_atlas_handoff_probe.glyph_atlas_pages
        << " runtime_ui_renderer_atlas_handoff_glyph_atlas_bindings="
        << runtime_ui_renderer_atlas_handoff_probe.glyph_atlas_bindings
        << " runtime_ui_renderer_atlas_handoff_atlas_placement_rows="
        << runtime_ui_renderer_atlas_handoff_probe.atlas_placement_rows
        << " runtime_ui_renderer_atlas_handoff_atlas_budget_rows="
        << runtime_ui_renderer_atlas_handoff_probe.atlas_budget_rows
        << " runtime_ui_renderer_atlas_handoff_atlas_eviction_diagnostic_rows="
        << runtime_ui_renderer_atlas_handoff_probe.atlas_eviction_diagnostic_rows
        << " runtime_ui_renderer_atlas_handoff_texture_upload_handoff_rows="
        << runtime_ui_renderer_atlas_handoff_probe.texture_upload_handoff_rows
        << " runtime_ui_renderer_atlas_handoff_renderer_submission_counter_rows="
        << runtime_ui_renderer_atlas_handoff_probe.renderer_submission_counter_rows
        << " runtime_ui_renderer_atlas_handoff_text_glyphs_available="
        << runtime_ui_renderer_atlas_handoff_probe.text_glyphs_available
        << " runtime_ui_renderer_atlas_handoff_text_glyphs_resolved="
        << runtime_ui_renderer_atlas_handoff_probe.text_glyphs_resolved
        << " runtime_ui_renderer_atlas_handoff_text_glyphs_missing="
        << runtime_ui_renderer_atlas_handoff_probe.text_glyphs_missing
        << " runtime_ui_renderer_atlas_handoff_text_glyph_sprites_submitted="
        << runtime_ui_renderer_atlas_handoff_probe.text_glyph_sprites_submitted
        << " runtime_ui_renderer_atlas_handoff_image_placeholders_available="
        << runtime_ui_renderer_atlas_handoff_probe.image_placeholders_available
        << " runtime_ui_renderer_atlas_handoff_image_resources_resolved="
        << runtime_ui_renderer_atlas_handoff_probe.image_resources_resolved
        << " runtime_ui_renderer_atlas_handoff_image_resources_missing="
        << runtime_ui_renderer_atlas_handoff_probe.image_resources_missing
        << " runtime_ui_renderer_atlas_handoff_image_sprites_submitted="
        << runtime_ui_renderer_atlas_handoff_probe.image_sprites_submitted
        << " runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted="
        << runtime_ui_renderer_atlas_handoff_probe.renderer_sprites_submitted
        << " runtime_ui_renderer_atlas_handoff_unsupported_claim_rows="
        << runtime_ui_renderer_atlas_handoff_probe.unsupported_claim_rows
        << " runtime_ui_renderer_atlas_handoff_side_effect_rows="
        << runtime_ui_renderer_atlas_handoff_probe.side_effect_rows
        << " runtime_ui_renderer_atlas_handoff_requested_renderer_texture_upload_api="
        << (runtime_ui_renderer_atlas_handoff_probe.requested_renderer_texture_upload_api ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_requested_public_native_handle="
        << (runtime_ui_renderer_atlas_handoff_probe.requested_public_native_handle ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_invoked_source_image_decode="
        << (runtime_ui_renderer_atlas_handoff_probe.invoked_source_image_decode ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_invoked_live_glyph_atlas_generation="
        << (runtime_ui_renderer_atlas_handoff_probe.invoked_live_glyph_atlas_generation ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_invoked_renderer_upload="
        << (runtime_ui_renderer_atlas_handoff_probe.invoked_renderer_upload ? 1 : 0)
        << " runtime_ui_renderer_atlas_handoff_diagnostics=" << runtime_ui_renderer_atlas_handoff_probe.diagnostics
        << " runtime_ui_renderer_atlas_handoff_replay_hash=" << runtime_ui_renderer_atlas_handoff_probe.replay_hash
        << " audio_production_status=" << audio_production_status_name(audio_production_probe.status)
        << " audio_production_reviewed=" << (audio_production_probe.reviewed ? 1 : 0)
        << " audio_production_ready=" << (audio_production_probe.production_audio_ready ? 1 : 0)
        << " audio_production_selected_package_ready="
        << (audio_production_probe.selected_package_evidence_ready ? 1 : 0)
        << " audio_production_package_evidence_ready=" << (audio_production_probe.package_evidence_ready ? 1 : 0)
        << " audio_production_decoded_source_rows=" << audio_production_probe.decoded_source_rows
        << " audio_production_streaming_chunk_rows=" << audio_production_probe.streaming_chunk_rows
        << " audio_production_format_conversion_policy_rows=" << audio_production_probe.format_conversion_policy_rows
        << " audio_production_bus_budget_rows=" << audio_production_probe.bus_budget_rows
        << " audio_production_voice_budget_rows=" << audio_production_probe.voice_budget_rows
        << " audio_production_dsp_graph_rows=" << audio_production_probe.dsp_graph_rows
        << " audio_production_listener_rows=" << audio_production_probe.listener_rows
        << " audio_production_spatial_source_rows=" << audio_production_probe.spatial_source_rows
        << " audio_production_hrtf_host_gate_rows=" << audio_production_probe.hrtf_host_gate_rows
        << " audio_production_device_lifecycle_rows=" << audio_production_probe.device_lifecycle_rows
        << " audio_production_device_host_evidence=" << (audio_production_probe.device_host_evidence_available ? 1 : 0)
        << " audio_production_hrtf_host_evidence=" << (audio_production_probe.hrtf_host_evidence_available ? 1 : 0)
        << " audio_production_unsupported_claim_rows=" << audio_production_probe.unsupported_claim_rows
        << " audio_production_invoked_codec_decode=" << (audio_production_probe.invoked_codec_decode ? 1 : 0)
        << " audio_production_invoked_background_streaming="
        << (audio_production_probe.invoked_background_streaming ? 1 : 0)
        << " audio_production_invoked_middleware=" << (audio_production_probe.invoked_middleware ? 1 : 0)
        << " audio_production_invoked_hrtf=" << (audio_production_probe.invoked_hrtf ? 1 : 0)
        << " audio_production_invoked_device_callback=" << (audio_production_probe.invoked_device_callback ? 1 : 0)
        << " audio_production_invoked_device_io=" << (audio_production_probe.invoked_device_io ? 1 : 0)
        << " audio_production_diagnostics=" << audio_production_probe.diagnostics
        << " audio_production_replay_hash=" << audio_production_probe.replay_hash
        << " hud_boxes=" << game.hud_boxes_submitted() << " audio_commands=" << game.audio_commands()
        << " audio_underruns=" << game.audio_underruns()
        << " audio_gameplay_mixer_ready=" << (audio_gameplay_mixer.ready ? 1 : 0)
        << " audio_gameplay_mixer_diagnostics=" << audio_gameplay_mixer.diagnostics
        << " audio_gameplay_mixer_buses=" << audio_gameplay_mixer.buses
        << " audio_gameplay_mixer_cues=" << audio_gameplay_mixer.cues
        << " audio_gameplay_mixer_triggers=" << audio_gameplay_mixer.triggers
        << " audio_gameplay_mixer_commands=" << audio_gameplay_mixer.commands
        << " audio_gameplay_mixer_paused_buses=" << audio_gameplay_mixer.paused_buses
        << " audio_gameplay_mixer_faded_buses=" << audio_gameplay_mixer.faded_buses
        << " audio_gameplay_mixer_looping_commands=" << audio_gameplay_mixer.looping_commands
        << " audio_gameplay_mixer_spatial_commands=" << audio_gameplay_mixer.spatial_commands
        << " audio_gameplay_mixer_render_commands=" << audio_gameplay_mixer.render_commands
        << " audio_gameplay_mixer_render_frames=" << audio_gameplay_mixer.render_frames
        << " audio_gameplay_mixer_render_samples=" << audio_gameplay_mixer.render_samples
        << " audio_gameplay_mixer_sample_abs_sum=" << audio_gameplay_mixer.sample_abs_sum
        << " audio_gameplay_mixer_payload_diagnostics=" << audio_gameplay_mixer.payload_diagnostics
        << " package_records=" << package_records << " package_scene_sprites=" << game.package_scene_sprites() << '\n';
    print_presentation_report("sample_2d_desktop_runtime_package", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                  << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }

    if (options.require_native_2d_sprites &&
        (!report.native_ui_overlay_requested || !report.native_ui_overlay_ready ||
         !report.native_ui_texture_overlay_atlas_ready || report.native_ui_overlay_sprites_submitted == 0 ||
         report.native_ui_texture_overlay_sprites_submitted == 0 ||
         report.native_ui_texture_overlay_texture_binds == 0 || report.native_ui_overlay_draws == 0 ||
         report.native_ui_texture_overlay_draws == 0 || report.renderer_stats.native_sprite_batches_executed == 0 ||
         report.renderer_stats.native_sprite_batch_sprites_executed == 0 ||
         report.renderer_stats.native_sprite_batch_textured_sprites_executed == 0 ||
         report.renderer_stats.native_sprite_batch_texture_binds == 0)) {
        std::cout << "sample_2d_desktop_runtime_package required_native_2d_sprites_unavailable renderer="
                  << host.presentation_backend_name() << " native_2d_sprites_status="
                  << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
                  << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
                  << " native_2d_textured_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
                  << " native_2d_texture_binds=" << report.native_ui_texture_overlay_texture_binds
                  << " native_2d_sprite_batches_executed=" << report.renderer_stats.native_sprite_batches_executed
                  << '\n';
        return 9;
    }

    if (options.require_sprite_animation &&
        (game.sprite_animation_frames_sampled() == 0 || game.sprite_animation_frames_applied() == 0 ||
         game.sprite_animation_selected_frame_sum() == 0 || game.sprite_animation_diagnostics() != 0 ||
         game.sprite_flipbook_ticks() == 0 || game.sprite_flipbook_frames_sampled() == 0 ||
         game.sprite_flipbook_frames_applied() == 0 || game.sprite_flipbook_selected_frame_sum() == 0 ||
         game.sprite_flipbook_direction_sets() != 2U || game.sprite_flipbook_event_rows() != 2U ||
         game.sprite_flipbook_playback_modes() != 2U || game.sprite_flipbook_gameplay_state_rows() != 2U ||
         game.sprite_flipbook_events_sampled() == 0 || game.sprite_flipbook_playback_diagnostics() != 0 ||
         game.sprite_flipbook_diagnostics() != 0)) {
        std::cout << "sample_2d_desktop_runtime_package required_sprite_animation_unavailable"
                  << " sprite_animation_frames_sampled=" << game.sprite_animation_frames_sampled()
                  << " sprite_animation_frames_applied=" << game.sprite_animation_frames_applied()
                  << " sprite_animation_diagnostics=" << game.sprite_animation_diagnostics()
                  << " sprite_flipbook_ticks=" << game.sprite_flipbook_ticks()
                  << " sprite_flipbook_frames_sampled=" << game.sprite_flipbook_frames_sampled()
                  << " sprite_flipbook_frames_applied=" << game.sprite_flipbook_frames_applied()
                  << " sprite_flipbook_diagnostics=" << game.sprite_flipbook_diagnostics()
                  << " sprite_flipbook_direction_sets=" << game.sprite_flipbook_direction_sets()
                  << " sprite_flipbook_event_rows=" << game.sprite_flipbook_event_rows()
                  << " sprite_flipbook_playback_modes=" << game.sprite_flipbook_playback_modes()
                  << " sprite_flipbook_gameplay_state_rows=" << game.sprite_flipbook_gameplay_state_rows()
                  << " sprite_flipbook_events_sampled=" << game.sprite_flipbook_events_sampled()
                  << " sprite_flipbook_playback_diagnostics=" << game.sprite_flipbook_playback_diagnostics() << '\n';
        return 10;
    }

    if (options.require_sprite_sorting_layer &&
        (game.sprite_sort_layers_applied() < 2U || game.sprite_sorted_draws() == 0U)) {
        std::cout << "sample_2d_desktop_runtime_package required_sprite_sorting_layer_unavailable"
                  << " sprite_sort_layers_applied=" << game.sprite_sort_layers_applied()
                  << " sprite_sorted_draws=" << game.sprite_sorted_draws() << '\n';
        return 23;
    }

    if (options.require_sprite_9slice_tiled &&
        (game.sprite_9slice_tiled_source_sprites() == 0U || game.sprite_9slice_expanded_quads() == 0U ||
         game.sprite_tiled_expanded_quads() == 0U)) {
        std::cout << "sample_2d_desktop_runtime_package required_sprite_9slice_tiled_unavailable"
                  << " sprite_9slice_tiled_source_sprites=" << game.sprite_9slice_tiled_source_sprites()
                  << " sprite_9slice_expanded_quads=" << game.sprite_9slice_expanded_quads()
                  << " sprite_tiled_expanded_quads=" << game.sprite_tiled_expanded_quads() << '\n';
        return 24;
    }

    if (options.require_sprite_collision_hitbox &&
        (!game.sprite_collision_hitbox_ready() || game.sprite_collision_hitbox_hits() != 1U ||
         game.sprite_collision_hitbox_gameplay_events() != 1U ||
         game.sprite_collision_hitbox_interaction_rows() != 1U || game.sprite_collision_hitbox_feedback_rows() != 1U ||
         game.sprite_collision_hitbox_diagnostics() != 0U)) {
        std::cout << "sample_2d_desktop_runtime_package required_sprite_collision_hitbox_unavailable"
                  << " sprite_collision_hitbox_ready=" << (game.sprite_collision_hitbox_ready() ? 1 : 0)
                  << " sprite_collision_hitbox_hits=" << game.sprite_collision_hitbox_hits()
                  << " sprite_collision_hitbox_gameplay_events=" << game.sprite_collision_hitbox_gameplay_events()
                  << " sprite_collision_hitbox_interaction_rows=" << game.sprite_collision_hitbox_interaction_rows()
                  << " sprite_collision_hitbox_feedback_rows=" << game.sprite_collision_hitbox_feedback_rows()
                  << " sprite_collision_hitbox_diagnostics=" << game.sprite_collision_hitbox_diagnostics() << '\n';
        return 25;
    }

    if (options.require_sprite_effects_particles && !game.sprite_effect_particles_ready()) {
        std::cout << "sample_2d_desktop_runtime_package required_sprite_effects_particles_unavailable"
                  << " sprite_effect_particles_status="
                  << sprite_effect_particle_status_name(game.sprite_effect_particles_status())
                  << " sprite_effect_particles_ready=" << (game.sprite_effect_particles_ready() ? 1 : 0)
                  << " sprite_effect_particles_spawn_events=" << game.sprite_effect_particles_spawn_events()
                  << " sprite_effect_particles_spawned_particles=" << game.sprite_effect_particles_spawned_particles()
                  << " sprite_effect_particles_render_rows=" << game.sprite_effect_particles_render_rows()
                  << " sprite_effect_particles_submitted=" << game.sprite_effect_particles_submitted()
                  << " sprite_effect_particles_diagnostics=" << game.sprite_effect_particles_diagnostics()
                  << " sprite_effect_particles_budget_diagnostics=" << game.sprite_effect_particles_budget_diagnostics()
                  << '\n';
        return 26;
    }

    if (options.require_tilemap_runtime_ux &&
        (game.tilemap_layers() == 0 || game.tilemap_visible_layers() == 0 || game.tilemap_tiles() == 0 ||
         game.tilemap_non_empty_cells() == 0 || game.tilemap_sampled_cells() == 0 || game.tilemap_diagnostics() != 0)) {
        std::cout << "sample_2d_desktop_runtime_package required_tilemap_runtime_ux_unavailable"
                  << " tilemap_layers=" << game.tilemap_layers()
                  << " tilemap_visible_layers=" << game.tilemap_visible_layers()
                  << " tilemap_cells_sampled=" << game.tilemap_sampled_cells()
                  << " tilemap_diagnostics=" << game.tilemap_diagnostics() << '\n';
        return 11;
    }

    if (options.require_procedural_generation && !game.gameplay_systems_procedural_generation_ready()) {
        std::cout << "sample_2d_desktop_runtime_package required_procedural_generation_unavailable"
                  << " gameplay_systems_procedural_generation_diagnostics="
                  << game.gameplay_systems_procedural_generation_diagnostics()
                  << " gameplay_systems_procedural_generation_rows="
                  << game.gameplay_systems_procedural_generation_rows()
                  << " gameplay_systems_procedural_generation_object_rows="
                  << game.gameplay_systems_procedural_generation_object_rows()
                  << " gameplay_systems_procedural_generation_encounter_rows="
                  << game.gameplay_systems_procedural_generation_encounter_rows()
                  << " gameplay_systems_procedural_generation_loot_rows="
                  << game.gameplay_systems_procedural_generation_loot_rows()
                  << " gameplay_systems_procedural_generation_replay_hash="
                  << game.gameplay_systems_procedural_generation_replay_hash()
                  << " gameplay_systems_procedural_generation_package_visible_rows="
                  << game.gameplay_systems_procedural_generation_package_visible_rows()
                  << " gameplay_systems_procedural_generation_placement_intent_rows="
                  << game.gameplay_systems_procedural_generation_placement_intent_rows() << '\n';
        return 13;
    }

    if (options.require_gameplay_systems &&
        (!game.gameplay_systems_passed(options.max_frames) || !game.gameplay_systems_scene_binding_ready() ||
         !input_context_rebinding.ready || !rpg_systems_probe.ready || !sandbox_world_probe.ready ||
         !simulation_management_probe.ready || !network_replication_probe.reviewed ||
         !network_production_security_probe.reviewed)) {
        std::cout
            << "sample_2d_desktop_runtime_package required_gameplay_systems_unavailable"
            << " gameplay_systems_status="
            << gameplay_2d_systems_status_name(game.gameplay_systems_status(options.max_frames))
            << " gameplay_systems_physics_contacts=" << game.gameplay_systems_physics_contacts()
            << " gameplay_systems_navigation_plan_status="
            << navigation_grid_agent_path_status_name(game.gameplay_systems_navigation_plan_status())
            << " gameplay_systems_behavior_status="
            << behavior_tree_status_name(game.gameplay_systems_behavior_status())
            << " gameplay_systems_behavior_authoring_ready="
            << (game.gameplay_systems_behavior_authoring_ready() ? 1 : 0)
            << " gameplay_systems_behavior_authoring_readiness_status="
            << behavior_authoring_readiness_status_name(game.gameplay_systems_behavior_authoring_readiness_status())
            << " gameplay_systems_behavior_authoring_readiness_diagnostic="
            << behavior_authoring_readiness_diagnostic_name(
                   game.gameplay_systems_behavior_authoring_readiness_diagnostic())
            << " gameplay_systems_behavior_authoring_diagnostics="
            << game.gameplay_systems_behavior_authoring_diagnostics()
            << " gameplay_systems_behavior_authoring_readiness_diagnostics="
            << game.gameplay_systems_behavior_authoring_readiness_diagnostics()
            << " gameplay_systems_behavior_authoring_trace_nodes="
            << game.gameplay_systems_behavior_authoring_trace_nodes()
            << " gameplay_systems_behavior_authoring_deterministic_trace_ready="
            << (game.gameplay_systems_behavior_authoring_deterministic_trace_ready() ? 1 : 0)
            << " gameplay_systems_quest_dialogue_ready=" << (game.gameplay_systems_quest_dialogue_ready() ? 1 : 0)
            << " gameplay_systems_quest_dialogue_diagnostics=" << game.gameplay_systems_quest_dialogue_diagnostics()
            << " gameplay_systems_quest_dialogue_transition_rows="
            << game.gameplay_systems_quest_dialogue_transition_rows()
            << " gameplay_systems_inventory_items_ready=" << (game.gameplay_systems_inventory_items_ready() ? 1 : 0)
            << " gameplay_systems_inventory_items_diagnostics=" << game.gameplay_systems_inventory_items_diagnostics()
            << " gameplay_systems_inventory_items_transition_rows="
            << game.gameplay_systems_inventory_items_transition_rows()
            << " gameplay_systems_interaction_ready=" << (game.gameplay_systems_interaction_ready() ? 1 : 0)
            << " gameplay_systems_interaction_diagnostics=" << game.gameplay_systems_interaction_diagnostics()
            << " gameplay_systems_interaction_rows=" << game.gameplay_systems_interaction_rows()
            << " gameplay_systems_interaction_feedback_rows=" << game.gameplay_systems_interaction_feedback_rows()
            << " gameplay_systems_interaction_final_session_state="
            << runtime_gameplay_session_state_name(game.gameplay_systems_interaction_final_session_state())
            << " gameplay_systems_scene_binding_ready=" << (game.gameplay_systems_scene_binding_ready() ? 1 : 0)
            << " gameplay_systems_scene_binding_source_rows=" << game.gameplay_systems_scene_binding_source_rows()
            << " gameplay_systems_scene_binding_rows=" << game.gameplay_systems_scene_binding_rows()
            << " gameplay_systems_scene_binding_systems=" << game.gameplay_systems_scene_binding_systems()
            << " gameplay_systems_scene_binding_component_rows=" << game.gameplay_systems_scene_binding_component_rows()
            << " gameplay_systems_scene_binding_diagnostics=" << game.gameplay_systems_scene_binding_diagnostics()
            << " gameplay_systems_scene_interaction_rows=" << game.gameplay_systems_scene_interaction_rows()
            << " gameplay_systems_scene_interaction_diagnostics="
            << game.gameplay_systems_scene_interaction_diagnostics()
            << " gameplay_systems_scene_interaction_final_session_state="
            << runtime_scene_gameplay_session_state_name(game.gameplay_systems_scene_interaction_final_session_state())
            << " input_context_rebinding_ready=" << (input_context_rebinding.ready ? 1 : 0)
            << " input_context_rebinding_layers=" << input_context_rebinding.requested_layers
            << " input_context_rebinding_active_contexts=" << input_context_rebinding.active_contexts
            << " input_context_rebinding_capture_active=" << (input_context_rebinding.capture_context_active ? 1 : 0)
            << " input_context_rebinding_gameplay_consumed="
            << (input_context_rebinding.gameplay_input_consumed ? 1 : 0)
            << " input_rebinding_profile_overlays_applied=" << input_context_rebinding.profile_overlays_applied
            << " input_rebinding_action_capture_status="
            << runtime_input_rebinding_capture_status_name(input_context_rebinding.action_capture_status)
            << " input_rebinding_axis_capture_status="
            << runtime_input_rebinding_capture_status_name(input_context_rebinding.axis_capture_status)
            << " input_rebinding_focus_consumed=" << (input_context_rebinding.focus_gameplay_consumed ? 1 : 0)
            << " input_rebinding_focus_retained=" << (input_context_rebinding.focus_retained ? 1 : 0)
            << " input_rebinding_presentation_rows=" << input_context_rebinding.presentation_rows
            << " input_rebinding_glyph_lookup_keys=" << input_context_rebinding.glyph_lookup_keys
            << " input_rebinding_diagnostics=" << input_context_rebinding.diagnostics
            << " sprite_collision_hitbox_ready=" << (game.sprite_collision_hitbox_ready() ? 1 : 0)
            << " sprite_collision_hitbox_hits=" << game.sprite_collision_hitbox_hits()
            << " sprite_collision_hitbox_gameplay_events=" << game.sprite_collision_hitbox_gameplay_events()
            << " sprite_collision_hitbox_interaction_rows=" << game.sprite_collision_hitbox_interaction_rows()
            << " sprite_collision_hitbox_feedback_rows=" << game.sprite_collision_hitbox_feedback_rows()
            << " sprite_collision_hitbox_diagnostics=" << game.sprite_collision_hitbox_diagnostics()
            << " runtime_profile_resume_ready=" << (game.gameplay_systems_runtime_profile_resume_ready() ? 1 : 0)
            << " runtime_profile_resume_status="
            << runtime_session_profile_resume_status_name(game.gameplay_systems_runtime_profile_resume_status())
            << " runtime_profile_resume_diagnostics=" << game.gameplay_systems_runtime_profile_resume_diagnostics()
            << " gameplay_systems_construction_placement_ready="
            << (game.gameplay_systems_construction_placement_ready() ? 1 : 0)
            << " gameplay_systems_construction_placement_diagnostics="
            << game.gameplay_systems_construction_placement_diagnostics()
            << " gameplay_systems_construction_placement_intent_rows="
            << game.gameplay_systems_construction_placement_intent_rows()
            << " gameplay_systems_procedural_generation_ready="
            << (game.gameplay_systems_procedural_generation_ready() ? 1 : 0)
            << " gameplay_systems_procedural_generation_diagnostics="
            << game.gameplay_systems_procedural_generation_diagnostics()
            << " gameplay_systems_procedural_generation_rows=" << game.gameplay_systems_procedural_generation_rows()
            << " gameplay_systems_procedural_generation_package_visible_rows="
            << game.gameplay_systems_procedural_generation_package_visible_rows()
            << " rpg_systems_status=" << rpg_systems_status_name(rpg_systems_probe.status)
            << " rpg_systems_ready=" << (rpg_systems_probe.ready ? 1 : 0)
            << " rpg_systems_diagnostics=" << rpg_systems_probe.diagnostics
            << " rpg_systems_replay_hash=" << rpg_systems_probe.replay_hash
            << " sandbox_world_status=" << sandbox_world_status_name(sandbox_world_probe.status)
            << " sandbox_world_ready=" << (sandbox_world_probe.ready ? 1 : 0)
            << " sandbox_world_diagnostics=" << sandbox_world_probe.diagnostics
            << " sandbox_world_replay_hash=" << sandbox_world_probe.replay_hash
            << " simulation_management_status=" << simulation_management_status_name(simulation_management_probe.status)
            << " simulation_management_ready=" << (simulation_management_probe.ready ? 1 : 0)
            << " simulation_management_diagnostics=" << simulation_management_probe.diagnostics
            << " simulation_management_replay_hash=" << simulation_management_probe.replay_hash
            << " network_replication_status=" << network_replication_status_name(network_replication_probe.status)
            << " network_replication_reviewed=" << (network_replication_probe.reviewed ? 1 : 0)
            << " network_replication_ready=" << (network_replication_probe.ready ? 1 : 0)
            << " network_replication_object_rows=" << network_replication_probe.object_rows
            << " network_replication_input_rows=" << network_replication_probe.input_rows
            << " network_replication_snapshot_rows=" << network_replication_probe.snapshot_rows
            << " network_replication_rollback_rows=" << network_replication_probe.rollback_rows
            << " network_replication_rejected_unsafe_rows=" << network_replication_probe.rejected_unsafe_rows
            << " network_replication_transport_host_evidence="
            << (network_replication_probe.has_transport_host_evidence ? 1 : 0)
            << " network_replication_diagnostics=" << network_replication_probe.diagnostics
            << " network_replication_replay_hash=" << network_replication_probe.replay_hash
            << " network_production_security_status="
            << network_production_security_status_name(network_production_security_probe.status)
            << " network_production_security_reviewed=" << (network_production_security_probe.reviewed ? 1 : 0)
            << " network_production_security_threat_model_reviewed="
            << (network_production_security_probe.threat_model_reviewed ? 1 : 0)
            << " network_production_security_loopback_host_evidence="
            << (network_production_security_probe.loopback_host_evidence ? 1 : 0)
            << " network_production_security_replication_evidence_ready="
            << (network_production_security_probe.replication_evidence_ready ? 1 : 0)
            << " network_production_security_diagnostics=" << network_production_security_probe.diagnostics
            << " network_production_security_replay_hash=" << network_production_security_probe.replay_hash << '\n';
        return 12;
    }

    if (options.require_runtime_profile_resume &&
        (!game.gameplay_systems_runtime_profile_resume_ready() ||
         game.gameplay_systems_runtime_profile_resume_loaded_documents() != 3U ||
         game.gameplay_systems_runtime_profile_resume_defaulted_documents() != 0U ||
         game.gameplay_systems_runtime_profile_resume_save_schema_version() != 3U ||
         game.gameplay_systems_runtime_profile_resume_settings_schema_version() != 2U)) {
        std::cout << "sample_2d_desktop_runtime_package required_runtime_profile_resume_unavailable"
                  << " runtime_profile_resume_status="
                  << runtime_session_profile_resume_status_name(game.gameplay_systems_runtime_profile_resume_status())
                  << " runtime_profile_resume_diagnostics="
                  << game.gameplay_systems_runtime_profile_resume_diagnostics()
                  << " runtime_profile_resume_loaded_documents="
                  << game.gameplay_systems_runtime_profile_resume_loaded_documents()
                  << " runtime_profile_resume_defaulted_documents="
                  << game.gameplay_systems_runtime_profile_resume_defaulted_documents()
                  << " runtime_profile_resume_save_schema_version="
                  << game.gameplay_systems_runtime_profile_resume_save_schema_version()
                  << " runtime_profile_resume_settings_schema_version="
                  << game.gameplay_systems_runtime_profile_resume_settings_schema_version() << '\n';
        return 20;
    }

    if (options.require_runtime_menu_hud &&
        (!game.gameplay_systems_runtime_menu_hud_ready() ||
         game.gameplay_systems_runtime_menu_hud_diagnostics() != 0U ||
         game.gameplay_systems_runtime_menu_hud_display_rows() != 6U ||
         game.gameplay_systems_runtime_menu_hud_command_rows() != 2U ||
         game.gameplay_systems_runtime_menu_hud_dialogue_rows() != 1U ||
         game.gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() != 1U)) {
        std::cout << "sample_2d_desktop_runtime_package required_runtime_menu_hud_unavailable"
                  << " runtime_menu_hud_ready=" << (game.gameplay_systems_runtime_menu_hud_ready() ? 1 : 0)
                  << " runtime_menu_hud_diagnostics=" << game.gameplay_systems_runtime_menu_hud_diagnostics()
                  << " runtime_menu_hud_display_rows=" << game.gameplay_systems_runtime_menu_hud_display_rows()
                  << " runtime_menu_hud_command_rows=" << game.gameplay_systems_runtime_menu_hud_command_rows()
                  << " runtime_menu_hud_dialogue_rows=" << game.gameplay_systems_runtime_menu_hud_dialogue_rows()
                  << " runtime_menu_hud_input_binding_prompt_rows="
                  << game.gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() << '\n';
        return 21;
    }

    if (options.require_audio_gameplay_mixer && !audio_gameplay_mixer.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_audio_gameplay_mixer_unavailable"
                  << " audio_gameplay_mixer_ready=" << (audio_gameplay_mixer.ready ? 1 : 0)
                  << " audio_gameplay_mixer_diagnostics=" << audio_gameplay_mixer.diagnostics
                  << " audio_gameplay_mixer_buses=" << audio_gameplay_mixer.buses
                  << " audio_gameplay_mixer_cues=" << audio_gameplay_mixer.cues
                  << " audio_gameplay_mixer_triggers=" << audio_gameplay_mixer.triggers
                  << " audio_gameplay_mixer_commands=" << audio_gameplay_mixer.commands
                  << " audio_gameplay_mixer_paused_buses=" << audio_gameplay_mixer.paused_buses
                  << " audio_gameplay_mixer_faded_buses=" << audio_gameplay_mixer.faded_buses
                  << " audio_gameplay_mixer_looping_commands=" << audio_gameplay_mixer.looping_commands
                  << " audio_gameplay_mixer_spatial_commands=" << audio_gameplay_mixer.spatial_commands
                  << " audio_gameplay_mixer_render_commands=" << audio_gameplay_mixer.render_commands
                  << " audio_gameplay_mixer_render_frames=" << audio_gameplay_mixer.render_frames
                  << " audio_gameplay_mixer_render_samples=" << audio_gameplay_mixer.render_samples
                  << " audio_gameplay_mixer_sample_abs_sum=" << audio_gameplay_mixer.sample_abs_sum
                  << " audio_gameplay_mixer_payload_diagnostics=" << audio_gameplay_mixer.payload_diagnostics << '\n';
        return 22;
    }

    if (options.require_world_region_streaming && !world_region_streaming_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_world_region_streaming_unavailable"
                  << " world_region_streaming_status="
                  << world_region_streaming_status_name(world_region_streaming_probe.status)
                  << " world_region_streaming_plan_rows=" << world_region_streaming_probe.plan_rows
                  << " world_region_streaming_load_rows=" << world_region_streaming_probe.load_rows
                  << " world_region_streaming_keep_rows=" << world_region_streaming_probe.keep_rows
                  << " world_region_streaming_unload_rows=" << world_region_streaming_probe.unload_rows
                  << " world_region_streaming_reviewed_package_adoptions="
                  << world_region_streaming_probe.reviewed_package_adoptions
                  << " world_region_streaming_missing_region_diagnostics="
                  << world_region_streaming_probe.missing_region_diagnostics
                  << " world_region_streaming_safe_point_diagnostics="
                  << world_region_streaming_probe.safe_point_diagnostics
                  << " world_region_streaming_large_scene_readiness_status="
                  << world_streaming_large_scene_readiness_status_name(
                         world_region_streaming_probe.large_scene_readiness_status)
                  << " world_region_streaming_large_scene_readiness_diagnostic="
                  << world_streaming_large_scene_readiness_diagnostic_name(
                         world_region_streaming_probe.large_scene_readiness_diagnostic)
                  << " world_region_streaming_large_scene_readiness_diagnostics="
                  << world_region_streaming_probe.large_scene_readiness_diagnostics
                  << " world_region_streaming_navigation_resident_regions="
                  << world_region_streaming_probe.navigation_resident_regions
                  << " world_region_streaming_navigation_missing_resident_regions="
                  << world_region_streaming_probe.navigation_missing_resident_regions
                  << " world_region_streaming_navigation_path_cache_ready="
                  << (world_region_streaming_probe.navigation_path_cache_ready ? 1 : 0) << '\n';
        return 14;
    }

    if (options.require_entity_scale_culling && !entity_scale_culling_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_entity_scale_culling_unavailable"
                  << " entity_scale_culling_status="
                  << entity_scale_culling_status_name(entity_scale_culling_probe.status)
                  << " entity_scale_culling_rows=" << entity_scale_culling_probe.rows
                  << " entity_scale_culling_visible_rows=" << entity_scale_culling_probe.visible_rows
                  << " entity_scale_culling_culled_rows=" << entity_scale_culling_probe.culled_rows
                  << " entity_scale_culling_lod_rows=" << entity_scale_culling_probe.lod_rows
                  << " entity_scale_culling_projected_draw_cost=" << entity_scale_culling_probe.projected_draw_cost
                  << " entity_scale_culling_projected_update_cost=" << entity_scale_culling_probe.projected_update_cost
                  << " entity_scale_culling_budget_diagnostics=" << entity_scale_culling_probe.budget_diagnostics
                  << '\n';
        return 15;
    }

    if (options.require_scripting_sandbox_policy && !scripting_sandbox_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_scripting_sandbox_policy_unavailable"
                  << " scripting_sandbox_status=" << scripting_sandbox_status_name(scripting_sandbox_probe.status)
                  << " scripting_sandbox_entrypoint_rows=" << scripting_sandbox_probe.entrypoint_rows
                  << " scripting_sandbox_permission_rows=" << scripting_sandbox_probe.permission_rows
                  << " scripting_sandbox_denied_permission_rows=" << scripting_sandbox_probe.denied_permission_rows
                  << " scripting_sandbox_rejected_unsafe_capability_rows="
                  << scripting_sandbox_probe.rejected_unsafe_capability_rows
                  << " scripting_sandbox_budget_diagnostics=" << scripting_sandbox_probe.budget_diagnostics
                  << " scripting_sandbox_replay_seed_rows=" << scripting_sandbox_probe.replay_seed_rows
                  << " scripting_sandbox_diagnostics=" << scripting_sandbox_probe.diagnostics
                  << " scripting_sandbox_execution_status="
                  << scripting_sandbox_execution_status_name(scripting_sandbox_probe.execution_status)
                  << " scripting_sandbox_execution_dispatches=" << scripting_sandbox_probe.execution_dispatches
                  << " scripting_sandbox_execution_diagnostics=" << scripting_sandbox_probe.execution_diagnostics
                  << '\n';
        return 16;
    }

    if (options.require_networking_foundation_policy && !networking_foundation_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_networking_foundation_policy_unavailable"
                  << " networking_foundation_status="
                  << networking_foundation_status_name(networking_foundation_probe.status)
                  << " networking_foundation_session_rows=" << networking_foundation_probe.session_rows
                  << " networking_foundation_transport_rows=" << networking_foundation_probe.transport_rows
                  << " networking_foundation_channel_rows=" << networking_foundation_probe.channel_rows
                  << " networking_foundation_rejected_unsafe_transport_rows="
                  << networking_foundation_probe.rejected_unsafe_transport_rows
                  << " networking_foundation_replay_prerequisite_rows="
                  << networking_foundation_probe.replay_prerequisite_rows
                  << " networking_foundation_security_diagnostics=" << networking_foundation_probe.security_diagnostics
                  << " networking_foundation_diagnostics=" << networking_foundation_probe.diagnostics << '\n';
        return 17;
    }

    if (options.require_simulation_orchestration && !simulation_orchestration_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_simulation_orchestration_unavailable"
                  << " simulation_orchestration_status="
                  << simulation_orchestration_status_name(simulation_orchestration_probe.status)
                  << " simulation_orchestration_planned_steps=" << simulation_orchestration_probe.planned_steps
                  << " simulation_orchestration_step_rows=" << simulation_orchestration_probe.step_rows
                  << " simulation_orchestration_command_rows=" << simulation_orchestration_probe.command_rows
                  << " simulation_orchestration_command_playback_rows="
                  << simulation_orchestration_probe.command_playback_rows
                  << " simulation_orchestration_budget_limited_status="
                  << simulation_orchestration_status_name(simulation_orchestration_probe.budget_limited_status)
                  << " simulation_orchestration_invalid_command_diagnostics="
                  << simulation_orchestration_probe.invalid_command_diagnostics
                  << " simulation_orchestration_diagnostics=" << simulation_orchestration_probe.diagnostics << '\n';
        return 18;
    }

    if (options.require_simulation_orchestration && !simulation_management_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_simulation_management_unavailable"
                  << " simulation_management_status="
                  << simulation_management_status_name(simulation_management_probe.status)
                  << " simulation_management_tick_count=" << simulation_management_probe.tick_count
                  << " simulation_management_resource_balance_rows="
                  << simulation_management_probe.resource_balance_rows
                  << " simulation_management_job_assignment_rows=" << simulation_management_probe.job_assignment_rows
                  << " simulation_management_logistics_transfer_rows="
                  << simulation_management_probe.logistics_transfer_rows
                  << " simulation_management_need_deficit_rows=" << simulation_management_probe.need_deficit_rows
                  << " simulation_management_dashboard_rows=" << simulation_management_probe.dashboard_rows
                  << " simulation_management_diagnostics=" << simulation_management_probe.diagnostics << '\n';
        return 19;
    }

    if (options.require_simulation_orchestration && !gameplay_runtime_scheduler_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_gameplay_runtime_scheduler_unavailable"
                  << " gameplay_runtime_scheduler_status="
                  << gameplay_runtime_scheduler_status_name(gameplay_runtime_scheduler_probe.status)
                  << " gameplay_runtime_scheduler_steps=" << gameplay_runtime_scheduler_probe.step_rows
                  << " gameplay_runtime_scheduler_system_rows=" << gameplay_runtime_scheduler_probe.system_rows
                  << " gameplay_runtime_scheduler_command_rows=" << gameplay_runtime_scheduler_probe.command_rows
                  << " gameplay_runtime_scheduler_budget_limited="
                  << (gameplay_runtime_scheduler_probe.budget_limited ? 1 : 0)
                  << " gameplay_runtime_scheduler_diagnostics=" << gameplay_runtime_scheduler_probe.diagnostics << '\n';
        return 18;
    }

    if (options.require_simulation_orchestration && !world_entity_model_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_world_entity_model_unavailable"
                  << " world_entity_model_status=" << world_entity_model_status_name(world_entity_model_probe.status)
                  << " world_entity_model_entities=" << world_entity_model_probe.entity_rows
                  << " world_entity_model_components=" << world_entity_model_probe.component_rows
                  << " world_entity_model_region_ownership_rows=" << world_entity_model_probe.region_ownership_rows
                  << " world_entity_model_lifecycle_rows=" << world_entity_model_probe.lifecycle_rows
                  << " world_entity_model_persistence_rows=" << world_entity_model_probe.persistence_rows
                  << " world_entity_model_streaming_region_rows=" << world_entity_model_probe.streaming_region_rows
                  << " world_entity_model_duplicate_entity_diagnostics="
                  << world_entity_model_probe.duplicate_entity_diagnostics
                  << " world_entity_model_bridge_rejection_status="
                  << world_entity_model_status_name(world_entity_model_probe.bridge_rejection_status)
                  << " world_entity_model_bridge_rejection_diagnostics="
                  << world_entity_model_probe.bridge_rejection_diagnostics
                  << " world_entity_model_bridge_rejection_persistence_rows="
                  << world_entity_model_probe.bridge_rejection_persistence_rows
                  << " world_entity_model_bridge_rejection_streaming_region_rows="
                  << world_entity_model_probe.bridge_rejection_streaming_region_rows
                  << " world_entity_model_bridge_rejection_streaming_diagnostics_present="
                  << world_entity_model_probe.bridge_rejection_streaming_diagnostics_present
                  << " world_entity_model_bridge_rejection_fail_closed="
                  << (world_entity_model_probe.bridge_rejection_fail_closed ? 1 : 0)
                  << " world_entity_model_diagnostics=" << world_entity_model_probe.diagnostics << '\n';
        return 18;
    }

    if (options.require_simulation_orchestration && !addressable_content_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_addressable_content_unavailable"
                  << " addressable_content_status=" << addressable_content_status_name(addressable_content_probe.status)
                  << " addressable_content_address_rows=" << addressable_content_probe.address_rows
                  << " addressable_content_dependency_rows=" << addressable_content_probe.dependency_rows
                  << " addressable_content_load_rows=" << addressable_content_probe.load_rows
                  << " addressable_content_release_rows=" << addressable_content_probe.release_rows
                  << " addressable_content_refcount_rows=" << addressable_content_probe.refcount_rows
                  << " addressable_content_resident_bytes=" << addressable_content_probe.resident_bytes
                  << " addressable_content_budget_rejection_status="
                  << addressable_content_status_name(addressable_content_probe.budget_rejection_status)
                  << " addressable_content_budget_rejection_diagnostics="
                  << addressable_content_probe.budget_rejection_diagnostics
                  << " addressable_content_package_io=" << (addressable_content_probe.package_io ? 1 : 0)
                  << " addressable_content_async_execution=" << (addressable_content_probe.async_execution ? 1 : 0)
                  << " addressable_content_committed=" << (addressable_content_probe.committed ? 1 : 0)
                  << " addressable_content_diagnostics=" << addressable_content_probe.diagnostics << '\n';
        return 18;
    }

    if (options.require_gameplay_authoring_review && !gameplay_authoring_review_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_gameplay_authoring_review_unavailable"
                  << " gameplay_authoring_review_status="
                  << gameplay_authoring_review_status_name(gameplay_authoring_review_probe)
                  << " gameplay_authoring_review_feature_rows=" << gameplay_authoring_review_probe.feature_rows
                  << " gameplay_authoring_review_accepted_rows=" << gameplay_authoring_review_probe.accepted_rows
                  << " gameplay_authoring_review_mutation_ledger_rows="
                  << gameplay_authoring_review_probe.mutation_ledger_rows
                  << " gameplay_authoring_review_remediation_rows=" << gameplay_authoring_review_probe.remediation_rows
                  << " gameplay_authoring_review_missing_required_capability_diagnostics="
                  << gameplay_authoring_review_probe.missing_required_capability_diagnostics
                  << " gameplay_authoring_review_missing_validation_recipe_diagnostics="
                  << gameplay_authoring_review_probe.missing_validation_recipe_diagnostics
                  << " gameplay_authoring_review_missing_package_evidence_diagnostics="
                  << gameplay_authoring_review_probe.missing_package_evidence_diagnostics
                  << " gameplay_authoring_review_unsupported_claim_diagnostics="
                  << gameplay_authoring_review_probe.unsupported_claim_diagnostics
                  << " gameplay_authoring_review_diagnostics=" << gameplay_authoring_review_probe.diagnostics << '\n';
        return 19;
    }

    if (options.require_production_authoring_workflows && !production_authoring_workflow_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_production_authoring_workflows_unavailable"
                  << " production_authoring_workflow_status="
                  << production_authoring_workflow_status_name(production_authoring_workflow_probe)
                  << " production_authoring_workflow_rows=" << production_authoring_workflow_probe.workflow_rows
                  << " production_authoring_workflow_accepted_rows="
                  << production_authoring_workflow_probe.accepted_rows
                  << " production_authoring_workflow_mutation_ledger_rows="
                  << production_authoring_workflow_probe.mutation_ledger_rows
                  << " production_authoring_workflow_validation_repair_rows="
                  << production_authoring_workflow_probe.validation_repair_rows
                  << " production_authoring_workflow_shared_surface_mutation_diagnostics="
                  << production_authoring_workflow_probe.shared_surface_mutation_diagnostics
                  << " production_authoring_workflow_arbitrary_shell_diagnostics="
                  << production_authoring_workflow_probe.arbitrary_shell_diagnostics
                  << " production_authoring_workflow_cooked_package_mutation_diagnostics="
                  << production_authoring_workflow_probe.cooked_package_mutation_diagnostics
                  << " production_authoring_workflow_native_backend_term_diagnostics="
                  << production_authoring_workflow_probe.native_backend_term_diagnostics
                  << " production_authoring_workflow_invalid_target_path_diagnostics="
                  << production_authoring_workflow_probe.invalid_target_path_diagnostics
                  << " production_authoring_workflow_invoked_file_mutation="
                  << (production_authoring_workflow_probe.invoked_file_mutation ? 1 : 0)
                  << " production_authoring_workflow_invoked_package_io="
                  << (production_authoring_workflow_probe.invoked_package_io ? 1 : 0)
                  << " production_authoring_workflow_invoked_command_execution="
                  << (production_authoring_workflow_probe.invoked_command_execution ? 1 : 0)
                  << " production_authoring_workflow_diagnostics=" << production_authoring_workflow_probe.diagnostics
                  << '\n';
        return 20;
    }

    if (options.require_runtime_ui_workbench && !runtime_ui_workbench_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_runtime_ui_workbench_unavailable"
                  << " runtime_ui_workbench_status="
                  << runtime_ui_workbench_status_name(runtime_ui_workbench_probe.status)
                  << " runtime_ui_workbench_panels=" << runtime_ui_workbench_probe.panels
                  << " runtime_ui_workbench_table_rows=" << runtime_ui_workbench_probe.table_rows
                  << " runtime_ui_workbench_graph_series=" << runtime_ui_workbench_probe.graph_series
                  << " runtime_ui_workbench_item_rows=" << runtime_ui_workbench_probe.item_rows
                  << " runtime_ui_workbench_text_inputs=" << runtime_ui_workbench_probe.text_inputs
                  << " runtime_ui_workbench_focus_edges=" << runtime_ui_workbench_probe.focus_edges
                  << " runtime_ui_workbench_localization_refs=" << runtime_ui_workbench_probe.localization_refs
                  << " runtime_ui_workbench_accessibility_refs=" << runtime_ui_workbench_probe.accessibility_refs
                  << " runtime_ui_workbench_diagnostics=" << runtime_ui_workbench_probe.diagnostics << '\n';
        return 27;
    }

    if (options.require_runtime_ui_production_stack && !runtime_ui_production_stack_probe.package_evidence_ready) {
        std::cout
            << "sample_2d_desktop_runtime_package required_runtime_ui_production_stack_unavailable"
            << " runtime_ui_production_stack_status="
            << mirakana::ui::runtime_ui_production_stack_status_name(runtime_ui_production_stack_probe.status)
            << " runtime_ui_production_stack_reviewed=" << (runtime_ui_production_stack_probe.reviewed ? 1 : 0)
            << " runtime_ui_production_stack_rows=" << runtime_ui_production_stack_probe.rows
            << " runtime_ui_production_stack_ready_rows=" << runtime_ui_production_stack_probe.ready_rows
            << " runtime_ui_production_stack_host_gated_rows=" << runtime_ui_production_stack_probe.host_gated_rows
            << " runtime_ui_production_stack_dependency_gated_rows="
            << runtime_ui_production_stack_probe.dependency_gated_rows
            << " runtime_ui_production_stack_skipped_rows=" << runtime_ui_production_stack_probe.skipped_rows
            << " runtime_ui_production_stack_adapter_invoked_rows="
            << runtime_ui_production_stack_probe.adapter_invoked_rows
            << " runtime_ui_production_stack_unsupported_rows=" << runtime_ui_production_stack_probe.unsupported_rows
            << " runtime_ui_production_stack_text_contract_ready="
            << (runtime_ui_production_stack_probe.text_contract_ready ? 1 : 0)
            << " runtime_ui_production_stack_selected_package_evidence_ready="
            << (runtime_ui_production_stack_probe.selected_package_evidence_ready ? 1 : 0)
            << " runtime_ui_production_stack_ime_session_rows=" << runtime_ui_production_stack_probe.ime_session_rows
            << " runtime_ui_production_stack_ime_composition_rows="
            << runtime_ui_production_stack_probe.ime_composition_rows
            << " runtime_ui_production_stack_ime_candidate_rows="
            << runtime_ui_production_stack_probe.ime_candidate_rows
            << " runtime_ui_production_stack_ime_text_area_cursor_rows="
            << runtime_ui_production_stack_probe.ime_text_area_cursor_rows
            << " runtime_ui_production_stack_ime_committed_text_rows="
            << runtime_ui_production_stack_probe.ime_committed_text_rows
            << " runtime_ui_production_stack_ime_clipboard_rows="
            << runtime_ui_production_stack_probe.ime_clipboard_rows
            << " runtime_ui_production_stack_ime_platform_adapter_proof_rows="
            << runtime_ui_production_stack_probe.ime_platform_adapter_proof_rows
            << " runtime_ui_production_stack_ime_platform_host_gate_rows="
            << runtime_ui_production_stack_probe.ime_platform_host_gate_rows
            << " runtime_ui_production_stack_ime_platform_parity_ready="
            << (runtime_ui_production_stack_probe.ime_platform_parity_ready ? 1 : 0)
            << " runtime_ui_production_stack_accessibility_role_rows="
            << runtime_ui_production_stack_probe.accessibility_role_rows
            << " runtime_ui_production_stack_accessibility_name_rows="
            << runtime_ui_production_stack_probe.accessibility_name_rows
            << " runtime_ui_production_stack_accessibility_description_rows="
            << runtime_ui_production_stack_probe.accessibility_description_rows
            << " runtime_ui_production_stack_accessibility_state_rows="
            << runtime_ui_production_stack_probe.accessibility_state_rows
            << " runtime_ui_production_stack_accessibility_focus_rows="
            << runtime_ui_production_stack_probe.accessibility_focus_rows
            << " runtime_ui_production_stack_accessibility_action_rows="
            << runtime_ui_production_stack_probe.accessibility_action_rows
            << " runtime_ui_production_stack_accessibility_relationship_rows="
            << runtime_ui_production_stack_probe.accessibility_relationship_rows
            << " runtime_ui_production_stack_accessibility_live_region_rows="
            << runtime_ui_production_stack_probe.accessibility_live_region_rows
            << " runtime_ui_production_stack_accessibility_keyboard_pattern_rows="
            << runtime_ui_production_stack_probe.accessibility_keyboard_pattern_rows
            << " runtime_ui_production_stack_accessibility_publication_status_rows="
            << runtime_ui_production_stack_probe.accessibility_publication_status_rows
            << " runtime_ui_production_stack_accessibility_uia_host_gate_rows="
            << runtime_ui_production_stack_probe.accessibility_uia_host_gate_rows
            << " runtime_ui_production_stack_accessibility_platform_host_gate_rows="
            << runtime_ui_production_stack_probe.accessibility_platform_host_gate_rows
            << " runtime_ui_production_stack_accessibility_platform_parity_ready="
            << (runtime_ui_production_stack_probe.accessibility_platform_parity_ready ? 1 : 0)
            << " runtime_ui_production_stack_diagnostics=" << runtime_ui_production_stack_probe.diagnostics << '\n';
        return 28;
    }

    if (options.require_runtime_ui_renderer_atlas_handoff && !runtime_ui_renderer_atlas_handoff_probe.ready) {
        std::cout << "sample_2d_desktop_runtime_package required_runtime_ui_renderer_atlas_handoff_unavailable"
                  << " runtime_ui_renderer_atlas_handoff_status="
                  << runtime_ui_renderer_atlas_handoff_status_name(runtime_ui_renderer_atlas_handoff_probe.status)
                  << " runtime_ui_renderer_atlas_handoff_ready="
                  << (runtime_ui_renderer_atlas_handoff_probe.ready ? 1 : 0)
                  << " runtime_ui_renderer_atlas_handoff_selected_package_evidence_ready="
                  << (runtime_ui_renderer_atlas_handoff_probe.selected_package_evidence_ready ? 1 : 0)
                  << " runtime_ui_renderer_atlas_handoff_image_atlas_bindings="
                  << runtime_ui_renderer_atlas_handoff_probe.image_atlas_bindings
                  << " runtime_ui_renderer_atlas_handoff_glyph_atlas_bindings="
                  << runtime_ui_renderer_atlas_handoff_probe.glyph_atlas_bindings
                  << " runtime_ui_renderer_atlas_handoff_renderer_sprites_submitted="
                  << runtime_ui_renderer_atlas_handoff_probe.renderer_sprites_submitted
                  << " runtime_ui_renderer_atlas_handoff_diagnostics="
                  << runtime_ui_renderer_atlas_handoff_probe.diagnostics << '\n';
        return 29;
    }

    if (options.smoke &&
        (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
         !game.passed(options.max_frames) || package_records != 7U)) {
        return 3;
    }
    return 0;
}
