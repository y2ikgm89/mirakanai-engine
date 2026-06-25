add_library(MK_rhi_metal
    src/metal_backend.cpp
    src/metal_environment_weather_solver.cpp
    src/metal_mavg_mesh_lod.cpp
)

target_include_directories(MK_rhi_metal
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(MK_rhi_metal PUBLIC MK_rhi)

if(APPLE)
    enable_language(OBJCXX)
    find_program(MK_APPLE_XCRUN xcrun REQUIRED)
    set(MK_METAL_ENVIRONMENT_EVIDENCE_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/environment_feature_evidence.metal")
    set(MK_METAL_ENVIRONMENT_EVIDENCE_AIR "${CMAKE_CURRENT_BINARY_DIR}/environment_feature_evidence.air")
    set(MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB "${CMAKE_CURRENT_BINARY_DIR}/environment_feature_evidence.metallib")
    add_custom_command(
        OUTPUT "${MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB}"
        COMMAND "${MK_APPLE_XCRUN}" --sdk macosx metal -c "${MK_METAL_ENVIRONMENT_EVIDENCE_SOURCE}" -o "${MK_METAL_ENVIRONMENT_EVIDENCE_AIR}"
        COMMAND "${MK_APPLE_XCRUN}" --sdk macosx metallib "${MK_METAL_ENVIRONMENT_EVIDENCE_AIR}" -o "${MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB}"
        DEPENDS "${MK_METAL_ENVIRONMENT_EVIDENCE_SOURCE}"
        BYPRODUCTS "${MK_METAL_ENVIRONMENT_EVIDENCE_AIR}"
        VERBATIM
    )
    add_custom_target(MK_metal_environment_evidence_metallib
        DEPENDS "${MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB}"
    )
    set_property(GLOBAL PROPERTY MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB "${MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB}")
    set(MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_SOURCE
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders/visible_renderer_package_evidence.metal")
    set(MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_AIR
        "${CMAKE_CURRENT_BINARY_DIR}/visible_renderer_package_evidence.air")
    set(MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB
        "${CMAKE_CURRENT_BINARY_DIR}/visible_renderer_package_evidence.metallib")
    add_custom_command(
        OUTPUT "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB}"
        COMMAND "${MK_APPLE_XCRUN}" --sdk macosx metal -c
            "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_SOURCE}"
            -o "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_AIR}"
        COMMAND "${MK_APPLE_XCRUN}" --sdk macosx metallib
            "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_AIR}"
            -o "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB}"
        DEPENDS "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_SOURCE}"
        BYPRODUCTS "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_AIR}"
        VERBATIM
    )
    add_custom_target(MK_metal_visible_renderer_package_evidence_metallib
        DEPENDS "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB}"
    )
    set_property(GLOBAL PROPERTY MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB
        "${MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB}")
    set(MK_METAL_ENVIRONMENT_WEATHER_SOLVER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/shaders/weather_simulation.metal")
    set(MK_METAL_ENVIRONMENT_WEATHER_SOLVER_AIR "${CMAKE_CURRENT_BINARY_DIR}/weather_simulation.air")
    set(MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB "${CMAKE_CURRENT_BINARY_DIR}/weather_simulation.metallib")
    add_custom_command(
        OUTPUT "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB}"
        COMMAND "${MK_APPLE_XCRUN}" --sdk macosx metal -c "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_SOURCE}" -o "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_AIR}"
        COMMAND "${MK_APPLE_XCRUN}" --sdk macosx metallib "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_AIR}" -o "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB}"
        DEPENDS "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_SOURCE}"
        BYPRODUCTS "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_AIR}"
        VERBATIM
    )
    add_custom_target(MK_metal_environment_weather_solver_metallib
        DEPENDS "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB}"
    )
    set_property(GLOBAL PROPERTY MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB "${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB}")
    target_sources(MK_rhi_metal PRIVATE
        src/metal_native.mm
        src/metal_environment_weather_solver_native.mm
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
