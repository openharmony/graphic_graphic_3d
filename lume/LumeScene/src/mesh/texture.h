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

#ifndef SCENE_SRC_MESH_TEXTURE_H
#define SCENE_SRC_MESH_TEXTURE_H

#include <scene/ext/named_scene_object.h>
#include <scene/interface/intf_texture.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "../component/material_component.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(Texture, "07e6dc19-57ca-4a3a-aa71-a53db9bf2e58", META_NS::ObjectCategoryBits::NO_CATEGORY)

class Texture : public META_NS::IntroduceInterfaces<EcsLazyPropertyFwd, META_NS::INamed, ITexture> {
    META_OBJECT(Texture, ClassId::Texture, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, IBitmap::Ptr, Image, "image")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, TextureSampler, Sampler, "sampler")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, BASE_NS::Math::Vec4, Factor, "factor")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, BASE_NS::Math::Vec2, Translation, "transform.translation")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, float, Rotation, "transform.rotation")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(ITexture, BASE_NS::Math::Vec2, Scale, "transform.scale")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)
    META_IMPLEMENT_PROPERTY(IBitmap::Ptr, Image)
    META_IMPLEMENT_PROPERTY(TextureSampler, Sampler)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec4, Factor)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec2, Translation)
    META_IMPLEMENT_PROPERTY(float, Rotation)
    META_IMPLEMENT_PROPERTY(BASE_NS::Math::Vec2, Scale)

    bool Build(const META_NS::IMetadata::Ptr&) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    BASE_NS::string GetName() const override
    {
        return Name()->GetValue();
    }

private:
    size_t index_ = -1;
};

SCENE_END_NAMESPACE()

#endif