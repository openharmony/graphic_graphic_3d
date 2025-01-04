
cmake_minimum_required(VERSION 3.19)

project (MetaObjectBenchmarks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(MetaObjectAPI REQUIRED)
find_package(MetaObjectExtAPI REQUIRED)
find_package(MetaObjectHighLevelAPI REQUIRED)
find_package(MetaObject QUIET)

add_executable(MetaObjectBenchmarks)

target_sources(MetaObjectBenchmarks
        PRIVATE
        src/main.cpp
        src/metadata_benchmarks.cpp
        src/object_registry_benchmarks.cpp
        src/object_benchmarks.cpp
        src/property_benchmarks.cpp
        src/property_utils.h
        src/shared_ptr_benchmarks.cpp
        src/task_queue_benchmarks.cpp
        src/utils.cpp
        src/utils.h
        )

target_link_libraries(MetaObjectBenchmarks PRIVATE benchmark)
target_link_libraries(MetaObjectBenchmarks PRIVATE MetaObject::ExtAPI
                                          MetaObject::HighLevelAPI
                                          MetaObject::EnvironmentSetup
)

include(TargetPluginsDependencies)

target_plugins_dependencies(MetaObjectBenchmarks
        PLUGINS AGPEngine::AGPEngineDLL
        RELATIVE_DEPLOY_PATH .
        VERBOSE)

target_plugins_dependencies(MetaObjectBenchmarks
        PLUGINS MetaObject::Plugin
        RELATIVE_DEPLOY_PATH "plugins"
        VERBOSE)
