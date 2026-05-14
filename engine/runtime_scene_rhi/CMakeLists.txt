add_library(mirakana_runtime_scene_rhi
    src/runtime_scene_rhi.cpp
)

target_include_directories(mirakana_runtime_scene_rhi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_runtime_scene_rhi
    PUBLIC
        mirakana_runtime
        mirakana_runtime_rhi
        mirakana_rhi
        mirakana_scene_renderer
)

mirakana_apply_common_target_options(mirakana_runtime_scene_rhi)

if(NOT TARGET MK_runtime_scene_rhi)
    add_library(MK_runtime_scene_rhi ALIAS mirakana_runtime_scene_rhi)
endif()
