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

#ifndef SCENE_SRC_RESOURCE_OCCLUSION_MATERIAL_H
#define SCENE_SRC_RESOURCE_OCCLUSION_MATERIAL_H

#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/resource/types.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/api/resource/derived_from_template.h>
#include <meta/api/resource/resource_template_access.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>

#include "../component/material_component.h"

SCENE_BEGIN_NAMESPACE()

class OcclusionMaterial : public META_NS::IntroduceInterfaces<META_NS::DerivedFromTemplate, NamedSceneObject, IMaterial,
                              ICreateEntity, META_NS::Resource> {
    META_OBJECT(OcclusionMaterial, ClassId::OcclusionMaterial, IntroduceInterfaces)

public:
    static constexpr auto OCCLUSION_FLAGS = META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER | META_NS::ObjectFlagBits::INTERNAL;

    // Don't serialize any of the values
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(
        IMaterial, MaterialType, Type, MaterialType::OCCLUSION, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_STATIC_PROPERTY_DATA(IMaterial, float, AlphaCutoff, 0, OCCLUSION_FLAGS)
    META_STATIC_PROPERTY_DATA(IMaterial, IShader::Ptr, MaterialShader, {}, OCCLUSION_FLAGS)
    META_STATIC_PROPERTY_DATA(IMaterial, IShader::Ptr, DepthShader, {}, OCCLUSION_FLAGS)
    META_STATIC_PROPERTY_DATA(IMaterial, SCENE_NS::RenderSort, RenderSort, {}, OCCLUSION_FLAGS)
    META_STATIC_PROPERTY_DATA(IMaterial, SCENE_NS::LightingFlags, LightingFlags, {}, OCCLUSION_FLAGS)
    META_STATIC_PROPERTY_DATA(IMaterial, ITexture::Ptr, Textures, {}, OCCLUSION_FLAGS)
    META_STATIC_PROPERTY_DATA(IMaterial, META_NS::IMetadata::Ptr, CustomProperties, {}, OCCLUSION_FLAGS)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(MaterialType, Type)
    META_IMPLEMENT_PROPERTY(float, AlphaCutoff)
    META_IMPLEMENT_PROPERTY(IShader::Ptr, MaterialShader)
    META_IMPLEMENT_PROPERTY(IShader::Ptr, DepthShader)
    META_IMPLEMENT_PROPERTY(SCENE_NS::RenderSort, RenderSort)
    META_IMPLEMENT_PROPERTY(SCENE_NS::LightingFlags, LightingFlags)
    META_IMPLEMENT_READONLY_ARRAY_PROPERTY(ITexture::Ptr, Textures)
    META_IMPLEMENT_READONLY_PROPERTY(META_NS::IMetadata::Ptr, CustomProperties)

    META_NS::IMetadata::Ptr GetCustomProperties() const override
    {
        return {};
    }
    META_NS::IProperty::Ptr GetCustomProperty(BASE_NS::string_view name) const override
    {
        return {};
    }

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::OcclusionMaterialResource.Id().ToUid();
    }
    META_NS::ObjectId GetDefaultAccess() const override
    {
        return ClassId::OcclusionMaterialTemplateAccess;
    }

private:
    IInternalMaterial::Ptr material_;
};

class OcclusionMaterialTemplateAccess
    : public META_NS::IntroduceInterfaces<META_NS::ResourceTemplateAccess, META_NS::BaseObject> {
    META_OBJECT(OcclusionMaterialTemplateAccess, ClassId::OcclusionMaterialTemplateAccess, IntroduceInterfaces)
public:
    OcclusionMaterialTemplateAccess() : Super(ClassId::OcclusionMaterial, ClassId::OcclusionMaterialResourceTemplate) {}

    bool SetValuesFromTemplate(
        const CORE_NS::IResource::ConstPtr& templ, const CORE_NS::IResource::Ptr& resource) const override;
};

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_RESOURCE_OCCLUSION_MATERIAL_H
