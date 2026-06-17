add_library(mirakana_environment
    src/cloud_layer.cpp
    src/environment_commercial_readiness.cpp
    src/environment_io.cpp
    src/environment_preset_pack.cpp
    src/environment_profile.cpp
    src/environment_quality_budget.cpp
    src/material_weathering.cpp
    src/time_of_day.cpp
    src/weather.cpp
    src/weather_simulation.cpp
)

target_include_directories(mirakana_environment
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_environment PUBLIC mirakana_math)

mirakana_apply_common_target_options(mirakana_environment)

if(NOT TARGET MK_environment)
    add_library(MK_environment ALIAS mirakana_environment)
endif()
