add_library(mirakana_ui
    src/ui.cpp
)

target_include_directories(mirakana_ui
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_ui)

if(NOT TARGET MK_ui)
    add_library(MK_ui ALIAS mirakana_ui)
endif()
