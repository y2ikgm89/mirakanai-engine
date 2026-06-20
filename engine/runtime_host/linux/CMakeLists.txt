add_library(MK_runtime_host_linux
    src/linux_desktop_game_host.cpp
)

target_link_libraries(MK_runtime_host_linux
    PUBLIC
        MK_runtime_host
    PRIVATE
        ${CMAKE_DL_LIBS}
)

MK_apply_common_target_options(MK_runtime_host_linux)
