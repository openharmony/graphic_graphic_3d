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

#include "util/uri_lookup.h"

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/skin_ibm_component.h>
#include <3d/ecs/components/uri_component.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/property/intf_property_handle.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

template<typename ComponentManager>
Entity LookupResourceByUri(
    string_view uri, const IUriComponentManager& uriManager, const ComponentManager& componentManager)
{
    const auto& entityManager = uriManager.GetEcs().GetEntityManager();
    const auto uriComponents = uriManager.GetComponentCount();
    for (auto i = 0u; i < uriComponents; ++i) {
        if (Entity entity = uriManager.GetEntity(i); entityManager.IsAlive(entity)) {
            if (const auto uriHandle = uriManager.Read(i); uriHandle) {
                const bool found = uriHandle->uri == uri;
                if (found && componentManager.HasComponent(entity)) {
                    return entity;
                }
            }
        }
    }
    return {};
}

template Entity LookupResourceByUri<>(
    string_view uri, const IUriComponentManager& uriManager, const IAnimationComponentManager& componentManager);

template Entity LookupResourceByUri<>(
    string_view uri, const IUriComponentManager& uriManager, const IRenderHandleComponentManager& componentManager);

template Entity LookupResourceByUri<>(
    string_view uri, const IUriComponentManager& uriManager, const IMaterialComponentManager& componentManager);

template Entity LookupResourceByUri<>(
    string_view uri, const IUriComponentManager& uriManager, const IMeshComponentManager& componentManager);

template Entity LookupResourceByUri<>(
    string_view uri, const IUriComponentManager& uriManager, const ISkinIbmComponentManager& componentManager);
CORE3D_END_NAMESPACE()
