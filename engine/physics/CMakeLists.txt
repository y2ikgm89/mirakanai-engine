add_library(mirakana_physics
    src/physics2d.cpp
    src/physics3d.cpp
)

target_include_directories(mirakana_physics
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_physics PUBLIC mirakana_math)

mirakana_apply_common_target_options(mirakana_physics)

if(NOT TARGET MK_physics)
    add_library(MK_physics ALIAS mirakana_physics)
endif()
