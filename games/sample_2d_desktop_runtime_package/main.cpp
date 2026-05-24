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
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/entity_scale_culling.hpp"
#include "mirakana/runtime/gameplay_interaction.hpp"
#include "mirakana/runtime/inventory_items.hpp"
#include "mirakana/runtime/networking_foundation.hpp"
#include "mirakana/runtime/procedural_generation.hpp"
#include "mirakana/runtime/quest_dialogue.hpp"
#include "mirakana/runtime/runtime_diagnostics.hpp"
#include "mirakana/runtime/scripting_sandbox.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/runtime/simulation_orchestration.hpp"
#include "mirakana/runtime/sprite_collision_hitbox.hpp"
#include "mirakana/runtime/sprite_effect_particles.hpp"
#include "mirakana/runtime/world_region_streaming.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"
#include "mirakana/scene/playable_2d.hpp"
#include "mirakana/scene/render_packet.hpp"
#include "mirakana/scene/scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/tools/gameplay_authoring_tool.hpp"
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
    bool require_runtime_profile_resume{false};
    bool require_runtime_menu_hud{false};
    bool require_audio_gameplay_mixer{false};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
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

[[nodiscard]] mirakana::AssetId packaged_sprite_texture_asset_id();

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

[[nodiscard]] mirakana::AssetId packaged_sprite_animation_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/player-sprite-animation");
}

[[nodiscard]] mirakana::AssetId packaged_tilemap_asset_id() {
    return asset_id_from_game_asset_key("sample/2d-desktop-runtime-package/tilemap");
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

        const auto behavior_authoring = validate_gameplay_2d_behavior_authoring();
        behavior_authoring_ready_ = behavior_authoring.succeeded;
        behavior_authoring_diagnostics_ = behavior_authoring.diagnostics.size();
        behavior_authoring_trace_nodes_ = behavior_authoring.trace.size();

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
               last_perception_target_count_ == 1U && last_perception_visible_count_ == 1U &&
               last_blackboard_status_ == mirakana::AiPerceptionBlackboardStatus::ready && blackboard_has_target_ &&
               blackboard_needs_move_ && last_tree_result_.status == mirakana::BehaviorTreeStatus::success &&
               last_tree_result_.visited_nodes.size() == 4U && behavior_authoring_ready_ &&
               behavior_authoring_diagnostics_ == 0U && behavior_authoring_trace_nodes_ == 4U &&
               quest_dialogue_ready_ && quest_dialogue_diagnostics_ == 0U && quest_dialogue_transition_rows_ == 3U &&
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

    [[nodiscard]] std::size_t behavior_authoring_diagnostic_count() const noexcept {
        return behavior_authoring_diagnostics_;
    }

    [[nodiscard]] std::size_t behavior_authoring_trace_node_count() const noexcept {
        return behavior_authoring_trace_nodes_;
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
        };
        const auto perception = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 100U,
                                                   .position =
                                                       mirakana::AiPerceptionPoint2{.x = navigation_agent_.position.x,
                                                                                    .y = navigation_agent_.position.y},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 4.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 0.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{route_targets},
        });
        last_perception_status_ = perception.status;
        last_perception_target_count_ = perception.targets.size();
        last_perception_has_primary_target_ = perception.has_primary_target;
        last_perception_visible_count_ = perception.visible_count;

        mirakana::BehaviorTreeBlackboard blackboard;
        const auto blackboard_result =
            mirakana::write_ai_perception_blackboard(perception,
                                                     mirakana::AiPerceptionBlackboardKeys{
                                                         .has_target_key = kGameplay2dHasTargetKey,
                                                         .target_id_key = kGameplay2dTargetIdKey,
                                                         .target_distance_key = kGameplay2dTargetDistanceKey,
                                                         .visible_count_key = kGameplay2dVisibleTargetsKey,
                                                         .audible_count_key = kGameplay2dAudibleTargetsKey,
                                                         .target_state_key = kGameplay2dTargetStateKey,
                                                     },
                                                     blackboard);
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
    std::size_t physics_contact_count_{0U};
    std::size_t physics_trigger_overlap_count_{0U};
    std::size_t navigation_path_point_count_{0U};
    std::size_t last_perception_target_count_{0U};
    std::size_t last_perception_visible_count_{0U};
    std::size_t behavior_authoring_diagnostics_{0U};
    std::size_t behavior_authoring_trace_nodes_{0U};
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
    bool blackboard_has_target_{false};
    bool blackboard_needs_move_{false};
    bool behavior_authoring_ready_{false};
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

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_diagnostics() const noexcept {
        return gameplay_systems_.behavior_authoring_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_authoring_trace_nodes() const noexcept {
        return gameplay_systems_.behavior_authoring_trace_node_count();
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
    std::cout << "sample_2d_desktop_runtime_package [--smoke] [--max-frames N] [--video-driver NAME] "
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
                 "[--require-gameplay-authoring-review] [--require-runtime-profile-resume] "
                 "[--require-runtime-menu-hud] [--require-audio-gameplay-mixer]\n";
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
        if (arg == "--require-runtime-profile-resume") {
            options.require_runtime_profile_resume = true;
            continue;
        }
        if (arg == "--require-runtime-menu-hud") {
            options.require_runtime_menu_hud = true;
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
        if (arg == "--video-driver") {
            if (index + 1 >= argc) {
                std::cerr << "--video-driver requires a driver name\n";
                return false;
            }
            options.video_driver_hint = argv[index + 1];
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
        if (options.video_driver_hint.empty()) {
            options.video_driver_hint = "dummy";
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

[[nodiscard]] mirakana::SdlDesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::SdlDesktopPresentationShaderBytecode{
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
                                    packaged_sprite_animation_asset_id()},
        .resident_resource_kinds =
            {
                mirakana::AssetKind::texture,
                mirakana::AssetKind::material,
                mirakana::AssetKind::scene,
                mirakana::AssetKind::audio,
                mirakana::AssetKind::tilemap,
                mirakana::AssetKind::sprite_animation,
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

        probe.status = load_result.succeeded() && unload_result.succeeded()
                           ? mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::completed
                           : mirakana::runtime::RuntimeWorldRegionStreamingSafePointStatus::failed;
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
        probe.ready = load_plan.succeeded() && unload_plan.succeeded() && load_result.succeeded() &&
                      unload_result.succeeded() && probe.load_rows == 1U && probe.unload_rows == 1U &&
                      probe.reviewed_package_adoptions > 0U && probe.missing_region_diagnostics > 0U &&
                      probe.safe_point_diagnostics == 0U;
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

void print_presentation_report(std::string_view prefix, const mirakana::SdlDesktopGameHost& host) {
    const auto report = host.presentation_report();
    std::cout << prefix << " presentation_report=requested="
              << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " native_2d_sprites_status="
              << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " native_2d_sprites_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " native_2d_texture_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::sdl_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::sdl_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.message << '\n';
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
    const auto gameplay_authoring_review_probe = options.require_gameplay_authoring_review
                                                     ? validate_gameplay_authoring_review_package_evidence()
                                                     : GameplayAuthoringReviewProbeResult{};

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

    std::optional<mirakana::SdlDesktopPresentationD3d12RendererDesc> d3d12_renderer;
    if (shader_bytecode.ready()) {
        d3d12_renderer.emplace(mirakana::SdlDesktopPresentationD3d12RendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(shader_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(shader_bytecode.fragment_shader),
            .native_sprite_overlay_vertex_shader =
                native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(native_sprite_overlay_shader_bytecode.vertex_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_fragment_shader =
                native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(native_sprite_overlay_shader_bytecode.fragment_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_package = runtime_package.has_value() ? &*runtime_package : nullptr,
            .native_sprite_overlay_atlas_asset = packaged_sprite_texture_asset_id(),
            .enable_native_sprite_overlay = options.require_native_2d_sprites && !options.require_vulkan_renderer,
            .enable_native_sprite_overlay_textures =
                options.require_native_2d_sprites && !options.require_vulkan_renderer,
        });
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanRendererDesc> vulkan_renderer;
    if (vulkan_shader_bytecode.ready()) {
        vulkan_renderer.emplace(mirakana::SdlDesktopPresentationVulkanRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(vulkan_shader_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(vulkan_shader_bytecode.fragment_shader),
            .native_sprite_overlay_vertex_shader =
                vulkan_native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(vulkan_native_sprite_overlay_shader_bytecode.vertex_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_fragment_shader =
                vulkan_native_sprite_overlay_shader_bytecode.ready()
                    ? to_presentation_shader_bytecode(vulkan_native_sprite_overlay_shader_bytecode.fragment_shader)
                    : mirakana::SdlDesktopPresentationShaderBytecode{},
            .native_sprite_overlay_package = runtime_package.has_value() ? &*runtime_package : nullptr,
            .native_sprite_overlay_atlas_asset = packaged_sprite_texture_asset_id(),
            .enable_native_sprite_overlay = options.require_native_2d_sprites && options.require_vulkan_renderer,
            .enable_native_sprite_overlay_textures =
                options.require_native_2d_sprites && options.require_vulkan_renderer,
        });
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
        .title = "Sample 2D Desktop Runtime Package",
        .extent = mirakana::WindowExtent{.width = 960, .height = 540},
        .video_driver_hint = options.video_driver_hint,
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_renderer.has_value()) {
        host_desc.d3d12_renderer = &*d3d12_renderer;
    }
    if (vulkan_renderer.has_value()) {
        host_desc.vulkan_renderer = &*vulkan_renderer;
    }

    mirakana::SdlDesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::d3d12) {
        std::cout << "sample_2d_desktop_runtime_package required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_2d_desktop_runtime_package", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        return 6;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout << "sample_2d_desktop_runtime_package required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_2d_desktop_runtime_package", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_2d_desktop_runtime_package presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
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
    const auto package_records = runtime_package.has_value() ? runtime_package->records().size() : 0U;
    std::cout
        << "sample_2d_desktop_runtime_package status=" << status_name(result.status)
        << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
        << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
        << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
        << " presentation_backend_reports=" << report.backend_reports_count
        << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
        << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
        << " native_2d_sprites_requested=" << (report.native_ui_overlay_requested ? 1 : 0)
        << " native_2d_sprites_status="
        << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
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
        << ((game.gameplay_systems_passed(options.max_frames) && game.gameplay_systems_scene_binding_ready()) ? 1 : 0)
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
        << " gameplay_systems_blackboard_status="
        << ai_perception_blackboard_status_name(game.gameplay_systems_blackboard_status())
        << " gameplay_systems_blackboard_has_target=" << (game.gameplay_systems_blackboard_has_target() ? 1 : 0)
        << " gameplay_systems_blackboard_needs_move=" << (game.gameplay_systems_blackboard_needs_move() ? 1 : 0)
        << " gameplay_systems_behavior_status=" << behavior_tree_status_name(game.gameplay_systems_behavior_status())
        << " gameplay_systems_behavior_nodes=" << game.gameplay_systems_behavior_nodes()
        << " gameplay_systems_behavior_authoring_ready=" << (game.gameplay_systems_behavior_authoring_ready() ? 1 : 0)
        << " gameplay_systems_behavior_authoring_diagnostics=" << game.gameplay_systems_behavior_authoring_diagnostics()
        << " gameplay_systems_behavior_authoring_trace_nodes=" << game.gameplay_systems_behavior_authoring_trace_nodes()
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
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
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
                  << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
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
         !input_context_rebinding.ready)) {
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
            << " gameplay_systems_behavior_authoring_diagnostics="
            << game.gameplay_systems_behavior_authoring_diagnostics()
            << " gameplay_systems_behavior_authoring_trace_nodes="
            << game.gameplay_systems_behavior_authoring_trace_nodes()
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
            << game.gameplay_systems_procedural_generation_package_visible_rows() << '\n';
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
                  << " world_region_streaming_unload_rows=" << world_region_streaming_probe.unload_rows
                  << " world_region_streaming_reviewed_package_adoptions="
                  << world_region_streaming_probe.reviewed_package_adoptions
                  << " world_region_streaming_missing_region_diagnostics="
                  << world_region_streaming_probe.missing_region_diagnostics
                  << " world_region_streaming_safe_point_diagnostics="
                  << world_region_streaming_probe.safe_point_diagnostics << '\n';
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

    if (options.smoke &&
        (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
         !game.passed(options.max_frames) || package_records != 6U)) {
        return 3;
    }
    return 0;
}
