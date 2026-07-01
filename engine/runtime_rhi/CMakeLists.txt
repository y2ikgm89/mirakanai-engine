add_library(mirakana_runtime_rhi
    src/mavg_cluster_streaming_safe_point_adoption.cpp
    src/mavg_cluster_streaming_residency_closeout.cpp
    src/mavg_autonomous_streaming_scheduler.cpp
    src/mavg_conventional_upload.cpp
    src/mavg_gpu_memory_residency.cpp
    src/mavg_mesh_shader_capability_gate.cpp
    src/mavg_streamed_cluster_gpu_upload.cpp
    src/mavg_advanced_backend_evidence.cpp
    src/mavg_async_overlap_performance.cpp
    src/mavg_async_overlap_performance_proof.cpp
    src/mavg_broad_optimization_evidence.cpp
    src/mavg_directstorage_gpu_destination_execution.cpp
    src/mavg_directstorage_gpu_decompression_policy.cpp
    src/mavg_streaming_upload_overlap_evidence.cpp
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
