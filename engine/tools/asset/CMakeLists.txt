add_library(MK_tools_asset OBJECT
    asset_axis_unit_preview.cpp
    asset_coordinate_normalization.cpp
    asset_file_scanner.cpp
    asset_import_adapters.cpp
    asset_import_batch_reimport_tool.cpp
    asset_import_regression_runner.cpp
    asset_runtime_package_hot_reload_tool.cpp
    asset_import_tool.cpp
    asset_package_tool.cpp
    2d_originality_review.cpp
    2d_source_pulse.cpp
    environment_texture_pipeline_v2.cpp
    environment_texture_source_review.cpp
    morph_mesh_cpu_source_bridge.cpp
    registered_source_asset_cook_package_tool.cpp
    source_asset_registration_tool.cpp
    source_image_decode.cpp
    sprite_atlas_tool.cpp
    sandbox_world_authoring.cpp
    material_tool.cpp
    tilemap_tool.cpp
    gameplay_authoring_tool.cpp
    placeholder_asset_tool.cpp
    production_authoring_workflows.cpp
    mavg_cluster_cook.cpp
    ui_atlas_tool.cpp
)

target_include_directories(MK_tools_asset PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
)

target_link_libraries(MK_tools_asset PUBLIC
    MK_animation
    MK_assets
    mirakana_environment
    MK_platform
    MK_runtime
    MK_scene
    MK_ui
)

MK_apply_common_target_options(MK_tools_asset)
