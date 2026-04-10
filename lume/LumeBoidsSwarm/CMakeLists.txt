#
# Copyright (C) 2025 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(PluginBoidsSwarm)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Setup root directories for cmake tools and modules
if (NOT DEFINED BASE_ROOT_DIRECTORY)
    set(BASE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../LumeBase" CACHE PATH "Base root dir")
endif()
if (EXISTS "${BASE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake")
    include(${BASE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)
else()
    message(FATAL_ERROR "Base directory not found")
endif()

if (NOT DEFINED CORE_ROOT_DIRECTORY)
    set(CORE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../LumeEngine" CACHE PATH "Path engine sdk root dir")
endif()

if (NOT DEFINED RENDER_ROOT_DIRECTORY)
    set(RENDER_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../LumeRender" CACHE PATH "Path render sdk root dir")
endif()

list(APPEND CMAKE_MODULE_PATH "${BASE_ROOT_DIRECTORY}/cmake")

find_package(AGPBaseAPI REQUIRED)
find_package(AGPEngineAPI REQUIRED)
find_package(AGPEngineDLL REQUIRED)
find_package(AGPRenderAPI REQUIRED)
find_package(AGP3DAPI REQUIRED)
find_package(ComponentHelperSDK REQUIRED)

set(boids_swarm_api
    api/boids_swarm/namespace.h
    api/boids_swarm/implementation_uids.h

    api/boids_swarm/ecs/components/boids_swarm_component.h
    api/boids_swarm/ecs/components/boids_swarm_gravity_component.h
    api/boids_swarm/ecs/components/boids_swarm_repulsion_component.h
    api/boids_swarm/ecs/components/boids_swarm_state_component.h

    api/boids_swarm/ecs/systems/intf_boids_swarm_system.h
)

#
# App source and data.
#
set(sources
    src/plugin.cpp

    src/ecs/components/boids_swarm_component_manager.cpp
    src/ecs/components/boids_swarm_gravity_component_manager.cpp
    src/ecs/components/boids_swarm_repulsion_component_manager.cpp
    src/ecs/components/boids_swarm_state_component_manager.cpp

    src/ecs/systems/boids_swarm_system.h
    src/ecs/systems/boids_swarm_system.cpp
)

set(data
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/api/ PREFIX api FILES ${boids_swarm_api})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${sources})
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${data})

#
# Plugin library
#
add_library(PluginBoidsSwarm SHARED
    ${sources}
    ${data}
    ${boids_swarm_api}
)

target_include_directories(PluginBoidsSwarm PRIVATE api src)

target_compile_definitions(PluginBoidsSwarm PRIVATE CORE_PLUGIN=1)

option(BOIDSSWARM_DEV_ENABLED "Build PluginBoidsSwarm with development time define" ${CORE_DEV_ENABLED})
if(BOIDSSWARM_DEV_ENABLED)
    target_compile_definitions(PluginBoidsSwarm PRIVATE BOIDSSWARM_DEV_ENABLED=1)
endif()

target_link_libraries(PluginBoidsSwarm PRIVATE
    AGPEngine::AGPEngineAPI
    AGPEngine::ComponentHelperSDK
    AGPRender::AGPRenderAPI
    AGP3D::AGP3DAPI
)

#
# Collect version information from git.
#
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
    COMMAND ${CMAKE_COMMAND} -P
            ${CMAKE_CURRENT_SOURCE_DIR}/version.cmake
)
target_sources(PluginBoidsSwarm PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

set_property(TARGET ${executable} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../..)
set_property(TARGET ${executable} PROPERTY VS_DEBUGGER_ENVIRONMENT "PATH=$<TARGET_FILE_DIR:AGPEngine::AGPEngineDLL>")

#
# Plugin API
#
add_library(PluginBoidsSwarmAPI INTERFACE)
target_include_directories(PluginBoidsSwarmAPI SYSTEM INTERFACE api)
