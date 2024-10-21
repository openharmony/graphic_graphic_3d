#
# Copyright (C) 2024 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)
project(EcsSerializer LANGUAGES CXX)

set(target_name EcsSerializer)

find_package(AGPBaseAPI REQUIRED)
find_package(AGPEngineAPI REQUIRED)
find_package(AGPRenderAPI REQUIRED)
find_package(AGP3DAPI REQUIRED)
find_package(ComponentHelperSDK REQUIRED)

include(${BASE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)

#
# Library target.
#
add_library(${target_name} STATIC)

set(sources
    include/ecs_serializer/api.h
    include/ecs_serializer/ecs_animation_util.h
    include/ecs_serializer/ecs_clone_util.h
    include/ecs_serializer/ecs_property_util.h
    include/ecs_serializer/intf_ecs_asset_loader.h
    include/ecs_serializer/intf_ecs_asset_manager.h
    include/ecs_serializer/intf_ecs_serializer.h
    include/ecs_serializer/intf_entity_collection.h
    include/ecs_serializer/namespace.h

    src/ecs_serializer/asset_migration.cpp
    src/ecs_serializer/asset_migration.h
    src/ecs_serializer/ecs_asset_loader.cpp
    src/ecs_serializer/ecs_asset_manager.cpp
    src/ecs_serializer/ecs_serializer.cpp
    src/ecs_serializer/entity_collection.cpp
)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${sources})

target_sources(${target_name} PRIVATE
    ${sources}
)

target_include_directories(${target_name} PUBLIC include PRIVATE src)

target_link_libraries(${target_name} PRIVATE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
    AGPRender::AGPRenderAPI
    AGP3D::AGP3DAPI
    RuntimeUtil
    AGPEngine::ComponentHelperSDK
)

set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 17)
set_property(TARGET ${target_name} PROPERTY CXX_EXTENSIONS OFF)

setup_warnings(${target_name})

# Fix for min/max macro conflicts on windows.
if(WIN32)
    target_compile_definitions(${target_name} PRIVATE NOMINMAX)
endif()

# Disable RTTI
if(MSVC)
    target_compile_options(${target_name} PRIVATE /GR-)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${target_name} PRIVATE -fno-rtti)
endif()

#
# Clang format target
#
set(formatTarget "Format${target_name}")
add_custom_target(${formatTarget}
    COMMAND clang-format -verbose -i ${src}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(${formatTarget} PROPERTIES FOLDER Formatting)
