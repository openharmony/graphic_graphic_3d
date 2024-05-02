/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <scene_plugin/namespace.h>

#include <meta/interface/intf_object_registry.h>
///////////////////////////////////////////////////////////////
// Specialisations
///////////////////////////////////////////////////////////////

#include "anim_impl.h"
#include "camera_impl.h"
#include "env_impl.h"
#include "graphicsstate_impl.h"
#include "light_impl.h"
#include "material_impl.h"
#include "mesh_impl.h"
#include "multimesh_impl.h"
#include "node_impl.h"
#include "postprocess_effect_impl.h"
#include "postprocess_impl.h"
#include "propertyholder_impl.h"
#include "render_configuration_impl.h"
#include "resource_container.h"
#include "shader_impl.h"
#include "submesh_impl.h"
#include "submeshhandler.h"
#include "textureinfo_impl.h"

SCENE_BEGIN_NAMESPACE()

void RegisterNodes()
{
    auto& registry = META_NS::GetObjectRegistry();
    RegisterNodeImpl();
    RegisterEnvImpl();
    RegisterPostprocessImpl();
    RegisterPostprocessEffectImpl();
    RegisterRenderConfigurationImpl();
    RegisterCameraImpl();

    RegisterLightImpl();
    RegisterMaterialImpl();
    RegisterMeshImpl();
    RegisterMultiMeshImpl();
    RegisterAnimImpl();
    // not nodes, but data for the nodes
    RegisterGraphicsStateImpl();
    RegisterPropertyHolderImpl();
    RegisterShaderImpl();
    RegisterSubMeshHandler();
    RegisterSubMeshImpl();
    RegisterTextureInfoImpl();

    RegisterResourceContainerImpl();
}

void UnregisterNodes()
{
    auto& registry = META_NS::GetObjectRegistry();
    UnregisterNodeImpl();
    UnregisterEnvImpl();
    UnregisterCameraImpl();
    UnregisterPostprocessImpl();
    UnregisterPostprocessEffectImpl();
    UnregisterRenderConfigurationImpl();
    UnregisterLightImpl();
    UnregisterMaterialImpl();
    UnregisterMeshImpl();
    UnregisterMultiMeshImpl();
    UnregisterAnimImpl();
    // not nodes, but data for the nodes
    UnregisterGraphicsStateImpl();
    UnregisterPropertyHolderImpl();
    UnregisterShaderImpl();
    UnregisterSubMeshHandler();
    UnregisterSubMeshImpl();
    UnregisterTextureInfoImpl();

    UnregisterResourceContainerImpl();
}

SCENE_END_NAMESPACE()
