add_library(mirakana_rhi_vulkan
    src/vulkan_backend.cpp
)

target_include_directories(mirakana_rhi_vulkan
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_rhi_vulkan PUBLIC mirakana_rhi)
target_link_libraries(mirakana_rhi_vulkan PRIVATE ${CMAKE_DL_LIBS})

mirakana_apply_common_target_options(mirakana_rhi_vulkan)

if(NOT TARGET MK_rhi_vulkan)
    add_library(MK_rhi_vulkan ALIAS mirakana_rhi_vulkan)
endif()

