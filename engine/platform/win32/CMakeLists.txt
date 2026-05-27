add_library(mirakana_platform_win32
    src/win32_cursor.cpp
    src/win32_event_pump.cpp
    src/win32_runtime.cpp
    src/win32_window.cpp
)

target_include_directories(mirakana_platform_win32
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_platform_win32
    PUBLIC
        mirakana_platform
    PRIVATE
        user32
)

mirakana_apply_common_target_options(mirakana_platform_win32)

if(NOT TARGET MK_platform_win32)
    add_library(MK_platform_win32 ALIAS mirakana_platform_win32)
endif()
