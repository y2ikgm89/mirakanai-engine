add_library(MK_runtime_host_win32
    src/win32_desktop_event_pump.cpp
    src/win32_mavg_payload_io.cpp
)

target_include_directories(MK_runtime_host_win32
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime_host_win32
    PUBLIC
        MK_runtime
        MK_runtime_host
        MK_platform_win32
)

MK_apply_common_target_options(MK_runtime_host_win32)

if(MK_ENABLE_DIRECTSTORAGE_SDK)
    add_library(MK_runtime_host_win32_directstorage
        src/win32_mavg_directstorage_payload_io.cpp
    )

    target_include_directories(MK_runtime_host_win32_directstorage
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../rhi/d3d12/src>
    )

    target_link_libraries(MK_runtime_host_win32_directstorage
        PUBLIC
            MK_runtime_host_win32
            MK_rhi
        PRIVATE
            MK_rhi_d3d12
            Microsoft::DirectStorage
            d3d12
            dxgi
    )
    target_compile_definitions(MK_runtime_host_win32_directstorage
        PUBLIC
            MK_RUNTIME_HOST_WIN32_ENABLE_DIRECTSTORAGE_SDK=1
    )

    MK_apply_common_target_options(MK_runtime_host_win32_directstorage)
endif()

add_library(MK_runtime_host_win32_presentation
    src/win32_desktop_game_host.cpp
    src/win32_desktop_presentation.cpp
)

target_include_directories(MK_runtime_host_win32_presentation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime_host_win32_presentation
    PUBLIC
        MK_runtime_host_win32
        MK_platform_win32
        MK_renderer
        MK_rhi
    PRIVATE
        MK_runtime_scene_rhi
)

if(WIN32 AND TARGET MK_rhi_d3d12)
    target_link_libraries(MK_runtime_host_win32_presentation PRIVATE MK_rhi_d3d12)
    target_compile_definitions(MK_runtime_host_win32_presentation PRIVATE MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_D3D12=1)
endif()

if(WIN32 AND TARGET MK_rhi_vulkan)
    target_link_libraries(MK_runtime_host_win32_presentation PRIVATE MK_rhi_vulkan)
    target_compile_definitions(MK_runtime_host_win32_presentation PRIVATE MK_RUNTIME_HOST_WIN32_PRESENTATION_HAS_VULKAN=1)
endif()

MK_apply_common_target_options(MK_runtime_host_win32_presentation)
