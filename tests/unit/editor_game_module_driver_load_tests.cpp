// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/editor/game_module_driver.hpp"
#include "mirakana/editor/play_in_editor.hpp"
#include "mirakana/editor/scene_authoring.hpp"
#include "mirakana/platform/dynamic_library.hpp"

#include <filesystem>
#include <string_view>

#ifndef MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH
#error "MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH must be defined for MK_editor_game_module_driver_load_tests"
#endif

namespace {

using ProbeResetFn = void (*)();
using ProbeCountFn = int (*)();
using ProbeLastDeltaFn = double (*)();
using ProbeLastFrameFn = std::uint64_t (*)();

template <typename Fn>
[[nodiscard]] Fn resolve_probe_function(const mirakana::DynamicLibrary& library, const std::string_view symbol_name) {
    const auto symbol = mirakana::resolve_dynamic_library_symbol(library, symbol_name);
    MK_REQUIRE(symbol.status == mirakana::DynamicLibrarySymbolStatus::resolved);
    MK_REQUIRE(symbol.address != nullptr);
    return reinterpret_cast<Fn>(symbol.address);
}

} // namespace

MK_TEST("editor game module driver loads real dynamic probe and ticks isolated session") {
    const std::filesystem::path probe_path{MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH};
    MK_REQUIRE(probe_path.is_absolute());

    mirakana::editor::EditorGameModuleDriverLoadDesc desc;
    desc.id = "reviewed_dynamic_probe";
    desc.label = "Reviewed Dynamic Probe";
    desc.module_path = probe_path.string();
    desc.factory_symbol = std::string(mirakana::editor::editor_game_module_driver_factory_symbol_v1);
    MK_REQUIRE(desc.factory_symbol == "mirakana_create_editor_game_module_driver_v1");

    const auto load_model = mirakana::editor::make_editor_game_module_driver_load_model(desc);
    MK_REQUIRE(load_model.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(load_model.can_load);
    MK_REQUIRE(load_model.unsupported_claims.empty());
    MK_REQUIRE(load_model.abi_contract == mirakana::editor::editor_game_module_driver_abi_name_v1);

    auto load_result = mirakana::load_dynamic_library(probe_path);
    MK_REQUIRE(load_result.status == mirakana::DynamicLibraryLoadStatus::loaded);
    MK_REQUIRE(load_result.library.loaded());

    const auto reset_probe =
        resolve_probe_function<ProbeResetFn>(load_result.library, "MK_editor_game_module_driver_probe_reset");
    const auto begin_count =
        resolve_probe_function<ProbeCountFn>(load_result.library, "MK_editor_game_module_driver_probe_begin_count");
    const auto tick_count =
        resolve_probe_function<ProbeCountFn>(load_result.library, "MK_editor_game_module_driver_probe_tick_count");
    const auto end_count =
        resolve_probe_function<ProbeCountFn>(load_result.library, "MK_editor_game_module_driver_probe_end_count");
    const auto destroy_count =
        resolve_probe_function<ProbeCountFn>(load_result.library, "MK_editor_game_module_driver_probe_destroy_count");
    const auto last_frame = resolve_probe_function<ProbeLastFrameFn>(
        load_result.library, "MK_editor_game_module_driver_probe_last_frame_index");
    const auto last_delta =
        resolve_probe_function<ProbeLastDeltaFn>(load_result.library, "MK_editor_game_module_driver_probe_last_delta");

    reset_probe();
    MK_REQUIRE(begin_count() == 0);
    MK_REQUIRE(tick_count() == 0);
    MK_REQUIRE(end_count() == 0);
    MK_REQUIRE(destroy_count() == 0);

    const auto factory_symbol = mirakana::resolve_dynamic_library_symbol(
        load_result.library, mirakana::editor::editor_game_module_driver_factory_symbol_v1);
    MK_REQUIRE(factory_symbol.status == mirakana::DynamicLibrarySymbolStatus::resolved);
    MK_REQUIRE(factory_symbol.address != nullptr);
    const auto factory = reinterpret_cast<mirakana::editor::EditorGameModuleDriverFactoryFn>(factory_symbol.address);

    auto create_result = mirakana::editor::make_editor_game_module_driver_from_symbol(factory);
    MK_REQUIRE(create_result.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(create_result.driver != nullptr);
    auto driver = std::move(create_result.driver);

    mirakana::Scene scene("Editor Scene");
    const auto player = scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.begin(document, *driver) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(begin_count() == 1);
    MK_REQUIRE(tick_count() == 0);
    MK_REQUIRE(document.scene().nodes().size() == 1);
    MK_REQUIRE(document.scene().find_node(player)->name == "Player");
    MK_REQUIRE(session.simulation_scene() != nullptr);
    MK_REQUIRE(session.simulation_scene()->nodes().size() == 2);

    MK_REQUIRE(session.tick(0.125) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(tick_count() == 1);
    MK_REQUIRE(last_frame() == 0);
    MK_REQUIRE(last_delta() == 0.125);
    MK_REQUIRE(session.simulation_frame_count() == 1);
    MK_REQUIRE(session.simulation_scene()->nodes().size() == 3);
    MK_REQUIRE(document.scene().nodes().size() == 1);

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(end_count() == 1);
    MK_REQUIRE(destroy_count() == 0);

    driver.reset();
    MK_REQUIRE(destroy_count() == 1);
}

MK_TEST("editor game module driver stopped state reload transaction reloads probe dll") {
    const std::filesystem::path probe_path{MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH};
    MK_REQUIRE(probe_path.is_absolute());

    mirakana::editor::EditorGameModuleDriverLoadDesc desc;
    desc.id = "reviewed_dynamic_probe_reload_tx";
    desc.label = "Reviewed Dynamic Probe Reload Tx";
    desc.module_path = probe_path.string();
    desc.factory_symbol = std::string(mirakana::editor::editor_game_module_driver_factory_symbol_v1);

    const auto assert_ready_load_model = [&] {
        const auto load_model = mirakana::editor::make_editor_game_module_driver_load_model(desc);
        MK_REQUIRE(load_model.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
        MK_REQUIRE(load_model.can_load);
        MK_REQUIRE(load_model.blocked_by.empty());
    };

    assert_ready_load_model();

    const auto run_one_session = [&](mirakana::DynamicLibrary& library) {
        const auto reset_probe =
            resolve_probe_function<ProbeResetFn>(library, "MK_editor_game_module_driver_probe_reset");
        const auto begin_count =
            resolve_probe_function<ProbeCountFn>(library, "MK_editor_game_module_driver_probe_begin_count");
        const auto tick_count =
            resolve_probe_function<ProbeCountFn>(library, "MK_editor_game_module_driver_probe_tick_count");
        const auto end_count =
            resolve_probe_function<ProbeCountFn>(library, "MK_editor_game_module_driver_probe_end_count");
        const auto destroy_count =
            resolve_probe_function<ProbeCountFn>(library, "MK_editor_game_module_driver_probe_destroy_count");
        const auto last_frame =
            resolve_probe_function<ProbeLastFrameFn>(library, "MK_editor_game_module_driver_probe_last_frame_index");
        const auto last_delta =
            resolve_probe_function<ProbeLastDeltaFn>(library, "MK_editor_game_module_driver_probe_last_delta");

        reset_probe();
        MK_REQUIRE(begin_count() == 0);
        MK_REQUIRE(tick_count() == 0);
        MK_REQUIRE(end_count() == 0);
        MK_REQUIRE(destroy_count() == 0);

        const auto factory_symbol = mirakana::resolve_dynamic_library_symbol(
            library, mirakana::editor::editor_game_module_driver_factory_symbol_v1);
        MK_REQUIRE(factory_symbol.status == mirakana::DynamicLibrarySymbolStatus::resolved);
        const auto factory =
            reinterpret_cast<mirakana::editor::EditorGameModuleDriverFactoryFn>(factory_symbol.address);

        auto create_result = mirakana::editor::make_editor_game_module_driver_from_symbol(factory);
        MK_REQUIRE(create_result.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
        MK_REQUIRE(create_result.driver != nullptr);
        auto driver = std::move(create_result.driver);

        mirakana::Scene scene("Editor Scene Reload Tx");
        (void)scene.create_node("Player");
        auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");

        mirakana::editor::EditorPlaySession session;
        MK_REQUIRE(session.begin(document, *driver) == mirakana::editor::EditorPlaySessionActionStatus::applied);
        MK_REQUIRE(begin_count() == 1);

        MK_REQUIRE(session.tick(0.05) == mirakana::editor::EditorPlaySessionActionStatus::applied);
        MK_REQUIRE(tick_count() == 1);
        MK_REQUIRE(last_frame() == 0);
        MK_REQUIRE(last_delta() == 0.05);

        MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
        MK_REQUIRE(end_count() == 1);

        driver.reset();
        MK_REQUIRE(destroy_count() == 1);
    };

    auto first = mirakana::load_dynamic_library(probe_path);
    MK_REQUIRE(first.status == mirakana::DynamicLibraryLoadStatus::loaded);
    run_one_session(first.library);
    first.library.reset();

    assert_ready_load_model();

    auto second = mirakana::load_dynamic_library(probe_path);
    MK_REQUIRE(second.status == mirakana::DynamicLibraryLoadStatus::loaded);
    run_one_session(second.library);
    second.library.reset();
}

MK_TEST("editor game module driver paired dynamic library handles keep module resident on windows") {
    const std::filesystem::path probe_path{MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH};
    MK_REQUIRE(probe_path.is_absolute());

    auto first = mirakana::load_dynamic_library(probe_path);
    auto second = mirakana::load_dynamic_library(probe_path);
    MK_REQUIRE(first.status == mirakana::DynamicLibraryLoadStatus::loaded);
    MK_REQUIRE(second.status == mirakana::DynamicLibraryLoadStatus::loaded);
    MK_REQUIRE(first.library.loaded());
    MK_REQUIRE(second.library.loaded());

    const auto reset_primary =
        resolve_probe_function<ProbeResetFn>(first.library, "MK_editor_game_module_driver_probe_reset");
    const auto destroy_count_primary =
        resolve_probe_function<ProbeCountFn>(first.library, "MK_editor_game_module_driver_probe_destroy_count");
    const auto begin_count_secondary =
        resolve_probe_function<ProbeCountFn>(second.library, "MK_editor_game_module_driver_probe_begin_count");

    reset_primary();
    MK_REQUIRE(destroy_count_primary() == 0);

    const auto factory_symbol = mirakana::resolve_dynamic_library_symbol(
        first.library, mirakana::editor::editor_game_module_driver_factory_symbol_v1);
    MK_REQUIRE(factory_symbol.status == mirakana::DynamicLibrarySymbolStatus::resolved);
    const auto factory = reinterpret_cast<mirakana::editor::EditorGameModuleDriverFactoryFn>(factory_symbol.address);

    auto create_result = mirakana::editor::make_editor_game_module_driver_from_symbol(factory);
    MK_REQUIRE(create_result.status == mirakana::editor::EditorGameModuleDriverStatus::ready);
    MK_REQUIRE(create_result.driver != nullptr);
    auto driver = std::move(create_result.driver);

    mirakana::Scene scene("Editor Scene Paired Handles");
    (void)scene.create_node("Player");
    auto document = mirakana::editor::SceneAuthoringDocument::from_scene(scene, "scenes/start.scene");
    mirakana::editor::EditorPlaySession session;
    MK_REQUIRE(session.begin(document, *driver) == mirakana::editor::EditorPlaySessionActionStatus::applied);
    MK_REQUIRE(begin_count_secondary() == 1);

    MK_REQUIRE(session.stop() == mirakana::editor::EditorPlaySessionActionStatus::applied);
    driver.reset();
    MK_REQUIRE(destroy_count_primary() == 1);

    first.library.reset();
    MK_REQUIRE(second.library.loaded());

    const auto reset_after_first_free =
        resolve_probe_function<ProbeResetFn>(second.library, "MK_editor_game_module_driver_probe_reset");
    reset_after_first_free();
    MK_REQUIRE(begin_count_secondary() == 0);

    second.library.reset();
}

int main() {
    return mirakana::test::run_all();
}
