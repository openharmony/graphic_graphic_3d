#
# Copyright (C) 2020 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(PluginDotfield)

# Setup c++17 standard and no extensions.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT DEFINED RENDER_ROOT_DIRECTORY)
    set(RENDER_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/LumeRender" CACHE PATH "Path render sdk root dir")
endif()
include(${RENDER_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (NOT TARGET AGPEngine::AGPEngineAPI)
    find_package(AGPEngineAPI REQUIRED)
endif()
if (NOT TARGET AGPEngine::ComponentHelperSDK)
    find_package(ComponentHelperSDK REQUIRED)
endif()
if (NOT TARGET AGPRender::AGPRenderAPI)
    find_package(AGPRenderAPI REQUIRED)
endif()
if (NOT TARGET AGP3D::AGP3DAPI)
    find_package(AGP3DAPI REQUIRED)
endif()

set(lib_dest "lib/$<CONFIG>")
set(inc_dest "include")

#
# Plugin API.
#
add_library(PluginDotfieldAPI INTERFACE)
add_library(PluginDotfield::PluginDotfieldAPI ALIAS PluginDotfieldAPI)

set (PluginDotfieldAPI_HeaderPath
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>$<INSTALL_INTERFACE:${inc_dest}/api>
)

set(dotfield_api
    dotfield/implementation_uids.h
    dotfield/ecs/systems/dotfield_system.h
    dotfield/ecs/components/dotfield_component.h
    dotfield/render/intf_render_data_store_default_dotfield.h
)

set_list_prefix(Api_Sources_Generator dotfield_api "${PluginDotfieldAPI_HeaderPath}/")
target_sources(PluginDotfieldAPI INTERFACE ${Api_Sources_Generator})

set_list_prefix(Api_Sources_Build dotfield_api "${CMAKE_CURRENT_LIST_DIR}/api/")
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/api
    PREFIX api
    FILES ${Api_Sources_Build}
)

target_include_directories(PluginDotfieldAPI SYSTEM INTERFACE ${PluginDotfieldAPI_HeaderPath})

install(DIRECTORY api/ DESTINATION ${inc_dest}/api)

install(TARGETS PluginDotfieldAPI EXPORT PluginDotfieldAPIConfig
    DESTINATION ${lib_dest}
)
install(EXPORT PluginDotfieldAPIConfig
    FILE PluginDotfieldAPIConfig.cmake
    NAMESPACE PluginDotfield::
    DESTINATION "share/cmake"
)

# Make it possible to import from a build tree.
export(TARGETS PluginDotfieldAPI NAMESPACE PluginDotfield:: FILE PluginDotfieldAPIConfig.cmake)


set(managers
    src/components/dotfield_component.cpp
)

set(systems
    src/systems/dotfield_system.cpp
)

set(nodes
    src/render/render_data_store_default_dotfield.h
    src/render/render_data_store_default_dotfield.cpp
    src/render/render_node_dotfield_render.h
    src/render/render_node_dotfield_render.cpp
    src/render/render_node_dotfield_simulation.h
    src/render/render_node_dotfield_simulation.cpp
)

set(data
    assets/app/shaders/common/dotfield_common.h
    assets/app/shaders/common/dotfield_struct_common.h

    assets/app/shaders/computeshader/dotfield_simulation.comp
    assets/app/shaders/computeshader/dotfield_simulation.shader

    assets/app/shaders/shader/default_material_dotfield.vert
    assets/app/shaders/shader/default_material_dotfield.frag
    assets/app/shaders/shader/default_material_dotfield.shader

    assets/app/pipelinelayouts/default_material_dotfield.shaderpl
    assets/app/pipelinelayouts/dotfield_simulation.shaderpl

    assets/app/vertexinputdeclarations/default_material_dotfield.shadervid
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${systems} ${managers} ${nodes} src/component_dll.cpp)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} PREFIX assets FILES ${data})

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
# Plugin library.
#
add_library(PluginDotfield SHARED
    src/component_dll.cpp
    ${managers}
    ${systems}
    ${nodes}
    ${data}
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

target_include_directories(PluginDotfield PRIVATE api src assets)
target_compile_definitions(PluginDotfield PRIVATE CORE_PLUGIN=1)
target_link_libraries(PluginDotfield PRIVATE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
    AGPEngine::ComponentHelperSDK
    AGPRender::AGPRenderAPI
    AGP3D::AGP3DAPI
    PluginDotfield::PluginDotfieldAPI
)

setup_warnings(PluginDotfield)

append_link_flags(PluginDotfield "${BASE_SO_LINK_FLAGS}")

if (ANDROID)
    append_link_flags(PluginDotfield "-Wl,--exclude-libs,ALL")
endif()

set_target_properties(PluginDotfield PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1)

set_target_properties(PluginDotfield PROPERTIES FOLDER SamplePlugins)

# Build shaders and assets
compile_shaders(TARGET CompileDotfieldShaders SOURCE "${CMAKE_CURRENT_LIST_DIR}/assets/app/shaders" INCLUDE AGPRender::AGPRenderAPI AGP3D::AGP3DAPI)

build_rofs(CompileDotfieldAssets PluginDotfield "${CMAKE_CURRENT_LIST_DIR}/assets/app" "/" "DOTFIELD_BIN" "DOTFIELD_BIN_SIZE" "dotfieldrofs")
set_target_properties(CompileDotfieldAssets PROPERTIES FOLDER Tools)

add_dependencies(CompileDotfieldAssets CompileDotfieldShaders)
add_dependencies(PluginDotfield CompileDotfieldAssets)


#
# Export cmake "config" module.
#
install(TARGETS PluginDotfield EXPORT PluginDotfieldConfig
    DESTINATION ${lib_dest}
)
install(EXPORT PluginDotfieldConfig
    FILE PluginDotfieldConfig.cmake
    NAMESPACE PluginDotfield::
    DESTINATION "share/cmake"
)

# Make it possible to import from a build tree.
export(TARGETS PluginDotfield NAMESPACE PluginDotfield:: FILE PluginDotfieldTargets.cmake)

configure_file(cmake/PluginDotfieldConfig.cmake.in PluginDotfieldConfig.cmake @ONLY)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PluginDotfieldConfig.cmake"
    DESTINATION "share/cmake"
)
