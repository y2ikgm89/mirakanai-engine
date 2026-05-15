add_library(mirakana_scene_renderer
    src/scene_renderer.cpp
)

target_include_directories(mirakana_scene_renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_scene_renderer
    PUBLIC
        mirakana_renderer
        mirakana_runtime
        mirakana_runtime_scene
        mirakana_scene
)

mirakana_apply_common_target_options(mirakana_scene_renderer)

if(NOT TARGET MK_scene_renderer)
    add_library(MK_scene_renderer ALIAS mirakana_scene_renderer)
endif()
