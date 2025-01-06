#
# Copyright (C) 2021 Huawei Technologies Co, Ltd.
#

#
# Plugin library.
#

add_library(SceneAPI INTERFACE)
set_target_properties(SceneAPI PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1)

target_sources(SceneAPI PUBLIC
    scene/base/namespace.h
    scene/base/types.h
    scene/ext/component_fwd.h
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
    scene/interface/intf_bitmap.h
    scene/interface/intf_camera.h
    scene/interface/intf_environment.h
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
)

target_include_directories(SceneAPI INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/)
add_library(Scene::API ALIAS SceneAPI)

get_target_property(API_PATH  Scene::API INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(API_SOURCES Scene::API INTERFACE_SOURCES)
source_group(TREE  ${API_PATH} PREFIX "Api Headers" FILES ${API_SOURCES} )
