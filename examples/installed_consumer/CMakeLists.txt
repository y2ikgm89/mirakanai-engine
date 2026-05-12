cmake_minimum_required(VERSION 3.30)

project(MirakanaiInstalledConsumer
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(Mirakanai CONFIG REQUIRED)

if(NOT EXISTS "${Mirakanai_AGENT_MANIFEST}")
    message(FATAL_ERROR "Mirakanai_AGENT_MANIFEST does not point to an installed manifest")
endif()

if(NOT EXISTS "${Mirakanai_SCHEMA_DIR}/engine-agent.schema.json")
    message(FATAL_ERROR "Mirakanai_SCHEMA_DIR does not contain engine-agent.schema.json")
endif()

add_executable(gameengine_installed_consumer main.cpp)
target_link_libraries(gameengine_installed_consumer PRIVATE
    mirakana::core
    mirakana::math
    mirakana::tools
    mirakana::ui
)
target_compile_features(gameengine_installed_consumer PRIVATE cxx_std_23)
set_target_properties(gameengine_installed_consumer PROPERTIES
    CXX_EXTENSIONS OFF
    CXX_STANDARD_REQUIRED ON
)

if(MSVC)
    target_compile_options(gameengine_installed_consumer PRIVATE /W4 /permissive- /EHsc)
else()
    target_compile_options(gameengine_installed_consumer PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(WIN32)
    add_custom_command(TARGET gameengine_installed_consumer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy -t $<TARGET_FILE_DIR:gameengine_installed_consumer> $<TARGET_RUNTIME_DLLS:gameengine_installed_consumer>
        COMMAND_EXPAND_LISTS
    )
endif()

enable_testing()
add_test(NAME gameengine_installed_consumer COMMAND gameengine_installed_consumer)

