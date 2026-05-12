add_library(mirakana_runtime
    src/asset_runtime.cpp
    src/package_streaming.cpp
    src/physics_collision_runtime.cpp
    src/resource_runtime.cpp
    src/runtime_diagnostics.cpp
    src/session_services.cpp
)

target_include_directories(mirakana_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_runtime
    PUBLIC
        mirakana_assets
        mirakana_platform
        mirakana_physics
)

mirakana_apply_common_target_options(mirakana_runtime)

if(NOT TARGET MK_runtime)
    add_library(MK_runtime ALIAS mirakana_runtime)
endif()

