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

#ifndef SCENE_SRC_TEMPLATE_MATERIAL_TEMPLATE_H
#define SCENE_SRC_TEMPLATE_MATERIAL_TEMPLATE_H

#include <scene/interface/intf_material.h>
#include <scene/interface/resource/types.h>

#include "resource_template_base.h"

SCENE_BEGIN_NAMESPACE()

class MaterialTemplate : public META_NS::IntroduceInterfaces<ResourceTemplateBase> {
    META_OBJECT(MaterialTemplate, ClassId::MaterialTemplate, IntroduceInterfaces)

public:
    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::MaterialResourceTemplate.Id().ToUid();
    }

    META_NS::ObjectId GetTemplateClassId() const override
    {
        return ClassId::MaterialTemplate;
    }

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    void InitTextures();

protected:
    bool ValidateResourceType(const CORE_NS::IResource& res) const override;
    bool CopyFromTyped(const META_NS::IObject& source, bool onlySetValues) override;
    void CloneSubObjects(const META_NS::IMetadata& srcMeta, META_NS::IMetadata& cloneMeta) const override;
    bool ApplyOptions(CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) const override;
    bool ApplyToTarget(META_NS::IObject& target, bool asDefault) const override;

private:
    bool CopyFromMaterial(const IMaterial& material, bool onlySetValues);

    auto Type()
    {
        return PropertyByName<MaterialType>("Type");
    }
    auto AlphaCutoff()
    {
        return PropertyByName<float>("AlphaCutoff");
    }
    auto LightingFlags()
    {
        return PropertyByName<SCENE_NS::LightingFlags>("LightingFlags");
    }
    auto RenderSort()
    {
        return PropertyByName<SCENE_NS::RenderSort>("RenderSort");
    }

    META_NS::ArrayProperty<META_NS::IObject::Ptr> Textures()
    {
        META_NS::IMetadata& meta = *this;
        return meta.GetArrayProperty<META_NS::IObject::Ptr>("Textures", META_NS::MetadataQuery::EXISTING);
    }
};

SCENE_END_NAMESPACE()

#endif
