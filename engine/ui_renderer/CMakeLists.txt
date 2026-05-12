add_library(mirakana_ui_renderer
    src/ui_renderer.cpp
)

target_include_directories(mirakana_ui_renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_ui_renderer PUBLIC mirakana_renderer mirakana_runtime mirakana_ui)

mirakana_apply_common_target_options(mirakana_ui_renderer)

if(NOT TARGET MK_ui_renderer)
    add_library(MK_ui_renderer ALIAS mirakana_ui_renderer)
endif()

