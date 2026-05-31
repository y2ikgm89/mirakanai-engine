add_library(MK_editor_core
    core/src/ai_command_panel.cpp
    core/src/ai_operation_surface.cpp
    core/src/asset_pipeline.cpp
    core/src/command.cpp
    core/src/content_browser.cpp
    core/src/content_browser_import_panel.cpp
    core/src/editor_dock_layout.cpp
    core/src/editor_rich_text.cpp
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

add_library(MK_editor_shell_common
    src/first_party_editor_document.cpp
    src/native_editor_app.cpp
    src/native_editor_launch.cpp
    src/native_editor_text_atlas_handoff.cpp
    src/native_editor_text_input.cpp
    src/native_material_preview_cache.cpp
    src/native_viewport_surface.cpp
)

target_include_directories(MK_editor_shell_common
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(MK_editor_shell_common PUBLIC MK_editor_core)

if(WIN32)
    target_sources(MK_editor_shell_common PRIVATE
        src/native_editor_text_font_adapters.cpp
        src/native_editor_tsf_text_input.cpp
        src/native_editor_win32_services.cpp
    )
    target_link_libraries(MK_editor_shell_common PRIVATE MK_platform_win32 dwrite)
endif()

MK_apply_common_target_options(MK_editor_shell_common)

if(NOT MK_ENABLE_DESKTOP_EDITOR)
    return()
endif()

if(NOT WIN32)
    message(FATAL_ERROR "MK_ENABLE_DESKTOP_EDITOR is Windows-only for the native Win32 editor shell")
endif()

if(NOT TARGET MK_platform_win32)
    message(FATAL_ERROR "MK_ENABLE_DESKTOP_EDITOR requires MK_platform_win32")
endif()

add_library(MK_editor_shell_win32
    src/win32_first_party_editor_host.cpp
)

target_include_directories(MK_editor_shell_win32
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(MK_editor_shell_win32
    PRIVATE
        MK_editor_core
        MK_editor_shell_common
        MK_renderer
        MK_platform_win32
        MK_ui_renderer
        d3d12
        dxgi
)

# Native MK_editor shell is SDL3-free and uses private Win32/first-party UI/D3D12
# lifecycle code behind the editor executable target.
MK_apply_common_target_options(MK_editor_shell_win32)

add_executable(MK_editor
    src/main.cpp
)

target_link_libraries(MK_editor PRIVATE MK_editor_shell_common MK_editor_shell_win32)

MK_apply_common_target_options(MK_editor)
