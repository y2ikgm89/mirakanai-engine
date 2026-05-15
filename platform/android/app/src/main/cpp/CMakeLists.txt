cmake_minimum_required(VERSION 3.25)

project(MirakanaiAndroidPackage LANGUAGES C CXX)

set(MK_ANDROID_GAME_NAME "sample_headless" CACHE STRING "Mirakanai game directory name")
set(MK_ANDROID_GAME_MANIFEST "" CACHE FILEPATH "Path to games/<name>/game.agent.json")
set(MK_ANDROID_MIRAKANAI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../.." CACHE PATH "Mirakanai source root")

if(NOT EXISTS "${MK_ANDROID_MIRAKANAI_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "MK_ANDROID_MIRAKANAI_ROOT must point at the Mirakanai repository root")
endif()

if(NOT EXISTS "${MK_ANDROID_GAME_MANIFEST}")
    message(FATAL_ERROR "MK_ANDROID_GAME_MANIFEST does not exist: ${MK_ANDROID_GAME_MANIFEST}")
endif()

find_package(game-activity REQUIRED CONFIG)

add_subdirectory("${MK_ANDROID_MIRAKANAI_ROOT}" "${CMAKE_BINARY_DIR}/mirakanai")

add_library(mirakanai_android SHARED
    android_audio_output.cpp
    android_vulkan_surface_bridge.cpp
    game_activity_bridge.cpp
)

target_compile_features(mirakanai_android PRIVATE cxx_std_23)
target_compile_definitions(mirakanai_android PRIVATE
    MK_ANDROID_GAME_NAME="${MK_ANDROID_GAME_NAME}"
    MK_ANDROID_GAME_MANIFEST="${MK_ANDROID_GAME_MANIFEST}"
)

target_link_options(mirakanai_android
    PRIVATE
        "-u" "Java_com_google_androidgamesdk_GameActivity_initializeNativeCode"
)

target_link_libraries(mirakanai_android
    PRIVATE
        MK_core
        MK_audio
        MK_math
        MK_platform
        MK_ui
        aaudio
        android
        log
        vulkan
        game-activity::game-activity_static
)
