add_library(MK_rhi_metal
    src/metal_backend.cpp
)

target_include_directories(MK_rhi_metal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_rhi_metal PUBLIC MK_rhi)

if(APPLE)
    enable_language(OBJCXX)
    target_sources(MK_rhi_metal PRIVATE
        src/metal_native.mm
    )
    find_library(MK_APPLE_FOUNDATION_FRAMEWORK Foundation REQUIRED)
    find_library(MK_APPLE_METAL_FRAMEWORK Metal REQUIRED)
    find_library(MK_APPLE_QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
    target_link_libraries(MK_rhi_metal PRIVATE
        ${MK_APPLE_FOUNDATION_FRAMEWORK}
        ${MK_APPLE_METAL_FRAMEWORK}
        ${MK_APPLE_QUARTZCORE_FRAMEWORK}
    )
    target_compile_options(MK_rhi_metal PRIVATE $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)
endif()

MK_apply_common_target_options(MK_rhi_metal)

