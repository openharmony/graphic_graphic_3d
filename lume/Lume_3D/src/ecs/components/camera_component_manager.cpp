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

#include <3d/ecs/components/camera_component.h>
#include <core/property/property_types.h>

#include "ComponentTools/base_manager.h"
#include "ComponentTools/base_manager.inl"
#include "ecs/components/layer_flag_bits_metadata.h"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;
using BASE_NS::Format;
using BASE_NS::vector;

using CORE3D_NS::CameraComponent;

// Extend propertysystem with the enums
DECLARE_PROPERTY_TYPE(CameraComponent::Projection);
DECLARE_PROPERTY_TYPE(CameraComponent::RenderingPipeline);
DECLARE_PROPERTY_TYPE(CameraComponent::Culling);
DECLARE_PROPERTY_TYPE(CameraComponent::TargetUsage);
DECLARE_PROPERTY_TYPE(vector<CameraComponent::TargetUsage>);
DECLARE_PROPERTY_TYPE(Format);
DECLARE_PROPERTY_TYPE(CameraComponent::SampleCount);

// Declare their metadata
ENUM_TYPE_METADATA(CameraComponent::Projection, ENUM_VALUE(ORTHOGRAPHIC, "Orthographic"),
    ENUM_VALUE(PERSPECTIVE, "Perspective"), ENUM_VALUE(FRUSTUM, "Frustum"), ENUM_VALUE(CUSTOM, "Custom Projection"))

ENUM_TYPE_METADATA(CameraComponent::RenderingPipeline, ENUM_VALUE(LIGHT_FORWARD, "Light-Weight Forward"),
    ENUM_VALUE(FORWARD, "Forward"), ENUM_VALUE(DEFERRED, "Deferred"), ENUM_VALUE(CUSTOM, "Custom"))

ENUM_TYPE_METADATA(CameraComponent::Culling, ENUM_VALUE(NONE, "None"), ENUM_VALUE(VIEW_FRUSTUM, "View Frustum"))

ENUM_TYPE_METADATA(CameraComponent::SceneFlagBits, ENUM_VALUE(ACTIVE_RENDER_BIT, "Active Render"),
    ENUM_VALUE(MAIN_CAMERA_BIT, "Main Camera"))

ENUM_TYPE_METADATA(CameraComponent::PipelineFlagBits, ENUM_VALUE(CLEAR_DEPTH_BIT, "Clear Depth"),
    ENUM_VALUE(CLEAR_COLOR_BIT, "Clear Color"), ENUM_VALUE(MSAA_BIT, "MSAA"),
    ENUM_VALUE(ALLOW_COLOR_PRE_PASS_BIT, "Allow Color Pre-Pass"),
    ENUM_VALUE(FORCE_COLOR_PRE_PASS_BIT, "Force Color Pre-Pass"),
    ENUM_VALUE(HISTORY_BIT, "Create And Store History Image"),
    ENUM_VALUE(JITTER_BIT, "Jitter Camera, Offset Within Pixel"),
    ENUM_VALUE(VELOCITY_OUTPUT_BIT, "Output Samplable Velocity"),
    ENUM_VALUE(DEPTH_OUTPUT_BIT, "Output Samplable Depth"),
    ENUM_VALUE(MULTI_VIEW_ONLY_BIT, "Camera Will Be Used Only As A Multi-View Layer"),
    ENUM_VALUE(DISALLOW_REFLECTION_BIT, "Disallows Reflection Plane For Camera"),
    ENUM_VALUE(CUBEMAP_BIT, "Camera Will Have Automatic Cubemap Targets"))

// define some meaningful image formats for camera component usage
ENUM_TYPE_METADATA(Format, ENUM_VALUE(BASE_FORMAT_UNDEFINED, "BASE_FORMAT_UNDEFINED"),
    ENUM_VALUE(BASE_FORMAT_R8G8B8A8_UNORM, "BASE_FORMAT_R8G8B8A8_UNORM"),
    ENUM_VALUE(BASE_FORMAT_R8G8B8A8_SRGB, "BASE_FORMAT_R8G8B8A8_SRGB"),
    ENUM_VALUE(BASE_FORMAT_A2R10G10B10_UNORM_PACK32, "BASE_FORMAT_A2R10G10B10_UNORM_PACK32"),
    ENUM_VALUE(BASE_FORMAT_R16G16B16A16_UNORM, "BASE_FORMAT_R16G16B16A16_UNORM"),
    ENUM_VALUE(BASE_FORMAT_R16G16B16A16_SFLOAT, "BASE_FORMAT_R16G16B16A16_SFLOAT"),
    ENUM_VALUE(BASE_FORMAT_B10G11R11_UFLOAT_PACK32, "BASE_FORMAT_B10G11R11_UFLOAT_PACK32"),
    ENUM_VALUE(BASE_FORMAT_D16_UNORM, "BASE_FORMAT_D16_UNORM"),
    ENUM_VALUE(BASE_FORMAT_D32_SFLOAT, "BASE_FORMAT_D32_SFLOAT"),
    ENUM_VALUE(BASE_FORMAT_D24_UNORM_S8_UINT, "BASE_FORMAT_D24_UNORM_S8_UINT"))

ENUM_TYPE_METADATA(CameraComponent::SampleCount, ENUM_VALUE(SAMPLE_COUNT_2, "2 Samples"),
    ENUM_VALUE(SAMPLE_COUNT_4, "4 Samples"), ENUM_VALUE(SAMPLE_COUNT_8, "8 Samples"))

DATA_TYPE_METADATA(
    CameraComponent::TargetUsage, MEMBER_PROPERTY(format, "Format", 0), MEMBER_PROPERTY(usageFlags, "Usage", 0))

CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;
using CORE_NS::PropertyFlags;

class CameraComponentManager final : public BaseManager<CameraComponent, ICameraComponentManager> {
    BEGIN_PROPERTY(CameraComponent, componentMetaData_)
#include <3d/ecs/components/camera_component.h>
    END_PROPERTY();

public:
    explicit CameraComponentManager(IEcs& ecs)
        : BaseManager<CameraComponent, ICameraComponentManager>(ecs, CORE_NS::GetName<CameraComponent>())
    {}

    ~CameraComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* ICameraComponentManagerInstance(IEcs& ecs)
{
    return new CameraComponentManager(ecs);
}

void ICameraComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<CameraComponentManager*>(instance);
}
CORE3D_END_NAMESPACE()
