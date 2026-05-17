add_library(mirakana_runtime_rhi
    src/package_streaming_frame_graph.cpp
    src/runtime_upload.cpp
)

target_include_directories(mirakana_runtime_rhi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_runtime_rhi
    PUBLIC
        mirakana_runtime
        mirakana_renderer
        mirakana_rhi
)

mirakana_apply_common_target_options(mirakana_runtime_rhi)

if(NOT TARGET MK_runtime_rhi)
    add_library(MK_runtime_rhi ALIAS mirakana_runtime_rhi)
endif()
