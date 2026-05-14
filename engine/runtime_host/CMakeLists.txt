add_library(MK_runtime_host
    src/desktop_runner.cpp
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

MK_apply_common_target_options(MK_runtime_host)

if(MK_DESKTOP_RUNTIME_ENABLED)
    add_subdirectory(sdl3)
endif()
