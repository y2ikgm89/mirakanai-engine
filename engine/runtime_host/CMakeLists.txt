add_library(MK_runtime_host
    src/desktop_runner.cpp
    src/linux_desktop_host_contract.cpp
    src/shader_bytecode.cpp
)

target_include_directories(MK_runtime_host
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_runtime_host
    PUBLIC
        MK_core
        MK_platform
        MK_renderer
)

if(UNIX AND NOT APPLE)
    target_link_libraries(MK_runtime_host PRIVATE MK_rhi_vulkan ${CMAKE_DL_LIBS})
endif()

MK_apply_common_target_options(MK_runtime_host)

if(WIN32 AND MK_DESKTOP_RUNTIME_ENABLED)
    add_subdirectory(win32)
endif()

if(UNIX AND NOT APPLE AND MK_DESKTOP_RUNTIME_ENABLED)
    add_subdirectory(linux)
endif()
