add_library(mirakana_math
    src/math.cpp
)

target_include_directories(mirakana_math
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_math)

if(NOT TARGET MK_math)
    add_library(MK_math ALIAS mirakana_math)
endif()
