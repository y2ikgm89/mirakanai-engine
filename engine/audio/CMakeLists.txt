add_library(MK_audio
    src/audio_mixer.cpp
)

target_include_directories(MK_audio
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_audio PUBLIC MK_assets)

MK_apply_common_target_options(MK_audio)

if(MK_DESKTOP_RUNTIME_ENABLED)
    add_subdirectory(sdl3)
endif()
