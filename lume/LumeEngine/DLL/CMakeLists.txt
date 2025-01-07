# 
# Copyright (c) Huawei Technologies Co., Ltd. 2019-2020. All rights reserved.
# Description: This script contains builds shared libraries for the engine
# Author: Jani Kattelus
# Create: 2020-01-28
# 
# NOTE: this will not currently work standalone, since it fetches the library names from targets. could be fixed easily if needed.
# also the names and dll options are kind of arbitrary at the moment.
# also the resulting targets from here are referenced by the sample apps also.
cmake_minimum_required(VERSION 3.18)
project(AGPDLL)

#this include file should actually come from engine. (usually one directory up from current dir)
set(CORE_ROOT_DIRECTORY ".." CACHE PATH "Path engine sdk root dir")
if (NOT EXISTS "${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake")
    message(FATAL_ERROR "Engine core root directory not found")
endif()

list(APPEND CMAKE_MODULE_PATH "${CORE_ROOT_DIRECTORY}/cmake")

if (NOT TARGET AGPBase::AGPBaseAPI)
    find_package(AGPBaseAPI REQUIRED)
endif()
if (NOT TARGET AGPEngine::AGPEngineAPI)
    find_package(AGPEngineAPI REQUIRED)
endif()
if (NOT TARGET AGPEngine::AGPEngine)
    find_package(AGPEngine REQUIRED)
endif()

#1. generation of public symbol lists for def / linkerscripts.
#2. "conversion" of libs to dll/so.

#construct exports*.def with dumpsyms.bat (or shell script)
#construct agpengine.version with dumpelf.bat (or shell script)
#or build the GenerateDefines* targets. (which use cmake) (these targets are excluded from all, since they are supposed to be built manually when needed)
add_custom_target(GenerateAllDefines COMMAND echo "Generated all defs / linker scripts.")
set_target_properties(GenerateAllDefines PROPERTIES FOLDER Tools)
#NOTE: we are explicitly setting the EXCLUDE_FROM_ALL  and  EXCLUDE_FROM_DEFAULT_BUILD (vsonly) properties to all generatedefine targets
#since we do not want it to be built automatically. (custom targets should be automatically excluded, but in this case only the master target was correctly excluded.)
set_target_properties(GenerateAllDefines PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

if (${CMAKE_SIZEOF_VOID_P} EQUAL "4") 
    set(CORE_BITS 32)
elseif (${CMAKE_SIZEOF_VOID_P} EQUAL "8") 
    set(CORE_BITS 64)
endif()
if(WIN32)
    set(CORE_SYM_EXT _${CORE_BITS}.def)
elseif(UNIX)
    set(CORE_SYM_EXT _${CORE_BITS}.ld)
elseif(OHOS)
    set(CORE_SYM_EXT _${CORE_BITS}.ld)
endif()

add_custom_target(GenerateDefinesEngine
    COMMAND ${CMAKE_COMMAND}
            -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME} -DANDROID_TOOLCHAIN_PREFIX=${ANDROID_TOOLCHAIN_PREFIX} -DVERSION_CODE=AGPEngineDLL_1.0
            -P ${CORE_ROOT_DIRECTORY}/cmake/define.cmake ${CMAKE_CURRENT_LIST_DIR}/src/agpengine${CORE_SYM_EXT} $<TARGET_FILE:AGPEngine::AGPEngine>)
set_target_properties(GenerateDefinesEngine PROPERTIES FOLDER Tools)
add_dependencies(GenerateAllDefines AGPEngine::AGPEngine GenerateDefinesEngine)
set_target_properties(GenerateDefinesEngine PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
set_source_files_properties(${CMAKE_CURRENT_LIST_DIR}/src/agpengine${CORE_SYM_EXT} PROPERTIES GENERATED true )

set(dll_sources src/exports.cpp)

# collect build type flags..
unset(build_flags)
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    set(build_flags ${build_flags} "CORE_PUBLIC=__declspec(dllimport)")
else()
    set(build_flags ${build_flags} "CORE_PUBLIC=")
endif()

#Link core engine.dll

add_library(AGPEngineDLL SHARED 
    $<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_LIST_DIR}/src/agpengine${CORE_SYM_EXT}>
    ${dll_sources}
)

target_link_libraries(AGPEngineDLL INTERFACE AGPEngine::AGPEngineAPI
    PRIVATE
    $<$<PLATFORM_ID:Linux>:-Wl,${CMAKE_CURRENT_LIST_DIR}/src/agpengine${CORE_SYM_EXT}>
    $<$<PLATFORM_ID:Android>:-Wl,${CMAKE_CURRENT_LIST_DIR}/src/agpengine${CORE_SYM_EXT}>
    $<$<PLATFORM_ID:OHOS>:-Wl,${CMAKE_CURRENT_LIST_DIR}/src/agpengine${CORE_SYM_EXT}>
    $<$<PLATFORM_ID:Darwin>:-Wl,-exported_symbols_list>
    $<$<PLATFORM_ID:Darwin>:-Wl,${CMAKE_CURRENT_LIST_DIR}/src/agpengine.exports>
    AGPEngine::AGPEngine
)

target_compile_definitions(AGPEngineDLL INTERFACE ${build_flags})

add_library(AGPEngine::AGPEngineDLL ALIAS AGPEngineDLL)

append_link_flags(AGPEngineDLL "${BASE_SO_LINK_FLAGS}")

setup_warnings(AGPEngineDLL)

# Disable RTTI
disable_rtti(AGPEngineDLL)

# Sanitizers for unit tests
if(CORE_UNIT_TESTS_ENABLED AND CORE_UNIT_TESTS_WITH_SANITIZERS AND ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND NOT ANDROID AND NOT OHOS)
    target_compile_options(AGPEngineDLL PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize-recover=address)
    target_link_options(AGPEngineDLL PRIVATE -fsanitize=address -fsanitize=undefined)
endif()

set_target_properties(AGPEngineDLL PROPERTIES FOLDER DLLs)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/../cmake/AGPEngineDLLConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/AGPEngineDLLConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AGPEngineDLL
)

install(
    TARGETS AGPEngineDLL
    EXPORT AGPEngineDLLTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(
    EXPORT AGPEngineDLLTargets
    FILE AGPEngineDLLTargets.cmake
    NAMESPACE AGPEngine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AGPEngineDLL
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/AGPEngineDLLConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AGPEngineDLL
)

export(EXPORT AGPEngineDLLTargets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/AGPEngineDLLTargets.cmake"
       NAMESPACE AGPEngine::
)
