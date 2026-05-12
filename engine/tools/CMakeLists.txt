if(MK_ENABLE_ASSET_IMPORTERS)
    find_package(SPNG CONFIG REQUIRED)
    find_package(fastgltf CONFIG REQUIRED)
    find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h" REQUIRED)
endif()

add_subdirectory(shader)
add_subdirectory(gltf)
add_subdirectory(asset)
add_subdirectory(scene)

add_library(MK_tools STATIC
    $<TARGET_OBJECTS:MK_tools_shader>
    $<TARGET_OBJECTS:MK_tools_gltf>
    $<TARGET_OBJECTS:MK_tools_asset>
    $<TARGET_OBJECTS:MK_tools_scene>
)

target_include_directories(MK_tools
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_tools
    PUBLIC
        MK_animation
        MK_assets
        MK_platform
        MK_physics
        MK_runtime_scene
        MK_scene
        MK_ui
)

if(MK_ENABLE_ASSET_IMPORTERS)
    foreach(_mk_tools_obj IN ITEMS MK_tools_gltf MK_tools_asset)
        target_compile_definitions(${_mk_tools_obj} PRIVATE MK_HAS_ASSET_IMPORTERS=1)
        target_include_directories(${_mk_tools_obj} PRIVATE ${MINIAUDIO_INCLUDE_DIRS})
    endforeach()
    target_link_libraries(MK_tools PRIVATE spng::spng fastgltf::fastgltf)
else()
    foreach(_mk_tools_obj IN ITEMS MK_tools_gltf MK_tools_asset)
        target_compile_definitions(${_mk_tools_obj} PRIVATE MK_HAS_ASSET_IMPORTERS=0)
    endforeach()
endif()

MK_apply_common_target_options(MK_tools)
