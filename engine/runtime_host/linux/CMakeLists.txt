add_library(MK_runtime_host_linux
    src/linux_vulkan_runtime_probe.cpp
)

target_include_directories(MK_runtime_host_linux
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime_host_linux
    PUBLIC
        MK_runtime_host
        MK_platform_linux
        MK_rhi_vulkan
)

MK_apply_common_target_options(MK_runtime_host_linux)
