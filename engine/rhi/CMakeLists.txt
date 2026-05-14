add_library(mirakana_rhi
    src/async_overlap.cpp
    src/backend_capabilities.cpp
    src/null_rhi.cpp
    src/resource_lifetime.cpp
    src/upload_staging.cpp
)

target_include_directories(mirakana_rhi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_rhi)

if(NOT TARGET MK_rhi)
    add_library(MK_rhi ALIAS mirakana_rhi)
endif()

if(WIN32)
    add_subdirectory(d3d12)
endif()

add_subdirectory(vulkan)
add_subdirectory(metal)
