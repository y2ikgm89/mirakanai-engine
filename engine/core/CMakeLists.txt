add_library(mirakana_core
    src/application.cpp
    src/diagnostics.cpp
    src/log.cpp
    src/registry.cpp
    src/time.cpp
    src/version.cpp
)

target_include_directories(mirakana_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_core)

if(NOT TARGET MK_core)
    add_library(MK_core ALIAS mirakana_core)
endif()

