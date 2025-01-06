#
# Copyright (C) 2021 Huawei Technologies Co, Ltd.
#

#
# Plugin library.
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

add_library(ScenePlugin SHARED)

target_sources(ScenePlugin PRIVATE
    asset/asset_object.cpp
    asset/asset_object.h
    bitmap.cpp
    bitmap.h
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
    ecs_animation.cpp
    ecs_animation.h
    environment.cpp
    environment.h
    mesh/material.cpp
    mesh/material.h
    mesh/mesh.cpp
    mesh/mesh.h
    mesh/mesh_creator.cpp
    mesh/mesh_creator.h
    mesh/mesh_resource.h
    mesh/shader.cpp
    mesh/shader.h
    mesh/submesh.cpp
    mesh/submesh.h
    mesh/texture.cpp
    mesh/texture.h
    node/camera_node.cpp
    node/camera_node.h
    node/light_node.cpp
    node/light_node.h
    node/mesh_node.cpp
    node/mesh_node.h
    node/node.cpp
    node/node.h
    node/text_node.cpp
    node/text_node.h
    plugin.cpp
    postprocess/bloom.h
    postprocess/postprocess.cpp
    postprocess/postprocess.h
    postprocess/tonemap.h
    register_engine_access.cpp
    render_configuration.cpp
    render_configuration.h
    scene.cpp
    scene.h
    scene_manager.cpp
    scene_manager.h
    util.h
)

set_source_files_properties(plugin.cpp PROPERTIES COMPILE_FLAGS $<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/bigobj>)

target_compile_definitions(ScenePlugin PRIVATE CORE_PLUGIN=1)

target_link_libraries(ScenePlugin PRIVATE
    Scene::API
    MetaObject::ExtAPI
    AGPRender::AGPRenderAPI
    AGP3D::AGP3DAPI
    AGPEngine::ComponentHelperSDK
    EcsSerializer
    RuntimeUtil
    AGP3DText::AGP3DTextAPI
    PluginFont::PluginFontAPI
)
set_target_properties(ScenePlugin PROPERTIES
    CXX_STANDARD 17
    CXX_EXTENSIONS off
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
    OUTPUT_NAME PluginScene)
