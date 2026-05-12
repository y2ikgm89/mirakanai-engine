add_library(mirakana_runtime_scene
    src/runtime_scene.cpp
)

target_include_directories(mirakana_runtime_scene
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_runtime_scene
    PUBLIC
        mirakana_animation
        mirakana_runtime
        mirakana_scene
)

mirakana_apply_common_target_options(mirakana_runtime_scene)

if(NOT TARGET MK_runtime_scene)
    add_library(MK_runtime_scene ALIAS mirakana_runtime_scene)
endif()

