find_package(SDL3 CONFIG REQUIRED)

add_library(mirakana_audio_sdl3
    src/sdl_audio_device.cpp
)

target_include_directories(mirakana_audio_sdl3
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_audio_sdl3
    PUBLIC
        MK_audio
    PRIVATE
        SDL3::SDL3
)

mirakana_apply_common_target_options(mirakana_audio_sdl3)

if(NOT TARGET MK_audio_sdl3)
    add_library(MK_audio_sdl3 ALIAS mirakana_audio_sdl3)
endif()
