MK_add_game(sample_headless
    SOURCES
        sample_headless/main.cpp
)

MK_add_game(sample_input_renderer
    SOURCES
        sample_input_renderer/main.cpp
)

MK_add_game(sample_gameplay_foundation
    SOURCES
        sample_gameplay_foundation/main.cpp
)

MK_add_game(sample_ai_navigation
    SOURCES
        sample_ai_navigation/main.cpp
)

MK_add_game(sample_ui_audio_assets
    SOURCES
        sample_ui_audio_assets/main.cpp
)

MK_add_game(sample_2d_playable_foundation
    SOURCES
        sample_2d_playable_foundation/main.cpp
)

if(MK_DESKTOP_RUNTIME_ENABLED)
    find_package(SDL3 CONFIG REQUIRED)

    function(MK_add_desktop_runtime_game target_name)
        cmake_parse_arguments(MK_DESKTOP_GAME "REQUIRES_D3D12_SHADERS;REQUIRES_VULKAN_SHADERS;PACKAGE_FILES_FROM_MANIFEST" "GAME_MANIFEST" "SOURCES;SMOKE_ARGS;PACKAGE_SMOKE_ARGS;PACKAGE_FILES" ${ARGN})
        if(MK_DESKTOP_GAME_UNPARSED_ARGUMENTS)
            message(FATAL_ERROR "MK_add_desktop_runtime_game received unknown arguments for ${target_name}: ${MK_DESKTOP_GAME_UNPARSED_ARGUMENTS}")
        endif()
        if(NOT MK_DESKTOP_GAME_SOURCES)
            message(FATAL_ERROR "MK_add_desktop_runtime_game requires SOURCES")
        endif()
        if(NOT MK_DESKTOP_GAME_SMOKE_ARGS)
            message(FATAL_ERROR "MK_add_desktop_runtime_game requires SMOKE_ARGS for ${target_name}")
        endif()
        if(NOT MK_DESKTOP_GAME_GAME_MANIFEST)
            message(FATAL_ERROR "MK_add_desktop_runtime_game requires GAME_MANIFEST for ${target_name}")
        endif()
        if(IS_ABSOLUTE "${MK_DESKTOP_GAME_GAME_MANIFEST}")
            message(FATAL_ERROR "MK_add_desktop_runtime_game GAME_MANIFEST must be repository-relative for ${target_name}")
        endif()
        if(NOT MK_DESKTOP_GAME_GAME_MANIFEST MATCHES "^games/[a-z][a-z0-9_]*/game\\.agent\\.json$")
            message(FATAL_ERROR "MK_add_desktop_runtime_game GAME_MANIFEST must point at games/<game_name>/game.agent.json for ${target_name}")
        endif()
        if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${MK_DESKTOP_GAME_GAME_MANIFEST}")
            message(FATAL_ERROR "MK_add_desktop_runtime_game GAME_MANIFEST does not exist for ${target_name}: ${MK_DESKTOP_GAME_GAME_MANIFEST}")
        endif()
        if(NOT MK_DESKTOP_GAME_PACKAGE_SMOKE_ARGS)
            set(MK_DESKTOP_GAME_PACKAGE_SMOKE_ARGS ${MK_DESKTOP_GAME_SMOKE_ARGS})
        endif()
        get_filename_component(MK_DESKTOP_GAME_DIRECTORY "${MK_DESKTOP_GAME_GAME_MANIFEST}" DIRECTORY)
        file(REAL_PATH "${MK_DESKTOP_GAME_DIRECTORY}" MK_DESKTOP_GAME_DIRECTORY_REAL BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
        if(MK_DESKTOP_GAME_PACKAGE_FILES_FROM_MANIFEST)
            if(MK_DESKTOP_GAME_PACKAGE_FILES)
                message(FATAL_ERROR "MK_add_desktop_runtime_game cannot mix PACKAGE_FILES_FROM_MANIFEST with literal PACKAGE_FILES for ${target_name}")
            endif()
            file(READ "${CMAKE_SOURCE_DIR}/${MK_DESKTOP_GAME_GAME_MANIFEST}" MK_DESKTOP_GAME_MANIFEST_JSON)
            string(JSON MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_TYPE ERROR_VARIABLE MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_TYPE_ERROR
                TYPE "${MK_DESKTOP_GAME_MANIFEST_JSON}" runtimePackageFiles
            )
            if(NOT MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_TYPE_ERROR STREQUAL "NOTFOUND")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES_FROM_MANIFEST requires runtimePackageFiles in ${MK_DESKTOP_GAME_GAME_MANIFEST} for ${target_name}: ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_TYPE_ERROR}")
            endif()
            if(NOT MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_TYPE STREQUAL "ARRAY")
                message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles must be an array in ${MK_DESKTOP_GAME_GAME_MANIFEST} for ${target_name}")
            endif()
            string(JSON MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_LENGTH
                LENGTH "${MK_DESKTOP_GAME_MANIFEST_JSON}" runtimePackageFiles
            )
            if(MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_LENGTH EQUAL 0)
                message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles must not be empty when PACKAGE_FILES_FROM_MANIFEST is used for ${target_name}")
            endif()

            math(EXPR MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_LAST_INDEX "${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_LENGTH} - 1")
            foreach(MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_INDEX RANGE 0 ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILES_LAST_INDEX})
                string(JSON MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_TYPE
                    TYPE "${MK_DESKTOP_GAME_MANIFEST_JSON}" runtimePackageFiles ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_INDEX}
                )
                if(NOT MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_TYPE STREQUAL "STRING")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must be strings in ${MK_DESKTOP_GAME_GAME_MANIFEST} for ${target_name}")
                endif()
                string(JSON MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE
                    GET "${MK_DESKTOP_GAME_MANIFEST_JSON}" runtimePackageFiles ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_INDEX}
                )
                string(REPLACE "\\" "/" MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED "${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE}")
                if(MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED STREQUAL "")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must not be empty in ${MK_DESKTOP_GAME_GAME_MANIFEST} for ${target_name}")
                endif()
                if(MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED MATCHES ";")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must not contain CMake list separators for ${target_name}: ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE}")
                endif()
                if(IS_ABSOLUTE "${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED}" OR
                   MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED MATCHES "^[A-Za-z]:")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must be game-relative for ${target_name}: ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE}")
                endif()
                if(MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED MATCHES "(^|/)\\.\\.(/|$)")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must not contain parent-directory segments for ${target_name}: ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE}")
                endif()
                if(MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED MATCHES "^games/")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must be game-relative, not repository-relative, for ${target_name}: ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE}")
                endif()
                if(NOT MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED MATCHES "^[a-z0-9_./]+$")
                    message(FATAL_ERROR "MK_add_desktop_runtime_game runtimePackageFiles entries must use lowercase snake_case path segments for ${target_name}: ${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE}")
                endif()
                list(APPEND MK_DESKTOP_GAME_PACKAGE_FILES "${MK_DESKTOP_GAME_DIRECTORY}/${MK_DESKTOP_GAME_RUNTIME_PACKAGE_FILE_NORMALIZED}")
            endforeach()
        endif()
        set(MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED "")
        set(MK_DESKTOP_GAME_PACKAGE_FILE_REAL_PATHS "")
        set(MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE_PATHS "")
        set(MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATHS "")
        foreach(MK_DESKTOP_GAME_PACKAGE_FILE IN LISTS MK_DESKTOP_GAME_PACKAGE_FILES)
            if(IS_ABSOLUTE "${MK_DESKTOP_GAME_PACKAGE_FILE}")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entries must be repository-relative for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE}")
            endif()
            string(REPLACE "\\" "/" MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED "${MK_DESKTOP_GAME_PACKAGE_FILE}")
            if(MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED MATCHES "(^|/)\\.\\.(/|$)")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entries must not contain parent-directory segments for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE}")
            endif()
            if(NOT MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED MATCHES "^${MK_DESKTOP_GAME_DIRECTORY}/.+")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entries must stay under ${MK_DESKTOP_GAME_DIRECTORY}/ for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE}")
            endif()
            if(NOT EXISTS "${CMAKE_SOURCE_DIR}/${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entry does not exist for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}")
            endif()
            file(REAL_PATH "${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}" MK_DESKTOP_GAME_PACKAGE_FILE_REAL BASE_DIRECTORY "${CMAKE_SOURCE_DIR}")
            if(IS_DIRECTORY "${MK_DESKTOP_GAME_PACKAGE_FILE_REAL}")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entries must be files, not directories, for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}")
            endif()
            file(SIZE "${MK_DESKTOP_GAME_PACKAGE_FILE_REAL}" MK_DESKTOP_GAME_PACKAGE_FILE_SIZE)
            list(FIND MK_DESKTOP_GAME_PACKAGE_FILE_REAL_PATHS "${MK_DESKTOP_GAME_PACKAGE_FILE_REAL}" MK_DESKTOP_GAME_PACKAGE_FILE_DUPLICATE)
            if(NOT MK_DESKTOP_GAME_PACKAGE_FILE_DUPLICATE EQUAL -1)
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entry resolves to a duplicate source file for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}")
            endif()
            file(RELATIVE_PATH MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE
                "${MK_DESKTOP_GAME_DIRECTORY_REAL}"
                "${MK_DESKTOP_GAME_PACKAGE_FILE_REAL}"
            )
            string(REPLACE "\\" "/" MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE "${MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE}")
            if(IS_ABSOLUTE "${MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE}" OR
               MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE STREQUAL ".." OR
               MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE MATCHES "^\\.\\./")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entry escaped the game directory for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}")
            endif()
            file(RELATIVE_PATH MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE
                "${CMAKE_SOURCE_DIR}"
                "${MK_DESKTOP_GAME_PACKAGE_FILE_REAL}"
            )
            string(REPLACE "\\" "/" MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE "${MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE}")
            if(IS_ABSOLUTE "${MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE}" OR
               MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE STREQUAL ".." OR
               MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE MATCHES "^\\.\\./" OR
               NOT MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE MATCHES "^${MK_DESKTOP_GAME_DIRECTORY}/.+")
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entry must resolve inside ${MK_DESKTOP_GAME_DIRECTORY}/ for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE_NORMALIZED}")
            endif()
            set(MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATH "${CMAKE_INSTALL_BINDIR}/${MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE}")
            list(FIND MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATHS "${MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATH}" MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_DUPLICATE)
            if(NOT MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_DUPLICATE EQUAL -1)
                message(FATAL_ERROR "MK_add_desktop_runtime_game PACKAGE_FILES entries resolve to duplicate install paths for ${target_name}: ${MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATH}")
            endif()
            list(APPEND MK_DESKTOP_GAME_PACKAGE_FILE_REAL_PATHS "${MK_DESKTOP_GAME_PACKAGE_FILE_REAL}")
            list(APPEND MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED "${MK_DESKTOP_GAME_PACKAGE_FILE_REPOSITORY_RELATIVE}")
            list(APPEND MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE_PATHS "${MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE}")
            list(APPEND MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATHS "${MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATH}")
        endforeach()

        add_executable(${target_name}
            ${MK_DESKTOP_GAME_SOURCES}
        )
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${target_name}"
        )

        target_link_libraries(${target_name}
            PRIVATE
                MK_core
                MK_math
                MK_platform
                MK_platform_sdl3
                MK_renderer
                MK_runtime_host
                MK_runtime_host_sdl3
                MK_runtime_host_sdl3_presentation
                SDL3::SDL3
        )

        MK_apply_common_target_options(${target_name})
        set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_GAME TRUE)
        set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_GAME_MANIFEST "${MK_DESKTOP_GAME_GAME_MANIFEST}")
        set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_SMOKE_ARGS "${MK_DESKTOP_GAME_SMOKE_ARGS}")
        set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_PACKAGE_SMOKE_ARGS "${MK_DESKTOP_GAME_PACKAGE_SMOKE_ARGS}")
        set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_PACKAGE_FILES "${MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED}")
        set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_PACKAGE_FILE_INSTALL_PATHS "${MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATHS}")
        if(MK_DESKTOP_GAME_REQUIRES_D3D12_SHADERS)
            set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS TRUE)
        else()
            set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS FALSE)
        endif()
        if(MK_DESKTOP_GAME_REQUIRES_VULKAN_SHADERS)
            set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS TRUE)
        else()
            set_property(TARGET ${target_name} PROPERTY MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS FALSE)
        endif()
        get_property(MK_DESKTOP_RUNTIME_GAME_TARGETS GLOBAL PROPERTY MK_DESKTOP_RUNTIME_GAMES)
        list(APPEND MK_DESKTOP_RUNTIME_GAME_TARGETS ${target_name})
        set_property(GLOBAL PROPERTY MK_DESKTOP_RUNTIME_GAMES "${MK_DESKTOP_RUNTIME_GAME_TARGETS}")

        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SDL3::SDL3>
                $<TARGET_FILE_DIR:${target_name}>
        )

        if(BUILD_TESTING)
            add_test(NAME ${target_name}_smoke COMMAND ${target_name} ${MK_DESKTOP_GAME_SMOKE_ARGS})
        endif()

        if(MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED)
            set(MK_DESKTOP_GAME_PACKAGE_STAGE_COMMANDS "")
            list(LENGTH MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED MK_DESKTOP_GAME_PACKAGE_FILE_COUNT)
            math(EXPR MK_DESKTOP_GAME_PACKAGE_FILE_LAST_INDEX "${MK_DESKTOP_GAME_PACKAGE_FILE_COUNT} - 1")
            foreach(MK_DESKTOP_GAME_PACKAGE_FILE_INDEX RANGE 0 ${MK_DESKTOP_GAME_PACKAGE_FILE_LAST_INDEX})
                list(GET MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED ${MK_DESKTOP_GAME_PACKAGE_FILE_INDEX} MK_DESKTOP_GAME_PACKAGE_SOURCE)
                list(GET MK_DESKTOP_GAME_PACKAGE_FILE_RELATIVE_PATHS ${MK_DESKTOP_GAME_PACKAGE_FILE_INDEX} MK_DESKTOP_GAME_PACKAGE_RUNTIME_RELATIVE)
                get_filename_component(MK_DESKTOP_GAME_PACKAGE_RUNTIME_DIRECTORY "${MK_DESKTOP_GAME_PACKAGE_RUNTIME_RELATIVE}" DIRECTORY)
                list(APPEND MK_DESKTOP_GAME_PACKAGE_STAGE_COMMANDS
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        "$<TARGET_FILE_DIR:${target_name}>/${MK_DESKTOP_GAME_PACKAGE_RUNTIME_DIRECTORY}"
                    COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${CMAKE_SOURCE_DIR}/${MK_DESKTOP_GAME_PACKAGE_SOURCE}"
                        "$<TARGET_FILE_DIR:${target_name}>/${MK_DESKTOP_GAME_PACKAGE_RUNTIME_RELATIVE}"
                )
            endforeach()
            add_custom_target(${target_name}_runtime_files
                ${MK_DESKTOP_GAME_PACKAGE_STAGE_COMMANDS}
                DEPENDS ${MK_DESKTOP_GAME_PACKAGE_FILE_REAL_PATHS}
                VERBATIM
            )
            add_dependencies(${target_name} ${target_name}_runtime_files)
        endif()

        if(MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED AND MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL target_name)
            list(LENGTH MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED MK_DESKTOP_GAME_PACKAGE_FILE_COUNT)
            math(EXPR MK_DESKTOP_GAME_PACKAGE_FILE_LAST_INDEX "${MK_DESKTOP_GAME_PACKAGE_FILE_COUNT} - 1")
            foreach(MK_DESKTOP_GAME_PACKAGE_FILE_INDEX RANGE 0 ${MK_DESKTOP_GAME_PACKAGE_FILE_LAST_INDEX})
                list(GET MK_DESKTOP_GAME_PACKAGE_FILES_NORMALIZED ${MK_DESKTOP_GAME_PACKAGE_FILE_INDEX} MK_DESKTOP_GAME_PACKAGE_SOURCE)
                list(GET MK_DESKTOP_GAME_PACKAGE_FILE_INSTALL_PATHS ${MK_DESKTOP_GAME_PACKAGE_FILE_INDEX} MK_DESKTOP_GAME_PACKAGE_INSTALL)
                get_filename_component(MK_DESKTOP_GAME_PACKAGE_INSTALL_DIRECTORY "${MK_DESKTOP_GAME_PACKAGE_INSTALL}" DIRECTORY)
                install(FILES
                    "${CMAKE_SOURCE_DIR}/${MK_DESKTOP_GAME_PACKAGE_SOURCE}"
                    DESTINATION "${MK_DESKTOP_GAME_PACKAGE_INSTALL_DIRECTORY}"
                )
            endforeach()
        endif()
    endfunction()

    if(WIN32)
        install(FILES
            $<TARGET_FILE:SDL3::SDL3>
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()

    MK_add_desktop_runtime_game(sample_desktop_runtime_shell
        SOURCES
            sample_desktop_runtime_shell/main.cpp
        GAME_MANIFEST
            games/sample_desktop_runtime_shell/game.agent.json
        SMOKE_ARGS
            --smoke
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-d3d12-shaders
            --require-vulkan-shaders
        REQUIRES_D3D12_SHADERS
        REQUIRES_VULKAN_SHADERS
    )
    set_property(TARGET sample_desktop_runtime_shell PROPERTY MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS
        "shaders/runtime_shell.vs.dxil;shaders/runtime_shell.ps.dxil"
    )
    set_property(TARGET sample_desktop_runtime_shell PROPERTY MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS
        "shaders/runtime_shell.vs.spv;shaders/runtime_shell.ps.spv"
    )

    function(MK_configure_desktop_runtime_scene_shader_artifacts)
        cmake_parse_arguments(MK_SCENE_SHADER "" "TARGET;GAME_NAME;SCENE_SHADER_SOURCE;POSTPROCESS_SHADER_SOURCE;MORPH_VERTEX_ENTRY;COMPUTE_MORPH_VERTEX_ENTRY;COMPUTE_MORPH_ENTRY;COMPUTE_MORPH_TANGENT_VERTEX_ENTRY;COMPUTE_MORPH_TANGENT_ENTRY;COMPUTE_MORPH_SKINNED_VERTEX_ENTRY;COMPUTE_MORPH_SKINNED_ENTRY;SHADOW_SHADER_SOURCE;SHADOW_RECEIVER_ENTRY;SHADOW_VERTEX_ENTRY;SHADOW_FRAGMENT_ENTRY;UI_OVERLAY_SHADER_SOURCE" "" ${ARGN})
        foreach(MK_SCENE_SHADER_REQUIRED_ARG IN ITEMS TARGET GAME_NAME SCENE_SHADER_SOURCE POSTPROCESS_SHADER_SOURCE)
            if(NOT MK_SCENE_SHADER_${MK_SCENE_SHADER_REQUIRED_ARG})
                message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts requires ${MK_SCENE_SHADER_REQUIRED_ARG}")
            endif()
        endforeach()
        if(NOT TARGET ${MK_SCENE_SHADER_TARGET})
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts TARGET does not exist: ${MK_SCENE_SHADER_TARGET}")
        endif()
        if(NOT EXISTS "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}")
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts missing scene shader source for ${MK_SCENE_SHADER_TARGET}: ${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}")
        endif()
        if(NOT EXISTS "${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}")
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts missing postprocess shader source for ${MK_SCENE_SHADER_TARGET}: ${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}")
        endif()
        set(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS OFF)
        if(MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE)
            if(NOT EXISTS "${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}")
                message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts missing UI overlay shader source for ${MK_SCENE_SHADER_TARGET}: ${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}")
            endif()
            set(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS ON)
        endif()
        if((MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_ENTRY AND NOT MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY) OR
           (MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY AND NOT MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_ENTRY))
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts compute morph entries must be provided together for ${MK_SCENE_SHADER_TARGET}")
        endif()
        if((MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_ENTRY AND NOT MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY) OR
           (MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY AND NOT MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_ENTRY))
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts compute morph tangent-frame entries must be provided together for ${MK_SCENE_SHADER_TARGET}")
        endif()
        if((MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_ENTRY AND NOT MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY) OR
           (MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY AND NOT MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_ENTRY))
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts compute morph skinned entries must be provided together for ${MK_SCENE_SHADER_TARGET}")
        endif()
        set(MK_SCENE_SHADER_SHADOW_ARG_COUNT 0)
        foreach(MK_SCENE_SHADER_SHADOW_ARG IN ITEMS SHADOW_SHADER_SOURCE SHADOW_RECEIVER_ENTRY SHADOW_VERTEX_ENTRY SHADOW_FRAGMENT_ENTRY)
            if(MK_SCENE_SHADER_${MK_SCENE_SHADER_SHADOW_ARG})
                math(EXPR MK_SCENE_SHADER_SHADOW_ARG_COUNT "${MK_SCENE_SHADER_SHADOW_ARG_COUNT} + 1")
            endif()
        endforeach()
        if(NOT MK_SCENE_SHADER_SHADOW_ARG_COUNT EQUAL 0 AND NOT MK_SCENE_SHADER_SHADOW_ARG_COUNT EQUAL 4)
            message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts shadow source and entries must be provided together for ${MK_SCENE_SHADER_TARGET}")
        endif()
        set(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS OFF)
        if(MK_SCENE_SHADER_SHADOW_SHADER_SOURCE)
            if(NOT EXISTS "${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}")
                message(FATAL_ERROR "MK_configure_desktop_runtime_scene_shader_artifacts missing shadow shader source for ${MK_SCENE_SHADER_TARGET}: ${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}")
            endif()
            set(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS ON)
        endif()

        set(MK_SCENE_SHADER_D3D12_ARTIFACTS
            "shaders/${MK_SCENE_SHADER_TARGET}_scene.vs.dxil;shaders/${MK_SCENE_SHADER_TARGET}_scene.ps.dxil;shaders/${MK_SCENE_SHADER_TARGET}_postprocess.vs.dxil;shaders/${MK_SCENE_SHADER_TARGET}_postprocess.ps.dxil"
        )
        set(MK_SCENE_SHADER_VULKAN_ARTIFACTS
            "shaders/${MK_SCENE_SHADER_TARGET}_scene.vs.spv;shaders/${MK_SCENE_SHADER_TARGET}_scene.ps.spv;shaders/${MK_SCENE_SHADER_TARGET}_postprocess.vs.spv;shaders/${MK_SCENE_SHADER_TARGET}_postprocess.ps.spv"
        )
        if(MK_SCENE_SHADER_MORPH_VERTEX_ENTRY)
            list(APPEND MK_SCENE_SHADER_D3D12_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_morph.vs.dxil"
            )
            list(APPEND MK_SCENE_SHADER_VULKAN_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_morph.vs.spv"
            )
        endif()
        if(MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY)
            list(APPEND MK_SCENE_SHADER_D3D12_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.vs.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.cs.dxil"
            )
            list(APPEND MK_SCENE_SHADER_VULKAN_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.vs.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.cs.spv"
            )
        endif()
        if(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY)
            list(APPEND MK_SCENE_SHADER_D3D12_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.vs.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.cs.dxil"
            )
            list(APPEND MK_SCENE_SHADER_VULKAN_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.vs.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.cs.spv"
            )
        endif()
        if(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY)
            list(APPEND MK_SCENE_SHADER_D3D12_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.vs.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.cs.dxil"
            )
            list(APPEND MK_SCENE_SHADER_VULKAN_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.vs.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.cs.spv"
            )
        endif()
        if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
            list(APPEND MK_SCENE_SHADER_D3D12_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow_receiver.ps.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow_receiver_shifted.ps.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow.vs.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow.ps.dxil"
            )
            list(APPEND MK_SCENE_SHADER_VULKAN_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow_receiver.ps.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow_receiver_shifted.ps.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow.vs.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_shadow.ps.spv"
            )
        endif()
        if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
            list(APPEND MK_SCENE_SHADER_D3D12_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.vs.dxil"
                "shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.ps.dxil"
            )
            list(APPEND MK_SCENE_SHADER_VULKAN_ARTIFACTS
                "shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.vs.spv"
                "shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.ps.spv"
            )
        endif()
        set_property(TARGET ${MK_SCENE_SHADER_TARGET} PROPERTY MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS
            "${MK_SCENE_SHADER_D3D12_ARTIFACTS}"
        )
        set_property(TARGET ${MK_SCENE_SHADER_TARGET} PROPERTY MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS
            "${MK_SCENE_SHADER_VULKAN_ARTIFACTS}"
        )

        if(NOT WIN32)
            return()
        endif()

        set(MK_SCENE_SHADER_REQUIRES_DXIL OFF)
        set(MK_SCENE_SHADER_REQUIRES_SPIRV OFF)
        if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "${MK_SCENE_SHADER_TARGET}")
            get_target_property(MK_SCENE_SHADER_REQUIRES_DXIL_FOR_PACKAGE
                ${MK_SCENE_SHADER_TARGET}
                MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS
            )
            if(MK_SCENE_SHADER_REQUIRES_DXIL_FOR_PACKAGE)
                set(MK_SCENE_SHADER_REQUIRES_DXIL ON)
            endif()
            get_target_property(MK_SCENE_SHADER_REQUIRES_SPIRV_FOR_PACKAGE
                ${MK_SCENE_SHADER_TARGET}
                MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS
            )
            if(MK_SCENE_SHADER_REQUIRES_SPIRV_FOR_PACKAGE)
                set(MK_SCENE_SHADER_REQUIRES_SPIRV ON)
            endif()
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_DXIL)
            set(MK_SCENE_SHADER_REQUIRES_DXIL ON)
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_SPIRV)
            set(MK_SCENE_SHADER_REQUIRES_SPIRV ON)
        endif()

        if(MK_SCENE_SHADER_REQUIRES_DXIL AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            message(FATAL_ERROR "DXC is required to package ${MK_SCENE_SHADER_TARGET} scene shader artifacts")
        endif()
        if(MK_SCENE_SHADER_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
            message(FATAL_ERROR "DXC with SPIR-V CodeGen is required to package ${MK_SCENE_SHADER_TARGET} Vulkan scene shader artifacts")
        endif()
        if(MK_SCENE_SHADER_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            message(FATAL_ERROR "spirv-val is required to package ${MK_SCENE_SHADER_TARGET} Vulkan scene shader artifacts")
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            set(MK_SCENE_SHADER_OUTPUT_DIR
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${MK_SCENE_SHADER_TARGET}/shaders"
            )
            set(MK_SCENE_SHADER_VERTEX_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene.vs.dxil"
            )
            set(MK_SCENE_SHADER_FRAGMENT_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene.ps.dxil"
            )
            set(MK_SCENE_SHADER_POSTPROCESS_VERTEX_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_postprocess.vs.dxil"
            )
            set(MK_SCENE_SHADER_POSTPROCESS_FRAGMENT_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_postprocess.ps.dxil"
            )
            if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
                set(MK_SCENE_SHADER_UI_OVERLAY_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_ui_overlay.vs.dxil"
                )
                set(MK_SCENE_SHADER_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_ui_overlay.ps.dxil"
                )
            endif()
            if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
                set(MK_SCENE_SHADER_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow_receiver.ps.dxil"
                )
                set(MK_SCENE_SHADER_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow_receiver_shifted.ps.dxil"
                )
                set(MK_SCENE_SHADER_SHADOW_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow.vs.dxil"
                )
                set(MK_SCENE_SHADER_SHADOW_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow.ps.dxil"
                )
            endif()
            set(MK_SCENE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene.vs.spv"
            )
            set(MK_SCENE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene.ps.spv"
            )
            set(MK_SCENE_SHADER_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_postprocess.vs.spv"
            )
            set(MK_SCENE_SHADER_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT
                "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_postprocess.ps.spv"
            )
            if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
                set(MK_SCENE_SHADER_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_ui_overlay.vs.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_ui_overlay.ps.spv"
                )
            endif()
            if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
                set(MK_SCENE_SHADER_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow_receiver.ps.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow_receiver_shifted.ps.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow.vs.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_shadow.ps.spv"
                )
            endif()
            if(MK_SCENE_SHADER_MORPH_VERTEX_ENTRY)
                set(MK_SCENE_SHADER_MORPH_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_morph.vs.dxil"
                )
                set(MK_SCENE_SHADER_VULKAN_MORPH_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_morph.vs.spv"
                )
            endif()
            if(MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY)
                set(MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.vs.dxil"
                )
                set(MK_SCENE_SHADER_COMPUTE_MORPH_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.cs.dxil"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.vs.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph.cs.spv"
                )
            endif()
            if(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY)
                set(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.vs.dxil"
                )
                set(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.cs.dxil"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.vs.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_tangent_frame.cs.spv"
                )
            endif()
            if(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY)
                set(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.vs.dxil"
                )
                set(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.cs.dxil"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.vs.spv"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT
                    "${MK_SCENE_SHADER_OUTPUT_DIR}/${MK_SCENE_SHADER_TARGET}_scene_compute_morph_skinned.cs.spv"
                )
            endif()

            set(MK_SCENE_SHADER_D3D12_OUTPUTS
                "${MK_SCENE_SHADER_VERTEX_SHADER_OUTPUT}"
                "${MK_SCENE_SHADER_FRAGMENT_SHADER_OUTPUT}"
                "${MK_SCENE_SHADER_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                "${MK_SCENE_SHADER_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
            )
            set(MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_main -T vs_6_0
                    -Fo "${MK_SCENE_SHADER_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_main -T ps_6_0
                    -Fo "${MK_SCENE_SHADER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_postprocess -T vs_6_0
                    -Fo "${MK_SCENE_SHADER_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_postprocess -T ps_6_0
                    -Fo "${MK_SCENE_SHADER_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}"
            )
            if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
                list(APPEND MK_SCENE_SHADER_D3D12_OUTPUTS
                    "${MK_SCENE_SHADER_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                )
                list(APPEND MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_native_ui_overlay -T vs_6_0
                        -Fo "${MK_SCENE_SHADER_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_native_ui_overlay -T ps_6_0
                        -Fo "${MK_SCENE_SHADER_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}"
                )
            endif()
            if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
                list(APPEND MK_SCENE_SHADER_D3D12_OUTPUTS
                    "${MK_SCENE_SHADER_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_SHADOW_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                )
                list(APPEND MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_SHADOW_RECEIVER_ENTRY}" -T ps_6_0
                        -Fo "${MK_SCENE_SHADER_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -DGE_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS=1 -E "${MK_SCENE_SHADER_SHADOW_RECEIVER_ENTRY}" -T ps_6_0
                        -Fo "${MK_SCENE_SHADER_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_SHADOW_VERTEX_ENTRY}" -T vs_6_0
                        -Fo "${MK_SCENE_SHADER_SHADOW_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_SHADOW_FRAGMENT_ENTRY}" -T ps_6_0
                        -Fo "${MK_SCENE_SHADER_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}"
                )
            endif()
            if(MK_SCENE_SHADER_MORPH_VERTEX_ENTRY)
                list(APPEND MK_SCENE_SHADER_D3D12_OUTPUTS
                    "${MK_SCENE_SHADER_MORPH_VERTEX_SHADER_OUTPUT}"
                )
                list(APPEND MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_MORPH_VERTEX_ENTRY}" -T vs_6_0
                        -Fo "${MK_SCENE_SHADER_MORPH_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                )
            endif()
            if(MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY)
                list(APPEND MK_SCENE_SHADER_D3D12_OUTPUTS
                    "${MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_COMPUTE_MORPH_SHADER_OUTPUT}"
                )
                list(APPEND MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_ENTRY}" -T vs_6_0
                        -Fo "${MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY}" -T cs_6_0
                        -Fo "${MK_SCENE_SHADER_COMPUTE_MORPH_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                )
            endif()
            if(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY)
                list(APPEND MK_SCENE_SHADER_D3D12_OUTPUTS
                    "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT}"
                )
                list(APPEND MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_ENTRY}" -T vs_6_0
                        -Fo "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY}" -T cs_6_0
                        -Fo "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                )
            endif()
            if(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY)
                list(APPEND MK_SCENE_SHADER_D3D12_OUTPUTS
                    "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT}"
                )
                list(APPEND MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_ENTRY}" -T vs_6_0
                        -Fo "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY}" -T cs_6_0
                        -Fo "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                )
            endif()

            set(MK_SCENE_SHADER_SOURCE_DEPENDS
                "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                "${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}"
            )
            if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
                list(APPEND MK_SCENE_SHADER_SOURCE_DEPENDS
                    "${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}"
                )
            endif()
            if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
                list(APPEND MK_SCENE_SHADER_SOURCE_DEPENDS
                    "${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}"
                )
            endif()

            add_custom_command(
                OUTPUT
                    ${MK_SCENE_SHADER_D3D12_OUTPUTS}
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${MK_SCENE_SHADER_OUTPUT_DIR}"
                ${MK_SCENE_SHADER_D3D12_COMPILE_COMMANDS}
                DEPENDS
                    ${MK_SCENE_SHADER_SOURCE_DEPENDS}
                COMMENT "Compiling ${MK_SCENE_SHADER_TARGET} D3D12 scene shader artifacts"
                VERBATIM
            )
            add_custom_target(${MK_SCENE_SHADER_TARGET}_shaders
                DEPENDS
                    ${MK_SCENE_SHADER_D3D12_OUTPUTS}
            )
            if(BUILD_TESTING)
                get_target_property(MK_SCENE_SHADER_SOURCE_SMOKE_ARGS
                    ${MK_SCENE_SHADER_TARGET}
                    MK_DESKTOP_RUNTIME_SMOKE_ARGS
                )
                add_dependencies(${MK_SCENE_SHADER_TARGET} ${MK_SCENE_SHADER_TARGET}_shaders)
                add_test(
                    NAME ${MK_SCENE_SHADER_TARGET}_shader_artifacts_smoke
                    COMMAND ${MK_SCENE_SHADER_TARGET}
                        ${MK_SCENE_SHADER_SOURCE_SMOKE_ARGS}
                        --require-d3d12-scene-shaders
                )
            endif()

            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
                set(MK_SCENE_SHADER_VULKAN_OUTPUTS
                    "${MK_SCENE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                    "${MK_SCENE_SHADER_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                )
                set(MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_main -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SCENE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_main -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SCENE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_postprocess -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SCENE_SHADER_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_postprocess -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SCENE_SHADER_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_POSTPROCESS_SHADER_SOURCE}"
                )
                if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
                    list(APPEND MK_SCENE_SHADER_VULKAN_OUTPUTS
                        "${MK_SCENE_SHADER_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    )
                    list(APPEND MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E vs_native_ui_overlay -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E ps_native_ui_overlay -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_UI_OVERLAY_SHADER_SOURCE}"
                    )
                endif()
                if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
                    list(APPEND MK_SCENE_SHADER_VULKAN_OUTPUTS
                        "${MK_SCENE_SHADER_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                    )
                    list(APPEND MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_SHADOW_RECEIVER_ENTRY}" -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -DGE_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS=1 -E "${MK_SCENE_SHADER_SHADOW_RECEIVER_ENTRY}" -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_SHADOW_VERTEX_ENTRY}" -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_SHADOW_FRAGMENT_ENTRY}" -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SHADOW_SHADER_SOURCE}"
                    )
                endif()
                if(MK_SCENE_SHADER_MORPH_VERTEX_ENTRY)
                    list(APPEND MK_SCENE_SHADER_VULKAN_OUTPUTS
                        "${MK_SCENE_SHADER_VULKAN_MORPH_VERTEX_SHADER_OUTPUT}"
                    )
                    list(APPEND MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_MORPH_VERTEX_ENTRY}" -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_MORPH_VERTEX_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    )
                endif()
                if(MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY)
                    list(APPEND MK_SCENE_SHADER_VULKAN_OUTPUTS
                        "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SHADER_OUTPUT}"
                    )
                    list(APPEND MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_VERTEX_ENTRY}" -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY}" -T cs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    )
                endif()
                if(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY)
                    list(APPEND MK_SCENE_SHADER_VULKAN_OUTPUTS
                        "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT}"
                    )
                    list(APPEND MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_VERTEX_ENTRY}" -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY}" -T cs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    )
                endif()
                if(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY)
                    list(APPEND MK_SCENE_SHADER_VULKAN_OUTPUTS
                        "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT}"
                        "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT}"
                    )
                    list(APPEND MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_VERTEX_ENTRY}" -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                            -nologo -E "${MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY}" -T cs_6_0 -spirv -fspv-target-env=vulkan1.3
                            -Fo "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT}"
                            "${MK_SCENE_SHADER_SCENE_SHADER_SOURCE}"
                    )
                endif()
                add_custom_command(
                    OUTPUT
                        ${MK_SCENE_SHADER_VULKAN_OUTPUTS}
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${MK_SCENE_SHADER_OUTPUT_DIR}"
                    ${MK_SCENE_SHADER_VULKAN_COMPILE_COMMANDS}
                    DEPENDS
                        ${MK_SCENE_SHADER_SOURCE_DEPENDS}
                    COMMENT "Compiling ${MK_SCENE_SHADER_TARGET} Vulkan scene SPIR-V shader artifacts"
                    VERBATIM
                )
                add_custom_target(${MK_SCENE_SHADER_TARGET}_vulkan_shaders
                    DEPENDS
                        ${MK_SCENE_SHADER_VULKAN_OUTPUTS}
                )
                if(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
                    set(MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SCENE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SCENE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SCENE_SHADER_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SCENE_SHADER_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                    )
                    if(MK_SCENE_SHADER_HAS_UI_OVERLAY_ARTIFACTS)
                        list(APPEND MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        )
                    endif()
                    if(MK_SCENE_SHADER_HAS_SHADOW_ARTIFACTS)
                        list(APPEND MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_SHIFTED_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        )
                    endif()
                    if(MK_SCENE_SHADER_MORPH_VERTEX_ENTRY)
                        list(APPEND MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_MORPH_VERTEX_SHADER_OUTPUT}"
                        )
                    endif()
                    if(MK_SCENE_SHADER_COMPUTE_MORPH_ENTRY)
                        list(APPEND MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_VERTEX_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SHADER_OUTPUT}"
                        )
                    endif()
                    if(MK_SCENE_SHADER_COMPUTE_MORPH_TANGENT_ENTRY)
                        list(APPEND MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_VERTEX_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_TANGENT_SHADER_OUTPUT}"
                        )
                    endif()
                    if(MK_SCENE_SHADER_COMPUTE_MORPH_SKINNED_ENTRY)
                        list(APPEND MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_VERTEX_SHADER_OUTPUT}"
                            COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                                "${MK_SCENE_SHADER_VULKAN_COMPUTE_MORPH_SKINNED_SHADER_OUTPUT}"
                        )
                    endif()
                    add_custom_target(${MK_SCENE_SHADER_TARGET}_vulkan_shader_validation
                        ${MK_SCENE_SHADER_VULKAN_VALIDATION_COMMANDS}
                        DEPENDS
                            ${MK_SCENE_SHADER_TARGET}_vulkan_shaders
                        COMMENT "Validating ${MK_SCENE_SHADER_TARGET} Vulkan scene SPIR-V shader artifacts"
                        VERBATIM
                    )
                endif()
            endif()

            if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "${MK_SCENE_SHADER_TARGET}")
                add_dependencies(${MK_SCENE_SHADER_TARGET} ${MK_SCENE_SHADER_TARGET}_shaders)
                install(FILES
                    ${MK_SCENE_SHADER_D3D12_OUTPUTS}
                    DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                )
                if(MK_SCENE_SHADER_REQUIRES_SPIRV)
                    if(TARGET ${MK_SCENE_SHADER_TARGET}_vulkan_shader_validation)
                        add_dependencies(${MK_SCENE_SHADER_TARGET} ${MK_SCENE_SHADER_TARGET}_vulkan_shader_validation)
                    else()
                        add_dependencies(${MK_SCENE_SHADER_TARGET} ${MK_SCENE_SHADER_TARGET}_vulkan_shaders)
                    endif()
                    install(FILES
                        ${MK_SCENE_SHADER_VULKAN_OUTPUTS}
                        DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                    )
                endif()
            endif()
        endif()
    endfunction()

    function(MK_configure_desktop_runtime_2d_sprite_shader_artifacts)
        cmake_parse_arguments(MK_SPRITE_SHADER "" "TARGET;SPRITE_SHADER_SOURCE" "" ${ARGN})
        foreach(MK_SPRITE_SHADER_REQUIRED_ARG IN ITEMS TARGET SPRITE_SHADER_SOURCE)
            if(NOT MK_SPRITE_SHADER_${MK_SPRITE_SHADER_REQUIRED_ARG})
                message(FATAL_ERROR "MK_configure_desktop_runtime_2d_sprite_shader_artifacts requires ${MK_SPRITE_SHADER_REQUIRED_ARG}")
            endif()
        endforeach()
        if(NOT TARGET ${MK_SPRITE_SHADER_TARGET})
            message(FATAL_ERROR "MK_configure_desktop_runtime_2d_sprite_shader_artifacts TARGET does not exist: ${MK_SPRITE_SHADER_TARGET}")
        endif()
        if(NOT EXISTS "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}")
            message(FATAL_ERROR "MK_configure_desktop_runtime_2d_sprite_shader_artifacts missing sprite shader source for ${MK_SPRITE_SHADER_TARGET}: ${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}")
        endif()

        set(MK_SPRITE_SHADER_D3D12_ARTIFACTS
            "shaders/${MK_SPRITE_SHADER_TARGET}_sprite.vs.dxil;shaders/${MK_SPRITE_SHADER_TARGET}_sprite.ps.dxil;shaders/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.vs.dxil;shaders/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.ps.dxil"
        )
        set(MK_SPRITE_SHADER_VULKAN_ARTIFACTS
            "shaders/${MK_SPRITE_SHADER_TARGET}_sprite.vs.spv;shaders/${MK_SPRITE_SHADER_TARGET}_sprite.ps.spv;shaders/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.vs.spv;shaders/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.ps.spv"
        )
        set_property(TARGET ${MK_SPRITE_SHADER_TARGET} PROPERTY MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS
            "${MK_SPRITE_SHADER_D3D12_ARTIFACTS}"
        )
        set_property(TARGET ${MK_SPRITE_SHADER_TARGET} PROPERTY MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS
            "${MK_SPRITE_SHADER_VULKAN_ARTIFACTS}"
        )

        if(NOT WIN32)
            return()
        endif()

        set(MK_SPRITE_SHADER_REQUIRES_DXIL OFF)
        set(MK_SPRITE_SHADER_REQUIRES_SPIRV OFF)
        if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "${MK_SPRITE_SHADER_TARGET}")
            get_target_property(MK_SPRITE_SHADER_REQUIRES_DXIL_FOR_PACKAGE
                ${MK_SPRITE_SHADER_TARGET}
                MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS
            )
            if(MK_SPRITE_SHADER_REQUIRES_DXIL_FOR_PACKAGE)
                set(MK_SPRITE_SHADER_REQUIRES_DXIL ON)
            endif()
            get_target_property(MK_SPRITE_SHADER_REQUIRES_SPIRV_FOR_PACKAGE
                ${MK_SPRITE_SHADER_TARGET}
                MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS
            )
            if(MK_SPRITE_SHADER_REQUIRES_SPIRV_FOR_PACKAGE)
                set(MK_SPRITE_SHADER_REQUIRES_SPIRV ON)
            endif()
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_DXIL)
            set(MK_SPRITE_SHADER_REQUIRES_DXIL ON)
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_SPIRV)
            set(MK_SPRITE_SHADER_REQUIRES_SPIRV ON)
        endif()

        if(MK_SPRITE_SHADER_REQUIRES_DXIL AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            message(FATAL_ERROR "DXC is required to package ${MK_SPRITE_SHADER_TARGET} 2D sprite shader artifacts")
        endif()
        if(MK_SPRITE_SHADER_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
            message(FATAL_ERROR "DXC with SPIR-V CodeGen is required to package ${MK_SPRITE_SHADER_TARGET} Vulkan 2D sprite shader artifacts")
        endif()
        if(MK_SPRITE_SHADER_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            message(FATAL_ERROR "spirv-val is required to package ${MK_SPRITE_SHADER_TARGET} Vulkan 2D sprite shader artifacts")
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            set(MK_SPRITE_SHADER_OUTPUT_DIR
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/${MK_SPRITE_SHADER_TARGET}/shaders"
            )
            set(MK_SPRITE_SHADER_VERTEX_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_sprite.vs.dxil"
            )
            set(MK_SPRITE_SHADER_FRAGMENT_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_sprite.ps.dxil"
            )
            set(MK_SPRITE_SHADER_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.vs.dxil"
            )
            set(MK_SPRITE_SHADER_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.ps.dxil"
            )
            set(MK_SPRITE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_sprite.vs.spv"
            )
            set(MK_SPRITE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_sprite.ps.spv"
            )
            set(MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.vs.spv"
            )
            set(MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT
                "${MK_SPRITE_SHADER_OUTPUT_DIR}/${MK_SPRITE_SHADER_TARGET}_native_sprite_overlay.ps.spv"
            )

            add_custom_command(
                OUTPUT
                    "${MK_SPRITE_SHADER_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${MK_SPRITE_SHADER_OUTPUT_DIR}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_main -T vs_6_0
                    -Fo "${MK_SPRITE_SHADER_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_main -T ps_6_0
                    -Fo "${MK_SPRITE_SHADER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_native_sprite_overlay -T vs_6_0
                    -Fo "${MK_SPRITE_SHADER_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_native_sprite_overlay -T ps_6_0
                    -Fo "${MK_SPRITE_SHADER_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                DEPENDS
                    "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                COMMENT "Compiling ${MK_SPRITE_SHADER_TARGET} D3D12 2D sprite shader artifacts"
                VERBATIM
            )
            add_custom_target(${MK_SPRITE_SHADER_TARGET}_shaders
                DEPENDS
                    "${MK_SPRITE_SHADER_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
            )
            if(BUILD_TESTING)
                get_target_property(MK_SPRITE_SHADER_SOURCE_SMOKE_ARGS
                    ${MK_SPRITE_SHADER_TARGET}
                    MK_DESKTOP_RUNTIME_SMOKE_ARGS
                )
                add_dependencies(${MK_SPRITE_SHADER_TARGET} ${MK_SPRITE_SHADER_TARGET}_shaders)
                add_test(
                    NAME ${MK_SPRITE_SHADER_TARGET}_shader_artifacts_smoke
                    COMMAND ${MK_SPRITE_SHADER_TARGET}
                        ${MK_SPRITE_SHADER_SOURCE_SMOKE_ARGS}
                        --require-d3d12-shaders
                )
            endif()

            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
                add_custom_command(
                    OUTPUT
                        "${MK_SPRITE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${MK_SPRITE_SHADER_OUTPUT_DIR}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_main -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SPRITE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_main -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SPRITE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_native_sprite_overlay -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_native_sprite_overlay -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                    DEPENDS
                        "${MK_SPRITE_SHADER_SPRITE_SHADER_SOURCE}"
                    COMMENT "Compiling ${MK_SPRITE_SHADER_TARGET} Vulkan 2D sprite SPIR-V shader artifacts"
                    VERBATIM
                )
                add_custom_target(${MK_SPRITE_SHADER_TARGET}_vulkan_shaders
                    DEPENDS
                        "${MK_SPRITE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                )
                if(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
                    add_custom_target(${MK_SPRITE_SHADER_TARGET}_vulkan_shader_validation
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SPRITE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SPRITE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        DEPENDS
                            ${MK_SPRITE_SHADER_TARGET}_vulkan_shaders
                        COMMENT "Validating ${MK_SPRITE_SHADER_TARGET} Vulkan 2D sprite SPIR-V shader artifacts"
                        VERBATIM
                    )
                endif()
                if(BUILD_TESTING)
                    add_dependencies(${MK_SPRITE_SHADER_TARGET} ${MK_SPRITE_SHADER_TARGET}_vulkan_shaders)
                    add_test(
                        NAME ${MK_SPRITE_SHADER_TARGET}_vulkan_shader_artifacts_smoke
                        COMMAND ${MK_SPRITE_SHADER_TARGET}
                            ${MK_SPRITE_SHADER_SOURCE_SMOKE_ARGS}
                            --require-vulkan-shaders
                    )
                endif()
            endif()

            if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "${MK_SPRITE_SHADER_TARGET}")
                add_dependencies(${MK_SPRITE_SHADER_TARGET} ${MK_SPRITE_SHADER_TARGET}_shaders)
                install(FILES
                    "${MK_SPRITE_SHADER_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SPRITE_SHADER_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                )
                if(MK_SPRITE_SHADER_REQUIRES_SPIRV)
                    if(TARGET ${MK_SPRITE_SHADER_TARGET}_vulkan_shader_validation)
                        add_dependencies(${MK_SPRITE_SHADER_TARGET} ${MK_SPRITE_SHADER_TARGET}_vulkan_shader_validation)
                    else()
                        add_dependencies(${MK_SPRITE_SHADER_TARGET} ${MK_SPRITE_SHADER_TARGET}_vulkan_shaders)
                    endif()
                    install(FILES
                        "${MK_SPRITE_SHADER_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SPRITE_SHADER_VULKAN_NATIVE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                    )
                endif()
            endif()
        endif()
    endfunction()

    if(WIN32)
        set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE "")
        if(DEFINED ENV{MK_SHADER_TOOLCHAIN_ROOT})
            set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_CONFIGURED "$ENV{MK_SHADER_TOOLCHAIN_ROOT}/dxc.exe")
            if(EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_CONFIGURED}")
                set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_CONFIGURED}")
            endif()
            if(NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
                set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_CONFIGURED "$ENV{MK_SHADER_TOOLCHAIN_ROOT}/Bin/dxc.exe")
                if(EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_CONFIGURED}")
                    set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_CONFIGURED}")
                endif()
            endif()
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE AND DEFINED ENV{VULKAN_SDK})
            set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_SDK "$ENV{VULKAN_SDK}/Bin/dxc.exe")
            if(EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_SDK}")
                set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_SDK}")
            endif()
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            file(GLOB MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_CANDIDATES
                "C:/VulkanSDK/*/Bin/dxc.exe"
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_CANDIDATES)
                list(SORT MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
                list(GET MK_SAMPLE_DESKTOP_RUNTIME_DXC_VULKAN_CANDIDATES 0 MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            endif()
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            find_program(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE NAMES dxc)
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            file(GLOB MK_SAMPLE_DESKTOP_RUNTIME_DXC_CANDIDATES
                "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxc.exe"
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_CANDIDATES)
                list(SORT MK_SAMPLE_DESKTOP_RUNTIME_DXC_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
                list(GET MK_SAMPLE_DESKTOP_RUNTIME_DXC_CANDIDATES 0 MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            endif()
        endif()

        set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV OFF)
        if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_SOURCE
                "${CMAKE_CURRENT_BINARY_DIR}/dxc-spirv-probe.hlsl"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_OUTPUT
                "${CMAKE_CURRENT_BINARY_DIR}/dxc-spirv-probe.spv"
            )
            file(WRITE "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_SOURCE}"
                "struct VSOut { float4 position : SV_Position; };\n"
                "VSOut vs_main(uint id : SV_VertexID) { VSOut o; o.position = float4(0.0, 0.0, 0.0, 1.0); return o; }\n"
            )
            execute_process(
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_main -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_SOURCE}"
                RESULT_VARIABLE MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_RESULT
                OUTPUT_VARIABLE MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_STDOUT
                ERROR_VARIABLE MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_STDERR
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_RESULT EQUAL 0 AND
               EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_SPIRV_PROBE_OUTPUT}")
                set(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV ON)
            endif()
        endif()

        set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE "")
        if(DEFINED ENV{MK_SHADER_TOOLCHAIN_ROOT})
            set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_CONFIGURED "$ENV{MK_SHADER_TOOLCHAIN_ROOT}/spirv-val.exe")
            if(EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_CONFIGURED}")
                set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_CONFIGURED}")
            endif()
            if(NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
                set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_CONFIGURED "$ENV{MK_SHADER_TOOLCHAIN_ROOT}/Bin/spirv-val.exe")
                if(EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_CONFIGURED}")
                    set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_CONFIGURED}")
                endif()
            endif()
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE AND DEFINED ENV{VULKAN_SDK})
            set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_SDK "$ENV{VULKAN_SDK}/Bin/spirv-val.exe")
            if(EXISTS "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_SDK}")
                set(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_SDK}")
            endif()
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            file(GLOB MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_CANDIDATES
                "C:/VulkanSDK/*/Bin/spirv-val.exe"
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_CANDIDATES)
                list(SORT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_CANDIDATES COMPARE NATURAL ORDER DESCENDING)
                list(GET MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_VULKAN_CANDIDATES 0 MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            endif()
        endif()
        if(NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            find_program(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE NAMES spirv-val)
        endif()

        set(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_DXIL ${MK_REQUIRE_DESKTOP_RUNTIME_DXIL})
        set(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_SPIRV ${MK_REQUIRE_DESKTOP_RUNTIME_SPIRV})
        if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "sample_desktop_runtime_shell")
            get_target_property(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_DXIL_FOR_PACKAGE
                sample_desktop_runtime_shell
                MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_DXIL_FOR_PACKAGE)
                set(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_DXIL ON)
            endif()
            get_target_property(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_SPIRV_FOR_PACKAGE
                sample_desktop_runtime_shell
                MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_SPIRV_FOR_PACKAGE)
                set(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_SPIRV ON)
            endif()
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_DXIL AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            message(FATAL_ERROR "DXC is required to package sample_desktop_runtime_shell shader artifacts")
        endif()
        if(MK_SAMPLE_DESKTOP_RUNTIME_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
            message(FATAL_ERROR "DXC with SPIR-V CodeGen is required to package sample_desktop_runtime_shell Vulkan shader artifacts")
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            set(MK_SAMPLE_DESKTOP_RUNTIME_INSTALL_SHADER_ARTIFACTS ON)
            if(NOT MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "" AND
               NOT MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "sample_desktop_runtime_shell")
                set(MK_SAMPLE_DESKTOP_RUNTIME_INSTALL_SHADER_ARTIFACTS OFF)
            endif()

            set(MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/sample_desktop_runtime_shell/shaders/runtime_shell.hlsl"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/sample_desktop_runtime_shell/shaders"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/runtime_shell.vs.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/runtime_shell.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/runtime_shell.vs.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/runtime_shell.ps.spv"
            )
            add_custom_command(
                OUTPUT
                    "${MK_SAMPLE_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_main -T vs_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_main -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE}"
                DEPENDS
                    "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE}"
                COMMENT "Compiling sample_desktop_runtime_shell D3D12 shader artifacts"
                VERBATIM
            )
            add_custom_target(sample_desktop_runtime_shell_shaders
                DEPENDS
                    "${MK_SAMPLE_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
                add_custom_command(
                    OUTPUT
                        "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_main -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_main -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    DEPENDS
                        "${MK_SAMPLE_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    COMMENT "Compiling sample_desktop_runtime_shell Vulkan SPIR-V shader artifacts"
                    VERBATIM
                )
                add_custom_target(sample_desktop_runtime_shell_vulkan_shaders
                    DEPENDS
                        "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                )
            endif()
            if(MK_SAMPLE_DESKTOP_RUNTIME_INSTALL_SHADER_ARTIFACTS)
                add_dependencies(sample_desktop_runtime_shell sample_desktop_runtime_shell_shaders)
                install(FILES
                    "${MK_SAMPLE_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                )
                if(TARGET sample_desktop_runtime_shell_vulkan_shaders)
                    add_dependencies(sample_desktop_runtime_shell sample_desktop_runtime_shell_vulkan_shaders)
                    install(FILES
                        "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                    )
                endif()
                add_test(
                    NAME sample_desktop_runtime_shell_shader_artifacts_smoke
                    COMMAND sample_desktop_runtime_shell --smoke --require-d3d12-shaders
                )
                if(TARGET sample_desktop_runtime_shell_vulkan_shaders)
                    add_test(
                        NAME sample_desktop_runtime_shell_vulkan_shader_artifacts_smoke
                        COMMAND sample_desktop_runtime_shell --smoke --require-vulkan-shaders
                    )
                endif()
            endif()
        else()
            message(STATUS
                "DXC not found; sample_desktop_runtime_shell will use NullRenderer fallback unless shader artifacts are supplied beside the executable."
            )
        endif()
    endif()

    MK_add_desktop_runtime_game(sample_desktop_runtime_game
        SOURCES
            sample_desktop_runtime_game/main.cpp
        GAME_MANIFEST
            games/sample_desktop_runtime_game/game.agent.json
        SMOKE_ARGS
            --smoke
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_desktop_runtime_game.config
            --require-scene-package
            runtime/sample_desktop_runtime_game.geindex
            --require-d3d12-scene-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-scene-gpu-bindings
            --require-postprocess
            --require-postprocess-depth-input
            --require-directional-shadow
            --require-directional-shadow-filtering
            --require-renderer-quality-gates
            --require-native-ui-overlay
            --require-native-ui-textured-sprite-atlas
            --require-gpu-skinning
            --require-quaternion-animation
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries(sample_desktop_runtime_game
        PRIVATE
            MK_animation
            MK_runtime
            MK_runtime_rhi
            MK_runtime_scene
            MK_scene
            MK_scene_renderer
            MK_ui
            MK_ui_renderer
    )

    MK_add_desktop_runtime_game(sample_2d_desktop_runtime_package
        SOURCES
            sample_2d_desktop_runtime_package/main.cpp
        GAME_MANIFEST
            games/sample_2d_desktop_runtime_package/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_2d_desktop_runtime_package.config
            --require-scene-package
            runtime/sample_2d_desktop_runtime_package.geindex
            --require-sprite-animation
            --require-tilemap-runtime-ux
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_2d_desktop_runtime_package.config
            --require-scene-package
            runtime/sample_2d_desktop_runtime_package.geindex
            --require-d3d12-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-native-2d-sprites
            --require-sprite-animation
            --require-tilemap-runtime-ux
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries(sample_2d_desktop_runtime_package
        PRIVATE
            MK_runtime
            MK_runtime_scene
            MK_scene
            MK_scene_renderer
            MK_ui
            MK_ui_renderer
            MK_audio
    )
    set_property(TARGET sample_2d_desktop_runtime_package PROPERTY MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS
        "shaders/sample_2d_desktop_runtime_package_sprite.vs.dxil;shaders/sample_2d_desktop_runtime_package_sprite.ps.dxil;shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil;shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil"
    )
    set_property(TARGET sample_2d_desktop_runtime_package PROPERTY MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS
        "shaders/sample_2d_desktop_runtime_package_sprite.vs.spv;shaders/sample_2d_desktop_runtime_package_sprite.ps.spv;shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv;shaders/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv"
    )
    if(WIN32)
        set(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_DXIL OFF)
        set(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV OFF)
        if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "sample_2d_desktop_runtime_package")
            get_target_property(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_DXIL_FOR_PACKAGE
                sample_2d_desktop_runtime_package
                MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS
            )
            if(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_DXIL_FOR_PACKAGE)
                set(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_DXIL ON)
            endif()
            get_target_property(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV_FOR_PACKAGE
                sample_2d_desktop_runtime_package
                MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS
            )
            if(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV_FOR_PACKAGE)
                set(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV ON)
            endif()
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_DXIL)
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_DXIL ON)
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_SPIRV)
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV ON)
        endif()

        if(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_DXIL AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            message(FATAL_ERROR "DXC is required to package sample_2d_desktop_runtime_package shader artifacts")
        endif()
        if(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
            message(FATAL_ERROR "DXC with SPIR-V CodeGen is required to package sample_2d_desktop_runtime_package Vulkan shader artifacts")
        endif()
        if(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            message(FATAL_ERROR "spirv-val is required to package sample_2d_desktop_runtime_package Vulkan shader artifacts")
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/sample_2d_desktop_runtime_package/shaders/runtime_2d_sprite.hlsl"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/sample_2d_desktop_runtime_package/shaders"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_sprite.vs.dxil"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_sprite.ps.dxil"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_sprite.vs.spv"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_sprite.ps.spv"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv"
            )
            set(MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}/sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv"
            )
            add_custom_command(
                OUTPUT
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_main -T vs_6_0
                    -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_main -T ps_6_0
                    -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_native_sprite_overlay -T vs_6_0
                    -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_native_sprite_overlay -T ps_6_0
                    -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                DEPENDS
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                COMMENT "Compiling sample_2d_desktop_runtime_package D3D12 shader artifacts"
                VERBATIM
            )
            add_custom_target(sample_2d_desktop_runtime_package_shaders
                DEPENDS
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
            )
            if(BUILD_TESTING)
                get_target_property(MK_SAMPLE_2D_DESKTOP_RUNTIME_SOURCE_SMOKE_ARGS
                    sample_2d_desktop_runtime_package
                    MK_DESKTOP_RUNTIME_SMOKE_ARGS
                )
                add_dependencies(sample_2d_desktop_runtime_package sample_2d_desktop_runtime_package_shaders)
                add_test(
                    NAME sample_2d_desktop_runtime_package_shader_artifacts_smoke
                    COMMAND sample_2d_desktop_runtime_package
                        ${MK_SAMPLE_2D_DESKTOP_RUNTIME_SOURCE_SMOKE_ARGS}
                        --require-d3d12-shaders
                )
            endif()

            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
                add_custom_command(
                    OUTPUT
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_OUTPUT_DIR}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_main -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_main -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_native_sprite_overlay -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_native_sprite_overlay -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    DEPENDS
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_SHADER_SOURCE}"
                    COMMENT "Compiling sample_2d_desktop_runtime_package Vulkan SPIR-V shader artifacts"
                    VERBATIM
                )
                add_custom_target(sample_2d_desktop_runtime_package_vulkan_shaders
                    DEPENDS
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                )
                if(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
                    add_custom_target(sample_2d_desktop_runtime_package_vulkan_shader_validation
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        DEPENDS
                            sample_2d_desktop_runtime_package_vulkan_shaders
                        COMMENT "Validating sample_2d_desktop_runtime_package Vulkan SPIR-V shader artifacts"
                        VERBATIM
                    )
                endif()
                if(BUILD_TESTING)
                    add_dependencies(sample_2d_desktop_runtime_package sample_2d_desktop_runtime_package_vulkan_shaders)
                    add_test(
                        NAME sample_2d_desktop_runtime_package_vulkan_shader_artifacts_smoke
                        COMMAND sample_2d_desktop_runtime_package
                            ${MK_SAMPLE_2D_DESKTOP_RUNTIME_SOURCE_SMOKE_ARGS}
                            --require-vulkan-shaders
                    )
                endif()
            endif()

            if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "sample_2d_desktop_runtime_package")
                add_dependencies(sample_2d_desktop_runtime_package sample_2d_desktop_runtime_package_shaders)
                install(FILES
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_2D_DESKTOP_RUNTIME_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                )
                if(MK_SAMPLE_2D_DESKTOP_RUNTIME_REQUIRES_SPIRV)
                    if(TARGET sample_2d_desktop_runtime_package_vulkan_shader_validation)
                        add_dependencies(sample_2d_desktop_runtime_package sample_2d_desktop_runtime_package_vulkan_shader_validation)
                    else()
                        add_dependencies(sample_2d_desktop_runtime_package sample_2d_desktop_runtime_package_vulkan_shaders)
                    endif()
                    install(FILES
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_2D_DESKTOP_RUNTIME_VULKAN_NATIVE_SPRITE_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                    )
                endif()
            endif()
        endif()
    endif()
    set_property(TARGET sample_desktop_runtime_game PROPERTY MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS
        "shaders/sample_desktop_runtime_game_scene.vs.dxil;shaders/sample_desktop_runtime_game_scene_skinned.vs.dxil;shaders/sample_desktop_runtime_game_scene.ps.dxil;shaders/sample_desktop_runtime_game_shadow_receiver.ps.dxil;shaders/sample_desktop_runtime_game_shadow_receiver_skinned.ps.dxil;shaders/sample_desktop_runtime_game_postprocess.vs.dxil;shaders/sample_desktop_runtime_game_postprocess.ps.dxil;shaders/sample_desktop_runtime_game_shadow.vs.dxil;shaders/sample_desktop_runtime_game_shadow.ps.dxil;shaders/sample_desktop_runtime_game_ui_overlay.vs.dxil;shaders/sample_desktop_runtime_game_ui_overlay.ps.dxil"
    )
    set_property(TARGET sample_desktop_runtime_game PROPERTY MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS
        "shaders/sample_desktop_runtime_game_scene.vs.spv;shaders/sample_desktop_runtime_game_scene_skinned.vs.spv;shaders/sample_desktop_runtime_game_scene.ps.spv;shaders/sample_desktop_runtime_game_shadow_receiver.ps.spv;shaders/sample_desktop_runtime_game_shadow_receiver_skinned.ps.spv;shaders/sample_desktop_runtime_game_postprocess.vs.spv;shaders/sample_desktop_runtime_game_postprocess.ps.spv;shaders/sample_desktop_runtime_game_shadow.vs.spv;shaders/sample_desktop_runtime_game_shadow.ps.spv;shaders/sample_desktop_runtime_game_ui_overlay.vs.spv;shaders/sample_desktop_runtime_game_ui_overlay.ps.spv"
    )

    if(WIN32)
        set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_DXIL OFF)
        set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV OFF)
        if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "sample_desktop_runtime_game")
            get_target_property(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_DXIL_FOR_PACKAGE
                sample_desktop_runtime_game
                MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_DXIL_FOR_PACKAGE)
                set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_DXIL ON)
            endif()
            get_target_property(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV_FOR_PACKAGE
                sample_desktop_runtime_game
                MK_DESKTOP_RUNTIME_REQUIRES_VULKAN_SHADERS
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV_FOR_PACKAGE)
                set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV ON)
            endif()
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_DXIL)
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_DXIL ON)
        endif()
        if(MK_REQUIRE_DESKTOP_RUNTIME_SPIRV)
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV ON)
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_DXIL AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            message(FATAL_ERROR "DXC is required to package sample_desktop_runtime_game scene shader artifacts")
        endif()
        if(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
            message(FATAL_ERROR "DXC with SPIR-V CodeGen is required to package sample_desktop_runtime_game Vulkan scene shader artifacts")
        endif()
        if(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV AND NOT MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
            message(FATAL_ERROR "spirv-val is required to package sample_desktop_runtime_game Vulkan scene shader artifacts")
        endif()

        if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE)
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/sample_desktop_runtime_game/shaders/runtime_scene.hlsl"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/sample_desktop_runtime_game/shaders/runtime_postprocess.hlsl"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/sample_desktop_runtime_game/shaders/runtime_shadow.hlsl"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE
                "${CMAKE_CURRENT_SOURCE_DIR}/sample_desktop_runtime_game/shaders/runtime_ui_overlay.hlsl"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR
                "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/sample_desktop_runtime_game/shaders"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_scene.vs.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SKINNED_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_scene_skinned.vs.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_scene.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow_receiver.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow_receiver_skinned.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_scene.vs.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SKINNED_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_scene_skinned.vs.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_scene.ps.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow_receiver.ps.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow_receiver_skinned.ps.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_postprocess.vs.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_postprocess.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_postprocess.vs.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_postprocess.ps.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow.vs.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow.vs.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_shadow.ps.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_ui_overlay.vs.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_ui_overlay.ps.dxil"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_ui_overlay.vs.spv"
            )
            set(MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT
                "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}/sample_desktop_runtime_game_ui_overlay.ps.spv"
            )
            add_custom_command(
                OUTPUT
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SKINNED_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                COMMAND ${CMAKE_COMMAND} -E make_directory
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_main -T vs_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_skinned -T vs_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SKINNED_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_main -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_shadow_receiver -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -DGE_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS=1 -E ps_shadow_receiver -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_postprocess -T vs_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_postprocess -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_shadow -T vs_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_shadow -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E vs_native_ui_overlay -T vs_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE}"
                COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                    -nologo -E ps_native_ui_overlay -T ps_6_0
                    -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE}"
                DEPENDS
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE}"
                COMMENT "Compiling sample_desktop_runtime_game D3D12 scene shader artifacts"
                VERBATIM
            )
            add_custom_target(sample_desktop_runtime_game_shaders
                DEPENDS
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SKINNED_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
            )
            if(MK_SAMPLE_DESKTOP_RUNTIME_DXC_SUPPORTS_SPIRV)
                add_custom_command(
                    OUTPUT
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SKINNED_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    COMMAND ${CMAKE_COMMAND} -E make_directory
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_OUTPUT_DIR}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_main -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_skinned -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SKINNED_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_main -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_shadow_receiver -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -DGE_SAMPLE_SKINNED_SCENE_SHADOW_RECEIVER_PS=1 -E ps_shadow_receiver -T ps_6_0 -spirv
                        -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_postprocess -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_postprocess -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_shadow -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_shadow -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E vs_native_ui_overlay -T vs_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE}"
                    COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_DXC_EXECUTABLE}"
                        -nologo -E ps_native_ui_overlay -T ps_6_0 -spirv -fspv-target-env=vulkan1.3
                        -Fo "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE}"
                    DEPENDS
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADER_SOURCE}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_SHADER_SOURCE}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_SHADER_SOURCE}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_SHADER_SOURCE}"
                    COMMENT "Compiling sample_desktop_runtime_game Vulkan scene SPIR-V shader artifacts"
                    VERBATIM
                )
                add_custom_target(sample_desktop_runtime_game_vulkan_shaders
                    DEPENDS
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SKINNED_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                )
                if(MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE)
                    add_custom_target(sample_desktop_runtime_game_vulkan_shader_validation
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SKINNED_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        COMMAND "${MK_SAMPLE_DESKTOP_RUNTIME_SPIRV_VAL_EXECUTABLE}"
                            "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        DEPENDS
                            sample_desktop_runtime_game_vulkan_shaders
                        COMMENT "Validating sample_desktop_runtime_game Vulkan scene SPIR-V shader artifacts"
                        VERBATIM
                    )
                endif()
            endif()
            if(MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET STREQUAL "sample_desktop_runtime_game")
                add_dependencies(sample_desktop_runtime_game sample_desktop_runtime_game_shaders)
                install(FILES
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SKINNED_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                    "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                    DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                )
                if(MK_SAMPLE_DESKTOP_RUNTIME_GAME_REQUIRES_SPIRV)
                    if(TARGET sample_desktop_runtime_game_vulkan_shader_validation)
                        add_dependencies(sample_desktop_runtime_game sample_desktop_runtime_game_vulkan_shader_validation)
                    else()
                        add_dependencies(sample_desktop_runtime_game sample_desktop_runtime_game_vulkan_shaders)
                    endif()
                    install(FILES
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SKINNED_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_RECEIVER_SKINNED_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_POSTPROCESS_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_SHADOW_FRAGMENT_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_VERTEX_SHADER_OUTPUT}"
                        "${MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_UI_OVERLAY_FRAGMENT_SHADER_OUTPUT}"
                        DESTINATION "${CMAKE_INSTALL_BINDIR}/shaders"
                    )
                endif()
            endif()
        endif()
    endif()
endif()

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game(sample_generated_desktop_runtime_package
        SOURCES
            sample_generated_desktop_runtime_package/main.cpp
        GAME_MANIFEST
            games/sample_generated_desktop_runtime_package/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_package.config
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_package.config
        PACKAGE_FILES_FROM_MANIFEST
    )
endif()

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game(sample_generated_desktop_runtime_cooked_scene_package
        SOURCES
            sample_generated_desktop_runtime_cooked_scene_package/main.cpp
        GAME_MANIFEST
            games/sample_generated_desktop_runtime_cooked_scene_package/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_cooked_scene_package.config
            --require-scene-package
            runtime/sample_generated_desktop_runtime_cooked_scene_package.geindex
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_cooked_scene_package.config
            --require-scene-package
            runtime/sample_generated_desktop_runtime_cooked_scene_package.geindex
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries(sample_generated_desktop_runtime_cooked_scene_package
        PRIVATE
            MK_runtime
            MK_scene
            MK_scene_renderer
    )
endif()

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game(sample_generated_desktop_runtime_material_shader_package
        SOURCES
            sample_generated_desktop_runtime_material_shader_package/main.cpp
        GAME_MANIFEST
            games/sample_generated_desktop_runtime_material_shader_package/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_material_shader_package.config
            --require-scene-package
            runtime/sample_generated_desktop_runtime_material_shader_package.geindex
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_material_shader_package.config
            --require-scene-package
            runtime/sample_generated_desktop_runtime_material_shader_package.geindex
            --require-d3d12-scene-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-scene-gpu-bindings
            --require-postprocess
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries(sample_generated_desktop_runtime_material_shader_package
        PRIVATE
            MK_runtime
            MK_scene
            MK_scene_renderer
    )
    MK_configure_desktop_runtime_scene_shader_artifacts(
        TARGET sample_generated_desktop_runtime_material_shader_package
        GAME_NAME sample_generated_desktop_runtime_material_shader_package
        SCENE_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sample_generated_desktop_runtime_material_shader_package/shaders/runtime_scene.hlsl"
        POSTPROCESS_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sample_generated_desktop_runtime_material_shader_package/shaders/runtime_postprocess.hlsl"
    )
    # Vulkan strict package smoke can use --require-vulkan-scene-shaders with -RequireVulkanShaders.
endif()

if(MK_DESKTOP_RUNTIME_ENABLED)
    MK_add_desktop_runtime_game(sample_generated_desktop_runtime_3d_package
        SOURCES
            sample_generated_desktop_runtime_3d_package/main.cpp
        GAME_MANIFEST
            games/sample_generated_desktop_runtime_3d_package/game.agent.json
        SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_3d_package.config
            --require-scene-package
            runtime/sample_generated_desktop_runtime_3d_package.geindex
            --require-primary-camera-controller
            --require-transform-animation
            --require-morph-package
            --require-quaternion-animation
            --require-package-streaming-safe-point
            --require-gameplay-systems
            --require-scene-collision-package
        PACKAGE_SMOKE_ARGS
            --smoke
            --require-config
            runtime/sample_generated_desktop_runtime_3d_package.config
            --require-scene-package
            runtime/sample_generated_desktop_runtime_3d_package.geindex
            --require-primary-camera-controller
            --require-transform-animation
            --require-morph-package
            --require-compute-morph
            --require-compute-morph-normal-tangent
            --require-compute-morph-skin
            --require-compute-morph-async-telemetry
            --require-quaternion-animation
            --require-package-streaming-safe-point
            --require-gameplay-systems
            --require-d3d12-scene-shaders
            --video-driver
            windows
            --require-d3d12-renderer
            --require-scene-gpu-bindings
            --require-postprocess
            --require-postprocess-depth-input
            --require-renderer-quality-gates
            --require-playable-3d-slice
            --require-native-ui-overlay
            --require-visible-3d-production-proof
            --require-scene-collision-package
            --require-native-ui-textured-sprite-atlas
        REQUIRES_D3D12_SHADERS
        PACKAGE_FILES_FROM_MANIFEST
    )
    target_link_libraries(sample_generated_desktop_runtime_3d_package
        PRIVATE
            MK_ai
            MK_animation
            MK_audio
            MK_navigation
            MK_physics
            MK_runtime
            MK_runtime_rhi
            MK_runtime_scene
            MK_scene
            MK_scene_renderer
            MK_ui
            MK_ui_renderer
    )
    MK_configure_desktop_runtime_scene_shader_artifacts(
        TARGET sample_generated_desktop_runtime_3d_package
        GAME_NAME sample_generated_desktop_runtime_3d_package
        SCENE_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sample_generated_desktop_runtime_3d_package/shaders/runtime_scene.hlsl"
        POSTPROCESS_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sample_generated_desktop_runtime_3d_package/shaders/runtime_postprocess.hlsl"
        UI_OVERLAY_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sample_generated_desktop_runtime_3d_package/shaders/runtime_ui_overlay.hlsl"
        SHADOW_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/sample_generated_desktop_runtime_3d_package/shaders/runtime_shadow.hlsl"
        SHADOW_RECEIVER_ENTRY ps_shadow_receiver
        SHADOW_VERTEX_ENTRY vs_shadow
        SHADOW_FRAGMENT_ENTRY ps_shadow
        MORPH_VERTEX_ENTRY vs_morph
        COMPUTE_MORPH_VERTEX_ENTRY vs_compute_morph
        COMPUTE_MORPH_ENTRY cs_compute_morph_position
        COMPUTE_MORPH_TANGENT_VERTEX_ENTRY vs_compute_morph_tangent_frame
        COMPUTE_MORPH_TANGENT_ENTRY cs_compute_morph_tangent_frame
        COMPUTE_MORPH_SKINNED_VERTEX_ENTRY vs_compute_morph_skinned
        COMPUTE_MORPH_SKINNED_ENTRY cs_compute_morph_skinned_position
    )
    # Vulkan strict package smoke can use --require-vulkan-scene-shaders with -RequireVulkanShaders.
endif()
