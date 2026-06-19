add_library(mirakana_platform_linux
    src/linux_platform_probe.cpp
)

target_include_directories(mirakana_platform_linux
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_platform_linux
    PUBLIC
        mirakana_platform
)

mirakana_apply_common_target_options(mirakana_platform_linux)

if(NOT TARGET MK_platform_linux)
    add_library(MK_platform_linux ALIAS mirakana_platform_linux)
endif()
