add_library(mirakana_audio_wasapi
    src/wasapi_audio_device.cpp
)

target_include_directories(mirakana_audio_wasapi
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_audio_wasapi
    PUBLIC
        MK_audio
    PRIVATE
        ole32
)

mirakana_apply_common_target_options(mirakana_audio_wasapi)

if(NOT TARGET MK_audio_wasapi)
    add_library(MK_audio_wasapi ALIAS mirakana_audio_wasapi)
endif()
