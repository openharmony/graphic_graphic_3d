#
# Copyright (C) 2020 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(PluginJPG)

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

option(CORE_JPG_BUILD_ENGINE "Build AGPEngine from CORE_ROOT_DIRECTORY. " ON)

if (EXISTS "${CORE_ROOT_DIRECTORY}/CMakeLists.txt")
    message("Found engine source in ${CORE_ROOT_DIRECTORY}")
    if (NOT TARGET AGPEngine::AGPEngineAPI)
        if (CORE_JPG_BUILD_ENGINE)
            message("AGPEngine::AGPEngineAPI target does not exist yet, so add engine from the submodule")
            add_subdirectory("${CORE_ROOT_DIRECTORY}/" LumeEngine)
        endif()
    endif()
endif()

add_subdirectory(3rdparty)

set(loader
    src/jpg/image_loader_jpg.cpp
    src/jpg/image_loader_jpg.h
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
add_library(PluginJPGAPI INTERFACE)
add_library(PluginJPG::PluginJPGAPI ALIAS PluginJPGAPI)

set (PluginJPGAPI_HeaderPath
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/api>
)

set(jpg_api
    jpg/implementation_uids.h
)

set_list_prefix(Api_Sources_Generator jpg_api "${PluginJPGAPI_HeaderPath}/")
target_sources(PluginJPGAPI INTERFACE ${Api_Sources_Generator})

set_list_prefix(Api_Sources_Build jpg_api "${CMAKE_CURRENT_LIST_DIR}/api/")
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/api
    PREFIX api
    FILES ${Api_Sources_Build}
)

target_include_directories(PluginJPGAPI SYSTEM INTERFACE ${PluginJPGAPI_HeaderPath})
target_link_libraries(PluginJPGAPI INTERFACE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
)

install(TARGETS PluginJPGAPI 
    EXPORT PluginJPGAPITargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/PluginJPGAPIConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/PluginJPGAPIConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginJPGAPI
)

install(EXPORT PluginJPGAPITargets
    FILE PluginJPGAPITargets.cmake
    NAMESPACE PluginJPG::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginJPGAPI
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/PluginJPGAPIConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginJPGAPI
)

install(DIRECTORY api/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/api
)

export(EXPORT PluginJPGAPITargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/PluginJPGAPITargets.cmake"
    NAMESPACE PluginJPG::
)

#
# Plugin library.
#
add_library(PluginJPG SHARED
    src/component_dll.cpp
    ${loader}
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)
add_library(PluginJPG::PluginJPG ALIAS PluginJPG)

target_include_directories(PluginJPG PRIVATE src)
target_compile_definitions(PluginJPG PRIVATE CORE_PLUGIN=1)
add_dependencies(PluginJPG libjpeg-turbo)

get_target_property(JPEG_LINK_DIRS jpeg-static INTERFACE_LINK_DIRECTORIES)
target_link_directories(PluginJPG PRIVATE ${JPEG_LINK_DIRS})
target_link_libraries(PluginJPG PRIVATE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
    PluginJPG::PluginJPGAPI
    jpeg-static
)

set_target_properties(PluginJPG PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1)

set_target_properties(PluginJPG PROPERTIES FOLDER SamplePlugins)

append_link_flags(PluginJPG "${BASE_SO_LINK_FLAGS}")

if (ANDROID)
    append_link_flags(PluginJPG "-Wl,--exclude-libs,ALL")

    # Note: Stripping the android release native lib by default also in RelWithDebInfo.
    # (Android Gradle plugin uses RelWithDebInfo for the release build and the strips later
    # when packaging the apk). This was requested by the HQ to produce stripped .so by
    # default in the relase aar. This needs to be removed if debugging the release build is
    # necessary (or for getting symbol names from crash dumps).
    target_link_options(PluginJPG PRIVATE "$<$<CONFIG:RelWithDebInfo>:-s>")    
endif()

setup_warnings(PluginJPG)

disable_rtti(PluginJPG)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/PluginJPGConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/PluginJPGConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginJPG
)

install(TARGETS PluginJPG
    EXPORT PluginJPGTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
)

install(EXPORT PluginJPGTargets
    FILE PluginJPGTargets.cmake
    NAMESPACE PluginJPG::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginJPG
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/PluginJPGConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PluginJPG
)

export(EXPORT PluginJPGTargets
       FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/PluginJPGTargets.cmake"
       NAMESPACE PluginJPG::
)
