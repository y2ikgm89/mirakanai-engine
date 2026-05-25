add_library(mirakana_runtime
    src/addressable_content_streaming.cpp
    src/asset_runtime.cpp
    src/entity_scale_culling.cpp
    src/genre_rpg_systems.cpp
    src/genre_sandbox_world.cpp
    src/genre_simulation_management.cpp
    src/gameplay_runtime_scheduler.cpp
    src/gameplay_interaction.cpp
    src/inventory_items.cpp
    src/networking_foundation.cpp
    src/network_transport.cpp
    src/package_streaming.cpp
    src/physics_collision_runtime.cpp
    src/production_network_replication.cpp
    src/procedural_generation.cpp
    src/quest_dialogue.cpp
    src/resource_runtime.cpp
    src/runtime_diagnostics.cpp
    src/scripting_sandbox.cpp
    src/session_services.cpp
    src/simulation_orchestration.cpp
    src/sprite_collision_hitbox.cpp
    src/sprite_effect_particles.cpp
    src/world_entity_model.cpp
    src/world_region_streaming.cpp
)

target_include_directories(mirakana_runtime
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_runtime
    PUBLIC
        mirakana_assets
        mirakana_platform
        mirakana_physics
)

mirakana_apply_common_target_options(mirakana_runtime)

if(NOT TARGET MK_runtime)
    add_library(MK_runtime ALIAS mirakana_runtime)
endif()
