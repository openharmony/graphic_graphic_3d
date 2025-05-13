#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

project(LumeMetaPlugin LANGUAGES CXX)

#
# Plugin
#
set(PLUGIN_NAME LumeMeta)
add_library(${PLUGIN_NAME} SHARED)
add_library(${PLUGIN_NAME}::Plugin ALIAS ${PLUGIN_NAME})

target_sources(${PLUGIN_NAME} PRIVATE
    animation/animation.h
    animation/animation_controller.cpp
    animation/animation_controller.h
    animation/animation_modifier.h
    animation/animation_state.cpp
    animation/animation_state.h
    animation/interpolator.cpp
    animation/interpolator.h
    animation/intf_animation_internal.h
    animation/keyframe_animation.cpp
    animation/keyframe_animation.h
    animation/modifiers/loop.cpp
    animation/modifiers/loop.h
    animation/modifiers/reverse.cpp
    animation/modifiers/reverse.h
    animation/modifiers/speed.cpp
    animation/modifiers/speed.h
    animation/parallel_animation.cpp
    animation/parallel_animation.h
    animation/property_animation.cpp
    animation/property_animation.h
    animation/sequential_animation.cpp
    animation/sequential_animation.h
    animation/staggered_animation_state.cpp
    animation/staggered_animation_state.h
    animation/track_animation.cpp
    animation/track_animation.h
    animation/track_animation_state.cpp
    animation/track_animation_state.h
    attachment_container.cpp
    attachment_container.h
    base_object.h
    call_context.cpp
    call_context.h
    class_registry.cpp
    class_registry.h
    connector.cpp
    connector.h
    container/container.cpp
    container/container.h
    container/container_base.cpp
    container/container_base.h
    container/flat_container.cpp
    container/flat_container.h
    container/object_container.cpp
    container/object_container.h
    container/object_flat_container.cpp
    container/object_flat_container.h
    container_observer.cpp
    container_observer.h
    content_object.cpp
    curves/bezier_curve.cpp
    curves/bezier_curve.h
    curves/easing_curve.cpp
    curves/easing_curve.h
    engine/engine_input_property_manager.cpp
    engine/engine_input_property_manager.h
    engine/engine_value.cpp
    engine/engine_value.h
    engine/engine_value_manager.cpp
    engine/engine_value_manager.h
    functions.cpp
    functions.h
    future.cpp
    future.h
    loaders/class_content_loader.cpp
    loaders/class_content_loader.h
    loaders/csv_parser.cpp
    loaders/csv_parser.h
    loaders/csv_string_resource_loader.cpp
    loaders/csv_string_resource_loader.h
    loaders/json_content_loader.cpp
    loaders/json_content_loader.h
    manual_clock.cpp
    meta_object_lib.cpp
    meta_object_lib.h
    model/composite_object_provider.cpp
    model/composite_object_provider.h
    model/container_data_model.cpp
    model/container_data_model.h
    model/content_loader_object_provider.cpp
    model/content_loader_object_provider.h
    model/instantiating_object_provider.cpp
    model/instantiating_object_provider.h
    model/object_provider_base.cpp
    model/object_provider_base.h
    number.cpp
    number.h
    object.h
    object_context.cpp
    object_context.h
    object_data_container.cpp
    object_data_container.h
    object_hierarchy_observer.cpp
    object_hierarchy_observer.h
    object_name.h
    object_registry.cpp
    object_registry.h
    plugin.cpp
    polling_task_queue.cpp
    property/bind.cpp
    property/bind.h
    property/dependencies.cpp
    property/dependencies.h
    property/property.cpp
    property/property.h
    property/stack_property.cpp
    property/stack_property.h
    proxy_object.cpp
    proxy_object.h
    random.cpp
    random.h
    ref_uri_util.cpp
    ref_uri_util.h
    register_anys.cpp
    register_default_anims.cpp
    register_default_objs.cpp
    register_engine_access.cpp
    register_entities.cpp
    register_value_serializers.cpp
    serialization/backend/debug_output.cpp
    serialization/backend/debug_output.h
    serialization/backend/json_input.cpp
    serialization/backend/json_input.h
    serialization/backend/json_output.cpp
    serialization/backend/json_output.h
    serialization/exporter.cpp
    serialization/exporter.h
    serialization/importer.cpp
    serialization/importer.h
    serialization/json_exporter.cpp
    serialization/json_exporter.h
    serialization/json_importer.cpp
    serialization/json_importer.h
    serialization/metav1_compat.cpp
    serialization/metav1_compat.h
    startable_object_controller.cpp
    startable_object_controller.h
    system_clock.cpp
    task_queue.h
    threaded_task_queue.cpp
    resource/file_resource_manager.h
    resource/file_resource_manager.cpp
    resource/object_resource.h
    resource/object_resource.cpp
)

set_source_files_properties(register_default_props.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

target_compile_definitions(${PLUGIN_NAME} PRIVATE CORE_PLUGIN=1)

target_link_libraries(${PLUGIN_NAME} PRIVATE 
    ${PLUGIN_NAME}API
    ${PLUGIN_NAME}ExtAPI
    ${PLUGIN_NAME}HighLevelAPI
)

if(MSVC)
    target_compile_options(${PLUGIN_NAME} PRIVATE /GR-)
    if(CMAKE_CXX_FLAGS MATCHES "/EHs ")
      string(REPLACE "/EHs" "/EHs-c- /D_HAS_EXCEPTIONS=0" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    endif()
elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${PLUGIN_NAME} PRIVATE -Oz -fno-exceptions -fno-rtti)
endif()

# Utilize common linker flags from Base
include(${BASE_ROOT_DIRECTORY}/cmake/cmake_tools.cmake)
append_link_flags(${PLUGIN_NAME} "${BASE_SO_LINK_FLAGS}")

# Code inspection
include(LumeMetaTargetCodeInspection)
meta_object_target_code_inspection(${PLUGIN_NAME})

set_target_properties(${PLUGIN_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    OUTPUT_NAME Plugin${PLUGIN_NAME}
)

#
# IDE project tree
#
get_target_property(IMPL_SOURCES ${PLUGIN_NAME} SOURCES)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX src FILES ${IMPL_SOURCES})