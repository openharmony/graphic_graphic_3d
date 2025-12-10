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

#ifndef SCENE_SRC_POSTPROCESS_POSTPROCESS_H
#define SCENE_SRC_POSTPROCESS_POSTPROCESS_H

#include <scene/ext/intf_create_entity.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/resource/types.h>

#include <meta/api/resource/derived_from_template.h>
#include <meta/api/resource/resource_template_access.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>

#include "../component/postprocess_component.h"

SCENE_BEGIN_NAMESPACE()

class PostProcess : public META_NS::IntroduceInterfaces<META_NS::DerivedFromTemplate, NamedSceneObject, IPostProcess,
                        ICreateEntity, META_NS::Resource> {
    META_OBJECT(PostProcess, SCENE_NS::ClassId::PostProcess, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, ITonemap::Ptr, Tonemap, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IBloom::Ptr, Bloom, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IBlur::Ptr, Blur, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IMotionBlur::Ptr, MotionBlur, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IColorConversion::Ptr, ColorConversion, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IColorFringe::Ptr, ColorFringe, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IDepthOfField::Ptr, DepthOfField, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IFxaa::Ptr, Fxaa, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, ITaa::Ptr, Taa, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IVignette::Ptr, Vignette, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, ILensFlare::Ptr, LensFlare, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IUpscale::Ptr, Upscale, "")
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(ITonemap::Ptr, Tonemap)
    META_IMPLEMENT_READONLY_PROPERTY(IBloom::Ptr, Bloom)
    META_IMPLEMENT_READONLY_PROPERTY(IBlur::Ptr, Blur)
    META_IMPLEMENT_READONLY_PROPERTY(IMotionBlur::Ptr, MotionBlur)
    META_IMPLEMENT_READONLY_PROPERTY(IColorConversion::Ptr, ColorConversion)
    META_IMPLEMENT_READONLY_PROPERTY(IColorFringe::Ptr, ColorFringe)
    META_IMPLEMENT_READONLY_PROPERTY(IDepthOfField::Ptr, DepthOfField)
    META_IMPLEMENT_READONLY_PROPERTY(IFxaa::Ptr, Fxaa)
    META_IMPLEMENT_READONLY_PROPERTY(ITaa::Ptr, Taa)
    META_IMPLEMENT_READONLY_PROPERTY(IVignette::Ptr, Vignette)
    META_IMPLEMENT_READONLY_PROPERTY(ILensFlare::Ptr, LensFlare)
    META_IMPLEMENT_READONLY_PROPERTY(IUpscale::Ptr, Upscale)

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    IEcsObject::Ptr GetEcsObject() const override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::PostProcessResource.Id().ToUid();
    }
    META_NS::ObjectId GetDefaultAccess() const override
    {
        return ClassId::PostProcessTemplateAccess;
    }

private:
    template<typename T>
    bool InitEffect(const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id);
    META_NS::IObject::Ptr CreateEffect(
        const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id, BASE_NS::Uid pid);

private:
    IInternalPostProcess::Ptr pp_;
};

class PostProcessTemplateAccess
    : public META_NS::IntroduceInterfaces<META_NS::ResourceTemplateAccess, META_NS::BaseObject> {
    META_OBJECT(PostProcessTemplateAccess, ClassId::PostProcessTemplateAccess, IntroduceInterfaces)
public:
    PostProcessTemplateAccess() : Super(ClassId::PostProcess, ClassId::PostProcessResourceTemplate) {}
};

SCENE_END_NAMESPACE()

#endif
