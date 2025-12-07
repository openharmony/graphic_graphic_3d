#
# Copyright (C) 2018 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(ShaderCompilerSRCTest)
#
# Test runner exe/library.
#
set(executableName ShaderCompilerSRCTestRunner)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#
# Lume dependencies.
#
#if (NOT DEFINED SDK_ROOT_PATH)
#   set(SDK_ROOT_PATH "D:/LumeEditor/EditorQt/SDK")
#endif()
#list(APPEND CMAKE_MODULE_PATH "${SDK_ROOT_PATH}/external")
#list(APPEND CMAKE_MODULE_PATH "${SDK_ROOT_PATH}/external/LumeBase/cmake")


if(NOT TARGET gtest)
    add_subdirectory(../3rdparty test/3rdparty EXCLUDE_FROM_ALL)
endif()

set(test_sources 
	src/TestRunner.h
    src/TestRunner.cpp
	src/spirv/ShaderCompilerTest.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${test_sources})

if(ANDROID)
    find_library(android_lib android)
    find_library(log_lib log)
    add_library(${executableName} SHARED ${test_sources} ${CMAKE_CURRENT_LIST_DIR}/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(${executableName} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/android/native_app_glue)
    target_link_libraries(${executableName} ${android_lib} ${log_lib})
else()
    add_executable(${executableName} ${test_sources})
	target_compile_definitions(${executableName} PRIVATE WIN32_LEAN_AND_MEAN=1)
endif()


set(ROOT_DIRECTORY ../..)
# We need access to the private includes of the engine.
target_include_directories(${executableName} PRIVATE 
    ${ROOT_DIRECTORY}/src
	${ROOT_DIRECTORY}/src/lume
	${ROOT_DIRECTORY}/src/io
)

target_link_libraries(${executableName}
    gtest
	ShaderCompilerLib
)

set_target_properties(${executableName} PROPERTIES FOLDER Test)
set_property(TARGET ${executableName} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>")
# Disable RTTI
if(MSVC)
    target_compile_options(${executableName} PRIVATE /GR-)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${executableName} PRIVATE -fno-rtti)
endif()

if(MINGW)
    # Linking mingw and c/c++ standard libs statically for easier usage.
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

#
# Add ROFS for testing
#
#build_rofs(CompileTestAssets ${executableName} "${CMAKE_CURRENT_LIST_DIR}/../assets/" "/" "BINARYDATAFORTEST" "SIZEOFDATAFORTEST" "testrofs")
#add_dependencies(${executableName} CompileTestAssets)

#
# Clang format target
#
add_custom_target(Format${executableName}
    COMMAND clang-format -verbose -i ${test_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(Format${executableName} PROPERTIES FOLDER Formatting)
