add_library(MK_tools_shader OBJECT
    shader_compile_action.cpp
    shader_tool_process.cpp
    shader_toolchain.cpp
    material_graph_shader_pipeline.cpp
)

target_include_directories(MK_tools_shader PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(MK_tools_shader PUBLIC
    MK_assets
    MK_platform
)

MK_apply_common_target_options(MK_tools_shader)
