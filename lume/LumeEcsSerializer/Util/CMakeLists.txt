#
# Copyright (C) 2024 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)
project(Util LANGUAGES CXX)

set(target_name RuntimeUtil)

find_package(AGPBaseAPI REQUIRED)
find_package(AGPEngineAPI REQUIRED)

include(${BASE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)

#
# Library target.
#
add_library(${target_name} STATIC)

set(sources
    include/util/core_library_loader.h
    include/util/io_util.h
    include/util/json.h
    include/util/json_util.h
    include/util/namespace.h
    include/util/path_util.h

    src/util/io_util.cpp
    src/util/json.cpp
    src/util/path_util.cpp
)
source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${sources})

target_sources(${target_name} PRIVATE
    ${sources}
)

target_include_directories(${target_name} PUBLIC include PRIVATE src)

target_link_libraries(${target_name} PRIVATE
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
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
