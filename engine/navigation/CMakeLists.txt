add_library(mirakana_navigation
    src/local_avoidance.cpp
    src/navigation_agent.cpp
    src/navigation_grid.cpp
    src/navigation_path_planner.cpp
    src/navigation_replan.cpp
    src/path_following.cpp
    src/path_smoothing.cpp
)

target_include_directories(mirakana_navigation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_navigation)

if(NOT TARGET MK_navigation)
    add_library(MK_navigation ALIAS mirakana_navigation)
endif()

