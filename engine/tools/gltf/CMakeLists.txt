add_library(MK_tools_gltf OBJECT
    gltf_mesh_inspect.cpp
    gltf_skin_animation_inspect.cpp
    gltf_skin_animation_import.cpp
    gltf_morph_animation_import.cpp
    gltf_node_animation_import.cpp
)

target_include_directories(MK_tools_gltf PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(MK_tools_gltf PUBLIC
    MK_animation
    MK_assets
)

MK_apply_common_target_options(MK_tools_gltf)
