#
# Copyright (C) 2019 Huawei Technologies Co, Ltd.
#

cmake_minimum_required(VERSION 3.10)

project(LumeAssetCompiler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(sources 
    src/main.cpp
    src/dir.cpp
    src/dir.h
    src/elf_common.h
    src/elf32.h
    src/elf64.h
    src/coff.h
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${sources})

add_executable(LumeAssetCompiler ${sources})
target_include_directories(LumeAssetCompiler PRIVATE include src)
target_compile_definitions(LumeAssetCompiler PRIVATE _CRT_SECURE_NO_WARNINGS)
if(UNIX AND NOT APPLE)
#make a static linked executable for linux.
target_link_libraries(LumeAssetCompiler -static)
endif()