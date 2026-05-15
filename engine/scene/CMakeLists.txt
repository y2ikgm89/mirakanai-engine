add_library(mirakana_scene
    src/components.cpp
    src/prefab.cpp
    src/prefab_overrides.cpp
    src/playable_2d.cpp
    src/render_packet.cpp
    src/schema_v2.cpp
    src/scene.cpp
    src/scene_io.cpp
)

target_include_directories(mirakana_scene
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_scene PUBLIC mirakana_assets mirakana_math)

mirakana_apply_common_target_options(mirakana_scene)

if(NOT TARGET MK_scene)
    add_library(MK_scene ALIAS mirakana_scene)
endif()
