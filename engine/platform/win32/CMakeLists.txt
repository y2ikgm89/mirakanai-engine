add_library(mirakana_platform_win32
    src/win32_clipboard.cpp
    src/win32_cpu_sets.cpp
    src/win32_cursor.cpp
    src/win32_directstorage_byte_range_io.cpp
    src/win32_event_pump.cpp
    src/win32_file_dialog.cpp
    src/win32_input.cpp
    src/win32_runtime.cpp
    src/win32_text_input.cpp
    src/win32_ui_text_font.cpp
    src/win32_utf.cpp
    src/win32_window.cpp
)

target_include_directories(mirakana_platform_win32
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_platform_win32
    PUBLIC
        mirakana_core
        mirakana_platform
        mirakana_ui
    PRIVATE
        ole32
        dwrite
        shell32
        user32
)

if(MK_ENABLE_WIN32_DIRECTSTORAGE)
    find_package(dstorage CONFIG REQUIRED)
    target_compile_definitions(mirakana_platform_win32 PRIVATE MK_ENABLE_WIN32_DIRECTSTORAGE)
    target_link_libraries(mirakana_platform_win32 PRIVATE Microsoft::DirectStorage)
endif()

mirakana_apply_common_target_options(mirakana_platform_win32)

if(NOT TARGET MK_platform_win32)
    add_library(MK_platform_win32 ALIAS mirakana_platform_win32)
endif()
