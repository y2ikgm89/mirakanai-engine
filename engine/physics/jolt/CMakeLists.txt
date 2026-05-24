find_package(Jolt CONFIG REQUIRED)

add_library(mirakana_physics_jolt
    src/jolt_physics_adapter.cpp
)

target_include_directories(mirakana_physics_jolt
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_physics_jolt
    PUBLIC
        mirakana_physics
    PRIVATE
        Jolt::Jolt
)

mirakana_apply_common_target_options(mirakana_physics_jolt)

if(NOT TARGET MK_physics_jolt)
    add_library(MK_physics_jolt ALIAS mirakana_physics_jolt)
endif()
