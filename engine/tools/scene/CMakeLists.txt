add_library(MK_tools_scene OBJECT
    physics_collision_package_tool.cpp
    runtime_scene_package_validation_tool.cpp
    scene_prefab_authoring_tool.cpp
    scene_tool.cpp
    scene_v2_runtime_package_migration_tool.cpp
)

target_include_directories(MK_tools_scene PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(MK_tools_scene PUBLIC
    MK_assets
    MK_platform
    MK_physics
    MK_runtime_scene
    MK_scene
)

MK_apply_common_target_options(MK_tools_scene)
