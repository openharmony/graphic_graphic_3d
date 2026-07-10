/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PLUGIN_ECS_MRT_SYSTEM_H
#define PLUGIN_ECS_MRT_SYSTEM_H

#include <cmath>
#include <random>
#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/mathf.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/property_tools/property_api_impl.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_data.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_factory.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <ComponentTools/component_query.h>

#include "intf_mrt_system.h"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

MRT_BEGIN_NAMESPACE()

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

class MRTSystem final : public IMRTSystem {
public:
    explicit MRTSystem(IEcs& ecs);
    ~MRTSystem() override;

    // ISystem
    string_view GetName() const override;
    Uid GetUid() const override;

    IPropertyHandle* GetProperties() override;
    const IPropertyHandle* GetProperties() const override;
    void SetProperties(const IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    void Uninitialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;

    const IEcs& GetECS() const override;

    struct Properties {
        float speed{1.0f};
    };

private:
    IEcs& ecs_;
    void UpdateShaders(float zNear, float zFar, bool updateNearFarOnly);
    // TODO: How to initialize with NO properties?
    BEGIN_PROPERTY(Properties, ComponentMetadata)
    DEFINE_PROPERTY(Properties, speed, "Simulation speed", 0, )
    END_PROPERTY();

    Properties properties_{1.0f};

    CORE_NS::PropertyApiImpl<Properties> propertyApi_ =
        CORE_NS::PropertyApiImpl<Properties>(&properties_, ComponentMetadata);

    bool active_{true};

    bool inited_{false};

    float zNear_{0.f};
    float zFar_{0.f};

    IRenderContext* renderContext_{nullptr};
};

MRT_END_NAMESPACE()
#endif  // PLUGIN_ECS_MRT_SYSTEM_H