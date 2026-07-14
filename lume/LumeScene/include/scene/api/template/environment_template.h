/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_API_TEMPLATE_ENVIRONMENT_TEMPLATE_H
#define SCENE_API_TEMPLATE_ENVIRONMENT_TEMPLATE_H

#include <scene/api/resource.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>

#include <meta/api/interface_object.h>
#include <meta/api/object.h>

SCENE_BEGIN_NAMESPACE()

class EnvironmentTemplate : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(EnvironmentTemplate, META_NS::Object, IResourceTemplate)
    META_INTERFACE_OBJECT_INSTANTIATE(EnvironmentTemplate, ::SCENE_NS::ClassId::EnvironmentTemplate)

    bool ApplyTo(IEnvironment& target) const
    {
        if (auto obj = interface_cast<META_NS::IObject>(&target)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.ApplyTo(*obj); });
        }
        return false;
    }
    bool CopyFrom(const IEnvironment& source, bool onlySetValues = true)
    {
        if (auto obj = interface_cast<const META_NS::IObject>(&source)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.CopyFrom(*obj, onlySetValues); });
        }
        return false;
    }
    IEnvironment::Ptr CreateInstance(const IScene::Ptr& scene) const
    {
        if (!scene || !GetPtr()) {
            return nullptr;
        }
        auto env = interface_pointer_cast<IEnvironment>(scene->CreateObject(ClassId::Environment).GetResult());
        if (!env || !ApplyTo(*env)) {
            return nullptr;
        }
        return env;
    }

    template<typename Type>
    auto GetProperty(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        return meta.GetProperty<Type>(name);
    }

    auto Name() const
    {
        return GetProperty<BASE_NS::string>("Name");
    }
    auto Background() const
    {
        return GetProperty<EnvBackgroundType>("Background");
    }
    auto IndirectDiffuseFactor() const
    {
        return GetProperty<BASE_NS::Math::Vec4>("IndirectDiffuseFactor");
    }
    auto IndirectSpecularFactor() const
    {
        return GetProperty<BASE_NS::Math::Vec4>("IndirectSpecularFactor");
    }
    auto EnvMapFactor() const
    {
        return GetProperty<BASE_NS::Math::Vec4>("EnvMapFactor");
    }
    /// Image refs are stored on the template as CORE_NS::ResourceId. The apply path
    /// resolves them against the scene's resource manager and binds the resolved
    /// IImage::Ptr onto the live IEnvironment.
    auto RadianceImage() const
    {
        return GetProperty<CORE_NS::ResourceId>("RadianceImage");
    }
    auto RadianceCubemapMipCount() const
    {
        return GetProperty<uint32_t>("RadianceCubemapMipCount");
    }
    auto EnvironmentImage() const
    {
        return GetProperty<CORE_NS::ResourceId>("EnvironmentImage");
    }
    auto EnvironmentMapLodLevel() const
    {
        return GetProperty<float>("EnvironmentMapLodLevel");
    }
    auto EnvironmentRotation() const
    {
        return GetProperty<BASE_NS::Math::Quat>("EnvironmentRotation");
    }
    auto IrradianceCoefficients() const
    {
        auto meta = META_NS::Metadata(*this);
        return META_NS::ArrayProperty<BASE_NS::Math::Vec3>(meta.GetProperty("IrradianceCoefficients"));
    }
};

SCENE_END_NAMESPACE()

#endif  // SCENE_API_TEMPLATE_ENVIRONMENT_TEMPLATE_H
