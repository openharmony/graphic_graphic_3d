#
# Copyright (C) 2020 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(PluginPNG)

# Setup c++17 standard and no extensions.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(AGPBaseAPI REQUIRED)
find_package(AGPEngineAPI REQUIRED)

#This sets the CORE_ROOT_DIRECTORY (location of source/sdk) IF not defined before.
#(if not defined we expect that the LumeEngine folder contains the source/binary/whatever of engine)
set(CORE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/LumeEngine" CACHE PATH "Path engine sdk root dir")
if (EXISTS "${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake")
    #if using binary release, cmake_tools.cmake should define the targets also.. (not done yet)
    include(${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)
else()
    message(FATAL_ERROR "Engine core root directory not found")
endif()

option(CORE_PNG_BUILD_ENGINE "Build AGPEngine from CORE_ROOT_DIRECTORY. " ON)

if (EXISTS "${CORE_ROOT_DIRECTORY}/CMakeLists.txt")
    message("Found engine source in ${CORE_ROOT_DIRECTORY}")
    if (NOT TARGET AGPEngine::AGPEngineAPI)
        if (CORE_PNG_BUILD_ENGINE)
            message("AGPEngine::AGPEngineAPI target does not exist yet, so add engine from the submodule")
            add_subdirectory("${CORE_ROOT_DIRECTORY}/" LumeEngine)
        endif()
    endif()
endif()

add_subdirectory(3rdparty)

set(loader
    src/png/image_loader_png.cpp
    src/png/image_loader_png.h
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${loader} src/component_dll.cpp)

#
# Collect version information from git.
#
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/_version.cpp
           ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)

#
# Plugin API
#
add_library(PluginPNGAPI INTERFACE)
add_library(PluginPNG::PluginPNGAPI ALIAS PluginPNGAPI)

set (PluginPNGAPI_HeaderPath
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/api>
)

set(png_api
    png/implementation_uids.h
)

set_list_prefix(Api_Sources_Generator png_api "${PluginPNGAPI_HeaderPath}/")
target_sources(PluginPNGAPI INTERFACE ${Api_Sources_Generator})

set_list_prefix(Api_Sources_Build png_api "${CMAKE_CURRENT_LIST_DIR}/api/")
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/api
    PREFIX api
    FILES ${Api_Sources_Build}
)

target_include_directories(PluginPNGAPI SYSTEM INTERFACE ${PluginPNGAPI_HeaderPath})
target_link_libraries(PluginPNGAPI INTERFACE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
)

install(TARGETS PluginPNGAPI 
    EXPORT PluginPNGAPITargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/PluginPNGAPIConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/PluginPNGAPIConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginPNGAPI
)

install(EXPORT PluginPNGAPITargets
    FILE PluginPNGAPITargets.cmake
    NAMESPACE PluginPNG::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginPNGAPI
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/PluginPNGAPIConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginPNGAPI
)

install(DIRECTORY api/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/api
)

export(EXPORT PluginPNGAPITargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/PluginPNGAPITargets.cmake"
    NAMESPACE PluginPNG::
)

#
# Plugin library.
#
add_library(PluginPNG SHARED
    src/component_dll.cpp
    ${loader}
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)
add_library(PluginPNG::PluginPNG ALIAS PluginPNG)

target_include_directories(PluginPNG PRIVATE src)
target_compile_definitions(PluginPNG PRIVATE CORE_PLUGIN=1)
target_link_libraries(PluginPNG PRIVATE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
    PluginPNG::PluginPNGAPI
    png_static
)

set_target_properties(PluginPNG PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1)

set_target_properties(PluginPNG PROPERTIES FOLDER SamplePlugins)

append_link_flags(PluginPNG "${BASE_SO_LINK_FLAGS}")

if (ANDROID)
    append_link_flags(PluginPNG "-Wl,--exclude-libs,ALL")

    # Note: Stripping the android release native lib by default also in RelWithDebInfo.
    # (Android Gradle plugin uses RelWithDebInfo for the release build and the strips later
    # when packaging the apk). This was requested by the HQ to produce stripped .so by
    # default in the relase aar. This needs to be removed if debugging the release build is
    # necessary (or for getting symbol names from crash dumps).
    target_link_options(PluginPNG PRIVATE "$<$<CONFIG:RelWithDebInfo>:-s>")    
endif()

setup_warnings(PluginPNG)

disable_rtti(PluginPNG)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/PluginPNGConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/PluginPNGConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginPNG
)

install(TARGETS PluginPNG
    EXPORT PluginPNGTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
)

install(EXPORT PluginPNGTargets
    FILE PluginPNGTargets.cmake
    NAMESPACE PluginPNG::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginPNG
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/PluginPNGConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginPNG
)

export(EXPORT PluginPNGTargets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/PluginPNGTargets.cmake"
       NAMESPACE PluginPNG::
)
