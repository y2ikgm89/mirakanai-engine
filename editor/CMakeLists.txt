add_library(MK_editor_core
    core/src/ai_command_panel.cpp
    core/src/asset_pipeline.cpp
    core/src/command.cpp
    core/src/content_browser.cpp
    core/src/content_browser_import_panel.cpp
    core/src/game_module_driver.cpp
    core/src/generated_game_studio.cpp
    core/src/gltf_mesh_catalog.cpp
    core/src/history.cpp
    core/src/input_rebinding.cpp
    core/src/io.cpp
    core/src/material_asset_preview_panel.cpp
    core/src/material_authoring.cpp
    core/src/material_graph_authoring.cpp
    core/src/palette.cpp
    core/src/play_in_editor.cpp
    core/src/playtest_package_review.cpp
    core/src/prefab_variant_authoring.cpp
    core/src/profiler.cpp
    core/src/project.cpp
    core/src/project_wizard.cpp
    core/src/render_backend.cpp
    core/src/resource_panel.cpp
    core/src/scene_authoring.cpp
    core/src/scene_edit.cpp
    core/src/shader_artifact_io.cpp
    core/src/shader_compile.cpp
    core/src/shader_tool_discovery.cpp
    core/src/source_registry_browser.cpp
    core/src/ui_model.cpp
    core/src/viewport.cpp
    core/src/viewport_shader_artifacts.cpp
    core/src/workspace.cpp
)

target_include_directories(MK_editor_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/core/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_editor_core PUBLIC MK_animation MK_assets MK_core MK_platform MK_runtime MK_scene MK_tools MK_ui)

MK_apply_common_target_options(MK_editor_core)

if(NOT MK_ENABLE_DESKTOP_GUI)
    return()
endif()

message(FATAL_ERROR "MK_ENABLE_DESKTOP_GUI is deferred until the editor shell has a first-party native desktop backend.")
