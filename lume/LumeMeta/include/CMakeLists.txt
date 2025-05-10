#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

project(LumeMetaAPI LANGUAGES CXX)

#
# Dependencies
#
if (NOT TARGET AGPBaseAPI)
    find_package(AGPBaseAPI REQUIRED)
endif()
if (NOT TARGET AGPEngineAPI)
    find_package(AGPEngineAPI REQUIRED)
endif()

#
# API
#
set(PLUGIN_NAME LumeMeta)
add_library(${PLUGIN_NAME}Base INTERFACE)
target_sources(${PLUGIN_NAME}Base PUBLIC
    meta/base/algorithms.h
    meta/base/bit_field.h
    meta/base/capture.h
    meta/base/expected.h
    meta/base/ids.h
    meta/base/interface_macros.h
    meta/base/interface_traits.h
    meta/base/interface_utils.h
    meta/base/macros.h
    meta/base/meta_types.h
    meta/base/namespace.h
    meta/base/object_traits.h
    meta/base/plugin.h
    meta/base/ref_uri.h
    meta/base/time_span.h
    meta/base/type_traits.h
    meta/base/types.h
    meta/base/version.h
)

target_include_directories(${PLUGIN_NAME}Base INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(${PLUGIN_NAME}Base INTERFACE AGPBase::AGPBaseAPI AGPEngine::AGPEngineAPI)

# Meta object interface (for users of meta objects)
add_library(${PLUGIN_NAME}API INTERFACE)
add_library(${PLUGIN_NAME}::API ALIAS ${PLUGIN_NAME}API)
target_sources(${PLUGIN_NAME}API PUBLIC
    meta/interface/animation/builtin_animations.h
    meta/interface/animation/intf_animation.h
    meta/interface/animation/intf_animation_controller.h
    meta/interface/animation/intf_animation_modifier.h
    meta/interface/animation/intf_interpolator.h
    meta/interface/animation/modifiers/intf_loop.h
    meta/interface/animation/modifiers/intf_speed.h
    meta/interface/builtin_objects.h
    meta/interface/curves/intf_bezier.h
    meta/interface/curves/intf_curve.h
    meta/interface/curves/intf_curve_1d.h
    meta/interface/curves/intf_curve_2d.h
    meta/interface/curves/intf_easing_curve.h
    meta/interface/detail/any.h
    meta/interface/detail/any_pointer_compatibility.h
    meta/interface/detail/array_property.h
    meta/interface/detail/enum.h
    meta/interface/detail/multi_type_any.h
    meta/interface/detail/property.h
    meta/interface/engine/intf_engine_data.h
    meta/interface/engine/intf_engine_input_property_manager.h
    meta/interface/engine/intf_engine_type.h
    meta/interface/engine/intf_engine_value.h
    meta/interface/engine/intf_engine_value_manager.h
    meta/interface/enum_macros.h
    meta/interface/event.h
    meta/interface/interface_helpers.h
    meta/interface/interface_macros.h
    meta/interface/intf_any.h
    meta/interface/intf_attach.h
    meta/interface/intf_attachment.h
    meta/interface/intf_attachment_container.h
    meta/interface/intf_call_context.h
    meta/interface/intf_callable.h
    meta/interface/intf_class_registry.h
    meta/interface/intf_clock.h
    meta/interface/intf_cloneable.h
    meta/interface/intf_comparable.h
    meta/interface/intf_connector.h
    meta/interface/intf_containable.h
    meta/interface/intf_container.h
    meta/interface/intf_container_observer.h
    meta/interface/intf_container_query.h
    meta/interface/intf_content.h
    meta/interface/intf_derived.h
    meta/interface/intf_enum.h
    meta/interface/intf_event.h
    meta/interface/intf_function.h
    meta/interface/intf_future.h
    meta/interface/intf_iterable.h
    meta/interface/intf_lifecycle.h
    meta/interface/intf_lockable.h
    meta/interface/intf_manual_clock.h
    meta/interface/intf_meta_object_lib.h
    meta/interface/intf_metadata.h
    meta/interface/intf_named.h
    meta/interface/intf_notify_on_change.h
    meta/interface/intf_object.h
    meta/interface/intf_object_context.h
    meta/interface/intf_object_factory.h
    meta/interface/intf_object_flags.h
    meta/interface/intf_object_hierarchy_observer.h
    meta/interface/intf_object_registry.h
    meta/interface/intf_promise.h
    meta/interface/intf_proxy_object.h
    meta/interface/intf_recyclable.h
    meta/interface/intf_required_interfaces.h
    meta/interface/intf_startable.h
    meta/interface/intf_startable_controller.h
    meta/interface/intf_static_metadata.h
    meta/interface/intf_task_queue.h
    meta/interface/intf_task_queue_extend.h
    meta/interface/intf_task_queue_registry.h
    meta/interface/intf_tickable.h
    meta/interface/intf_tickable_controller.h
    meta/interface/intf_value.h
    meta/interface/loaders/intf_class_content_loader.h
    meta/interface/loaders/intf_content_loader.h
    meta/interface/loaders/intf_dynamic_content_loader.h
    meta/interface/loaders/intf_file_content_loader.h
    meta/interface/metadata_query.h
    meta/interface/model/data_model_index.h
    meta/interface/model/intf_data_model.h
    meta/interface/model/intf_object_provider.h
    meta/interface/object_macros.h
    meta/interface/object_type_info.h
    meta/interface/property/array_property.h
    meta/interface/property/construct_array_property.h
    meta/interface/property/construct_property.h
    meta/interface/property/intf_bind.h
    meta/interface/property/intf_modifier.h
    meta/interface/property/intf_property.h
    meta/interface/property/intf_property_internal.h
    meta/interface/property/intf_property_register.h
    meta/interface/property/intf_stack_property.h
    meta/interface/property/intf_stack_resetable.h
    meta/interface/property/property.h
    meta/interface/property/property_events.h
    meta/interface/resource/intf_dynamic_resource.h
    meta/interface/resource/intf_resource.h
    meta/interface/serialization/intf_export_context.h
    meta/interface/serialization/intf_exporter.h
    meta/interface/serialization/intf_global_serialization_data.h
    meta/interface/serialization/intf_import_context.h
    meta/interface/serialization/intf_importer.h
    meta/interface/serialization/intf_ser_input.h
    meta/interface/serialization/intf_ser_node.h
    meta/interface/serialization/intf_ser_output.h
    meta/interface/serialization/intf_ser_transformation.h
    meta/interface/serialization/intf_serializable.h
    meta/interface/serialization/intf_value_serializer.h
    meta/interface/serialization/ser_metadata.h
    meta/interface/simple_event.h
    meta/interface/static_object_metadata.h
    meta/interface/threading/primitive_api.h
)

target_include_directories(${PLUGIN_NAME}API INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(${PLUGIN_NAME}API INTERFACE ${PLUGIN_NAME}Base)

# Meta object helper implementations. (for implementers of meta objects)
add_library(${PLUGIN_NAME}HighLevelAPI INTERFACE)
add_library(${PLUGIN_NAME}::HighLevelAPI ALIAS ${PLUGIN_NAME}HighLevelAPI)
target_sources(${PLUGIN_NAME}HighLevelAPI PUBLIC
    meta/api/animation.h
    meta/api/array_util.h
    meta/api/call_context.h
    meta/api/compatible_value_util.h
    meta/api/connector.h
    meta/api/container/find_cache.h
    meta/api/container/find_containers.h
    meta/api/container/observer.h
    meta/api/curves.h
    meta/api/deferred_callback.h
    meta/api/engine/util.h
    meta/api/event_handler.h
    meta/api/function.h
    meta/api/future.h
    meta/api/internal/breadth_first_iteration.h
    meta/api/internal/depth_first_iteration.h
    meta/api/internal/iteration.h
    meta/api/internal/iteration_types.h
    meta/api/interface_object.h
    meta/api/iteration.h
    meta/api/locking.h
    meta/api/make_callback.h
    meta/api/number.h
    meta/api/object.h
    meta/api/property/array_element_bind.h
    meta/api/property/array_property_changes_recognizer.h
    meta/api/property/array_property_event_handler.h
    meta/api/property/binding.h
    meta/api/property/custom_value.h
    meta/api/property/default_value_bind.h
    meta/api/property/property_event_handler.h
    meta/api/property/property_info.h
    meta/api/task.h
    meta/api/task_queue.h
    meta/api/threading/mutex.h
    meta/api/threading/primitive_api.h
    meta/api/timer.h
    meta/api/util.h
)

target_include_directories(${PLUGIN_NAME}HighLevelAPI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(${PLUGIN_NAME}HighLevelAPI INTERFACE ${PLUGIN_NAME}API)

# Meta object helper implementations. (for implementers of meta objects)
add_library(${PLUGIN_NAME}ExtAPI INTERFACE)
add_library(${PLUGIN_NAME}::ExtAPI ALIAS ${PLUGIN_NAME}ExtAPI)
target_sources(${PLUGIN_NAME}ExtAPI PUBLIC
    meta/ext/animation/interpolator.h
    meta/ext/any_builder.h
    meta/ext/attachment/attachment.h
    meta/ext/attachment/behavior.h
    meta/ext/base_object.h
    meta/ext/base_object_fwd.h
    meta/ext/engine/core_any.h
    meta/ext/engine/core_enum_any.h
    meta/ext/engine/internal_access.h
    meta/ext/event_impl.h
    meta/ext/event_util.h
    meta/ext/implementation_macros.h
    meta/ext/metadata_helpers.h
    meta/ext/minimal_object.h
    meta/ext/object.h
    meta/ext/object_container.h
    meta/ext/object_factory.h
    meta/ext/object_fwd.h
    meta/ext/resolve_helper.h
    meta/ext/serialization/common_value_serializers.h
    meta/ext/serialization/serializer.h
    meta/ext/serialization/value_serializer.h
    meta/ext/task_queue.h
)

target_include_directories(${PLUGIN_NAME}ExtAPI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
target_link_libraries(${PLUGIN_NAME}ExtAPI INTERFACE ${PLUGIN_NAME}API ${PLUGIN_NAME}HighLevelAPI)

set_target_properties(${PLUGIN_NAME}Base PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)

set_target_properties(${PLUGIN_NAME}API PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)

set_target_properties(${PLUGIN_NAME}HighLevelAPI PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)

set_target_properties(${PLUGIN_NAME}ExtAPI PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)

#
# IDE project tree
#
get_target_property(BASE_PATH ${PLUGIN_NAME}Base INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(BASE_SOURCES ${PLUGIN_NAME}Base INTERFACE_SOURCES)
source_group(TREE ${BASE_PATH} FILES ${BASE_SOURCES})

get_target_property(API_PATH ${PLUGIN_NAME}API INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(API_SOURCES ${PLUGIN_NAME}API INTERFACE_SOURCES)
source_group(TREE ${API_PATH} FILES ${API_SOURCES})

get_target_property(HIGHLEVEL_API_PATH ${PLUGIN_NAME}HighLevelAPI INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(HIGHLEVEL_API_SOURCES ${PLUGIN_NAME}HighLevelAPI INTERFACE_SOURCES)
source_group(TREE ${HIGHLEVEL_API_PATH} FILES ${HIGHLEVEL_API_SOURCES})

get_target_property(EXT_API_PATH ${PLUGIN_NAME}ExtAPI INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(EXT_API_SOURCES ${PLUGIN_NAME}ExtAPI INTERFACE_SOURCES)
source_group(TREE ${EXT_API_PATH} FILES ${EXT_API_SOURCES})