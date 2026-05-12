add_library(mirakana_rhi_d3d12
    src/d3d12_backend.cpp
)

target_include_directories(mirakana_rhi_d3d12
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_rhi_d3d12
    PUBLIC
        mirakana_rhi
    PRIVATE
        d3d12
        dxgi
)

mirakana_apply_common_target_options(mirakana_rhi_d3d12)

if(NOT TARGET MK_rhi_d3d12)
    add_library(MK_rhi_d3d12 ALIAS mirakana_rhi_d3d12)
endif()

