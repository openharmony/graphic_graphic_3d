#
# Copyright (C) 2018 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.15)

cmake_policy(SET CMP0091 NEW) # Required by CMAKE_MSVC_RUNTIME_LIBRARY

project(LumeShaderCompiler)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipoSupported OUTPUT output)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # enable multi-processor compilation.
    add_compile_options(/MP)
    # enable whole program optimization
    add_compile_options(/GL)
    # enable link-time code generation
    add_link_options(/LTCG)

    # use the statically linked runtime libarary to not require a specific vs runtime from the user
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

#workaround for "PythonInterp" finds old version of python...
find_package (Python3 COMPONENTS Interpreter)
set(Python_EXECUTABLE ${Python3_EXECUTABLE} CACHE STRING "" FORCE)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    #make a staticly linked executable with hidden / gc symbols. (to minimize)
    #also we use libc++ so build with clang.
    set(CMAKE_THREAD_PREFER_PTHREAD true)
    SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -stdlib=libc++ -O3 -flto=thin -Wl,--gc-sections -Wl,--exclude-libs,ALL")
    add_compile_options(-stdlib=libc++ -fvisibility=hidden -ffunction-sections -fdata-sections)
    #workaround for spirv-tools forcibly linking LIBRT with out actually needing it (blocks static link)
    set(LIBRT "" CACHE STRING "" FORCE)
endif()

include(ExternalProject)
find_package(PythonInterp 3)
if(NOT ${PYTHONINTERP_FOUND})
    message(FATAL_ERROR "Python interpreter required to build shaderc/spirv-tools etc..")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

function(get_all_targets _result _dir)
    get_property(_subdirs DIRECTORY "${_dir}" PROPERTY SUBDIRECTORIES)
    foreach(_subdir IN LISTS _subdirs)
        get_all_targets(${_result} "${_subdir}")
    endforeach()
    get_property(_sub_targets DIRECTORY "${_dir}" PROPERTY BUILDSYSTEM_TARGETS)
    set(${_result} ${${_result}} ${_sub_targets} PARENT_SCOPE)
endfunction()

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN} EXCLUDE_FROM_ALL)
    get_all_targets(_targets "${_folder}")
    foreach(_target IN LISTS _targets)
        set_target_properties(
            ${_target}
            PROPERTIES FOLDER "${_folder_name}"
        )
    endforeach()
endfunction()

cmake_policy(SET CMP0077 OLD)
# if (__OHOS_PLATFORM__)

#set default and modified options for glslang
set(ENABLE_CTEST OFF CACHE BOOL "" FORCE) 
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "" FORCE)
set(ENABLE_SPVREMAPPER OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)

#set default and modified options for spirv-tools
set(SKIP_SPIRV_TOOLS_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_BUILD_COMPRESSION OFF CACHE BOOL "" FORCE)
set(SPIRV_WERROR OFF CACHE BOOL "" FORCE)
set(SPIRV_WARN_EVERYTHING OFF CACHE BOOL "" FORCE)
set(SPIRV_COLOR_TERMINAL ON CACHE BOOL "" FORCE)
set(SPIRV_LOG_DEBUG OFF CACHE BOOL "" FORCE)
set(SPIRV_SKIP_EXECUTABLES ON CACHE BOOL "" FORCE)
set(SPIRV_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SPIRV_CHECK_CONTEXT ON CACHE BOOL "" FORCE)
set(SPIRV_TOOLS_INSTALL_EMACS_HELPERS OFF CACHE BOOL "" FORCE)

#set default and modified options for spirv-cross
set(SPIRV_CROSS_EXCEPTIONS_TO_ASSERTIONS OFF CACHE BOOL "")
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_GLSL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)

string(REPLACE "/foundation" ";" TEMP_PATH ${CMAKE_CURRENT_LIST_FILE})
list(GET TEMP_PATH 0 OHOS_ROOT_PATH)
set(SPIRV_TOOLS_PATH "${OHOS_ROOT_PATH}/third_party/spirv-tools")
set(SPIRV_TOOLS_EXTENSION_PATH "${OHOS_ROOT_PATH}/third_party/vk-gl-cts/build/external/spirv-tools/spirv-tools")
# temp solution
#set(SPIRV_CROSS_PATH "${OHOS_ROOT_PATH}/third_party/spirv-cross")
set(SPIRV_CROSS_PATH "${OHOS_ROOT_PATH}/third_party/skia/third_party/externals/spirv-cross")
set(GLSLANG_PATH "${OHOS_ROOT_PATH}/third_party/glslang")
set(KHRONOS_GL "${OHOS_ROOT_PATH}/third_party/openGLES/api/GL")
set(KHRONOS_KHR "${OHOS_ROOT_PATH}/third_party/EGL/api")
# vulkan-sdk-1.3.275.0
# SPIRV-Headers causes "INTERFACE_LIBRARY targets may only have whitelisted properties.", but we don't need to include is as it's header only
add_library(SPIRV-Headers INTERFACE)
target_sources(SPIRV-Headers INTERFACE "${OHOS_ROOT_PATH}/third_party/spirv-headers")
set(SPIRV-Headers_SOURCE_DIR "${OHOS_ROOT_PATH}/third_party/spirv-headers")
message(STATUS "spirv-tools: ${SPIRV_TOOLS_PATH} spirv-cross: ${SPIRV_CROSS_PATH} spirv-headers: ${SPIRV-Headers_SOURCE_DIR} glslang: ${GLSLANG_PATH}")
add_subdirectory(${SPIRV_TOOLS_PATH} "3rdPartyST" EXCLUDE_FROM_ALL)
add_subdirectory(${GLSLANG_PATH} "3rdPartyGL" EXCLUDE_FROM_ALL)
add_subdirectory(${SPIRV_CROSS_PATH} "3rdPartCross" EXCLUDE_FROM_ALL )
set(GLSLANG_VERSION "120000")

# else()
    # add_subdirectory(3rdparty 3rdparty)

    # set(GLSLANG_VERSION 120000)
# endif()

set(sources 
    src/compiler_main.h
    src/compiler_main.cpp
    src/shader_type.h
    src/default_limits.h
    src/default_limits.cpp
    src/io/dev/FileMonitor.h
    src/io/dev/FileMonitor.cpp
    src/lume/Log.h
    src/lume/log/Logger.h
    src/lume/log/Logger.cpp
    src/lume/log/LoggerOutput.cpp
    src/array_view.h
    src/spirv_cross_helpers_gles.cpp
    src/spirv_cross_helpers_gles.h
    src/spirv_cross_helper_structs_gles.h
    src/spirv_opt_strip_extensions.h
    src/spirv_opt_strip_extensions.cpp
    src/spirv_opt_extensions.h
    src/spirv_opt_extensions.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

set(LibName "ShaderCompilerLib")
# build shaderCompiler as libary
add_library(${LibName} ${sources})
target_include_directories(${LibName} PUBLIC
    include
    src
    # 3rdparty/khronos
    # ${SPIRV_TOOLS_SOURCE_DIR}
    # ${CMAKE_BINARY_DIR}/3rdparty/SPIRV-Tools
    ${SPIRV_TOOLS_PATH}
    ${SPIRV_TOOLS_EXTENSION_PATH}
    ${KHRONOS_KHR}
    ${KHRONOS_GL}
)

# add_dependencies(${LibName} enum_string_mapping)
target_compile_definitions(${LibName} PRIVATE GLSLANG_VERSION=${GLSLANG_VERSION} )
target_compile_definitions(${LibName} PRIVATE WIN32_LEAN_AND_MEAN=1)
# Strip all debug logging code from a release build.
target_compile_definitions(${LibName} PRIVATE
    __STDC_WANT_LIB_EXT1__=1
    "$<$<CONFIG:Release>:LUME_LOG_NO_DEBUG>"
    "$<$<CONFIG:MinSizeRel>:LUME_LOG_NO_DEBUG>"
)

target_link_libraries(${LibName} PRIVATE
    glslang
    SPIRV
    SPIRV-Tools-opt
    spirv-cross-core
    spirv-cross-glsl
    $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
    $<$<AND:$<CXX_COMPILER_ID:Clang>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
)

if(ipoSupported)
    set_property(TARGET ${LibName} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

set(executable "LumeShaderCompiler")
add_executable(${executable} src/main.cpp)

target_link_libraries(${executable} PRIVATE
    ${LibName}
)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    target_link_libraries(${executable} PRIVATE c++abi)
    set_target_properties(${executable} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

if(ipoSupported)
    set_property(TARGET ${executable} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

option(SHADERCOMPILER_UNIT_TESTS_ENABLED "Build SHADERCOMPILER unit tests" OFF)
#
# Testing project. Not enabled by default.
#
if(SHADERCOMPILER_UNIT_TESTS_ENABLED)
	# API Unit tests
	add_subdirectory(test/src_unit_test)
	add_test(NAME shaderCompilerSRC COMMAND shaderCompilerSRCTestRunner)

	enable_testing()
endif()


