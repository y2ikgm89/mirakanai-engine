add_library(mirakana_animation
    src/chain_ik.cpp
    src/keyframe_animation.cpp
    src/morph.cpp
    src/two_bone_ik.cpp
    src/skin.cpp
    src/skeleton.cpp
    src/state_machine.cpp
    src/timeline.cpp
)

target_include_directories(mirakana_animation
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_animation PUBLIC mirakana_math)

mirakana_apply_common_target_options(mirakana_animation)

if(NOT TARGET MK_animation)
    add_library(MK_animation ALIAS mirakana_animation)
endif()

