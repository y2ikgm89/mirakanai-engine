add_library(MK_tools_asset OBJECT
    asset_file_scanner.cpp
    asset_import_adapters.cpp
    asset_runtime_package_hot_reload_tool.cpp
    asset_import_tool.cpp
    asset_package_tool.cpp
    morph_mesh_cpu_source_bridge.cpp
    registered_source_asset_cook_package_tool.cpp
    source_asset_registration_tool.cpp
    source_image_decode.cpp
    material_tool.cpp
    tilemap_tool.cpp
    ui_atlas_tool.cpp
)

target_include_directories(MK_tools_asset PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(MK_tools_asset PUBLIC
    MK_animation
    MK_assets
    MK_platform
    MK_runtime
    MK_scene
    MK_ui
)

MK_apply_common_target_options(MK_tools_asset)
