find_package(SDL3 CONFIG REQUIRED)

add_library(mirakana_platform_sdl3
    src/sdl_clipboard.cpp
    src/sdl_cursor.cpp
    src/sdl_file_dialog.cpp
    src/sdl_input.cpp
    src/sdl_ui_platform_integration.cpp
    src/sdl_window.cpp
)

target_include_directories(mirakana_platform_sdl3
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_platform_sdl3
    PUBLIC
        mirakana_platform
        mirakana_ui
    PRIVATE
        SDL3::SDL3
)

mirakana_apply_common_target_options(mirakana_platform_sdl3)

if(NOT TARGET MK_platform_sdl3)
    add_library(MK_platform_sdl3 ALIAS mirakana_platform_sdl3)
endif()
