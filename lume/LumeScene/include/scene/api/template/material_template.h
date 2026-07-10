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

#ifndef SCENE_API_TEMPLATE_MATERIAL_TEMPLATE_H
#define SCENE_API_TEMPLATE_MATERIAL_TEMPLATE_H

#include <scene/api/resource.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>

#include <meta/api/interface_object.h>
#include <meta/api/object.h>

SCENE_BEGIN_NAMESPACE()

class TemplateSamplerView {
public:
    explicit TemplateSamplerView(META_NS::IObject::Ptr obj) : obj_(BASE_NS::move(obj))
    {}
    explicit operator bool() const
    {
        return !!obj_;
    }

    template<typename T>
    auto GetProperty(BASE_NS::string_view name) const
    {
        return META_NS::Metadata(obj_).GetProperty<T>(name);
    }

    auto MagFilter() const
    {
        return GetProperty<SamplerFilter>("MagFilter");
    }
    auto MinFilter() const
    {
        return GetProperty<SamplerFilter>("MinFilter");
    }
    auto MipMapMode() const
    {
        return GetProperty<SamplerFilter>("MipMapMode");
    }
    auto AddressModeU() const
    {
        return GetProperty<SamplerAddressMode>("AddressModeU");
    }
    auto AddressModeV() const
    {
        return GetProperty<SamplerAddressMode>("AddressModeV");
    }
    auto AddressModeW() const
    {
        return GetProperty<SamplerAddressMode>("AddressModeW");
    }

private:
    META_NS::IObject::Ptr obj_;
};

// View onto a MaterialTemplate's sparse "Options" graphics-state sub-object.
// Only fields present in the imported JSON have a property; absent fields
// return a null Property.
class TemplateOptionsView {
public:
    explicit TemplateOptionsView(META_NS::IObject::Ptr obj) : obj_(BASE_NS::move(obj))
    {}
    explicit operator bool() const
    {
        return !!obj_;
    }

    template<typename T>
    auto GetProperty(BASE_NS::string_view name) const
    {
        return META_NS::Metadata(obj_).GetProperty<T>(name);
    }

    auto CullMode() const
    {
        return GetProperty<CullModeFlags>("CullMode");
    }
    auto PolygonMode() const
    {
        return GetProperty<::SCENE_NS::PolygonMode>("PolygonMode");
    }
    auto Blend() const
    {
        return GetProperty<bool>("Blend");
    }

private:
    META_NS::IObject::Ptr obj_;
};

class TemplateTextureView {
public:
    explicit TemplateTextureView(META_NS::IObject::Ptr obj) : obj_(BASE_NS::move(obj))
    {}
    explicit operator bool() const
    {
        return !!obj_;
    }

    template<typename T>
    auto GetProperty(BASE_NS::string_view name) const
    {
        return META_NS::Metadata(obj_).GetProperty<T>(name);
    }

    auto Name() const
    {
        return GetProperty<BASE_NS::string>("Name");
    }
    /// Texture slot's image is stored on the template as a CORE_NS::ResourceId. The
    /// apply path resolves it against the scene's resource manager when the template
    /// is applied to a live ITexture slot.
    auto Image() const
    {
        return GetProperty<CORE_NS::ResourceId>("Image");
    }
    auto Factor() const
    {
        return GetProperty<BASE_NS::Math::Vec4>("Factor");
    }
    auto Translation() const
    {
        return GetProperty<BASE_NS::Math::Vec2>("Translation");
    }
    auto Rotation() const
    {
        return GetProperty<float>("Rotation");
    }
    auto Scale() const
    {
        return GetProperty<BASE_NS::Math::Vec2>("Scale");
    }
    TemplateSamplerView Sampler() const
    {
        auto meta = META_NS::Metadata(obj_);
        return TemplateSamplerView(META_NS::GetPointer<META_NS::IObject>(meta.GetProperty("Sampler")));
    }

private:
    META_NS::IObject::Ptr obj_;
};

class TemplateTexturesView {
public:
    explicit TemplateTexturesView(META_NS::ArrayProperty<META_NS::IObject::Ptr> prop) : prop_(BASE_NS::move(prop))
    {}
    explicit operator bool() const
    {
        return !!prop_;
    }
    size_t GetSize() const
    {
        return prop_ ? prop_->GetSize() : 0;
    }
    TemplateTextureView GetValueAt(size_t index) const
    {
        return TemplateTextureView(prop_ ? prop_->GetValueAt(index) : nullptr);
    }

private:
    META_NS::ArrayProperty<META_NS::IObject::Ptr> prop_;
};

class MaterialTemplate : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(MaterialTemplate, META_NS::Object, IResourceTemplate)
    META_INTERFACE_OBJECT_INSTANTIATE(MaterialTemplate, ::SCENE_NS::ClassId::MaterialTemplate)

    bool ApplyTo(IMaterial& target) const
    {
        if (auto obj = interface_cast<META_NS::IObject>(&target)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.ApplyTo(*obj); });
        }
        return false;
    }
    bool CopyFrom(const IMaterial& source, bool onlySetValues = true)
    {
        if (auto obj = interface_cast<const META_NS::IObject>(&source)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.CopyFrom(*obj, onlySetValues); });
        }
        return false;
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
    auto Type() const
    {
        return GetProperty<MaterialType>("Type");
    }
    auto AlphaCutoff() const
    {
        return GetProperty<float>("AlphaCutoff");
    }
    auto LightingFlags() const
    {
        return GetProperty<::SCENE_NS::LightingFlags>("LightingFlags");
    }
    /// Shader refs are stored on the template as CORE_NS::ResourceId. The apply path
    /// resolves them against the scene's resource manager and binds the resolved
    /// IShader::Ptr onto the live IMaterial.
    auto MaterialShader() const
    {
        return GetProperty<CORE_NS::ResourceId>("MaterialShader");
    }
    auto DepthShader() const
    {
        return GetProperty<CORE_NS::ResourceId>("DepthShader");
    }
    auto RenderSort() const
    {
        return GetProperty<::SCENE_NS::RenderSort>("RenderSort");
    }
    TemplateTexturesView Textures() const
    {
        auto meta = META_NS::Metadata(*this);
        return TemplateTexturesView(META_NS::ArrayProperty<META_NS::IObject::Ptr>(meta.GetProperty("Textures")));
    }
    TemplateOptionsView Options() const
    {
        auto meta = META_NS::Metadata(*this);
        return TemplateOptionsView(META_NS::GetPointer<META_NS::IObject>(meta.GetProperty("Options")));
    }

    IMaterial::Ptr CreateInstance(const IScene::Ptr& scene) const
    {
        if (!scene || !GetPtr()) {
            return nullptr;
        }
        const auto matType = META_NS::GetValue(Type());
        const auto classId = matType == MaterialType::OCCLUSION ? ClassId::OcclusionMaterial : ClassId::Material;
        auto material = interface_pointer_cast<IMaterial>(scene->CreateObject(classId).GetResult());
        if (!material || !ApplyTo(*material)) {
            return nullptr;
        }
        return material;
    }
};

SCENE_END_NAMESPACE()

#endif  // SCENE_API_TEMPLATE_MATERIAL_TEMPLATE_H
