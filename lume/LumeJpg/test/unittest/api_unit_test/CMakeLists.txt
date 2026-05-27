#
# Copyright (C) 2026 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.18)

project(JpgAPITest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../cmake")

if(NOT TARGET PluginJPGAPI)
    # Point to the PluginJPG project if it was not defined yet.
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../../../ PluginJPG EXCLUDE_FROM_ALL)
endif()

if (NOT TARGET AGPBase::AGPBaseAPI)
    find_package(AGPBaseAPI REQUIRED)
endif()
if (NOT TARGET AGPEngine::AGPEngineDLL)
   find_package(AGPEngine REQUIRED)
endif()

if(NOT TARGET gtest)
    add_subdirectory(${BASE_ROOT_DIRECTORY}/test/3rdparty/googletest test/3rdparty/googletest EXCLUDE_FROM_ALL)
endif()

set(COMMON_DIR ${CORE_ROOT_DIRECTORY}/test/unittest/common)

set(test_sources
    src/test_runner.h
    src/test_runner.cpp
    src/image/jpg_loader_test.cpp
)

set(common_test_sources
    ${COMMON_DIR}/log_util.h
    ${COMMON_DIR}/log_util.cpp
    ${COMMON_DIR}/load_lib.h
    ${COMMON_DIR}/load_lib.cpp
    ${COMMON_DIR}/test_runner_common.h
    ${COMMON_DIR}/test_framework.h
)

if(ANDROID)
    list(APPEND test_sources
        src/test_runner_android.h
        src/test_runner_android.cpp
    )
elseif(OHOS)
    list(APPEND common_test_sources
        ${COMMON_DIR}/test_runner_ohos.h
        ${COMMON_DIR}/test_runner_ohos.cpp
    )
elseif(WIN32)
    list(APPEND common_test_sources
        ${COMMON_DIR}/test_runner_windows.h
        ${COMMON_DIR}/test_runner_windows.cpp
    )
else() # Linux
    list(APPEND common_test_sources
        ${COMMON_DIR}/test_runner_linux.h
        ${COMMON_DIR}/test_runner_linux.cpp
    )
endif()

source_group(TREE ${CMAKE_CURRENT_LIST_DIR} FILES ${test_sources})
source_group(TREE ${COMMON_DIR}/.. PREFIX common FILES ${common_test_sources})

#
# Test runner exe/library.
#
if(OHOS)
    set(executableName entry)
else()
    set(executableName JpgAPITestRunner)
endif()

if(ANDROID)
    find_library(android_lib android)
    find_library(log_lib log)
    add_library(${executableName} SHARED
        ${common_test_sources} ${test_sources}
        "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c"
    )
    target_include_directories(${executableName} PRIVATE "${CMAKE_ANDROID_NDK}/sources/android/native_app_glue")
    target_link_libraries(${executableName} ${android_lib} ${log_lib})
elseif(OHOS)
    add_library(${executableName} SHARED ${common_test_sources} ${test_sources})
    target_link_libraries(${executableName} libace_napi.z.so libace_ndk.z.so libhilog_ndk.z.so libbundle_ndk.z.so librawfile.z.so)
else()
    add_executable(${executableName} ${common_test_sources} ${test_sources})
    target_compile_definitions(${executableName} PRIVATE WIN32_LEAN_AND_MEAN=1)
endif()

add_dependencies(${executableName} AGPEngine::AGPEngineDLL PluginJPG)
target_compile_definitions(${executableName} PRIVATE CORE_DYNAMIC=1 NOMINMAX)

target_include_directories(${executableName} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${COMMON_DIR}
)

# Disable RTTI
if(MSVC)
    target_compile_options(${executableName} PRIVATE /GR-)
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${executableName} PRIVATE -fno-rtti)
    if(JPG_UNIT_TESTS_WITH_SANITIZERS AND NOT ANDROID AND NOT OHOS)
        target_compile_options(${executableName} PRIVATE -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer -fsanitize-recover=address)
        target_link_options(${executableName} PRIVATE -fsanitize=address -fsanitize=undefined)
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(X11 REQUIRED COMPONENTS xcb)
    include_directories(${X11_xcb_INCLUDE_PATH})
    target_link_libraries(${executableName} ${X11_xcb_LIB})
endif()

target_link_libraries(${executableName}
    AGPBase::AGPBaseAPI
    AGPEngine::AGPEngineAPI
    AGPEngine::ComponentHelperSDK
    PluginJPGAPI
    gtest
)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
endif()

# Required copies
if(OHOS)
    add_custom_command(
        TARGET ${executableName} POST_BUILD

        # Engine DLL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AGPEngine::AGPEngineDLL> $<TARGET_FILE_DIR:${executableName}>

        # Assets: LumeJpg-specific assets (security test files etc.)
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ $<TARGET_FILE_DIR:${executableName}>/../../../../res/default/resources/rawfile/

        # Assets: LumeEngine jpg test images
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CORE_ROOT_DIRECTORY}/test/unittest/assets/test_data/image/canine_284x323.jpg $<TARGET_FILE_DIR:${executableName}>/../../../../res/default/resources/rawfile/test_data/image/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CORE_ROOT_DIRECTORY}/test/unittest/assets/test_data/image/canine_512x512.jpg $<TARGET_FILE_DIR:${executableName}>/../../../../res/default/resources/rawfile/test_data/image/
    )
else()
    add_custom_command(
        TARGET ${executableName} POST_BUILD

        # Plugins dir
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/plugins/

        # Engine DLL and JPG plugin
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:AGPEngine::AGPEngineDLL> $<TARGET_FILE_DIR:${executableName}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:PluginJPG>               $<TARGET_FILE_DIR:${executableName}>/plugins/

        # Assets: LumeJpg-specific assets (security test files etc.)
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/../../assets/
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../assets/ $<TARGET_FILE_DIR:${executableName}>/../../assets/

        # Assets: LumeEngine jpg test images
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${executableName}>/../../assets/test_data/image/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CORE_ROOT_DIRECTORY}/test/unittest/assets/test_data/image/canine_284x323.jpg $<TARGET_FILE_DIR:${executableName}>/../../assets/test_data/image/
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CORE_ROOT_DIRECTORY}/test/unittest/assets/test_data/image/canine_512x512.jpg $<TARGET_FILE_DIR:${executableName}>/../../assets/test_data/image/
    )
endif()

set_target_properties(${executableName} PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${executableName}>"
    FOLDER Testing
)

#
# Clang format target
#
add_custom_target(Format${executableName}
    COMMAND clang-format -verbose -i ${test_sources}
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
set_target_properties(Format${executableName} PROPERTIES FOLDER Formatting)
