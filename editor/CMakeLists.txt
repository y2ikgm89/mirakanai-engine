add_library(MK_editor_core
    core/src/ai_command_panel.cpp
    core/src/ai_operation_surface.cpp
    core/src/asset_pipeline.cpp
    core/src/command.cpp
    core/src/content_browser.cpp
    core/src/content_browser_import_panel.cpp
    core/src/editor_cross_platform_shell.cpp
    core/src/editor_dock_layout.cpp
    core/src/environment_authoring.cpp
    core/src/editor_rich_text.cpp
    core/src/editor_ui_performance.cpp
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

target_link_libraries(MK_editor_core PUBLIC MK_animation MK_assets MK_core MK_environment MK_platform MK_runtime MK_scene MK_tools MK_ui)

MK_apply_common_target_options(MK_editor_core)

add_library(MK_editor_shell_common
    src/first_party_editor_document.cpp
    src/native_editor_app.cpp
    src/native_editor_launch.cpp
    src/native_editor_text_atlas_handoff.cpp
    src/native_editor_text_input.cpp
    src/native_editor_uia_provider.cpp
    src/native_editor_visible_texture_compositor.cpp
    src/native_material_preview_cache.cpp
    src/native_texture_display_adapter.cpp
    src/native_viewport_surface.cpp
)

target_include_directories(MK_editor_shell_common
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_link_libraries(MK_editor_shell_common PUBLIC MK_editor_core)
target_link_libraries(MK_editor_shell_common PRIVATE MK_renderer)

if(WIN32)
    target_sources(MK_editor_shell_common PRIVATE
        src/native_editor_text_font_adapters.cpp
        src/native_editor_tsf_text_input.cpp
        src/native_editor_win32_services.cpp
    )
    target_link_libraries(MK_editor_shell_common PRIVATE MK_platform_win32 dwrite uiautomationcore)
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
        MK_rhi
        MK_rhi_d3d12
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

set(MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE "")
if(DEFINED ENV{MK_SHADER_TOOLCHAIN_ROOT})
    set(MK_EDITOR_VISIBLE_TEXTURE_DXC_CONFIGURED "$ENV{MK_SHADER_TOOLCHAIN_ROOT}/dxc.exe")
    if(EXISTS "${MK_EDITOR_VISIBLE_TEXTURE_DXC_CONFIGURED}")
        set(MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE "${MK_EDITOR_VISIBLE_TEXTURE_DXC_CONFIGURED}")
    endif()
    if(NOT MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
        set(MK_EDITOR_VISIBLE_TEXTURE_DXC_CONFIGURED "$ENV{MK_SHADER_TOOLCHAIN_ROOT}/Bin/dxc.exe")
        if(EXISTS "${MK_EDITOR_VISIBLE_TEXTURE_DXC_CONFIGURED}")
            set(MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE "${MK_EDITOR_VISIBLE_TEXTURE_DXC_CONFIGURED}")
        endif()
    endif()
endif()
if(NOT MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE AND DEFINED ENV{VULKAN_SDK})
    set(MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_SDK "$ENV{VULKAN_SDK}/Bin/dxc.exe")
    if(EXISTS "${MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_SDK}")
        set(MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE "${MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_SDK}")
    endif()
endif()
if(NOT MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
    file(GLOB MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_CANDIDATES
        "C:/VulkanSDK/*/Bin/dxc.exe"
    )
    if(MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_CANDIDATES)
        list(SORT MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
        list(GET MK_EDITOR_VISIBLE_TEXTURE_DXC_VULKAN_CANDIDATES 0 MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
    endif()
endif()
if(NOT MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
    find_program(MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE NAMES dxc)
endif()
if(NOT MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
    file(GLOB MK_EDITOR_VISIBLE_TEXTURE_DXC_CANDIDATES
        "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxc.exe"
    )
    if(MK_EDITOR_VISIBLE_TEXTURE_DXC_CANDIDATES)
        list(SORT MK_EDITOR_VISIBLE_TEXTURE_DXC_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
        list(GET MK_EDITOR_VISIBLE_TEXTURE_DXC_CANDIDATES 0 MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
    endif()
endif()
if(NOT MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE)
    message(FATAL_ERROR "DXC is required for the native editor visible D3D12 texture compositor")
endif()

set(MK_EDITOR_VISIBLE_TEXTURE_SHADER_SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/native_editor_visible_texture.hlsl"
)
set(MK_EDITOR_VISIBLE_TEXTURE_SHADER_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/MK_editor/shaders"
)
set(MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_OUTPUT
    "${MK_EDITOR_VISIBLE_TEXTURE_SHADER_OUTPUT_DIR}/native_editor_visible_texture.vs.dxil"
)
set(MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_OUTPUT
    "${MK_EDITOR_VISIBLE_TEXTURE_SHADER_OUTPUT_DIR}/native_editor_visible_texture.ps.dxil"
)
add_custom_command(
    OUTPUT
        "${MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_OUTPUT}"
        "${MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_OUTPUT}"
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${MK_EDITOR_VISIBLE_TEXTURE_SHADER_OUTPUT_DIR}"
    COMMAND "${MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE}"
        -nologo -E vs_main -T vs_6_0
        -Fo "${MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_OUTPUT}"
        "${MK_EDITOR_VISIBLE_TEXTURE_SHADER_SOURCE}"
    COMMAND "${MK_EDITOR_VISIBLE_TEXTURE_DXC_EXECUTABLE}"
        -nologo -E ps_main -T ps_6_0
        -Fo "${MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_OUTPUT}"
        "${MK_EDITOR_VISIBLE_TEXTURE_SHADER_SOURCE}"
    DEPENDS
        "${MK_EDITOR_VISIBLE_TEXTURE_SHADER_SOURCE}"
    COMMENT "Compiling native editor visible D3D12 texture compositor shaders"
    VERBATIM
)
add_custom_target(MK_editor_visible_texture_shaders
    DEPENDS
        "${MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_OUTPUT}"
        "${MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_OUTPUT}"
)
add_dependencies(MK_editor MK_editor_visible_texture_shaders)
target_compile_definitions(MK_editor_shell_win32
    PRIVATE
        MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_PATH="${MK_EDITOR_VISIBLE_TEXTURE_VERTEX_SHADER_OUTPUT}"
        MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_PATH="${MK_EDITOR_VISIBLE_TEXTURE_FRAGMENT_SHADER_OUTPUT}"
)

MK_apply_common_target_options(MK_editor)
