find_package(unofficial-enet CONFIG REQUIRED)

add_library(mirakana_runtime_network_enet
    src/enet_network_transport_adapter.cpp
)

target_include_directories(mirakana_runtime_network_enet
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(mirakana_runtime_network_enet
    PUBLIC
        mirakana_runtime
    PRIVATE
        unofficial::enet::enet
)

if(WIN32)
    target_link_libraries(mirakana_runtime_network_enet
        PRIVATE
            winmm
            ws2_32
    )
endif()

mirakana_apply_common_target_options(mirakana_runtime_network_enet)

if(NOT TARGET MK_runtime_network_enet)
    add_library(MK_runtime_network_enet ALIAS mirakana_runtime_network_enet)
endif()
