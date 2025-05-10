#
# Copyright (C) 2018 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(AGPEngine)

if (NOT DEFINED CORE_ROOT_DIRECTORY)
    set(CORE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}" CACHE PATH "Path engine sdk root dir")
endif()
include(${CORE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)
force_static_link()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/sdk" CACHE PATH "<comment>" FORCE)
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

option(CORE_BUILD_BASE "Build AGPBase from BASE_ROOT_DIRECTORY. " ON)
set(BASE_ROOT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/LumeBase" CACHE PATH "Base root dir")
if (EXISTS "${BASE_ROOT_DIRECTORY}/CMakeLists.txt")
    message("Found base source in ${BASE_ROOT_DIRECTORY}")
    if (NOT TARGET AGPBase::AGPBaseAPI)
        if (CORE_BUILD_BASE)
            message("AGPBase::AGPBaseAPI target does not exist yet, so add base from the submodule")
#            add_subdirectory("${BASE_ROOT_DIRECTORY}/" LumeBase)
        endif()
    endif()
endif()

if (NOT TARGET AGPBase::AGPBaseAPI)
    find_package(AGPBaseAPI REQUIRED)
endif()
option(CORE_HIDE_SYMBOLS "Hide non CORE_PUBLIC symbols" ON)
#set(CORE_HIDE_SYMBOLS FALSE CACHE BOOL "" FORCE)
option(CORE_PERF_ENABLED "Build AGPEngine with performance timers" OFF)
#set(CORE_PERF_ENABLED TRUE CACHE BOOL "" FORCE)

#
# Pure interface api targets. (includes, shared sources only)
# todo: check install, create cmake modules.. etc
#
# Pure interface target for AGPEngine. exposes only the api folder!
add_library(AGPEngineAPI INTERFACE)
add_library(AGPEngine::AGPEngineAPI ALIAS AGPEngineAPI)

set (AGPEngineAPI_HeaderPath
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/api>
)

if(ANDROID)
    set (AGPEngineOSAPI_HeaderPath
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform/android>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/api/platform/android>
    )
elseif(OHOS)
    set (AGPEngineOSAPI_HeaderPath
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform/ohos_ndk>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/api/platform/ohos_ndk>
    )
else()
    set (AGPEngineOSAPI_HeaderPath
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/platform/common>$<INSTALL_INTERFACE:$<INSTALL_PREFIX>/${CMAKE_INSTALL_INCLUDEDIR}/api/platform/common>
    )
endif()

set(Api_OSSources
    core/os/intf_platform.h
    core/os/platform_create_info.h
)

set(Api_Sources
    core/ecs/entity.h
    core/ecs/entity_reference.h
    core/ecs/component_struct_macros.h
    core/ecs/intf_component_manager.h
    core/ecs/intf_ecs.h
    core/ecs/intf_entity_manager.h
    core/ecs/intf_system.h
    core/ecs/intf_system_graph_loader.h

    core/image/intf_image_container.h
    core/image/intf_image_loader_manager.h
    core/image/intf_animated_image.h

    core/io/intf_filesystem_api.h
    core/io/intf_directory.h
    core/io/intf_file.h
    core/io/intf_file_manager.h
    core/io/intf_file_system.h
    core/io/intf_file_monitor.h

    core/json/json.h

    core/perf/cpu_perf_scope.h
    core/perf/intf_performance_data_manager.h
    core/perf/intf_performance_trace.h

    core/plugin/intf_class_factory.h
    core/plugin/intf_class_info.h
    core/plugin/intf_class_register.h
    core/plugin/intf_plugin.h
    core/plugin/intf_plugin_decl.h
    core/plugin/intf_plugin_register.h
    core/plugin/intf_interface.h
    core/plugin/intf_interface_helper.h

    core/property/intf_property_api.h
    core/property/intf_property_handle.h
    core/property/property_handle_util.h
    core/property/property_types.h
    core/property/property.h
    core/property/scoped_handle.h

    core/property_tools/property_api_impl.inl
    core/property_tools/property_api_impl.h
    core/property_tools/property_data.cpp
    core/property_tools/property_data.h
    core/property_tools/property_macros.h
    core/property_tools/core_metadata.inl

    core/resources/intf_resource.h
    core/resources/intf_resource_manager.h

    core/threading/intf_thread_pool.h

    core/util/intf_frustum_util.h
    core/util/parallel_sort.h

    core/engine_info.h
    core/implementation_uids.h
    core/intf_engine.h
    core/intf_logger.h
    core/log.h
    core/namespace.h
)

# Prefixing the filenames here with either the generator expression or the relative path for source_group.
# (because source_group does not understand generator expressions).

set_list_prefix(Api_Sources_Generator Api_Sources "${AGPEngineAPI_HeaderPath}/")
set_list_prefix(Api_OSSources_Generator Api_OSSources "${AGPEngineOSAPI_HeaderPath}/")
target_sources(AGPEngineAPI INTERFACE ${Api_Sources_Generator}
                            INTERFACE ${Api_OSSources_Generator})

set_list_prefix(Api_Sources_Build Api_Sources "${CMAKE_CURRENT_SOURCE_DIR}/")
set_list_prefix(Api_OSSources_Build Api_OSSources "${CMAKE_CURRENT_SOURCE_DIR}/")
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
    FILES ${Api_Sources_Build}
    FILES ${Api_OSSources_Build}
)

target_include_directories(AGPEngineAPI SYSTEM INTERFACE
    ${AGPEngineAPI_HeaderPath}
)
target_include_directories(AGPEngineAPI SYSTEM INTERFACE
    ${AGPEngineOSAPI_HeaderPath}
)
target_link_libraries(AGPEngineAPI INTERFACE AGPBase::AGPBaseAPI)

# Temporarily set this always. Should vary depending on static/ shared.
if(APPLE)
    target_compile_definitions(AGPEngineAPI INTERFACE CORE_USE_COMPILER_GENERATED_STATIC_LIST=0)
else()
    target_compile_definitions(AGPEngineAPI INTERFACE CORE_USE_COMPILER_GENERATED_STATIC_LIST=1)
endif()

#
# Performance timers
#
if(CORE_PERF_ENABLED)
    target_compile_definitions(AGPEngineAPI INTERFACE CORE_PERF_ENABLED=1)
else()
    target_compile_definitions(AGPEngineAPI INTERFACE CORE_PERF_ENABLED=0)
endif()

# Make it possible to import from a build tree.
#export(TARGETS AGPEngineAPI NAMESPACE AGPEngine:: FILE AGPEngineAPIConfig.cmake)
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/../cmake/AGPEngineAPIConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/cmake/AGPEngineAPIConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AGPEngineAPI
)

install(TARGETS AGPEngineAPI 
    EXPORT AGPEngineAPITargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/$<CONFIG>
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/$<CONFIG>
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT AGPEngineAPITargets
    FILE AGPEngineAPITargets.cmake
    NAMESPACE AGPEngine::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AGPEngineAPI
)

install(FILES "${PROJECT_BINARY_DIR}/cmake/AGPEngineAPIConfig.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/AGPEngineAPI
)

install(DIRECTORY ./
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/api
)

export(EXPORT AGPEngineAPITargets
    FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/AGPEngineAPITargets.cmake"
    NAMESPACE AGPEngine::
)

#
# Clang format target.
#
add_custom_target(FormatEngineApi
    COMMAND clang-format -verbose -i ${Api_Sources_Build}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_target_properties(FormatEngineApi PROPERTIES FOLDER Formatting)
