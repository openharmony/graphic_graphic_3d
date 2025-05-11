#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

project(LumeSceneAPI LANGUAGES CXX)

#
# API
#
set(PLUGIN_NAME LumeScene)
add_library(${PLUGIN_NAME}API INTERFACE)
add_library(${PLUGIN_NAME}::API ALIAS ${PLUGIN_NAME}API)

target_sources(${PLUGIN_NAME}API PUBLIC
    scene/api/ecs_scene.h
    scene/api/external_node.h
    scene/api/node.h
    scene/api/post_process.h
    scene/api/resource.h
    scene/api/scene.h
    scene/base/namespace.h
    scene/base/types.h
    scene/ext/component.h
    scene/ext/component_util.h
    scene/ext/intf_component.h
    scene/ext/intf_component_factory.h
    scene/ext/intf_create_entity.h
    scene/ext/intf_ecs_context.h
    scene/ext/intf_ecs_event_listener.h
    scene/ext/intf_ecs_object.h
    scene/ext/intf_ecs_object_access.h
    scene/ext/intf_internal_camera.h
    scene/ext/intf_internal_scene.h
    scene/ext/util.h
    scene/interface/intf_camera.h
    scene/interface/intf_environment.h
    scene/interface/intf_image.h
    scene/interface/intf_input_receiver.h
    scene/interface/intf_layer.h
    scene/interface/intf_light.h
    scene/interface/intf_material.h
    scene/interface/intf_mesh.h
    scene/interface/intf_mesh_resource.h
    scene/interface/intf_node.h
    scene/interface/intf_node_import.h
    scene/interface/intf_postprocess.h
    scene/interface/intf_raycast.h
    scene/interface/intf_scene.h
    scene/interface/intf_scene_manager.h
    scene/interface/intf_shader.h
    scene/interface/intf_text.h
    scene/interface/intf_texture.h
    scene/interface/intf_transform.h
    scene/interface/postprocess/intf_bloom.h
    scene/interface/postprocess/intf_postprocess_effect.h
    scene/interface/postprocess/intf_tonemap.h
    scene/interface/resource/image_info.h
    scene/interface/resource/intf_render_resource_manager.h
    scene/interface/resource/types.h
)

target_include_directories(${PLUGIN_NAME}API INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)

set_target_properties(${PLUGIN_NAME}API PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
)

#
# IDE project tree
#
get_target_property(API_PATH ${PLUGIN_NAME}API INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(API_SOURCES ${PLUGIN_NAME}API INTERFACE_SOURCES)
source_group(TREE ${API_PATH} FILES ${API_SOURCES})
