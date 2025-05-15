#
# Copyright (C) 2022 Huawei Technologies Co, Ltd.
#

project(LumeScenePlugin LANGUAGES CXX)

#
# Dependencies
#
if (NOT TARGET AGP3DAPI)
    find_package(AGP3DAPI REQUIRED)
endif()
if (NOT TARGET PluginFontAPI)
    find_package(PluginFontAPI REQUIRED)
endif()
if (NOT TARGET AGP3DTextAPI)
    find_package(AGP3DTextAPI REQUIRED)
endif()
if (NOT TARGET AGPEngine::ComponentHelperSDK)
    find_package(ComponentHelperSDK REQUIRED)
endif()
if (NOT TARGET LumeMeta::HighLevelAPI)
    find_package(LumeMetaHighLevelAPI REQUIRED)
endif()
if (NOT TARGET LumeMeta::ExtAPI)
    find_package(LumeMetaExtAPI REQUIRED)
endif()

#
# Plugin
#
set(PLUGIN_NAME LumeScene)
add_library(${PLUGIN_NAME} SHARED)
add_library(${PLUGIN_NAME}::Plugin ALIAS ${PLUGIN_NAME})

target_sources(${PLUGIN_NAME} PRIVATE
    asset/asset_object.cpp
    asset/asset_object.h
    component/animation_component.cpp
    component/animation_component.h
    component/camera_component.cpp
    component/camera_component.h
    component/environment_component.cpp
    component/environment_component.h
    component/generic_component.cpp
    component/generic_component.h
    component/layer_component.cpp
    component/layer_component.h
    component/light_component.cpp
    component/light_component.h
    component/material_component.cpp
    component/material_component.h
    component/mesh_component.cpp
    component/mesh_component.h
    component/node_component.cpp
    component/node_component.h
    component/postprocess_component.cpp
    component/postprocess_component.h
    component/text_component.cpp
    component/text_component.h
    component/transform_component.cpp
    component/transform_component.h
    component_factory.h
    core/camera.cpp
    core/camera.h
    core/ecs.cpp
    core/ecs.h
    core/ecs_listener.cpp
    core/ecs_listener.h
    core/ecs_object.cpp
    core/ecs_object.h
    core/internal_scene.cpp
    core/internal_scene.h
    core/intf_internal_raycast.h
    ecs_component/entity_owner_component.h
    ecs_component/entity_owner_component_manager.cpp
    mesh/mesh.cpp
    mesh/mesh.h
    mesh/mesh_creator.cpp
    mesh/mesh_creator.h
    mesh/mesh_resource.h
    mesh/submesh.cpp
    mesh/submesh.h
    mesh/texture.cpp
    mesh/texture.h
    mesh/sampler.cpp
    mesh/sampler.h
    node/camera_node.cpp
    node/camera_node.h
    node/light_node.cpp
    node/light_node.h
    node/mesh_node.cpp
    node/mesh_node.h
    node/node.cpp
    node/node.h
    node/startable_handler.cpp
    node/startable_handler.h
    node/text_node.cpp
    node/text_node.h
    plugin.cpp
    postprocess/bloom.cpp
    postprocess/bloom.h
    postprocess/postprocess.cpp
    postprocess/postprocess.h
    postprocess/tonemap.cpp
    postprocess/tonemap.h
    postprocess/util.h
    register_anys.cpp
    register_engine_access.cpp
    register_serializers.cpp
    render_configuration.cpp
    render_configuration.h
    render_context.h
    resource/ecs_animation.cpp
    resource/ecs_animation.h
    resource/environment.cpp
    resource/environment.h
    resource/image.cpp
    resource/image.h
    resource/material.cpp
    resource/material.h
    resource/render_resource.cpp
    resource/render_resource.h
    resource/render_resource_manager.cpp
    resource/render_resource_manager.h
    resource/resource_types.cpp
    resource/resource_types.h
    resource/shader.cpp
    resource/shader.h
    resource/types/animation_type.cpp
    resource/types/animation_type.h
    resource/types/environment_type.cpp
    resource/types/environment_type.h
    resource/types/gltf_scene_type.cpp
    resource/types/gltf_scene_type.h
    resource/types/image_type.cpp
    resource/types/image_type.h
    resource/types/material_type.cpp
    resource/types/material_type.h
    resource/types/postprocess_type.cpp
    resource/types/postprocess_type.h
    resource/types/scene_type.cpp
    resource/types/scene_type.h
    resource/types/shader_type.cpp
    resource/types/shader_type.h
    scene.cpp
    scene.h
    scene_manager.cpp
    scene_manager.h
    serialization/external_node.h
    serialization/scene_exporter.cpp
    serialization/scene_exporter.h
    serialization/scene_importer.cpp
    serialization/scene_importer.h
    util.h
)

set_source_files_properties(plugin.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

target_compile_definitions(${PLUGIN_NAME} PRIVATE CORE_PLUGIN=1)

target_link_libraries(${PLUGIN_NAME} PRIVATE
    ${PLUGIN_NAME}::API
    AGPEngine::ComponentHelperSDK
    AGPRender::AGPRenderAPI
    AGP3D::AGP3DAPI
    LumeMeta::ExtAPI
    EcsSerializer
    RuntimeUtil
    AGP3DText::AGP3DTextAPI
    PluginFont::PluginFontAPI
)

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
