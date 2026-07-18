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

#ifndef SCENE_API_TEMPLATE_POSTPROCESS_TEMPLATE_H
#define SCENE_API_TEMPLATE_POSTPROCESS_TEMPLATE_H

#include <scene/api/resource.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>

#include <meta/api/interface_object.h>
#include <meta/api/object.h>

SCENE_BEGIN_NAMESPACE()

class TemplateEffectView {
public:
    explicit TemplateEffectView(META_NS::IObject::Ptr obj) : obj_(BASE_NS::move(obj))
    {}
    explicit operator bool() const
    {
        return !!obj_;
    }

    template <typename T>
    auto GetProperty(BASE_NS::string_view name) const
    {
        return META_NS::Metadata(obj_).GetProperty<T>(name);
    }

    auto Enabled() const
    {
        return GetProperty<bool>("Enabled");
    }

private:
    META_NS::IObject::Ptr obj_;
};

class PostProcessTemplate : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(PostProcessTemplate, META_NS::Object, IResourceTemplate)
    META_INTERFACE_OBJECT_INSTANTIATE(PostProcessTemplate, ::SCENE_NS::ClassId::PostProcessTemplate)

    bool ApplyTo(IPostProcess& target) const
    {
        if (auto obj = interface_cast<META_NS::IObject>(&target)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.ApplyTo(*obj); });
        }
        return false;
    }
    bool CopyFrom(const IPostProcess& source, bool onlySetValues = true)
    {
        if (auto obj = interface_cast<const META_NS::IObject>(&source)) {
            return CallPtr<IResourceTemplate>([&](auto& p) { return p.CopyFrom(*obj, onlySetValues); });
        }
        return false;
    }
    IPostProcess::Ptr CreateInstance(const IScene::Ptr& scene) const
    {
        if (!scene || !GetPtr()) {
            return nullptr;
        }
        auto pp = interface_pointer_cast<IPostProcess>(scene->CreateObject(ClassId::PostProcess).GetResult());
        if (!pp || !ApplyTo(*pp)) {
            return nullptr;
        }
        return pp;
    }

    template <typename Type>
    auto GetProperty(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        return meta.GetProperty<Type>(name);
    }

    TemplateEffectView Tonemap() const
    {
        return GetEffectView("Tonemap");
    }
    TemplateEffectView Bloom() const
    {
        return GetEffectView("Bloom");
    }
    TemplateEffectView Blur() const
    {
        return GetEffectView("Blur");
    }
    TemplateEffectView MotionBlur() const
    {
        return GetEffectView("MotionBlur");
    }
    TemplateEffectView ColorConversion() const
    {
        return GetEffectView("ColorConversion");
    }
    TemplateEffectView ColorFringe() const
    {
        return GetEffectView("ColorFringe");
    }
    TemplateEffectView DepthOfField() const
    {
        return GetEffectView("DepthOfField");
    }
    TemplateEffectView Fxaa() const
    {
        return GetEffectView("Fxaa");
    }
    TemplateEffectView Taa() const
    {
        return GetEffectView("Taa");
    }
    TemplateEffectView ColorAdjustments() const
    {
        return GetEffectView("ColorAdjustments");
    }
    TemplateEffectView Vignette() const
    {
        return GetEffectView("Vignette");
    }
    TemplateEffectView LensFlare() const
    {
        return GetEffectView("LensFlare");
    }
    TemplateEffectView Upscale() const
    {
        return GetEffectView("Upscale");
    }
    TemplateEffectView WhiteBalance() const
    {
        return GetEffectView("WhiteBalance");
    }

private:
    TemplateEffectView GetEffectView(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        return TemplateEffectView(META_NS::GetPointer<META_NS::IObject>(meta.GetProperty(name)));
    }
};

SCENE_END_NAMESPACE()

#endif  // SCENE_API_TEMPLATE_POSTPROCESS_TEMPLATE_H
