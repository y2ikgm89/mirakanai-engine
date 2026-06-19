cmake_minimum_required(VERSION 3.30)

project(Mirakanai
    VERSION 0.1.0
    DESCRIPTION "Clean C++ game engine core"
    LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(MK_ENABLE_DESKTOP_RUNTIME "Build desktop game runtime targets" OFF)
option(MK_ENABLE_DESKTOP_EDITOR "Build the native desktop editor shell target" OFF)
option(MK_ENABLE_SANITIZERS "Enable compiler sanitizer instrumentation where supported" OFF)
option(MK_ENABLE_CXX_MODULE_SCANNING "Enable CMake C++ module dependency scanning for targets" ON)
option(MK_ENABLE_IMPORT_STD "Enable CMake-managed import std support when the active toolchain and generator support it" ON)
option(MK_ENABLE_ASSET_IMPORTERS "Build optional libspng/fastgltf/miniaudio source asset importer adapters" OFF)
option(MK_ENABLE_PHYSICS_JOLT "Build optional Jolt Physics native middleware adapter" OFF)
option(MK_ENABLE_NETWORK_ENET "Build optional ENet loopback network transport adapter" OFF)
option(MK_REQUIRE_DESKTOP_RUNTIME_DXIL "Require sample_desktop_runtime_shell DXIL artifacts when packaging the desktop runtime shell" OFF)
option(MK_REQUIRE_DESKTOP_RUNTIME_SPIRV "Require Vulkan SPIR-V artifacts when packaging desktop runtime targets" OFF)
set(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET "" CACHE STRING "Registered desktop runtime game target to install for the desktop runtime package lane")
set(MK_LINUX_RUNTIME_PACKAGE_TARGET "" CACHE STRING "Linux runtime target to install for the Linux Vulkan runtime package lane")
set(MK_CXX_STANDARD "23" CACHE STRING "C++ standard level used by Mirakanai targets")
set_property(CACHE MK_CXX_STANDARD PROPERTY STRINGS 23)
set(MK_MSVC_CXX23_STANDARD_OPTION "/std:c++23preview" CACHE STRING "MSVC C++23 standard mode flag")
set_property(CACHE MK_MSVC_CXX23_STANDARD_OPTION PROPERTY STRINGS "/std:c++23preview" "/std:c++23")
set(MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES "2" CACHE STRING "MSVC /MP process count per compiler invocation")

if(NOT MK_CXX_STANDARD STREQUAL "23")
    message(FATAL_ERROR "MK_CXX_STANDARD must be 23")
endif()

set(MK_DESKTOP_RUNTIME_ENABLED ${MK_ENABLE_DESKTOP_RUNTIME})

if(MSVC)
    if(NOT MK_MSVC_CXX23_STANDARD_OPTION MATCHES "^/std:c\\+\\+23(preview)?$")
        message(FATAL_ERROR "MK_MSVC_CXX23_STANDARD_OPTION must be /std:c++23preview or /std:c++23")
    endif()
    if(NOT MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES MATCHES "^[1-9][0-9]*$")
        message(FATAL_ERROR "MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES must be an integer between 1 and 65536")
    endif()
    if(MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES GREATER 65536)
        message(FATAL_ERROR "MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES must be an integer between 1 and 65536")
    endif()
    set(CMAKE_CXX23_STANDARD_COMPILE_OPTION "${MK_MSVC_CXX23_STANDARD_OPTION}")
    set(CMAKE_CXX23_EXTENSION_COMPILE_OPTION "${MK_MSVC_CXX23_STANDARD_OPTION}")
    set(MK_MSVC_MULTIPROCESSOR_COMPILE_OPTION "/MP${MK_MSVC_MULTIPROCESSOR_COMPILE_PROCESSES}")
endif()

if(MK_ENABLE_CXX_MODULE_SCANNING)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
else()
    # Explicit OFF keeps configure logs truthful when presets or cache disable scanning (see docs/building.md).
    set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
endif()

set(MK_IMPORT_STD_AVAILABLE OFF)
if(MK_ENABLE_IMPORT_STD)
    if(23 IN_LIST CMAKE_CXX_COMPILER_IMPORT_STD)
        set(CMAKE_CXX_MODULE_STD ON)
        set(MK_IMPORT_STD_AVAILABLE ON)
    endif()
endif()

# Configure-time summary for C++ module scanning and CMake-managed import std (keep in sync with docs/building.md).
message(STATUS "Mirakanai CXX modules: MK_ENABLE_CXX_MODULE_SCANNING=${MK_ENABLE_CXX_MODULE_SCANNING} (CMAKE_CXX_SCAN_FOR_MODULES=${CMAKE_CXX_SCAN_FOR_MODULES})")
if(MK_ENABLE_IMPORT_STD)
    message(STATUS "Mirakanai import std: MK_ENABLE_IMPORT_STD=ON; CMAKE_CXX_COMPILER_IMPORT_STD='${CMAKE_CXX_COMPILER_IMPORT_STD}'")
    if(MK_IMPORT_STD_AVAILABLE)
        message(STATUS "Mirakanai import std: enabled for this toolchain (CXX_MODULE_STD on targets via MK_apply_common_target_options).")
    else()
        message(STATUS "Mirakanai import std: compiler does not report C++23 standard-library modules — use headers and FILE_SET CXX_MODULES; gate import std in sources.")
    endif()
else()
    message(STATUS "Mirakanai import std: MK_ENABLE_IMPORT_STD=OFF (targets will not set CXX_MODULE_STD).")
endif()

# Sanitizer policy: Clang/GCC use the usual ASan+UBSan pair (-fsanitize=address,undefined) per LLVM docs; MSVC uses /fsanitize=address. See docs/testing.md.
if(MK_ENABLE_SANITIZERS)
    if(MSVC)
        message(STATUS "Mirakanai sanitizers: MK_ENABLE_SANITIZERS=ON (MSVC: /fsanitize=address).")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        message(STATUS "Mirakanai sanitizers: MK_ENABLE_SANITIZERS=ON (Clang/GCC: -fsanitize=address,undefined on compile and link).")
    else()
        message(FATAL_ERROR "MK_ENABLE_SANITIZERS requires MSVC, Clang, or GNU (got ${CMAKE_CXX_COMPILER_ID})")
    endif()
else()
    message(STATUS "Mirakanai sanitizers: MK_ENABLE_SANITIZERS=OFF.")
endif()

function(MK_apply_common_target_options target_name)
    target_compile_features(${target_name} PUBLIC cxx_std_${MK_CXX_STANDARD})
    set_target_properties(${target_name} PROPERTIES
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
        CXX_SCAN_FOR_MODULES ${MK_ENABLE_CXX_MODULE_SCANNING}
    )

    if(MK_IMPORT_STD_AVAILABLE)
        set_target_properties(${target_name} PROPERTIES CXX_MODULE_STD ON)
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /EHsc ${MK_MSVC_MULTIPROCESSOR_COMPILE_OPTION} /Zf)
        set_target_properties(${target_name} PROPERTIES
            COMPILE_PDB_NAME ${target_name}
            COMPILE_PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/pdb"
        )
        get_target_property(MK_TARGET_TYPE ${target_name} TYPE)
        if(MK_TARGET_TYPE STREQUAL "EXECUTABLE" OR
            MK_TARGET_TYPE STREQUAL "SHARED_LIBRARY" OR
            MK_TARGET_TYPE STREQUAL "MODULE_LIBRARY")
            target_link_options(${target_name} PRIVATE /INCREMENTAL:NO)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    if(MK_ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
        elseif(MSVC)
            target_compile_options(${target_name} PRIVATE /fsanitize=address)
        else()
            message(FATAL_ERROR "MK_ENABLE_SANITIZERS is not configured for ${CMAKE_CXX_COMPILER_ID}")
        endif()
    endif()
endfunction()

function(MK_disable_module_scanning_for_non_module_executable target_name)
    set_target_properties(${target_name} PROPERTIES CXX_SCAN_FOR_MODULES OFF)
endfunction()

# Engine subdirectories use the mirakana_* target naming policy; forward to the shared implementation above.
macro(mirakana_apply_common_target_options target_name)
    MK_apply_common_target_options(${target_name})
endmacro()

# Engine module graph (add_subdirectory order): list order must respect target dependencies (each subdirectory creates MK_* targets consumed by later entries).
# math/core -> ai, ui, assets, animation, audio -> platform -> runtime -> physics, navigation -> tools (assets/platform/scene/runtime_scene/...) ->
# rhi -> runtime_rhi -> renderer -> runtime_host -> ui_renderer -> scene -> runtime_scene -> scene_renderer -> runtime_scene_rhi -> editor
add_subdirectory(engine/math)
add_subdirectory(engine/core)
add_subdirectory(engine/environment)
add_subdirectory(engine/ai)
add_subdirectory(engine/ui)
add_subdirectory(engine/assets)
add_subdirectory(engine/animation)
add_subdirectory(engine/audio)
add_subdirectory(engine/platform)
add_subdirectory(engine/runtime)
if(MK_ENABLE_NETWORK_ENET)
    add_subdirectory(engine/runtime/network/enet)
endif()
add_subdirectory(engine/physics)
if(MK_ENABLE_PHYSICS_JOLT)
    add_subdirectory(engine/physics/jolt)
endif()
add_subdirectory(engine/navigation)
add_subdirectory(engine/tools)
add_subdirectory(engine/rhi)
add_subdirectory(engine/runtime_rhi)
add_subdirectory(engine/renderer)
add_subdirectory(engine/runtime_host)
add_subdirectory(engine/ui_renderer)
add_subdirectory(engine/scene)
add_subdirectory(engine/runtime_scene)
add_subdirectory(engine/scene_renderer)
add_subdirectory(engine/runtime_scene_rhi)
add_subdirectory(editor)

add_executable(MK_sandbox examples/sandbox/main.cpp)
target_link_libraries(MK_sandbox PRIVATE
    MK_ai
    MK_core
    MK_math
    MK_ui
    MK_assets
    MK_runtime
    MK_runtime_scene
    MK_runtime_rhi
    MK_animation
    MK_audio
    MK_physics
    MK_navigation
)
MK_apply_common_target_options(MK_sandbox)
MK_disable_module_scanning_for_non_module_executable(MK_sandbox)

function(MK_add_game target_name)
    cmake_parse_arguments(MK_GAME "" "" "SOURCES" ${ARGN})
    if(NOT MK_GAME_SOURCES)
        message(FATAL_ERROR "MK_add_game requires SOURCES")
    endif()

    add_executable(${target_name} ${MK_GAME_SOURCES})
    target_link_libraries(${target_name} PRIVATE
        MK_core
        MK_ai
        MK_math
        MK_assets
        MK_runtime
        MK_runtime_scene
        MK_runtime_rhi
        MK_animation
        MK_audio
        MK_physics
        MK_navigation
        MK_platform
        MK_ui
        MK_ui_renderer
        MK_renderer
        MK_scene
        MK_scene_renderer
    )
    MK_apply_common_target_options(${target_name})
    MK_disable_module_scanning_for_non_module_executable(${target_name})

    if(BUILD_TESTING)
        add_test(NAME ${target_name} COMMAND ${target_name})
    endif()
endfunction()

add_subdirectory(games)

if(BUILD_TESTING)
    add_executable(MK_core_tests
        tests/unit/core_tests.cpp
    )
    target_link_libraries(MK_core_tests PRIVATE
        MK_core
        MK_math
        MK_assets
        MK_runtime
        MK_animation
        MK_audio
        MK_physics
        MK_platform
        MK_ui
        MK_ui_renderer
        MK_renderer
        MK_scene
        MK_scene_renderer
    )
    target_include_directories(MK_core_tests PRIVATE tests)
    MK_apply_common_target_options(MK_core_tests)
    add_test(NAME MK_core_tests COMMAND MK_core_tests)

    add_executable(MK_runtime_tests
        tests/unit/runtime_tests.cpp
    )
    target_link_libraries(MK_runtime_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_tests)
    add_test(NAME MK_runtime_tests COMMAND MK_runtime_tests)

    add_executable(MK_native_desktop_contract_public_api_compile
        tests/unit/native_desktop_contract_public_api_compile.cpp
    )
    target_link_libraries(MK_native_desktop_contract_public_api_compile PRIVATE
        MK_audio
        MK_editor_core
        MK_platform
        MK_renderer
        MK_runtime_host
        MK_runtime_rhi
        MK_ui
    )
    target_include_directories(MK_native_desktop_contract_public_api_compile PRIVATE
        tests
    )
    MK_apply_common_target_options(MK_native_desktop_contract_public_api_compile)
    add_test(NAME MK_native_desktop_contract_public_api_compile COMMAND MK_native_desktop_contract_public_api_compile)

    add_executable(MK_runtime_quest_dialogue_tests
        tests/unit/runtime_quest_dialogue_tests.cpp
    )
    target_link_libraries(MK_runtime_quest_dialogue_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_quest_dialogue_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_quest_dialogue_tests)
    add_test(NAME MK_runtime_quest_dialogue_tests COMMAND MK_runtime_quest_dialogue_tests)

    add_executable(MK_runtime_inventory_items_tests
        tests/unit/runtime_inventory_items_tests.cpp
    )
    target_link_libraries(MK_runtime_inventory_items_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_inventory_items_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_inventory_items_tests)
    add_test(NAME MK_runtime_inventory_items_tests COMMAND MK_runtime_inventory_items_tests)

    add_executable(MK_runtime_gameplay_interaction_tests
        tests/unit/runtime_gameplay_interaction_tests.cpp
    )
    target_link_libraries(MK_runtime_gameplay_interaction_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_gameplay_interaction_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_gameplay_interaction_tests)
    add_test(NAME MK_runtime_gameplay_interaction_tests COMMAND MK_runtime_gameplay_interaction_tests)

    add_executable(MK_runtime_sprite_collision_hitbox_tests
        tests/unit/runtime_sprite_collision_hitbox_tests.cpp
    )
    target_link_libraries(MK_runtime_sprite_collision_hitbox_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_sprite_collision_hitbox_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_sprite_collision_hitbox_tests)
    add_test(NAME MK_runtime_sprite_collision_hitbox_tests COMMAND MK_runtime_sprite_collision_hitbox_tests)

    add_executable(MK_runtime_sprite_effect_particles_tests
        tests/unit/runtime_sprite_effect_particles_tests.cpp
    )
    target_link_libraries(MK_runtime_sprite_effect_particles_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_sprite_effect_particles_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_sprite_effect_particles_tests)
    add_test(NAME MK_runtime_sprite_effect_particles_tests COMMAND MK_runtime_sprite_effect_particles_tests)

    add_executable(MK_runtime_procedural_generation_tests
        tests/unit/runtime_procedural_generation_tests.cpp
    )
    target_link_libraries(MK_runtime_procedural_generation_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_procedural_generation_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_procedural_generation_tests)
    add_test(NAME MK_runtime_procedural_generation_tests COMMAND MK_runtime_procedural_generation_tests)

    add_executable(MK_runtime_world_region_streaming_tests
        tests/unit/runtime_world_region_streaming_tests.cpp
    )
    target_link_libraries(MK_runtime_world_region_streaming_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_world_region_streaming_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_world_region_streaming_tests)
    add_test(NAME MK_runtime_world_region_streaming_tests COMMAND MK_runtime_world_region_streaming_tests)

    add_executable(MK_runtime_entity_scale_culling_tests
        tests/unit/runtime_entity_scale_culling_tests.cpp
    )
    target_link_libraries(MK_runtime_entity_scale_culling_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_entity_scale_culling_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_entity_scale_culling_tests)
    add_test(NAME MK_runtime_entity_scale_culling_tests COMMAND MK_runtime_entity_scale_culling_tests)

    add_executable(MK_runtime_scripting_sandbox_tests
        tests/unit/runtime_scripting_sandbox_tests.cpp
    )
    target_link_libraries(MK_runtime_scripting_sandbox_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_scripting_sandbox_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_scripting_sandbox_tests)
    add_test(NAME MK_runtime_scripting_sandbox_tests COMMAND MK_runtime_scripting_sandbox_tests)

    add_executable(MK_physics_native_adapter_tests
        tests/unit/physics_native_adapter_tests.cpp
    )
    target_link_libraries(MK_physics_native_adapter_tests PRIVATE MK_physics)
    target_include_directories(MK_physics_native_adapter_tests PRIVATE tests)
    MK_apply_common_target_options(MK_physics_native_adapter_tests)
    add_test(NAME MK_physics_native_adapter_tests COMMAND MK_physics_native_adapter_tests)

    add_executable(MK_physics_navigation_production_breadth_tests
        tests/unit/physics_navigation_production_breadth_tests.cpp
    )
    target_link_libraries(MK_physics_navigation_production_breadth_tests PRIVATE MK_physics MK_navigation)
    target_include_directories(MK_physics_navigation_production_breadth_tests PRIVATE tests)
    MK_apply_common_target_options(MK_physics_navigation_production_breadth_tests)
    add_test(NAME MK_physics_navigation_production_breadth_tests
        COMMAND MK_physics_navigation_production_breadth_tests)

    if(TARGET MK_physics_jolt)
        add_executable(MK_physics_jolt_tests
            tests/unit/physics_jolt_tests.cpp
        )
        target_link_libraries(MK_physics_jolt_tests PRIVATE MK_physics_jolt)
        target_include_directories(MK_physics_jolt_tests PRIVATE tests)
        MK_apply_common_target_options(MK_physics_jolt_tests)
        add_test(NAME MK_physics_jolt_tests COMMAND MK_physics_jolt_tests)
    endif()

    add_executable(MK_runtime_networking_foundation_tests
        tests/unit/runtime_networking_foundation_tests.cpp
    )
    target_link_libraries(MK_runtime_networking_foundation_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_networking_foundation_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_networking_foundation_tests)
    add_test(NAME MK_runtime_networking_foundation_tests COMMAND MK_runtime_networking_foundation_tests)

    add_executable(MK_runtime_network_transport_adapter_tests
        tests/unit/runtime_network_transport_adapter_tests.cpp
    )
    target_link_libraries(MK_runtime_network_transport_adapter_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_network_transport_adapter_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_network_transport_adapter_tests)
    add_test(NAME MK_runtime_network_transport_adapter_tests COMMAND MK_runtime_network_transport_adapter_tests)

    add_executable(MK_runtime_network_production_security_tests
        tests/unit/runtime_network_production_security_tests.cpp
    )
    target_link_libraries(MK_runtime_network_production_security_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_network_production_security_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_network_production_security_tests)
    add_test(NAME MK_runtime_network_production_security_tests COMMAND MK_runtime_network_production_security_tests)

    add_executable(MK_runtime_production_network_replication_tests
        tests/unit/runtime_production_network_replication_tests.cpp
    )
    target_link_libraries(MK_runtime_production_network_replication_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_production_network_replication_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_production_network_replication_tests)
    add_test(NAME MK_runtime_production_network_replication_tests COMMAND MK_runtime_production_network_replication_tests)

    if(TARGET MK_runtime_network_enet)
        add_executable(MK_runtime_network_enet_tests
            tests/unit/runtime_network_enet_tests.cpp
        )
        target_link_libraries(MK_runtime_network_enet_tests PRIVATE MK_runtime_network_enet)
        target_include_directories(MK_runtime_network_enet_tests PRIVATE tests)
        MK_apply_common_target_options(MK_runtime_network_enet_tests)
        add_test(NAME MK_runtime_network_enet_tests COMMAND MK_runtime_network_enet_tests)
    endif()

    add_executable(MK_runtime_simulation_orchestration_tests
        tests/unit/runtime_simulation_orchestration_tests.cpp
    )
    target_link_libraries(MK_runtime_simulation_orchestration_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_simulation_orchestration_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_simulation_orchestration_tests)
    add_test(NAME MK_runtime_simulation_orchestration_tests COMMAND MK_runtime_simulation_orchestration_tests)

    add_executable(MK_runtime_gameplay_scheduler_tests
        tests/unit/runtime_gameplay_scheduler_tests.cpp
    )
    target_link_libraries(MK_runtime_gameplay_scheduler_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_gameplay_scheduler_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_gameplay_scheduler_tests)
    add_test(NAME MK_runtime_gameplay_scheduler_tests COMMAND MK_runtime_gameplay_scheduler_tests)

    add_executable(MK_runtime_world_entity_model_tests
        tests/unit/runtime_world_entity_model_tests.cpp
    )
    target_link_libraries(MK_runtime_world_entity_model_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_world_entity_model_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_world_entity_model_tests)
    add_test(NAME MK_runtime_world_entity_model_tests COMMAND MK_runtime_world_entity_model_tests)

    add_executable(MK_runtime_genre_rpg_systems_tests
        tests/unit/runtime_genre_rpg_systems_tests.cpp
    )
    target_link_libraries(MK_runtime_genre_rpg_systems_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_genre_rpg_systems_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_genre_rpg_systems_tests)
    add_test(NAME MK_runtime_genre_rpg_systems_tests COMMAND MK_runtime_genre_rpg_systems_tests)

    add_executable(MK_runtime_genre_sandbox_world_tests
        tests/unit/runtime_genre_sandbox_world_tests.cpp
    )
    target_link_libraries(MK_runtime_genre_sandbox_world_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_genre_sandbox_world_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_genre_sandbox_world_tests)
    add_test(NAME MK_runtime_genre_sandbox_world_tests COMMAND MK_runtime_genre_sandbox_world_tests)

    add_executable(MK_runtime_sandbox_world_runtime_tests
        tests/unit/runtime_sandbox_world_runtime_tests.cpp
    )
    target_link_libraries(MK_runtime_sandbox_world_runtime_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_sandbox_world_runtime_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_sandbox_world_runtime_tests)
    add_test(NAME MK_runtime_sandbox_world_runtime_tests COMMAND MK_runtime_sandbox_world_runtime_tests)

    add_executable(MK_runtime_sandbox_world_persistence_tests
        tests/unit/runtime_sandbox_world_persistence_tests.cpp
    )
    target_link_libraries(MK_runtime_sandbox_world_persistence_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_sandbox_world_persistence_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_sandbox_world_persistence_tests)
    add_test(NAME MK_runtime_sandbox_world_persistence_tests COMMAND MK_runtime_sandbox_world_persistence_tests)

    add_executable(MK_runtime_sandbox_world_streaming_tests
        tests/unit/runtime_sandbox_world_streaming_tests.cpp
    )
    target_link_libraries(MK_runtime_sandbox_world_streaming_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_sandbox_world_streaming_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_sandbox_world_streaming_tests)
    add_test(NAME MK_runtime_sandbox_world_streaming_tests COMMAND MK_runtime_sandbox_world_streaming_tests)

    add_executable(MK_runtime_genre_simulation_management_tests
        tests/unit/runtime_genre_simulation_management_tests.cpp
    )
    target_link_libraries(MK_runtime_genre_simulation_management_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_genre_simulation_management_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_genre_simulation_management_tests)
    add_test(NAME MK_runtime_genre_simulation_management_tests COMMAND MK_runtime_genre_simulation_management_tests)

    add_executable(MK_runtime_scene_tests
        tests/unit/runtime_scene_tests.cpp
    )
    target_link_libraries(MK_runtime_scene_tests PRIVATE MK_runtime_scene)
    target_include_directories(MK_runtime_scene_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_scene_tests)
    add_test(NAME MK_runtime_scene_tests COMMAND MK_runtime_scene_tests)

    add_executable(MK_scene_schema_v2_tests
        tests/unit/scene_schema_v2_tests.cpp
    )
    target_link_libraries(MK_scene_schema_v2_tests PRIVATE MK_scene)
    target_include_directories(MK_scene_schema_v2_tests PRIVATE tests)
    MK_apply_common_target_options(MK_scene_schema_v2_tests)
    add_test(NAME MK_scene_schema_v2_tests COMMAND MK_scene_schema_v2_tests)

    add_executable(MK_playable_2d_tests
        tests/unit/playable_2d_tests.cpp
    )
    target_link_libraries(MK_playable_2d_tests PRIVATE MK_scene MK_scene_renderer MK_ui_renderer MK_audio MK_runtime)
    target_include_directories(MK_playable_2d_tests PRIVATE tests)
    MK_apply_common_target_options(MK_playable_2d_tests)
    add_test(NAME MK_playable_2d_tests COMMAND MK_playable_2d_tests)

    add_executable(MK_audio_tests
        tests/unit/audio_production_tests.cpp
    )
    target_link_libraries(MK_audio_tests PRIVATE MK_audio)
    target_include_directories(MK_audio_tests PRIVATE tests)
    MK_apply_common_target_options(MK_audio_tests)
    add_test(NAME MK_audio_tests COMMAND MK_audio_tests)

    add_executable(MK_asset_identity_runtime_resource_tests
        tests/unit/asset_identity_runtime_resource_tests.cpp
    )
    target_link_libraries(MK_asset_identity_runtime_resource_tests PRIVATE MK_assets MK_runtime MK_platform)
    target_include_directories(MK_asset_identity_runtime_resource_tests PRIVATE tests)
    MK_apply_common_target_options(MK_asset_identity_runtime_resource_tests)
    add_test(NAME MK_asset_identity_runtime_resource_tests COMMAND MK_asset_identity_runtime_resource_tests)

    add_executable(MK_runtime_package_index_discovery_tests
        tests/unit/runtime_package_index_discovery_tests.cpp
    )
    target_link_libraries(MK_runtime_package_index_discovery_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_index_discovery_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_index_discovery_tests)
    add_test(NAME MK_runtime_package_index_discovery_tests COMMAND MK_runtime_package_index_discovery_tests)

    add_executable(MK_runtime_addressable_content_streaming_tests
        tests/unit/runtime_addressable_content_streaming_tests.cpp
    )
    target_link_libraries(MK_runtime_addressable_content_streaming_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_addressable_content_streaming_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_addressable_content_streaming_tests)
    add_test(NAME MK_runtime_addressable_content_streaming_tests COMMAND MK_runtime_addressable_content_streaming_tests)

    add_executable(MK_runtime_package_candidate_load_tests
        tests/unit/runtime_package_candidate_load_tests.cpp
    )
    target_link_libraries(MK_runtime_package_candidate_load_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_candidate_load_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_candidate_load_tests)
    add_test(NAME MK_runtime_package_candidate_load_tests COMMAND MK_runtime_package_candidate_load_tests)

    add_executable(MK_runtime_package_candidate_resident_mount_tests
        tests/unit/runtime_package_candidate_resident_mount_tests.cpp
    )
    target_link_libraries(MK_runtime_package_candidate_resident_mount_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_candidate_resident_mount_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_candidate_resident_mount_tests)
    add_test(NAME MK_runtime_package_candidate_resident_mount_tests COMMAND MK_runtime_package_candidate_resident_mount_tests)

    add_executable(MK_rt_pkg_cand_res_mnt_review_evict_tests
        tests/unit/runtime_package_candidate_resident_mount_reviewed_evictions_tests.cpp
    )
    target_link_libraries(MK_rt_pkg_cand_res_mnt_review_evict_tests PRIVATE MK_runtime)
    target_include_directories(MK_rt_pkg_cand_res_mnt_review_evict_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rt_pkg_cand_res_mnt_review_evict_tests)
    add_test(NAME MK_runtime_package_candidate_resident_mount_reviewed_evictions_tests COMMAND MK_rt_pkg_cand_res_mnt_review_evict_tests)

    add_executable(MK_runtime_package_candidate_resident_replace_tests
        tests/unit/runtime_package_candidate_resident_replace_tests.cpp
    )
    target_link_libraries(MK_runtime_package_candidate_resident_replace_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_candidate_resident_replace_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_candidate_resident_replace_tests)
    add_test(NAME MK_runtime_package_candidate_resident_replace_tests COMMAND MK_runtime_package_candidate_resident_replace_tests)

    add_executable(MK_rt_pkg_cand_res_repl_review_evict_tests
        tests/unit/runtime_package_candidate_resident_replace_reviewed_evictions_tests.cpp
    )
    target_link_libraries(MK_rt_pkg_cand_res_repl_review_evict_tests PRIVATE MK_runtime)
    target_include_directories(MK_rt_pkg_cand_res_repl_review_evict_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rt_pkg_cand_res_repl_review_evict_tests)
    add_test(NAME MK_runtime_package_candidate_resident_replace_reviewed_evictions_tests COMMAND MK_rt_pkg_cand_res_repl_review_evict_tests)

    add_executable(MK_runtime_package_discovery_resident_commit_tests
        tests/unit/runtime_package_discovery_resident_commit_tests.cpp
    )
    target_link_libraries(MK_runtime_package_discovery_resident_commit_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_discovery_resident_commit_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_discovery_resident_commit_tests)
    add_test(NAME MK_runtime_package_discovery_resident_commit_tests COMMAND MK_runtime_package_discovery_resident_commit_tests)

    add_executable(MK_rt_pkg_disc_res_mnt_review_evict_tests
        tests/unit/runtime_package_discovery_resident_mount_reviewed_evictions_tests.cpp
    )
    target_link_libraries(MK_rt_pkg_disc_res_mnt_review_evict_tests PRIVATE MK_runtime)
    target_include_directories(MK_rt_pkg_disc_res_mnt_review_evict_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rt_pkg_disc_res_mnt_review_evict_tests)
    add_test(NAME MK_runtime_package_discovery_resident_mount_reviewed_evictions_tests COMMAND MK_rt_pkg_disc_res_mnt_review_evict_tests)

    add_executable(MK_rt_pkg_disc_res_repl_review_evict_tests
        tests/unit/runtime_package_discovery_resident_replace_reviewed_evictions_tests.cpp
    )
    target_link_libraries(MK_rt_pkg_disc_res_repl_review_evict_tests PRIVATE MK_runtime)
    target_include_directories(MK_rt_pkg_disc_res_repl_review_evict_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rt_pkg_disc_res_repl_review_evict_tests)
    add_test(NAME MK_runtime_package_discovery_resident_replace_reviewed_evictions_tests COMMAND MK_rt_pkg_disc_res_repl_review_evict_tests)

    add_executable(MK_runtime_package_hot_reload_candidate_review_tests
        tests/unit/runtime_package_hot_reload_candidate_review_tests.cpp
    )
    target_link_libraries(MK_runtime_package_hot_reload_candidate_review_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_hot_reload_candidate_review_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_hot_reload_candidate_review_tests)
    add_test(NAME MK_runtime_package_hot_reload_candidate_review_tests COMMAND MK_runtime_package_hot_reload_candidate_review_tests)

    add_executable(MK_rt_pkg_hot_reload_repl_intent_review_tests
        tests/unit/runtime_package_hot_reload_replacement_intent_review_tests.cpp
    )
    target_link_libraries(MK_rt_pkg_hot_reload_repl_intent_review_tests PRIVATE MK_runtime)
    target_include_directories(MK_rt_pkg_hot_reload_repl_intent_review_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rt_pkg_hot_reload_repl_intent_review_tests)
    add_test(NAME MK_runtime_package_hot_reload_replacement_intent_review_tests COMMAND MK_rt_pkg_hot_reload_repl_intent_review_tests)

    add_executable(MK_runtime_package_hot_reload_recook_change_review_tests
        tests/unit/runtime_package_hot_reload_recook_change_review_tests.cpp
    )
    target_link_libraries(MK_runtime_package_hot_reload_recook_change_review_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_hot_reload_recook_change_review_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_hot_reload_recook_change_review_tests)
    add_test(NAME MK_runtime_package_hot_reload_recook_change_review_tests COMMAND MK_runtime_package_hot_reload_recook_change_review_tests)

    add_executable(MK_runtime_package_hot_reload_recook_replacement_tests
        tests/unit/runtime_package_hot_reload_recook_replacement_tests.cpp
    )
    target_link_libraries(MK_runtime_package_hot_reload_recook_replacement_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_hot_reload_recook_replacement_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_hot_reload_recook_replacement_tests)
    add_test(NAME MK_runtime_package_hot_reload_recook_replacement_tests COMMAND MK_runtime_package_hot_reload_recook_replacement_tests)

    add_executable(MK_runtime_resource_resident_cache_tests
        tests/unit/runtime_resource_resident_cache_tests.cpp
    )
    target_link_libraries(MK_runtime_resource_resident_cache_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_resource_resident_cache_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_resource_resident_cache_tests)
    add_test(NAME MK_runtime_resource_resident_cache_tests COMMAND MK_runtime_resource_resident_cache_tests)

    add_executable(MK_runtime_mavg_lod_residency_tests
        tests/unit/runtime_mavg_lod_residency_tests.cpp
    )
    target_link_libraries(MK_runtime_mavg_lod_residency_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_mavg_lod_residency_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_mavg_lod_residency_tests)
    add_test(NAME MK_runtime_mavg_lod_residency_tests COMMAND MK_runtime_mavg_lod_residency_tests)

    add_executable(MK_runtime_mavg_page_streaming_tests
        tests/unit/runtime_mavg_page_streaming_tests.cpp
    )
    target_link_libraries(MK_runtime_mavg_page_streaming_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_mavg_page_streaming_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_mavg_page_streaming_tests)
    add_test(NAME MK_runtime_mavg_page_streaming_tests COMMAND MK_runtime_mavg_page_streaming_tests)

    add_executable(MK_runtime_mavg_payload_page_loader_tests
        tests/unit/runtime_mavg_payload_page_loader_tests.cpp
    )
    target_link_libraries(MK_runtime_mavg_payload_page_loader_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_mavg_payload_page_loader_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_mavg_payload_page_loader_tests)
    add_test(NAME MK_runtime_mavg_payload_page_loader_tests COMMAND MK_runtime_mavg_payload_page_loader_tests)

    add_executable(MK_runtime_package_streaming_resident_mount_tests
        tests/unit/runtime_package_streaming_resident_mount_tests.cpp
    )
    target_link_libraries(MK_runtime_package_streaming_resident_mount_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_package_streaming_resident_mount_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_package_streaming_resident_mount_tests)
    add_test(NAME MK_runtime_package_streaming_resident_mount_tests COMMAND MK_runtime_package_streaming_resident_mount_tests)

    add_executable(MK_runtime_resource_resident_unmount_tests
        tests/unit/runtime_resource_resident_unmount_tests.cpp
    )
    target_link_libraries(MK_runtime_resource_resident_unmount_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_resource_resident_unmount_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_resource_resident_unmount_tests)
    add_test(NAME MK_runtime_resource_resident_unmount_tests COMMAND MK_runtime_resource_resident_unmount_tests)

    add_executable(MK_runtime_resource_resident_replace_tests
        tests/unit/runtime_resource_resident_replace_tests.cpp
    )
    target_link_libraries(MK_runtime_resource_resident_replace_tests PRIVATE MK_runtime)
    target_include_directories(MK_runtime_resource_resident_replace_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_resource_resident_replace_tests)
    add_test(NAME MK_runtime_resource_resident_replace_tests COMMAND MK_runtime_resource_resident_replace_tests)

    add_executable(MK_assets_tests
        tests/unit/assets_sprite_atlas_packing_tests.cpp
    )
    target_link_libraries(MK_assets_tests PRIVATE MK_assets)
    target_include_directories(MK_assets_tests PRIVATE tests)
    MK_apply_common_target_options(MK_assets_tests)
    add_test(NAME MK_assets_tests COMMAND MK_assets_tests)

    add_executable(MK_mavg_cluster_graph_tests
        tests/unit/mavg_cluster_graph_tests.cpp
    )
    target_link_libraries(MK_mavg_cluster_graph_tests PRIVATE MK_assets)
    target_include_directories(MK_mavg_cluster_graph_tests PRIVATE tests)
    MK_apply_common_target_options(MK_mavg_cluster_graph_tests)
    add_test(NAME MK_mavg_cluster_graph_tests COMMAND MK_mavg_cluster_graph_tests)

    add_executable(MK_asset_import_production_review_tests
        tests/unit/asset_import_production_review_tests.cpp
    )
    target_link_libraries(MK_asset_import_production_review_tests PRIVATE MK_assets)
    target_include_directories(MK_asset_import_production_review_tests PRIVATE tests)
    MK_apply_common_target_options(MK_asset_import_production_review_tests)
    add_test(NAME MK_asset_import_production_review_tests COMMAND MK_asset_import_production_review_tests)

    add_executable(MK_asset_environment_source_pipeline_tests
        tests/unit/asset_environment_source_pipeline_tests.cpp
    )
    target_link_libraries(MK_asset_environment_source_pipeline_tests PRIVATE MK_assets)
    target_include_directories(MK_asset_environment_source_pipeline_tests PRIVATE tests)
    MK_apply_common_target_options(MK_asset_environment_source_pipeline_tests)
    add_test(NAME MK_asset_environment_source_pipeline_tests COMMAND MK_asset_environment_source_pipeline_tests)

    add_executable(MK_environment_texture_pipeline_v2_tests
        tests/unit/environment_texture_pipeline_v2_tests.cpp
    )
    target_link_libraries(MK_environment_texture_pipeline_v2_tests PRIVATE MK_tools)
    target_include_directories(MK_environment_texture_pipeline_v2_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_texture_pipeline_v2_tests)
    add_test(NAME MK_environment_texture_pipeline_v2_tests COMMAND MK_environment_texture_pipeline_v2_tests)

    add_executable(MK_runtime_rhi_tests
        tests/unit/runtime_rhi_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_tests)
    add_test(NAME MK_runtime_rhi_tests COMMAND MK_runtime_rhi_tests)

    add_executable(MK_runtime_rhi_mavg_conventional_upload_tests
        tests/unit/runtime_rhi_mavg_conventional_upload_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_conventional_upload_tests PRIVATE MK_runtime_rhi MK_scene_renderer)
    target_include_directories(MK_runtime_rhi_mavg_conventional_upload_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_conventional_upload_tests)
    add_test(NAME MK_runtime_rhi_mavg_conventional_upload_tests COMMAND MK_runtime_rhi_mavg_conventional_upload_tests)

    add_executable(MK_runtime_rhi_mavg_gpu_memory_residency_tests
        tests/unit/runtime_rhi_mavg_gpu_memory_residency_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_gpu_memory_residency_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_mavg_gpu_memory_residency_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_gpu_memory_residency_tests)
    add_test(NAME MK_runtime_rhi_mavg_gpu_memory_residency_tests COMMAND MK_runtime_rhi_mavg_gpu_memory_residency_tests)

    add_executable(MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests
        tests/unit/runtime_rhi_mavg_cluster_streaming_residency_closeout_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests)
    add_test(NAME MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests COMMAND MK_runtime_rhi_mavg_cluster_streaming_residency_closeout_tests)

    add_executable(MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests
        tests/unit/runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests)
    add_test(NAME MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests COMMAND MK_runtime_rhi_mavg_cluster_streaming_safe_point_adoption_tests)

    add_executable(MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests
        tests/unit/runtime_rhi_mavg_streamed_cluster_gpu_upload_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests)
    add_test(NAME MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests COMMAND MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests)

    add_executable(MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests
        tests/unit/runtime_rhi_mavg_streaming_upload_overlap_evidence_tests.cpp
    )
    target_link_libraries(MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests)
    add_test(NAME MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests COMMAND MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests)

    add_executable(MK_env_platform_v2_tests
        tests/unit/runtime_rhi_environment_platform_evidence_v2_tests.cpp
    )
    target_link_libraries(MK_env_platform_v2_tests PRIVATE MK_runtime_rhi)
    target_include_directories(MK_env_platform_v2_tests PRIVATE tests)
    MK_apply_common_target_options(MK_env_platform_v2_tests)
    add_test(NAME MK_runtime_rhi_environment_platform_evidence_v2_tests COMMAND MK_env_platform_v2_tests)

    add_executable(MK_runtime_scene_rhi_tests
        tests/unit/runtime_scene_rhi_tests.cpp
    )
    target_link_libraries(MK_runtime_scene_rhi_tests PRIVATE MK_runtime_scene_rhi MK_scene_renderer MK_runtime_rhi)
    target_include_directories(MK_runtime_scene_rhi_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_scene_rhi_tests)
    add_test(NAME MK_runtime_scene_rhi_tests COMMAND MK_runtime_scene_rhi_tests)

    add_executable(MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests
        tests/unit/runtime_scene_rhi_mavg_streamed_backend_draw_tests.cpp
    )
    target_link_libraries(MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests
        PRIVATE MK_runtime_scene_rhi MK_scene_renderer MK_runtime_rhi
    )
    target_include_directories(MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests)
    add_test(NAME MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests
        COMMAND MK_runtime_scene_rhi_mavg_streamed_backend_draw_tests
    )

    add_executable(MK_navigation_tests
        tests/unit/navigation_tests.cpp
    )
    target_link_libraries(MK_navigation_tests PRIVATE MK_navigation)
    target_include_directories(MK_navigation_tests PRIVATE tests)
    MK_apply_common_target_options(MK_navigation_tests)
    add_test(NAME MK_navigation_tests COMMAND MK_navigation_tests)

    add_executable(MK_ai_tests
        tests/unit/ai_tests.cpp
    )
    target_link_libraries(MK_ai_tests PRIVATE MK_ai)
    target_include_directories(MK_ai_tests PRIVATE tests)
    MK_apply_common_target_options(MK_ai_tests)
    add_test(NAME MK_ai_tests COMMAND MK_ai_tests)

    add_executable(MK_editor_core_tests
        tests/unit/editor_core_tests.cpp
    )
    target_link_libraries(MK_editor_core_tests PRIVATE MK_editor_core)
    target_include_directories(MK_editor_core_tests PRIVATE tests)
    MK_apply_common_target_options(MK_editor_core_tests)
    add_test(NAME MK_editor_core_tests COMMAND MK_editor_core_tests)

    add_executable(MK_editor_environment_tests
        tests/unit/editor_environment_tests.cpp
    )
    target_link_libraries(MK_editor_environment_tests PRIVATE MK_editor_shell_common MK_environment)
    target_include_directories(MK_editor_environment_tests PRIVATE tests editor/src)
    MK_apply_common_target_options(MK_editor_environment_tests)
    add_test(NAME MK_editor_environment_tests COMMAND MK_editor_environment_tests)

    add_executable(MK_editor_native_shell_tests
        tests/unit/editor_native_shell_tests.cpp
    )
    target_link_libraries(MK_editor_native_shell_tests PRIVATE MK_editor_shell_common MK_rhi)
    target_include_directories(MK_editor_native_shell_tests PRIVATE tests editor/src)
    MK_apply_common_target_options(MK_editor_native_shell_tests)
    add_test(NAME MK_editor_native_shell_tests COMMAND MK_editor_native_shell_tests)

    if(TARGET MK_editor)
        add_test(NAME MK_editor_smoke COMMAND MK_editor --smoke-frames 2 --smoke-resize --no-user-config)
        set_tests_properties(MK_editor_smoke PROPERTIES
            RUN_SERIAL TRUE
            PASS_REGULAR_EXPRESSION "editor_shell_status=ready;editor_shell_ui=first_party;editor_shell_backend=d3d12;editor_shell_imgui=0;editor_shell_panels=11;editor_shell_docking_status=single_window_ready;editor_shell_dock_tab_headers=11;editor_shell_dock_split_gutters=3;editor_shell_dock_active_panels=4;editor_shell_dock_focusable_controls=11;editor_shell_sdl3=0;editor_shell_file_dialog_service=win32;editor_shell_clipboard_service=win32;editor_shell_reviewed_process_runner=win32;editor_shell_viewport_status=d3d12_texture_ready;editor_shell_viewport_visible_texture_composites=[1-9][0-9]*;editor_shell_viewport_native_handles_exposed=0;editor_shell_material_preview_status=d3d12_texture_ready;editor_shell_material_preview_visible_texture_composites=[1-9][0-9]*;editor_shell_material_preview_native_handles_exposed=0;editor_shell_text_atlas_handoff_status=glyphs_ready_atlas_handoff_host_gated;editor_shell_text_font_adapter_invoked=1;editor_shell_text_font_glyphs_ready=1;editor_shell_text_font_fallback_used=0;editor_shell_text_atlas_handoff_host_gated_rows=1;editor_shell_text_atlas_handoff_unsupported_rows=1;editor_shell_text_font_native_handles_exposed=0;editor_shell_text_input_service=win32_tsf;editor_shell_ime_service=win32_tsf;editor_shell_ime_status=win32_tsf_selected;editor_shell_ime_text_input_session_rows=0;editor_shell_ime_composition_rows=0;editor_shell_ime_committed_text_rows=0;editor_shell_ime_caret_rect_rows=1;editor_shell_ime_surrounding_text_rows=1;editor_shell_ime_candidate_ui_host_owned=1;editor_shell_ime_native_handles_exposed=0;editor_shell_accessibility_service=win32_uia;editor_shell_accessibility_status=uia_provider_ready;editor_shell_accessibility_nodes=[1-9][0-9]*;editor_shell_accessibility_role_rows=[1-9][0-9]*;editor_shell_accessibility_name_rows=[1-9][0-9]*;editor_shell_accessibility_state_rows=[1-9][0-9]*;editor_shell_accessibility_focus_rows=1;editor_shell_accessibility_action_rows=[1-9][0-9]*;editor_shell_accessibility_relationship_rows=[1-9][0-9]*;editor_shell_accessibility_tree_navigation_rows=[1-9][0-9]*;editor_shell_accessibility_diagnostics=0;editor_shell_accessibility_missing_name_diagnostics=0;editor_shell_accessibility_missing_role_diagnostics=0;editor_shell_accessibility_invalid_bounds_diagnostics=0;editor_shell_accessibility_hidden_nodes=0;editor_shell_accessibility_unsupported_pattern_diagnostics=0;editor_shell_accessibility_native_handles_exposed=0;editor_shell_renderer_boxes_submitted=[1-9][0-9]*;editor_shell_renderer_text_runs_available=[1-9][0-9]*"
        )
    endif()

    add_executable(MK_rhi_tests
        tests/unit/rhi_tests.cpp
    )
    target_link_libraries(MK_rhi_tests PRIVATE MK_rhi)
    target_include_directories(MK_rhi_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rhi_tests)
    add_test(NAME MK_rhi_tests COMMAND MK_rhi_tests)

    add_executable(MK_rhi_resource_lifetime_tests
        tests/unit/rhi_resource_lifetime_tests.cpp
    )
    target_link_libraries(MK_rhi_resource_lifetime_tests PRIVATE MK_rhi)
    target_include_directories(MK_rhi_resource_lifetime_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rhi_resource_lifetime_tests)
    add_test(NAME MK_rhi_resource_lifetime_tests COMMAND MK_rhi_resource_lifetime_tests)

    add_executable(MK_rhi_upload_staging_tests
        tests/unit/rhi_upload_staging_tests.cpp
    )
    target_link_libraries(MK_rhi_upload_staging_tests PRIVATE MK_rhi)
    target_include_directories(MK_rhi_upload_staging_tests PRIVATE tests)
    MK_apply_common_target_options(MK_rhi_upload_staging_tests)
    add_test(NAME MK_rhi_upload_staging_tests COMMAND MK_rhi_upload_staging_tests)

    add_executable(MK_backend_scaffold_tests
        tests/unit/backend_scaffold_tests.cpp
    )
    target_link_libraries(MK_backend_scaffold_tests
        PRIVATE
            MK_rhi_vulkan
            MK_rhi_metal
            MK_runtime_rhi
            MK_runtime_scene_rhi
            MK_scene
            MK_scene_renderer
            MK_renderer
    )
    target_include_directories(MK_backend_scaffold_tests PRIVATE tests)
    MK_apply_common_target_options(MK_backend_scaffold_tests)
    add_test(NAME MK_backend_scaffold_tests COMMAND MK_backend_scaffold_tests)
    if(APPLE AND TARGET MK_metal_environment_evidence_metallib)
        add_dependencies(MK_backend_scaffold_tests MK_metal_environment_evidence_metallib)
        get_property(MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB GLOBAL PROPERTY MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB)
        set_tests_properties(MK_backend_scaffold_tests PROPERTIES
            ENVIRONMENT "MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB=${MK_METAL_ENVIRONMENT_EVIDENCE_METALLIB}"
        )
    endif()

    add_executable(MK_renderer_tests
        tests/unit/renderer_rhi_tests.cpp
    )
    target_link_libraries(MK_renderer_tests PRIVATE MK_renderer MK_runtime_rhi)
    target_include_directories(MK_renderer_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_tests)
    add_test(NAME MK_renderer_tests COMMAND MK_renderer_tests)

    add_executable(MK_mavg_lod_selection_tests
        tests/unit/mavg_lod_selection_tests.cpp
    )
    target_link_libraries(MK_mavg_lod_selection_tests PRIVATE MK_renderer)
    target_include_directories(MK_mavg_lod_selection_tests PRIVATE tests)
    MK_apply_common_target_options(MK_mavg_lod_selection_tests)
    add_test(NAME MK_mavg_lod_selection_tests COMMAND MK_mavg_lod_selection_tests)

    add_executable(MK_mavg_gpu_culling_tests
        tests/unit/mavg_gpu_culling_tests.cpp
    )
    target_link_libraries(MK_mavg_gpu_culling_tests PRIVATE MK_renderer)
    target_include_directories(MK_mavg_gpu_culling_tests PRIVATE tests)
    MK_apply_common_target_options(MK_mavg_gpu_culling_tests)
    add_test(NAME MK_mavg_gpu_culling_tests COMMAND MK_mavg_gpu_culling_tests)

    add_executable(MK_renderer_environment_policy_tests
        tests/unit/renderer_environment_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_environment_policy_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_environment_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_environment_policy_tests)
    add_test(NAME MK_renderer_environment_policy_tests COMMAND MK_renderer_environment_policy_tests)

    add_executable(MK_env_backend_parity_tests
        tests/unit/renderer_environment_parity_tests.cpp
    )
    target_link_libraries(MK_env_backend_parity_tests PRIVATE MK_renderer)
    target_include_directories(MK_env_backend_parity_tests PRIVATE tests)
    MK_apply_common_target_options(MK_env_backend_parity_tests)
    add_test(NAME MK_env_backend_parity_tests COMMAND MK_env_backend_parity_tests)

    add_executable(MK_env_backend_parity_v2_tests
        tests/unit/renderer_environment_backend_parity_v2_tests.cpp
    )
    target_link_libraries(MK_env_backend_parity_v2_tests PRIVATE MK_renderer)
    target_include_directories(MK_env_backend_parity_v2_tests PRIVATE tests)
    MK_apply_common_target_options(MK_env_backend_parity_v2_tests)
    add_test(NAME MK_env_backend_parity_v2_tests COMMAND MK_env_backend_parity_v2_tests)

    add_executable(MK_env_performance_tests
        tests/unit/renderer_environment_performance_tests.cpp
    )
    target_link_libraries(MK_env_performance_tests PRIVATE MK_renderer)
    target_include_directories(MK_env_performance_tests PRIVATE tests)
    MK_apply_common_target_options(MK_env_performance_tests)
    add_test(NAME MK_env_performance_tests COMMAND MK_env_performance_tests)

    add_executable(MK_renderer_physical_sky_policy_tests
        tests/unit/renderer_physical_sky_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_physical_sky_policy_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_physical_sky_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_physical_sky_policy_tests)
    add_test(NAME MK_renderer_physical_sky_policy_tests COMMAND MK_renderer_physical_sky_policy_tests)

    add_executable(MK_renderer_environment_lighting_policy_tests
        tests/unit/renderer_environment_lighting_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_environment_lighting_policy_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_environment_lighting_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_environment_lighting_policy_tests)
    add_test(NAME MK_renderer_environment_lighting_policy_tests COMMAND MK_renderer_environment_lighting_policy_tests)

    add_executable(MK_renderer_environment_fog_policy_tests
        tests/unit/renderer_environment_fog_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_environment_fog_policy_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_environment_fog_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_environment_fog_policy_tests)
    add_test(NAME MK_renderer_environment_fog_policy_tests COMMAND MK_renderer_environment_fog_policy_tests)

    add_executable(MK_renderer_volumetric_fog_policy_tests
        tests/unit/renderer_volumetric_fog_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_volumetric_fog_policy_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_volumetric_fog_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_volumetric_fog_policy_tests)
    add_test(NAME MK_renderer_volumetric_fog_policy_tests COMMAND MK_renderer_volumetric_fog_policy_tests)

    add_executable(MK_environment_cloud_tests
        tests/unit/environment_cloud_tests.cpp
    )
    target_link_libraries(MK_environment_cloud_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_cloud_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_cloud_tests)
    add_test(NAME MK_environment_cloud_tests COMMAND MK_environment_cloud_tests)

    add_executable(MK_environment_weather_tests
        tests/unit/environment_weather_tests.cpp
    )
    target_link_libraries(MK_environment_weather_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_weather_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_weather_tests)
    add_test(NAME MK_environment_weather_tests COMMAND MK_environment_weather_tests)

    add_executable(MK_environment_weather_simulation_tests
        tests/unit/environment_weather_simulation_tests.cpp
    )
    target_link_libraries(MK_environment_weather_simulation_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_weather_simulation_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_weather_simulation_tests)
    add_test(NAME MK_environment_weather_simulation_tests COMMAND MK_environment_weather_simulation_tests)

    add_executable(MK_environment_material_weathering_tests
        tests/unit/environment_material_weathering_tests.cpp
    )
    target_link_libraries(MK_environment_material_weathering_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_material_weathering_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_material_weathering_tests)
    add_test(NAME MK_environment_material_weathering_tests COMMAND MK_environment_material_weathering_tests)

    add_executable(MK_environment_time_of_day_tests
        tests/unit/environment_time_of_day_tests.cpp
    )
    target_link_libraries(MK_environment_time_of_day_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_time_of_day_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_time_of_day_tests)
    add_test(NAME MK_environment_time_of_day_tests COMMAND MK_environment_time_of_day_tests)

    add_executable(MK_renderer_cloud_layer_policy_tests
        tests/unit/renderer_cloud_layer_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_cloud_layer_policy_tests PRIVATE MK_renderer MK_environment)
    target_include_directories(MK_renderer_cloud_layer_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_cloud_layer_policy_tests)
    add_test(NAME MK_renderer_cloud_layer_policy_tests COMMAND MK_renderer_cloud_layer_policy_tests)

    add_executable(MK_renderer_precipitation_policy_tests
        tests/unit/renderer_precipitation_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_precipitation_policy_tests PRIVATE MK_renderer MK_environment)
    target_include_directories(MK_renderer_precipitation_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_precipitation_policy_tests)
    add_test(NAME MK_renderer_precipitation_policy_tests COMMAND MK_renderer_precipitation_policy_tests)

    add_executable(MK_renderer_material_weathering_policy_tests
        tests/unit/renderer_material_weathering_policy_tests.cpp
    )
    target_link_libraries(MK_renderer_material_weathering_policy_tests PRIVATE MK_renderer MK_environment)
    target_include_directories(MK_renderer_material_weathering_policy_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_material_weathering_policy_tests)
    add_test(NAME MK_renderer_material_weathering_policy_tests COMMAND MK_renderer_material_weathering_policy_tests)

    add_executable(MK_renderer_volumetric_cloud_tests
        tests/unit/renderer_volumetric_cloud_tests.cpp
    )
    target_link_libraries(MK_renderer_volumetric_cloud_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_volumetric_cloud_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_volumetric_cloud_tests)
    add_test(NAME MK_renderer_volumetric_cloud_tests COMMAND MK_renderer_volumetric_cloud_tests)

    add_executable(MK_renderer_tile_chunk_renderer_tests
        tests/unit/renderer_tile_chunk_renderer_tests.cpp
    )
    target_link_libraries(MK_renderer_tile_chunk_renderer_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_tile_chunk_renderer_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_tile_chunk_renderer_tests)
    add_test(NAME MK_renderer_tile_chunk_renderer_tests COMMAND MK_renderer_tile_chunk_renderer_tests)

    add_executable(MK_renderer_production_vfx_profiling_tests
        tests/unit/renderer_production_vfx_profiling_tests.cpp
    )
    target_link_libraries(MK_renderer_production_vfx_profiling_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_production_vfx_profiling_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_production_vfx_profiling_tests)
    add_test(NAME MK_renderer_production_vfx_profiling_tests COMMAND MK_renderer_production_vfx_profiling_tests)

    add_executable(MK_renderer_quality_matrix_tests
        tests/unit/renderer_quality_matrix_tests.cpp
    )
    target_link_libraries(MK_renderer_quality_matrix_tests PRIVATE MK_renderer)
    target_include_directories(MK_renderer_quality_matrix_tests PRIVATE tests)
    MK_apply_common_target_options(MK_renderer_quality_matrix_tests)
    add_test(NAME MK_renderer_quality_matrix_tests COMMAND MK_renderer_quality_matrix_tests)

    add_executable(MK_environment_tests
        tests/unit/environment_tests.cpp
    )
    target_link_libraries(MK_environment_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_tests)
    add_test(NAME MK_environment_tests COMMAND MK_environment_tests)

    add_executable(MK_environment_commercial_readiness_v2_tests
        tests/unit/environment_commercial_readiness_v2_tests.cpp
    )
    target_link_libraries(MK_environment_commercial_readiness_v2_tests PRIVATE MK_environment)
    target_include_directories(MK_environment_commercial_readiness_v2_tests PRIVATE tests)
    MK_apply_common_target_options(MK_environment_commercial_readiness_v2_tests)
    add_test(NAME MK_environment_commercial_readiness_v2_tests COMMAND MK_environment_commercial_readiness_v2_tests)

    add_executable(MK_runtime_host_tests
        tests/unit/runtime_host_tests.cpp
    )
    target_link_libraries(MK_runtime_host_tests PRIVATE MK_runtime_host MK_core MK_platform MK_renderer)
    target_include_directories(MK_runtime_host_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_host_tests)
    add_test(NAME MK_runtime_host_tests COMMAND MK_runtime_host_tests)

    add_executable(MK_scene_renderer_tests
        tests/unit/scene_renderer_tests.cpp
    )
    target_link_libraries(MK_scene_renderer_tests PRIVATE MK_scene_renderer MK_runtime_rhi)
    target_include_directories(MK_scene_renderer_tests PRIVATE tests)
    MK_apply_common_target_options(MK_scene_renderer_tests)
    add_test(NAME MK_scene_renderer_tests COMMAND MK_scene_renderer_tests)

    add_executable(MK_scene_renderer_mavg_lod_tests
        tests/unit/scene_renderer_mavg_lod_tests.cpp
    )
    target_link_libraries(MK_scene_renderer_mavg_lod_tests PRIVATE MK_scene_renderer)
    target_include_directories(MK_scene_renderer_mavg_lod_tests PRIVATE tests)
    MK_apply_common_target_options(MK_scene_renderer_mavg_lod_tests)
    add_test(NAME MK_scene_renderer_mavg_lod_tests COMMAND MK_scene_renderer_mavg_lod_tests)

    add_executable(MK_scene_environment_renderer_tests
        tests/unit/scene_environment_renderer_tests.cpp
    )
    target_link_libraries(MK_scene_environment_renderer_tests PRIVATE MK_scene_renderer)
    target_include_directories(MK_scene_environment_renderer_tests PRIVATE tests)
    MK_apply_common_target_options(MK_scene_environment_renderer_tests)
    add_test(NAME MK_scene_environment_renderer_tests COMMAND MK_scene_environment_renderer_tests)

    add_executable(MK_runtime_ui_workbench_tests
        tests/unit/runtime_ui_workbench_tests.cpp
    )
    target_link_libraries(MK_runtime_ui_workbench_tests PRIVATE MK_ui)
    target_include_directories(MK_runtime_ui_workbench_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_ui_workbench_tests)
    add_test(NAME MK_runtime_ui_workbench_tests COMMAND MK_runtime_ui_workbench_tests)

    add_executable(MK_runtime_ui_production_stack_tests
        tests/unit/runtime_ui_production_stack_tests.cpp
    )
    target_link_libraries(MK_runtime_ui_production_stack_tests PRIVATE MK_ui)
    target_include_directories(MK_runtime_ui_production_stack_tests PRIVATE tests)
    MK_apply_common_target_options(MK_runtime_ui_production_stack_tests)
    add_test(NAME MK_runtime_ui_production_stack_tests COMMAND MK_runtime_ui_production_stack_tests)

    add_executable(MK_ui_renderer_tests
        tests/unit/ui_renderer_tests.cpp
    )
    target_link_libraries(MK_ui_renderer_tests PRIVATE MK_ui_renderer)
    target_include_directories(MK_ui_renderer_tests PRIVATE tests)
    MK_apply_common_target_options(MK_ui_renderer_tests)
    add_test(NAME MK_ui_renderer_tests COMMAND MK_ui_renderer_tests)

    add_executable(MK_tools_tests
        tests/unit/tools_tests.cpp
    )
    target_link_libraries(MK_tools_tests PRIVATE MK_tools MK_runtime MK_runtime_scene)
    if(MK_ENABLE_ASSET_IMPORTERS)
        find_package(OpenEXR CONFIG REQUIRED)
        target_compile_definitions(MK_tools_tests PRIVATE MK_TESTS_HAS_ASSET_IMPORTERS=1)
        target_link_libraries(MK_tools_tests PRIVATE OpenEXR::OpenEXR)
    else()
        target_compile_definitions(MK_tools_tests PRIVATE MK_TESTS_HAS_ASSET_IMPORTERS=0)
    endif()
    target_include_directories(MK_tools_tests PRIVATE tests)
    MK_apply_common_target_options(MK_tools_tests)
    add_test(NAME MK_tools_tests COMMAND MK_tools_tests)

    add_executable(MK_tools_mavg_cluster_cook_tests
        tests/unit/tools_mavg_cluster_cook_tests.cpp
    )
    target_link_libraries(MK_tools_mavg_cluster_cook_tests PRIVATE MK_tools)
    target_include_directories(MK_tools_mavg_cluster_cook_tests PRIVATE tests)
    MK_apply_common_target_options(MK_tools_mavg_cluster_cook_tests)
    add_test(NAME MK_tools_mavg_cluster_cook_tests COMMAND MK_tools_mavg_cluster_cook_tests)

    add_executable(MK_tools_sandbox_world_authoring_tests
        tests/unit/tools_sandbox_world_authoring_tests.cpp
    )
    target_link_libraries(MK_tools_sandbox_world_authoring_tests PRIVATE MK_tools)
    target_include_directories(MK_tools_sandbox_world_authoring_tests PRIVATE tests)
    MK_apply_common_target_options(MK_tools_sandbox_world_authoring_tests)
    add_test(NAME MK_tools_sandbox_world_authoring_tests COMMAND MK_tools_sandbox_world_authoring_tests)

    add_executable(MK_tools_runtime_hot_reload_package_tests
        tests/unit/tools_runtime_hot_reload_package_tests.cpp
    )
    target_link_libraries(MK_tools_runtime_hot_reload_package_tests PRIVATE MK_tools MK_runtime)
    target_include_directories(MK_tools_runtime_hot_reload_package_tests PRIVATE tests)
    MK_apply_common_target_options(MK_tools_runtime_hot_reload_package_tests)
    add_test(NAME MK_tools_runtime_hot_reload_package_tests COMMAND MK_tools_runtime_hot_reload_package_tests)

    if(WIN32)
        add_executable(MK_process_probe
            tests/fixtures/process_probe.cpp
        )
        MK_apply_common_target_options(MK_process_probe)

        add_library(MK_dynamic_library_probe SHARED
            tests/fixtures/dynamic_library_probe.cpp
        )
        MK_apply_common_target_options(MK_dynamic_library_probe)

        add_library(MK_editor_game_module_driver_probe SHARED
            tests/fixtures/editor_game_module_driver_probe.cpp
        )
        target_link_libraries(MK_editor_game_module_driver_probe PRIVATE MK_editor_core)
        MK_apply_common_target_options(MK_editor_game_module_driver_probe)

        add_executable(MK_platform_process_tests
            tests/unit/platform_process_tests.cpp
        )
        target_link_libraries(MK_platform_process_tests PRIVATE MK_platform)
        target_include_directories(MK_platform_process_tests PRIVATE tests)
        target_compile_definitions(MK_platform_process_tests PRIVATE
            MK_PROCESS_PROBE_PATH="$<TARGET_FILE:MK_process_probe>"
            MK_DYNAMIC_LIBRARY_PROBE_PATH="$<TARGET_FILE:MK_dynamic_library_probe>"
        )
        add_dependencies(MK_platform_process_tests MK_dynamic_library_probe)
        MK_apply_common_target_options(MK_platform_process_tests)
        add_test(NAME MK_platform_process_tests COMMAND MK_platform_process_tests)
        set_tests_properties(MK_platform_process_tests PROPERTIES RUN_SERIAL TRUE)

        add_executable(MK_editor_game_module_driver_load_tests
            tests/unit/editor_game_module_driver_load_tests.cpp
        )
        target_link_libraries(MK_editor_game_module_driver_load_tests PRIVATE MK_editor_core MK_platform)
        target_include_directories(MK_editor_game_module_driver_load_tests PRIVATE tests)
        target_compile_definitions(MK_editor_game_module_driver_load_tests PRIVATE
            MK_EDITOR_GAME_MODULE_DRIVER_PROBE_PATH="$<TARGET_FILE:MK_editor_game_module_driver_probe>"
        )
        add_dependencies(MK_editor_game_module_driver_load_tests MK_editor_game_module_driver_probe)
        MK_apply_common_target_options(MK_editor_game_module_driver_load_tests)
        add_test(NAME MK_editor_game_module_driver_load_tests COMMAND MK_editor_game_module_driver_load_tests)

        add_executable(MK_d3d12_rhi_tests
            tests/unit/d3d12_rhi_tests.cpp
        )
        target_link_libraries(MK_d3d12_rhi_tests PRIVATE MK_runtime_scene_rhi MK_runtime_rhi MK_rhi_d3d12 MK_renderer d3dcompiler)
        target_include_directories(MK_d3d12_rhi_tests PRIVATE tests)
        MK_apply_common_target_options(MK_d3d12_rhi_tests)
        add_test(NAME MK_d3d12_rhi_tests COMMAND MK_d3d12_rhi_tests)
        set_tests_properties(MK_d3d12_rhi_tests PROPERTIES RUN_SERIAL TRUE)

        add_executable(MK_mavg_gpu_culling_dispatch_tests
            tests/unit/mavg_gpu_culling_dispatch_tests.cpp
        )
        target_link_libraries(MK_mavg_gpu_culling_dispatch_tests PRIVATE MK_renderer MK_rhi_d3d12)
        target_include_directories(MK_mavg_gpu_culling_dispatch_tests PRIVATE tests)
        MK_apply_common_target_options(MK_mavg_gpu_culling_dispatch_tests)
        add_test(NAME MK_mavg_gpu_culling_dispatch_tests COMMAND MK_mavg_gpu_culling_dispatch_tests)
        set_tests_properties(MK_mavg_gpu_culling_dispatch_tests PROPERTIES RUN_SERIAL TRUE)

        add_executable(MK_d3d12_environment_weather_solver_tests
            tests/unit/d3d12_environment_weather_solver_tests.cpp
        )
        target_link_libraries(MK_d3d12_environment_weather_solver_tests PRIVATE MK_rhi_d3d12)
        target_include_directories(MK_d3d12_environment_weather_solver_tests PRIVATE tests)
        MK_apply_common_target_options(MK_d3d12_environment_weather_solver_tests)
        add_test(NAME MK_d3d12_environment_weather_solver_tests COMMAND MK_d3d12_environment_weather_solver_tests)
        set_tests_properties(MK_d3d12_environment_weather_solver_tests PROPERTIES RUN_SERIAL TRUE)

        add_executable(MK_mavg_d3d12_compute_generated_indirect_consumption_tests
            tests/unit/mavg_d3d12_compute_generated_indirect_consumption_tests.cpp
        )
        target_link_libraries(MK_mavg_d3d12_compute_generated_indirect_consumption_tests PRIVATE MK_renderer MK_rhi_d3d12 d3dcompiler)
        target_include_directories(MK_mavg_d3d12_compute_generated_indirect_consumption_tests PRIVATE tests)
        MK_apply_common_target_options(MK_mavg_d3d12_compute_generated_indirect_consumption_tests)
        add_test(NAME MK_mavg_d3d12_compute_generated_indirect_consumption_tests
                 COMMAND MK_mavg_d3d12_compute_generated_indirect_consumption_tests)
        set_tests_properties(MK_mavg_d3d12_compute_generated_indirect_consumption_tests PROPERTIES RUN_SERIAL TRUE)
    endif()

    if(TARGET MK_rhi_vulkan AND (WIN32 OR (UNIX AND NOT APPLE)))
        add_executable(MK_mavg_vulkan_gpu_culling_dispatch_tests
            tests/unit/mavg_vulkan_gpu_culling_dispatch_tests.cpp
        )
        target_link_libraries(MK_mavg_vulkan_gpu_culling_dispatch_tests PRIVATE MK_renderer MK_rhi_vulkan)
        target_include_directories(MK_mavg_vulkan_gpu_culling_dispatch_tests PRIVATE tests)
        MK_apply_common_target_options(MK_mavg_vulkan_gpu_culling_dispatch_tests)
        add_test(NAME MK_mavg_vulkan_gpu_culling_dispatch_tests COMMAND MK_mavg_vulkan_gpu_culling_dispatch_tests)
        set_tests_properties(MK_mavg_vulkan_gpu_culling_dispatch_tests PROPERTIES RUN_SERIAL TRUE)

        add_executable(MK_mavg_vulkan_compute_generated_indirect_consumption_tests
            tests/unit/mavg_vulkan_compute_generated_indirect_consumption_tests.cpp
        )
        target_link_libraries(MK_mavg_vulkan_compute_generated_indirect_consumption_tests PRIVATE MK_renderer MK_rhi_vulkan)
        target_include_directories(MK_mavg_vulkan_compute_generated_indirect_consumption_tests PRIVATE tests)
        MK_apply_common_target_options(MK_mavg_vulkan_compute_generated_indirect_consumption_tests)
        add_test(NAME MK_mavg_vulkan_compute_generated_indirect_consumption_tests
                 COMMAND MK_mavg_vulkan_compute_generated_indirect_consumption_tests)
        set_tests_properties(MK_mavg_vulkan_compute_generated_indirect_consumption_tests PROPERTIES RUN_SERIAL TRUE)

        add_executable(MK_vulkan_environment_weather_solver_tests
            tests/unit/vulkan_environment_weather_solver_tests.cpp
        )
        target_link_libraries(MK_vulkan_environment_weather_solver_tests PRIVATE MK_rhi_vulkan)
        target_include_directories(MK_vulkan_environment_weather_solver_tests PRIVATE tests)
        MK_apply_common_target_options(MK_vulkan_environment_weather_solver_tests)
        add_test(NAME MK_vulkan_environment_weather_solver_tests COMMAND MK_vulkan_environment_weather_solver_tests)
        set_tests_properties(MK_vulkan_environment_weather_solver_tests PROPERTIES RUN_SERIAL TRUE)
    endif()

    if(TARGET MK_runtime_host_linux)
        add_executable(MK_linux_vulkan_runtime_probe_tests
            tests/unit/linux_vulkan_runtime_probe_tests.cpp
        )
        target_link_libraries(MK_linux_vulkan_runtime_probe_tests PRIVATE MK_runtime_host_linux)
        target_include_directories(MK_linux_vulkan_runtime_probe_tests PRIVATE tests)
        MK_apply_common_target_options(MK_linux_vulkan_runtime_probe_tests)
        add_test(NAME MK_linux_vulkan_runtime_probe_tests COMMAND MK_linux_vulkan_runtime_probe_tests)
    endif()

    add_executable(MK_metal_environment_weather_solver_tests
        tests/unit/metal_environment_weather_solver_tests.cpp
    )
    target_link_libraries(MK_metal_environment_weather_solver_tests PRIVATE MK_rhi_metal)
    target_include_directories(MK_metal_environment_weather_solver_tests PRIVATE tests)
    MK_apply_common_target_options(MK_metal_environment_weather_solver_tests)
    add_test(NAME MK_metal_environment_weather_solver_tests COMMAND MK_metal_environment_weather_solver_tests)
    set_tests_properties(MK_metal_environment_weather_solver_tests PROPERTIES RUN_SERIAL TRUE)
    if(APPLE AND TARGET MK_metal_environment_weather_solver_metallib)
        add_dependencies(MK_metal_environment_weather_solver_tests MK_metal_environment_weather_solver_metallib)
        get_property(MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB GLOBAL PROPERTY MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB)
        set_tests_properties(MK_metal_environment_weather_solver_tests PROPERTIES
            ENVIRONMENT "MK_METAL_TEST_ENVIRONMENT_WEATHER_SOLVER_METALLIB=${MK_METAL_ENVIRONMENT_WEATHER_SOLVER_METALLIB}"
        )
    endif()

    if((APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") OR (UNIX AND NOT APPLE))
        add_executable(MK_platform_native_file_watcher_tests
            tests/unit/platform_native_file_watcher_tests.cpp
        )
        target_link_libraries(MK_platform_native_file_watcher_tests PRIVATE MK_platform)
        target_include_directories(MK_platform_native_file_watcher_tests PRIVATE tests)
        MK_apply_common_target_options(MK_platform_native_file_watcher_tests)
        add_test(NAME MK_platform_native_file_watcher_tests COMMAND MK_platform_native_file_watcher_tests)
    endif()

    if(WIN32)
        add_executable(MK_win32_platform_tests
            tests/unit/win32_platform_tests.cpp
        )
        target_link_libraries(MK_win32_platform_tests PRIVATE MK_platform_win32)
        target_include_directories(MK_win32_platform_tests PRIVATE tests)
        MK_apply_common_target_options(MK_win32_platform_tests)
        add_test(NAME MK_win32_platform_tests COMMAND MK_win32_platform_tests)

        add_executable(MK_wasapi_audio_tests
            tests/unit/wasapi_audio_tests.cpp
        )
        target_link_libraries(MK_wasapi_audio_tests PRIVATE MK_audio_wasapi)
        target_include_directories(MK_wasapi_audio_tests PRIVATE tests)
        MK_apply_common_target_options(MK_wasapi_audio_tests)
        add_test(NAME MK_wasapi_audio_tests COMMAND MK_wasapi_audio_tests)

        if(TARGET MK_runtime_host_win32_presentation)
            add_executable(MK_runtime_host_win32_tests
                tests/unit/runtime_host_win32_tests.cpp
            )
            target_link_libraries(MK_runtime_host_win32_tests PRIVATE
                MK_runtime_host_win32
                MK_runtime_host_win32_presentation
                MK_core
                MK_platform_win32
                MK_renderer
            )
            if(TARGET MK_rhi_d3d12)
                target_link_libraries(MK_runtime_host_win32_tests PRIVATE MK_rhi_d3d12 d3dcompiler)
            endif()
            target_include_directories(MK_runtime_host_win32_tests PRIVATE tests)
            MK_apply_common_target_options(MK_runtime_host_win32_tests)
            add_test(NAME MK_runtime_host_win32_tests COMMAND MK_runtime_host_win32_tests)

            add_executable(MK_runtime_host_win32_public_api_compile
                tests/unit/runtime_host_win32_public_api_compile.cpp
            )
            target_link_libraries(MK_runtime_host_win32_public_api_compile PRIVATE MK_runtime_host_win32_presentation)
            MK_apply_common_target_options(MK_runtime_host_win32_public_api_compile)
            add_test(NAME MK_runtime_host_win32_public_api_compile COMMAND MK_runtime_host_win32_public_api_compile)
        endif()
    endif()

    get_property(MK_CURRENT_DIRECTORY_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(MK_CURRENT_DIRECTORY_TARGET IN LISTS MK_CURRENT_DIRECTORY_TARGETS)
        if(MK_CURRENT_DIRECTORY_TARGET MATCHES "^MK_.*(_tests|_probe|_public_api_compile)$")
            MK_disable_module_scanning_for_non_module_executable(${MK_CURRENT_DIRECTORY_TARGET})
        endif()
    endforeach()
endif()

function(MK_set_export_name target_name export_name)
    if(NOT TARGET ${target_name})
        return()
    endif()
    get_target_property(_mk_aliased_target ${target_name} ALIASED_TARGET)
    if(_mk_aliased_target)
        set_target_properties(${_mk_aliased_target} PROPERTIES EXPORT_NAME ${export_name})
    else()
        set_target_properties(${target_name} PROPERTIES EXPORT_NAME ${export_name})
    endif()
endfunction()

MK_set_export_name(MK_animation animation)
MK_set_export_name(MK_ai ai)
MK_set_export_name(MK_assets assets)
MK_set_export_name(MK_audio audio)
MK_set_export_name(MK_core core)
MK_set_export_name(MK_environment environment)
MK_set_export_name(MK_math math)
MK_set_export_name(MK_navigation navigation)
MK_set_export_name(MK_runtime_network_enet runtime_network_enet)
MK_set_export_name(MK_physics physics)
MK_set_export_name(MK_physics_jolt physics_jolt)
MK_set_export_name(MK_platform platform)
MK_set_export_name(MK_platform_win32 platform_win32)
MK_set_export_name(MK_renderer renderer)
MK_set_export_name(MK_rhi rhi)
MK_set_export_name(MK_runtime runtime)
MK_set_export_name(MK_runtime_host runtime_host)
MK_set_export_name(MK_runtime_host_win32 runtime_host_win32)
MK_set_export_name(MK_runtime_host_win32_presentation runtime_host_win32_presentation)
MK_set_export_name(MK_runtime_rhi runtime_rhi)
MK_set_export_name(MK_runtime_scene runtime_scene)
MK_set_export_name(MK_runtime_scene_rhi runtime_scene_rhi)
MK_set_export_name(MK_rhi_d3d12 rhi_d3d12)
MK_set_export_name(MK_rhi_metal rhi_metal)
MK_set_export_name(MK_rhi_vulkan rhi_vulkan)
MK_set_export_name(MK_scene scene)
MK_set_export_name(MK_scene_renderer scene_renderer)
MK_set_export_name(MK_tools tools)
MK_set_export_name(MK_ui ui)
MK_set_export_name(MK_ui_renderer ui_renderer)
MK_set_export_name(MK_audio_wasapi audio_wasapi)
MK_set_export_name(MK_editor_core editor_core)

set(MK_LIBRARY_TARGETS
    mirakana_ai
    mirakana_animation
    mirakana_assets
    MK_audio
    mirakana_core
    mirakana_environment
    mirakana_math
    mirakana_navigation
    mirakana_physics
    MK_platform
    mirakana_renderer
    mirakana_rhi
    mirakana_runtime
    MK_runtime_host
    mirakana_runtime_rhi
    mirakana_runtime_scene
    mirakana_runtime_scene_rhi
    MK_rhi_metal
    mirakana_rhi_vulkan
    mirakana_scene
    mirakana_scene_renderer
    MK_tools
    mirakana_ui
    mirakana_ui_renderer
)

set(MK_RUNTIME_TARGETS
    MK_sandbox
    sample_headless
    sample_2d_playable_foundation
    sample_ai_navigation
    sample_gameplay_foundation
    sample_input_renderer
    sample_ui_audio_assets
)

foreach(optional_target IN ITEMS mirakana_rhi_d3d12 mirakana_audio_wasapi mirakana_platform_win32
        MK_runtime_host_win32 MK_runtime_host_win32_presentation MK_editor_core mirakana_physics_jolt
        mirakana_runtime_network_enet mirakana_platform_linux MK_runtime_host_linux)
    if(TARGET ${optional_target})
        list(APPEND MK_LIBRARY_TARGETS ${optional_target})
    endif()
endforeach()

if(TARGET MK_editor)
    list(APPEND MK_RUNTIME_TARGETS MK_editor)
endif()

if(TARGET sample_desktop_runtime_shell)
    list(APPEND MK_RUNTIME_TARGETS sample_desktop_runtime_shell)
endif()

if(TARGET sample_desktop_runtime_game)
    list(APPEND MK_RUNTIME_TARGETS sample_desktop_runtime_game)
endif()

if(TARGET linux_vulkan_runtime_probe)
    list(APPEND MK_RUNTIME_TARGETS linux_vulkan_runtime_probe)
endif()

function(MK_json_escape output_variable value)
    string(REPLACE "\\" "\\\\" escaped_value "${value}")
    string(REPLACE "\"" "\\\"" escaped_value "${escaped_value}")
    string(REPLACE "\n" "\\n" escaped_value "${escaped_value}")
    set(${output_variable} "${escaped_value}" PARENT_SCOPE)
endfunction()

function(MK_json_string_array output_variable list_value)
    set(items ${list_value})
    set(json_items "")
    foreach(item IN LISTS items)
        MK_json_escape(escaped_item "${item}")
        if(json_items STREQUAL "")
            set(json_items "\"${escaped_item}\"")
        else()
            string(APPEND json_items ", \"${escaped_item}\"")
        endif()
    endforeach()
    set(${output_variable} "[${json_items}]" PARENT_SCOPE)
endfunction()

function(MK_normalize_target_property output_variable target_name property_name)
    get_target_property(property_value ${target_name} ${property_name})
    if(NOT property_value OR property_value MATCHES "-NOTFOUND$")
        set(property_value "")
    endif()
    set(${output_variable} "${property_value}" PARENT_SCOPE)
endfunction()

function(MK_json_package_file_array output_variable source_paths install_paths)
    set(json_items "")
    list(LENGTH source_paths source_count)
    list(LENGTH install_paths install_count)
    if(NOT source_count EQUAL install_count)
        message(FATAL_ERROR "Desktop runtime package file metadata source/install counts differ")
    endif()

    if(source_count GREATER 0)
        math(EXPR last_index "${source_count} - 1")
        foreach(index RANGE 0 ${last_index})
            list(GET source_paths ${index} source_path)
            list(GET install_paths ${index} install_path)
            MK_json_escape(source_path_json "${source_path}")
            MK_json_escape(install_path_json "${install_path}")
            if(NOT json_items STREQUAL "")
                string(APPEND json_items ", ")
            endif()
            string(APPEND json_items "{ \"source\": \"${source_path_json}\", \"install\": \"${install_path_json}\" }")
        endforeach()
    endif()

    set(${output_variable} "[${json_items}]" PARENT_SCOPE)
endfunction()

function(MK_write_desktop_runtime_game_metadata output_path selected_package_target)
    get_property(registered_games GLOBAL PROPERTY MK_DESKTOP_RUNTIME_GAMES)
    if(NOT registered_games)
        set(registered_games "")
    endif()

    MK_json_escape(selected_package_target_json "${selected_package_target}")
    file(WRITE "${output_path}" "{\n")
    file(APPEND "${output_path}" "  \"schemaVersion\": 1,\n")
    file(APPEND "${output_path}" "  \"selectedPackageTarget\": \"${selected_package_target_json}\",\n")
    file(APPEND "${output_path}" "  \"registeredGames\": [\n")

    set(first_entry TRUE)
    foreach(game_target IN LISTS registered_games)
        MK_normalize_target_property(game_manifest ${game_target} MK_DESKTOP_RUNTIME_GAME_MANIFEST)
        MK_normalize_target_property(smoke_args ${game_target} MK_DESKTOP_RUNTIME_SMOKE_ARGS)
        MK_normalize_target_property(package_smoke_args ${game_target} MK_DESKTOP_RUNTIME_PACKAGE_SMOKE_ARGS)
        MK_normalize_target_property(requires_d3d12_shaders ${game_target} MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS)
        MK_normalize_target_property(requires_vulkan_shaders ${game_target} MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS)
        MK_normalize_target_property(d3d12_shader_artifacts ${game_target} MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS)
        MK_normalize_target_property(vulkan_shader_artifacts ${game_target} MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS)
        MK_normalize_target_property(package_files ${game_target} MK_DESKTOP_RUNTIME_PACKAGE_FILES)
        MK_normalize_target_property(package_file_install_paths ${game_target} MK_DESKTOP_RUNTIME_PACKAGE_FILE_INSTALL_PATHS)
        if(NOT game_manifest)
            message(FATAL_ERROR "Desktop runtime game target must declare MK_DESKTOP_RUNTIME_GAME_MANIFEST: ${game_target}")
        endif()
        MK_json_string_array(smoke_args_json "${smoke_args}")
        MK_json_string_array(package_smoke_args_json "${package_smoke_args}")
        MK_json_string_array(d3d12_shader_artifacts_json "${d3d12_shader_artifacts}")
        MK_json_string_array(vulkan_shader_artifacts_json "${vulkan_shader_artifacts}")
        MK_json_package_file_array(package_files_json "${package_files}" "${package_file_install_paths}")
        MK_json_escape(game_target_json "${game_target}")
        MK_json_escape(game_manifest_json "${game_manifest}")
        if(requires_d3d12_shaders)
            set(requires_d3d12_shaders_json "true")
        else()
            set(requires_d3d12_shaders_json "false")
        endif()
        if(requires_vulkan_shaders)
            set(requires_vulkan_shaders_json "true")
        else()
            set(requires_vulkan_shaders_json "false")
        endif()

        if(first_entry)
            set(first_entry FALSE)
        else()
            file(APPEND "${output_path}" ",\n")
        endif()
        file(APPEND "${output_path}" "    {\n")
        file(APPEND "${output_path}" "      \"target\": \"${game_target_json}\",\n")
        file(APPEND "${output_path}" "      \"gameManifest\": \"${game_manifest_json}\",\n")
        file(APPEND "${output_path}" "      \"smokeArgs\": ${smoke_args_json},\n")
        file(APPEND "${output_path}" "      \"packageSmokeArgs\": ${package_smoke_args_json},\n")
        file(APPEND "${output_path}" "      \"requiresD3d12Shaders\": ${requires_d3d12_shaders_json},\n")
        file(APPEND "${output_path}" "      \"d3d12ShaderArtifacts\": ${d3d12_shader_artifacts_json},\n")
        file(APPEND "${output_path}" "      \"requiresVulkanShaders\": ${requires_vulkan_shaders_json},\n")
        file(APPEND "${output_path}" "      \"vulkanShaderArtifacts\": ${vulkan_shader_artifacts_json},\n")
        file(APPEND "${output_path}" "      \"packageFiles\": ${package_files_json}\n")
        file(APPEND "${output_path}" "    }")
    endforeach()

    file(APPEND "${output_path}" "\n  ]\n")
    file(APPEND "${output_path}" "}\n")
endfunction()

set(MK_INSTALL_RUNTIME_TARGETS ${MK_RUNTIME_TARGETS})
if(NOT MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "")
    if(NOT TARGET ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET})
        message(FATAL_ERROR "MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET does not name a CMake target: ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET}")
    endif()
    get_property(MK_SELECTED_DESKTOP_RUNTIME_GAME
        TARGET ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET}
        PROPERTY MK_DESKTOP_RUNTIME_GAME
    )
    if(NOT MK_SELECTED_DESKTOP_RUNTIME_GAME)
        message(FATAL_ERROR "MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET must name a target registered with MK_add_desktop_runtime_game: ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET}")
    endif()
    MK_normalize_target_property(MK_SELECTED_DESKTOP_RUNTIME_PACKAGE_SMOKE_ARGS
        ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET}
        MK_DESKTOP_RUNTIME_PACKAGE_SMOKE_ARGS
    )
    if(NOT MK_SELECTED_DESKTOP_RUNTIME_PACKAGE_SMOKE_ARGS)
        message(FATAL_ERROR "MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET must declare package smoke args through MK_add_desktop_runtime_game: ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET}")
    endif()
    set(MK_INSTALL_RUNTIME_TARGETS ${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET})
endif()

if(NOT MK_LINUX_RUNTIME_PACKAGE_TARGET STREQUAL "")
    if(NOT UNIX OR APPLE OR ANDROID)
        message(FATAL_ERROR "MK_LINUX_RUNTIME_PACKAGE_TARGET is supported only on Linux hosts")
    endif()
    if(NOT TARGET ${MK_LINUX_RUNTIME_PACKAGE_TARGET})
        message(FATAL_ERROR "MK_LINUX_RUNTIME_PACKAGE_TARGET does not name a CMake target: ${MK_LINUX_RUNTIME_PACKAGE_TARGET}")
    endif()
    get_property(MK_SELECTED_LINUX_VULKAN_RUNTIME_PROBE
        TARGET ${MK_LINUX_RUNTIME_PACKAGE_TARGET}
        PROPERTY MK_LINUX_VULKAN_RUNTIME_PROBE
    )
    if(NOT MK_SELECTED_LINUX_VULKAN_RUNTIME_PROBE)
        message(FATAL_ERROR "MK_LINUX_RUNTIME_PACKAGE_TARGET must name the Linux Vulkan runtime probe target: ${MK_LINUX_RUNTIME_PACKAGE_TARGET}")
    endif()
    set(MK_INSTALL_RUNTIME_TARGETS ${MK_LINUX_RUNTIME_PACKAGE_TARGET})
endif()

if(MK_DESKTOP_RUNTIME_ENABLED)
    set(MK_DESKTOP_RUNTIME_GAME_METADATA_FILE "${CMAKE_BINARY_DIR}/desktop-runtime-games.json")
    MK_write_desktop_runtime_game_metadata(
        "${MK_DESKTOP_RUNTIME_GAME_METADATA_FILE}"
        "${MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET}"
    )
    add_custom_target(MK_desktop_runtime_package_build)
    add_dependencies(MK_desktop_runtime_package_build
        ${MK_LIBRARY_TARGETS}
        ${MK_INSTALL_RUNTIME_TARGETS}
    )
    foreach(MK_DESKTOP_RUNTIME_PACKAGE_TEST_TARGET IN ITEMS
        MK_runtime_host_tests
        MK_runtime_host_win32_public_api_compile
        MK_runtime_host_win32_tests
        MK_win32_platform_tests
        MK_wasapi_audio_tests
    )
        if(TARGET ${MK_DESKTOP_RUNTIME_PACKAGE_TEST_TARGET})
            add_dependencies(MK_desktop_runtime_package_build ${MK_DESKTOP_RUNTIME_PACKAGE_TEST_TARGET})
        endif()
    endforeach()
endif()

if(NOT MK_LINUX_RUNTIME_PACKAGE_TARGET STREQUAL "")
    add_custom_target(MK_linux_vulkan_runtime_package_build)
    add_dependencies(MK_linux_vulkan_runtime_package_build
        ${MK_LIBRARY_TARGETS}
        ${MK_INSTALL_RUNTIME_TARGETS}
    )
    if(TARGET MK_linux_vulkan_runtime_probe_tests)
        add_dependencies(MK_linux_vulkan_runtime_package_build MK_linux_vulkan_runtime_probe_tests)
    endif()
endif()

# Install / export: MirakanaiTargets + MirakanaiConfig (see docs/building.md). Runtime executables are installed but not re-exported for find_package.
install(TARGETS ${MK_LIBRARY_TARGETS}
    EXPORT MirakanaiTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(TARGETS ${MK_INSTALL_RUNTIME_TARGETS}
    BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(MK_CMAKE_PACKAGE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Mirakanai")

install(EXPORT MirakanaiTargets
    FILE MirakanaiTargets.cmake
    NAMESPACE mirakana::
    DESTINATION ${MK_CMAKE_PACKAGE_DIR}
)

set(MK_PACKAGE_HAS_ASSET_IMPORTERS OFF)
if(MK_ENABLE_ASSET_IMPORTERS)
    set(MK_PACKAGE_HAS_ASSET_IMPORTERS ON)
endif()

set(MK_PACKAGE_HAS_PHYSICS_JOLT OFF)
if(TARGET MK_physics_jolt)
    set(MK_PACKAGE_HAS_PHYSICS_JOLT ON)
endif()

set(MK_PACKAGE_HAS_NETWORK_ENET OFF)
if(TARGET MK_runtime_network_enet)
    set(MK_PACKAGE_HAS_NETWORK_ENET ON)
endif()

configure_package_config_file(
    cmake/MirakanaiConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/MirakanaiConfig.cmake"
    INSTALL_DESTINATION ${MK_CMAKE_PACKAGE_DIR}
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR CMAKE_INSTALL_DATADIR CMAKE_INSTALL_DOCDIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MirakanaiConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MirakanaiConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MirakanaiConfigVersion.cmake"
    DESTINATION ${MK_CMAKE_PACKAGE_DIR}
)

install(DIRECTORY
    engine/ai/include/
    engine/animation/include/
    engine/assets/include/
    engine/audio/include/
    engine/core/include/
    engine/environment/include/
    engine/math/include/
    engine/navigation/include/
    engine/physics/include/
    engine/platform/include/
    engine/runtime/include/
    engine/runtime_host/include/
    engine/runtime_rhi/include/
    engine/runtime_scene/include/
    engine/runtime_scene_rhi/include/
    engine/renderer/include/
    engine/rhi/include/
    engine/scene/include/
    engine/scene_renderer/include/
    engine/tools/include/
    engine/ui/include/
    engine/ui_renderer/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(TARGET MK_rhi_d3d12)
    install(DIRECTORY engine/rhi/d3d12/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

install(DIRECTORY
    engine/rhi/metal/include/
    engine/rhi/vulkan/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

if(TARGET MK_platform_win32)
    install(DIRECTORY engine/platform/win32/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_platform_linux)
    install(DIRECTORY engine/platform/linux/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_runtime_host_win32)
    install(DIRECTORY engine/runtime_host/win32/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_runtime_host_linux)
    install(DIRECTORY engine/runtime_host/linux/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_audio_wasapi)
    install(DIRECTORY engine/audio/wasapi/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_physics_jolt)
    install(DIRECTORY engine/physics/jolt/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_runtime_network_enet)
    install(DIRECTORY engine/runtime/network/enet/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

if(TARGET MK_editor_core)
    install(DIRECTORY editor/core/include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()

set(MK_PACKAGE_SHARE_DIR "${CMAKE_INSTALL_DATADIR}/Mirakanai")

if(DEFINED MK_DESKTOP_RUNTIME_GAME_METADATA_FILE AND EXISTS "${MK_DESKTOP_RUNTIME_GAME_METADATA_FILE}")
    install(FILES
        "${MK_DESKTOP_RUNTIME_GAME_METADATA_FILE}"
        DESTINATION ${MK_PACKAGE_SHARE_DIR}
    )
endif()

install(FILES
    AGENTS.md
    CLAUDE.md
    THIRD_PARTY_NOTICES.md
    engine/agent/manifest.json
    DESTINATION ${MK_PACKAGE_SHARE_DIR}
)

install(DIRECTORY LICENSES/
    DESTINATION ${MK_PACKAGE_SHARE_DIR}/LICENSES
)

install(DIRECTORY schemas/
    DESTINATION ${MK_PACKAGE_SHARE_DIR}/schemas
    FILES_MATCHING
        PATTERN "*.json"
)

install(DIRECTORY tools/
    DESTINATION ${MK_PACKAGE_SHARE_DIR}/tools
    FILES_MATCHING
        PATTERN "*.ps1"
)

install(DIRECTORY examples/
    DESTINATION ${MK_PACKAGE_SHARE_DIR}/examples
    FILES_MATCHING
        PATTERN "CMakeLists.txt"
        PATTERN "*.cpp"
        PATTERN "*.hpp"
        PATTERN "*.md"
)

install(DIRECTORY games/
    DESTINATION ${MK_PACKAGE_SHARE_DIR}/samples
    FILES_MATCHING
        PATTERN "CMakeLists.txt"
        PATTERN "*.cpp"
        PATTERN "*.hpp"
        PATTERN "*.hlsl"
        PATTERN "*.config"
        PATTERN "*.md"
        PATTERN "game.agent.json"
)

install(DIRECTORY docs/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
    FILES_MATCHING
        PATTERN "*.md"
)

set(CPACK_PACKAGE_NAME "Mirakanai")
set(CPACK_PACKAGE_VENDOR "MIRAIKANAI contributors")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
set(CPACK_PACKAGE_CONTACT "MIRAIKANAI contributors")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://example.invalid/Mirakanai")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Mirakanai")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSES/LicenseRef-Proprietary.txt")
set(CPACK_PACKAGE_CHECKSUM SHA256)
set(CPACK_VERBATIM_VARIABLES YES)
set(CPACK_GENERATOR "ZIP")
include(CPack)
