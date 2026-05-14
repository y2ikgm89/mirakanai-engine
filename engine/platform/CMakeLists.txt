add_library(MK_platform
    src/clipboard.cpp
    src/cursor.cpp
    src/dynamic_library.cpp
    src/file_dialog.cpp
    src/linux_file_watcher.cpp
    src/macos_file_watcher.cpp
    src/file_watcher.cpp
    src/filesystem.cpp
    src/input.cpp
    src/lifecycle.cpp
    src/mobile.cpp
    src/process.cpp
    src/window.cpp
)

if(WIN32)
    target_sources(MK_platform PRIVATE
        src/windows_file_watcher.cpp
        src/win32_process.cpp
    )
endif()

target_include_directories(MK_platform
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_platform PUBLIC MK_math)

if(APPLE)
    target_link_libraries(MK_platform PRIVATE "-framework CoreServices")
endif()

MK_apply_common_target_options(MK_platform)

if(NOT TARGET mirakana_platform)
    add_library(mirakana_platform ALIAS MK_platform)
endif()

if(MK_DESKTOP_RUNTIME_ENABLED)
    add_subdirectory(sdl3)
endif()
