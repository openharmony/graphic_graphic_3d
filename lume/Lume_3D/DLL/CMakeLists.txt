
#
# Copyright (C) 2019 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)
project(PluginAGP3D)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

#This sets the CORE_ROOT_DIRECTORY (location of source/sdk) IF not defined before.
#(if not defined we expect that the LumeEngine folder contains the source/binary/whatever of engine)
set(CORE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/LumeEngine" CACHE PATH "Path engine sdk root dir")
if (EXISTS "${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake")
    #if using binary release, cmake_tools.cmake should define the targets also.. (not done yet)
    include(${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)
else()
    message(FATAL_ERROR "Engine core root directory not found")
endif()
force_static_link()

set(3d_plugin AGP3D)

option(CORE3D_BUILD_ENGINE "Build AGPEngine from CORE_ROOT_DIRECTORY. " ON)

if (EXISTS "${CORE_ROOT_DIRECTORY}/CMakeLists.txt")
    message("Found engine source in ${CORE_ROOT_DIRECTORY}")
    if (NOT TARGET AGPEngine::AGPEngineAPI)
        if (CORE3D_BUILD_ENGINE)
            message("AGPEngine::AGPEngineAPI target does not exist yet, so add engine from the submodule")
            add_subdirectory("${CORE_ROOT_DIRECTORY}/" LumeEngine)
        endif()
    endif()
endif()

if (NOT TARGET ${3d_plugin}::${3d_plugin}API)
    find_package(${3d_plugin}API REQUIRED)
endif()
if (NOT TARGET ${3d_plugin}::${3d_plugin})
    find_package(${3d_plugin} REQUIRED)
endif()

# link the plugin variant..
#
add_library(Plugin${3d_plugin} SHARED
    ../src/plugin/dynamic_plugin.cpp
    ../src/plugin/dynamic_registry.cpp
    ../src/util/json.cpp
)
add_library(${3d_plugin}::Plugin${3d_plugin} ALIAS Plugin${3d_plugin})

target_link_libraries(Plugin${3d_plugin}
    PRIVATE 
    AGPEngine::AGPEngineAPI AGPEngine::ComponentHelperSDK
    ${3d_plugin}::${3d_plugin} ${3d_plugin}::${3d_plugin}API
)

target_include_directories(Plugin${3d_plugin} PRIVATE ../src)

set_target_properties(Plugin${3d_plugin} PROPERTIES FOLDER DLLs)

#target_compile_definitions(Plugin${3d_plugin} PRIVATE CORE_PLUGIN=1 CORE_STATIC_PLUGIN_HEADER="${CORE_ROOT_DIRECTORY}/src/static_plugin_decl.h")
target_compile_definitions(Plugin${3d_plugin} PRIVATE CORE_PLUGIN=1)

append_link_flags(Plugin${3d_plugin} "${BASE_SO_LINK_FLAGS}")

if (ANDROID)
    append_link_flags(Plugin${3d_plugin} "-Wl,--exclude-libs,ALL")
endif()

if(MSVC)
    target_compile_options(Plugin${3d_plugin} PRIVATE /GR-)
    target_compile_options(Plugin${3d_plugin} INTERFACE)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(Plugin${3d_plugin} PRIVATE -fno-rtti)
    target_compile_options(Plugin${3d_plugin} INTERFACE)
endif()

setup_warnings(Plugin${3d_plugin})

# Sanitizers for unit tests
if(CORE3D_UNIT_TESTS_ENABLED AND CORE3D_UNIT_TESTS_WITH_SANITIZERS AND ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND NOT ANDROID)
    target_compile_options(Plugin${3d_plugin} PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize-recover=address)
    target_link_options(Plugin${3d_plugin} PRIVATE -fsanitize=address -fsanitize=undefined)
endif()

#
# Export cmake "config" module.
#
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/../cmake/Plugin${3d_plugin}Config.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/Plugin${3d_plugin}Config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Plugin${3d_plugin}
)

install(
    TARGETS Plugin${3d_plugin} 
    EXPORT Plugin${3d_plugin}Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
)

install(EXPORT Plugin${3d_plugin}Targets
    FILE Plugin${3d_plugin}Targets.cmake
    NAMESPACE ${3d_plugin}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Plugin${3d_plugin}
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/Plugin${3d_plugin}Config.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Plugin${3d_plugin}
)

export(EXPORT Plugin${3d_plugin}Targets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/Plugin${3d_plugin}Targets.cmake"
       NAMESPACE ${3d_plugin}::
)

