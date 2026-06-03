add_library(mirakana_core
    src/application.cpp
    src/diagnostics.cpp
    src/job_execution.cpp
    src/log.cpp
    src/memory.cpp
    src/registry.cpp
    src/simd_dispatch.cpp
    src/simd_dispatch_detail.hpp
    src/time.cpp
    src/version.cpp
)

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" MK_CORE_SYSTEM_PROCESSOR_LOWER)
set(MK_CORE_AVX2_COMPILE_OPTION "")
if(MK_CORE_SYSTEM_PROCESSOR_LOWER MATCHES "^(x86_64|amd64|x64|i[3-6]86)$")
    if(MSVC)
        set(MK_CORE_AVX2_COMPILE_OPTION /arch:AVX2)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(MK_CORE_AVX2_COMPILE_OPTION -mavx2)
    endif()
endif()

if(NOT MK_CORE_AVX2_COMPILE_OPTION STREQUAL "")
    add_library(mirakana_core_avx2 OBJECT
        src/simd_dispatch_avx2.cpp
        src/simd_dispatch_detail.hpp
    )
    target_include_directories(mirakana_core_avx2
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    mirakana_apply_common_target_options(mirakana_core_avx2)
    target_compile_options(mirakana_core_avx2 PRIVATE ${MK_CORE_AVX2_COMPILE_OPTION})
    target_compile_definitions(mirakana_core_avx2 PRIVATE MK_CORE_REVIEWED_AVX2_TARGET=1)
    target_compile_definitions(mirakana_core PRIVATE MK_CORE_HAS_REVIEWED_AVX2_TARGET=1)
    target_sources(mirakana_core PRIVATE $<TARGET_OBJECTS:mirakana_core_avx2>)
endif()

target_include_directories(mirakana_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

mirakana_apply_common_target_options(mirakana_core)

if(NOT TARGET MK_core)
    add_library(MK_core ALIAS mirakana_core)
endif()
