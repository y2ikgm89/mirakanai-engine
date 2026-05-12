add_library(mirakana_renderer
    src/frame_graph.cpp
    src/frame_graph_rhi.cpp
    src/null_renderer.cpp
    src/rhi_directional_shadow_smoke_frame_renderer.cpp
    src/rhi_frame_renderer.cpp
    src/rhi_native_ui_overlay.cpp
    src/rhi_postprocess_frame_renderer.cpp
    src/rhi_viewport_surface.cpp
    src/shadow_map.cpp
    src/sprite_batch.cpp
)

target_include_directories(mirakana_renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_renderer PUBLIC mirakana_assets mirakana_core mirakana_math mirakana_rhi)

mirakana_apply_common_target_options(mirakana_renderer)

if(NOT TARGET MK_renderer)
    add_library(MK_renderer ALIAS mirakana_renderer)
endif()

