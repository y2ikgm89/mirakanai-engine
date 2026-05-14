find_package(SDL3 CONFIG REQUIRED)

add_library(MK_runtime_host_sdl3
    src/sdl_desktop_event_pump.cpp
)

target_include_directories(MK_runtime_host_sdl3
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime_host_sdl3
    PUBLIC
        MK_runtime_host
        MK_platform_sdl3
    PRIVATE
        SDL3::SDL3
)

MK_apply_common_target_options(MK_runtime_host_sdl3)

add_library(MK_runtime_host_sdl3_presentation
    src/sdl_desktop_game_host.cpp
    src/sdl_desktop_presentation.cpp
)

target_include_directories(MK_runtime_host_sdl3_presentation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime_host_sdl3_presentation
    PUBLIC
        MK_runtime_host_sdl3
        MK_platform_sdl3
        MK_renderer
        MK_rhi
    PRIVATE
        MK_runtime_scene_rhi
        SDL3::SDL3
)

if(WIN32 AND TARGET MK_rhi_d3d12)
    target_link_libraries(MK_runtime_host_sdl3_presentation PRIVATE MK_rhi_d3d12)
    target_compile_definitions(MK_runtime_host_sdl3_presentation PRIVATE MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_D3D12=1)
endif()

if(WIN32 AND TARGET MK_rhi_vulkan)
    target_link_libraries(MK_runtime_host_sdl3_presentation PRIVATE MK_rhi_vulkan)
    target_compile_definitions(MK_runtime_host_sdl3_presentation PRIVATE MK_RUNTIME_HOST_SDL3_PRESENTATION_HAS_VULKAN=1)
endif()

MK_apply_common_target_options(MK_runtime_host_sdl3_presentation)
