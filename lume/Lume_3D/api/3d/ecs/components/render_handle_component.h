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

#if !defined(API_3D_ECS_COMPONENTS_RENDER_HANDLE_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_RENDER_HANDLE_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/namespace.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/**
 * Render handle component for sharing and naming rendering resources.
 * One can add Uri and NameComponents to the owning entity for easier resource identification. The entity owning this
 * component should be wrapped in an EntityReference to allow reference counted usage of the resource from other
 * components. When a RenderHandleComponent is destroyed it releases its render handle reference. Once all references
 * are gone the resource will be released.
 */
BEGIN_COMPONENT(IRenderHandleComponentManager, RenderHandleComponent)

    /** Handle to the render resources.
     * Further details of the image can be queried with IGpuResourceManager::IsGpuBuffer/Image/Sampler*,
     * IGpuResourceManager::GetBuffer/Image/SamplerDescriptor.
     */
    DEFINE_PROPERTY(RENDER_NS::RenderHandleReference, reference, "Render Handle Reference", 0, )

END_COMPONENT_EXT(
    IRenderHandleComponentManager, RenderHandleComponent, "fb5c16b5-c369-4f7b-bc02-5398ddfdfa1d",
    // The following getters are helpers for asking a handle or handle reference without first asking the component.
    /** Get render handle reference.
     * @param entity Entity.
     * @return If the entity had a RenderHandleComponent the component's handle reference is returned.
     */
    virtual RENDER_NS::RenderHandleReference GetRenderHandleReference(CORE_NS::Entity entity) const = 0;
    /** Get render handle reference.
     * @param index Index of the component.
     * @return The handle reference of the component at the given index is returned.
     */
    virtual RENDER_NS::RenderHandleReference GetRenderHandleReference(CORE_NS::IComponentManager::ComponentId index)
        const = 0;
    /** Get render handle.
     * @param entity Entity.
     * @return If the entity had a RenderHandleComponent the component's handle reference is returned.
     */
    virtual RENDER_NS::RenderHandle GetRenderHandle(CORE_NS::Entity entity) const = 0;
    /** Get render handle.
     * @param index Index of the component.
     * @return The handle of the component at the given index is returned.
     */
    virtual RENDER_NS::RenderHandle GetRenderHandle(CORE_NS::IComponentManager::ComponentId index) const = 0;
    /** Get an entity which has RenderHandleComponent referencing the given render handle.
     * @param handle Render handle to search for.
     * @return Entity with a RenderHandleComponent referencing the handle.
     */
    virtual CORE_NS::Entity GetEntityWithReference(const RENDER_NS::RenderHandleReference& handle) const = 0;)

#if !defined(IMPLEMENT_MANAGER)
/**
 * Helper function to get or create entity reference for render handle reference.
 * @param entityMgr Entity manager.
 * @param rhcMgr Render handle component manager.
 * @param handle Render handle reference for the render resource.
 * @return EntityReference for render handle reference
 */
inline CORE_NS::EntityReference GetOrCreateEntityReference(CORE_NS::IEntityManager& entityMgr,
    IRenderHandleComponentManager& rhcMgr, const RENDER_NS::RenderHandleReference& handle)
{
    CORE_NS::EntityReference entity = entityMgr.GetReferenceCounted(rhcMgr.GetEntityWithReference(handle));
    if (!entity) {
        entity = entityMgr.CreateReferenceCounted();
        rhcMgr.Create(entity);
        rhcMgr.Write(entity)->reference = handle;
    }
    return entity;
}

CORE3D_END_NAMESPACE()
#endif
#endif // API_3D_ECS_COMPONENTS_RENDER_HANDLE_COMPONENT_H
