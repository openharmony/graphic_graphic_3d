#
# Copyright (C) 2018 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(ComponentHelperSDK)

if (NOT DEFINED CORE_ROOT_DIRECTORY)
    set(CORE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../" CACHE PATH "Path engine sdk root dir")
endif()

if (NOT DEFINED BASE_ROOT_DIRECTORY)
    set(BASE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../LumeBase" CACHE PATH "Base root dir")
endif()

include(${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)
force_static_link()

#
# Pure interface api targets. (includes, shared sources only)
# todo: check install, create cmake modules.. etc
#
#Most of these are pure helpers which in theory might not even be used by actual component/system/etc implementations.
#This part will receive more cleanup. (ie. these files should be moved OUT of the engine src/include/api folders )
add_library(ComponentHelperSDK INTERFACE)
add_library(AGPEngine::ComponentHelperSDK ALIAS ComponentHelperSDK)

set (ComponentHelperSDK_HeaderPath
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>$<INSTALL_INTERFACE:${inc_dest}/ComponentHelperSDK>
)
set(ComponentHelperSDK_Sources
    ComponentTools/base_manager.inl
    ComponentTools/base_manager.h
    ComponentTools/component_query.h
    ComponentTools/component_query.cpp

    PropertyTools/property_api_impl.inl
    PropertyTools/property_api_impl.h
    PropertyTools/property_data.cpp
    PropertyTools/property_data.h
    PropertyTools/property_macros.h
    PropertyTools/property_value.h
    PropertyTools/core_metadata.inl
)

# Prefixing the filenames here with either the generator expression or the relative path for source_group.
# (because source_group does not understand generator expressions).

set_list_prefix(ComponentHelperSDK_Sources_Generator ComponentHelperSDK_Sources "${ComponentHelperSDK_HeaderPath}/")
target_sources(ComponentHelperSDK INTERFACE ${ComponentHelperSDK_Sources_Generator})

set_list_prefix(ComponentHelperSDK_Sources_Build ComponentHelperSDK_Sources "${CMAKE_CURRENT_LIST_DIR}/")
source_group(TREE ${CMAKE_CURRENT_LIST_DIR}/
    PREFIX component_sdk
    FILES ${ComponentHelperSDK_Sources_Build})

target_include_directories(ComponentHelperSDK SYSTEM INTERFACE
    ${ComponentHelperSDK_HeaderPath}
)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/
    DESTINATION ${inc_dest}/ComponentHelperSDK
)

install(TARGETS ComponentHelperSDK
    EXPORT ComponentHelperSDKConfig
    DESTINATION ${lib_dest}
)
install(EXPORT ComponentHelperSDKConfig
    FILE ComponentHelperSDKConfig.cmake
    NAMESPACE AGPEngine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ComponentHelperSDK
)

# Make it possible to import from a build tree.
export(TARGETS ComponentHelperSDK
    NAMESPACE AGPEngine::
    FILE ComponentHelperSDKConfig.cmake
)
