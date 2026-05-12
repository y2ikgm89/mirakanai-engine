cmake_minimum_required(VERSION 3.25)

project(MirakanaiApplePackage LANGUAGES CXX OBJCXX)

set(MK_IOS_GAME_NAME "sample_headless" CACHE STRING "Mirakanai game directory name")
set(MK_IOS_GAME_MANIFEST "" CACHE FILEPATH "Path to games/<name>/game.agent.json")
set(MK_IOS_MIRAKANAI_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." CACHE PATH "Mirakanai source root")
set(MK_IOS_BUNDLE_IDENTIFIER "dev.mirakanai.ios" CACHE STRING "iOS bundle identifier")
set(MK_IOS_DEVELOPMENT_TEAM "" CACHE STRING "Apple development team id for Xcode signing")
set(MK_IOS_CODE_SIGN_IDENTITY "" CACHE STRING "Optional Xcode code signing identity")
set(MK_IOS_CODE_SIGN_STYLE "Automatic" CACHE STRING "Xcode code signing style")
set(MK_IOS_CODE_SIGNING_ALLOWED "" CACHE STRING "Optional Xcode CODE_SIGNING_ALLOWED override")

if(NOT APPLE)
    message(FATAL_ERROR "Apple packaging requires an Apple host toolchain")
endif()

if(NOT EXISTS "${MK_IOS_MIRAKANAI_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "MK_IOS_MIRAKANAI_ROOT must point at the Mirakanai repository root")
endif()

if(NOT EXISTS "${MK_IOS_GAME_MANIFEST}")
    message(FATAL_ERROR "MK_IOS_GAME_MANIFEST does not exist: ${MK_IOS_GAME_MANIFEST}")
endif()

add_subdirectory("${MK_IOS_MIRAKANAI_ROOT}" "${CMAKE_BINARY_DIR}/mirakanai")

add_executable(MirakanaiIOS MACOSX_BUNDLE
    Sources/MirakanaiIOSApp/AppDelegate.mm
    "${MK_IOS_GAME_MANIFEST}"
    "${CMAKE_CURRENT_LIST_DIR}/Resources/.gitkeep"
)

target_compile_features(MirakanaiIOS PRIVATE cxx_std_23)
target_compile_definitions(MirakanaiIOS PRIVATE
    MK_IOS_GAME_NAME="${MK_IOS_GAME_NAME}"
    MK_IOS_GAME_MANIFEST="${MK_IOS_GAME_MANIFEST}"
)

target_link_libraries(MirakanaiIOS
    PRIVATE
        MK_core
        MK_math
        MK_platform
        MK_ui
        "-framework UIKit"
        "-framework Metal"
        "-framework QuartzCore"
)

set_source_files_properties("${MK_IOS_GAME_MANIFEST}" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/game"
)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/Resources/.gitkeep" PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources"
)

set_target_properties(MirakanaiIOS PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/Info.plist.in"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${MK_IOS_BUNDLE_IDENTIFIER}"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "${MK_IOS_CODE_SIGN_STYLE}"
)

if(NOT MK_IOS_DEVELOPMENT_TEAM STREQUAL "")
    set_property(TARGET MirakanaiIOS PROPERTY XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${MK_IOS_DEVELOPMENT_TEAM}")
endif()

if(NOT MK_IOS_CODE_SIGN_IDENTITY STREQUAL "")
    set_property(TARGET MirakanaiIOS PROPERTY XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${MK_IOS_CODE_SIGN_IDENTITY}")
endif()

if(NOT MK_IOS_CODE_SIGNING_ALLOWED STREQUAL "")
    set_property(TARGET MirakanaiIOS PROPERTY XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "${MK_IOS_CODE_SIGNING_ALLOWED}")
endif()

