add_library(mirakana_renderer
    src/cloud_layer_policy.cpp
    src/environment_fog_policy.cpp
    src/environment_lighting_policy.cpp
    src/environment_policy.cpp
    src/frame_graph.cpp
    src/frame_graph_rhi.cpp
    src/null_renderer.cpp
    src/physical_sky_policy.cpp
    src/precipitation_policy.cpp
    src/postprocess_policy.cpp
    src/production_vfx_profiling.cpp
    src/renderer_quality_matrix.cpp
    src/rhi_directional_shadow_smoke_frame_renderer.cpp
    src/rhi_frame_renderer.cpp
    src/rhi_native_ui_overlay.cpp
    src/rhi_postprocess_frame_renderer.cpp
    src/rhi_viewport_surface.cpp
    src/backend_renderer_parity_policy.cpp
    src/debug_profiling_policy.cpp
    src/gpu_memory_policy.cpp
    src/mavg_gpu_culling.cpp
    src/mavg_lod_selection.cpp
    src/scene_scale_policy.cpp
    src/shadow_map.cpp
    src/sprite_batch.cpp
    src/tile_chunk_renderer.cpp
    src/volumetric_fog_policy.cpp
    src/volumetric_cloud_policy.cpp
)

target_include_directories(mirakana_renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_renderer PUBLIC mirakana_assets mirakana_core mirakana_environment mirakana_math mirakana_rhi)

mirakana_apply_common_target_options(mirakana_renderer)

if(NOT TARGET MK_renderer)
    add_library(MK_renderer ALIAS mirakana_renderer)
endif()
