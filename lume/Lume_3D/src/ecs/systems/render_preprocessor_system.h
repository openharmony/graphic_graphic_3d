/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H
#define CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H

#include <ComponentTools/component_query.h>
#include <PropertyTools/property_api_impl.h>

#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/intf_graphics_context.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_render_context.h>

#include "property/property_handle.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IRenderDataStoreManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultLight;
class IRenderDataStoreDefaultMaterial;
class IRenderDataStoreDefaultScene;
class IRenderDataStoreMorph;

class RenderPreprocessorSystem final : public IRenderPreprocessorSystem {
public:
    explicit RenderPreprocessorSystem(CORE_NS::IEcs& ecs);
    ~RenderPreprocessorSystem() override;
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;
    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    void Uninitialize() override;
    bool Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime) override;

    const CORE_NS::IEcs& GetECS() const override;

private:
    CORE_NS::IEcs& ecs_;
    IGraphicsContext* graphicsContext_ { nullptr };
    RENDER_NS::IRenderContext* renderContext_ { nullptr };
    bool active_ { true };

    IRenderPreprocessorSystem::Properties properties_ {
        nullptr,
        "RenderDataStoreDefaultScene",
        "RenderDataStoreDefaultCamera",
        "RenderDataStoreDefaultLight",
        "RenderDataStoreDefaultMaterial",
        "RenderDataStoreMorph",
    };
    CORE_NS::PropertyApiImpl<IRenderPreprocessorSystem::Properties> RENDER_PREPROCESSOR_SYSTEM_PROPERTIES;

    void SetDataStorePointers(RENDER_NS::IRenderDataStoreManager& manager);

    IRenderDataStoreDefaultCamera* dsCamera_ { nullptr };
    IRenderDataStoreDefaultLight* dsLight_ { nullptr };
    IRenderDataStoreDefaultMaterial* dsMaterial_ { nullptr };
    IRenderDataStoreDefaultScene* dsScene_ { nullptr };
    IRenderDataStoreMorph* dsMorph_ { nullptr };
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_ECS_RENDER_PREPROCESSOR_SYSTEM_H
