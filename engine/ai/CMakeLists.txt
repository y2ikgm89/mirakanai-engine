add_library(mirakana_ai
    src/behavior_tree.cpp
    src/perception.cpp
)

target_include_directories(mirakana_ai
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_ai)

if(NOT TARGET MK_ai)
    add_library(MK_ai ALIAS mirakana_ai)
endif()
