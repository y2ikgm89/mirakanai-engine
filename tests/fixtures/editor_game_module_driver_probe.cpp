// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include <mirakana/editor/game_module_driver.hpp>
#include <mirakana/editor/play_in_editor.hpp>
#include <mirakana/scene/scene.hpp>

#include <cstdint>
#include <string>

#if defined(_WIN32)
#define MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT __declspec(dllexport)
#else
#define MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT __attribute__((visibility("default")))
#endif

namespace {

struct ProbeState {
    int begin_count{0};
    int tick_count{0};
    int end_count{0};
    int destroy_count{0};
    std::uint64_t last_frame_index{0};
    double last_delta{0.0};
};

ProbeState& probe_state() noexcept {
    static ProbeState state;
    return state;
}

void probe_begin(void* user_data, mirakana::Scene* scene) {
    auto* state = static_cast<ProbeState*>(user_data);
    if (state == nullptr || scene == nullptr) {
        return;
    }
    ++state->begin_count;
    (void)scene->create_node("DynamicProbeBegin");
}

void probe_tick(void* user_data, mirakana::Scene* scene,
                const mirakana::editor::EditorPlaySessionTickContext* context) {
    auto* state = static_cast<ProbeState*>(user_data);
    if (state == nullptr || scene == nullptr || context == nullptr) {
        return;
    }
    ++state->tick_count;
    state->last_frame_index = context->frame_index;
    state->last_delta = context->delta_seconds;
    (void)scene->create_node("DynamicProbeTick" + std::to_string(state->tick_count));
}

void probe_end(void* user_data, mirakana::Scene* /*unused*/) {
    auto* state = static_cast<ProbeState*>(user_data);
    if (state != nullptr) {
        ++state->end_count;
    }
}

void probe_destroy(void* user_data) noexcept {
    auto* state = static_cast<ProbeState*>(user_data);
    if (state != nullptr) {
        ++state->destroy_count;
    }
}

} // namespace

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT mirakana::editor::EditorGameModuleDriverApi
mirakana_create_editor_game_module_driver_v1() {
    mirakana::editor::EditorGameModuleDriverApi api;
    api.abi_version = mirakana::editor::editor_game_module_driver_abi_version_v1;
    api.user_data = &probe_state();
    api.begin = probe_begin;
    api.tick = probe_tick;
    api.end = probe_end;
    api.destroy = probe_destroy;
    return api;
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT void MK_editor_game_module_driver_probe_reset() {
    probe_state() = ProbeState{};
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT int MK_editor_game_module_driver_probe_begin_count() {
    return probe_state().begin_count;
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT int MK_editor_game_module_driver_probe_tick_count() {
    return probe_state().tick_count;
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT int MK_editor_game_module_driver_probe_end_count() {
    return probe_state().end_count;
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT int MK_editor_game_module_driver_probe_destroy_count() {
    return probe_state().destroy_count;
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT std::uint64_t
MK_editor_game_module_driver_probe_last_frame_index() {
    return probe_state().last_frame_index;
}

extern "C" MK_EDITOR_GAME_MODULE_DRIVER_PROBE_EXPORT double MK_editor_game_module_driver_probe_last_delta() {
    return probe_state().last_delta;
}
