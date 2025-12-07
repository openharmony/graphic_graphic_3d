#
# Copyright (C) 2018 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(LumeAssetCompilerTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT TARGET LumeAssetCompiler)
    # Point to the AGPBase project if it was not defined yet.
    add_subdirectory(.. LumeAssetCompiler EXCLUDE_FROM_ALL)
endif()


// need notice
string(REPLACE "/foundation" ";" TEMP_PATH ${CMAKE_CURRENT_LIST_FILE})
list(GET TEMP_PATH 0 OHOS_ROOT_PATH)

set(GTEST_PATH "${OHOS_ROOT_PATH}/third_party/googletest")
add_subdirectory(${GTEST_PATH} "3rdPartyGTest" EXCLUDE_FROM_ALL)

set(test_sources
    src/test.cpp
    src/TestRunner.cpp
    src/TestRunner.h
)

set(app_sources
    ${CMAKE_SOURCE_DIR}/src/maco.h
    ${CMAKE_SOURCE_DIR}/src/platform.cpp
    ${CMAKE_SOURCE_DIR}/src/app.cpp
    ${CMAKE_SOURCE_DIR}/src/dir.cpp
)

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${test_sources})

#
# Test runner exe/library.
#
set(executableName LumeAssetCompilerTestRunner)

if(ANDROID)
    find_library(android_lib android)
    find_library(log_lib log)
    add_library(${executableName} SHARED ${test_sources} ${CMAKE_CURRENT_LIST_DIR}/android/native_app_glue/android_native_app_glue.c)
    target_include_directories(${executableName} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/android/native_app_glue)
    target_link_libraries(${executableName} ${android_lib} ${log_lib})
else()
    add_executable(${executableName} ${test_sources}    ${app_sources})
	target_compile_definitions(${executableName} PRIVATE 
        WIN32_LEAN_AND_MEAN=1 
        _CRT_SECURE_NO_WARNINGS
        TEST_ASSET="${CMAKE_CURRENT_SOURCE_DIR}/assets/")
endif()

target_include_directories(${executableName} PRIVATE ${CMAKE_SOURCE_DIR}/src src)

# Disable RTTI
if(MSVC)
    target_compile_options(${executableName} PRIVATE /GR-)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${executableName} PRIVATE -fno-rtti)
endif()

if(WIN32)
  target_link_libraries(${executableName} wsock32 ws2_32)
endif()

target_link_libraries(${executableName} gtest)

if(MINGW)
    # Linking mingw and c/c++ standard libs statically for easier usage.
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

#
# Clang format target
#
add_custom_target(Format${executableName}
    COMMAND clang-format -verbose -i ${test_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(Format${executableName} PROPERTIES FOLDER Formatting)
