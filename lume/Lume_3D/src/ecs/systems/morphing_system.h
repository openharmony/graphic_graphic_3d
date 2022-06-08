/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ECS_MORPHINGSYSTEM_H
#define CORE_ECS_MORPHINGSYSTEM_H

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.h>

#include <3d/ecs/components/morph_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <base/containers/array_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/ecs/intf_ecs.h>
#include <core/namespace.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderHandleComponentManager;
class INodeComponentManager;
class IMeshComponentManager;
class IMorphComponentManager;
class IRenderDataStoreMorph;
class IRenderMeshComponentManager;
struct MeshComponent;
struct MorphComponent;

class MorphingSystem final : public IMorphingSystem,
                             private CORE_NS::IEcs::EntityListener,
                             private CORE_NS::IEcs::ComponentListener {
public:
    explicit MorphingSystem(CORE_NS::IEcs& ecs);
    ~MorphingSystem() override;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    void Uninitialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;
    const CORE_NS::IEcs& GetECS() const override;

private:
    void OnEntityEvent(
        CORE_NS::IEntityManager::EventType type, BASE_NS::array_view<const CORE_NS::Entity> entities) override;
    void OnComponentEvent(ComponentListener::EventType type, const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;

    bool Morph(const MeshComponent& mesh, const MorphComponent& mc, bool dirty);
    void SetDataStore(RENDER_NS::IRenderDataStoreManager* manager, const BASE_NS::string_view name);
    bool active_;
    CORE_NS::IEcs& ecs_;
    RENDER_NS::IRenderContext* renderContext_ = nullptr;
    IRenderDataStoreMorph* dataStore_;
    INodeComponentManager& nodeManager_;
    IMeshComponentManager& meshManager_;
    IMorphComponentManager& morphManager_;
    IRenderMeshComponentManager& renderMeshManager_;
    IRenderHandleComponentManager& gpuHandleManager_;

    IMorphingSystem::Properties properties_;
    CORE_NS::PropertyApiImpl<IMorphingSystem::Properties> MORPHING_SYSTEM_PROPERTIES;
    uint32_t lastGeneration_ { 0 };
    BASE_NS::unordered_map<CORE_NS::Entity, bool> dirty_;
    BASE_NS::vector<CORE_NS::Entity> reset_;
    CORE_NS::ComponentQuery nodeQuery_;
};
CORE3D_END_NAMESPACE()
#endif // CORE_ECS_MORPHINGSYSTEM_H
