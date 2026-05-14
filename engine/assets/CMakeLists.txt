add_library(mirakana_assets
    src/asset_dependency_graph.cpp
    src/asset_hot_reload.cpp
    src/asset_identity.cpp
    src/asset_import_metadata.cpp
    src/asset_import_pipeline.cpp
    src/asset_package.cpp
    src/asset_registry.cpp
    src/asset_source_format.cpp
    src/material.cpp
    src/material_graph.cpp
    src/material_graph_shader_export.cpp
    src/shader_metadata.cpp
    src/shader_pipeline.cpp
    src/source_asset_registry.cpp
    src/sprite_atlas_packing.cpp
    src/tilemap_metadata.cpp
    src/ui_atlas_metadata.cpp
)

target_include_directories(mirakana_assets
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_assets)

if(NOT TARGET MK_assets)
    add_library(MK_assets ALIAS mirakana_assets)
endif()
