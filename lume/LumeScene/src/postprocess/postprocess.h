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

#include <meta/api/resource/derived_from_resource.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>

#include "../component/postprocess_component.h"

SCENE_BEGIN_NAMESPACE()

class PostProcess : public META_NS::IntroduceInterfaces<META_NS::DerivedFromResource, NamedSceneObject, IPostProcess,
                        ICreateEntity, META_NS::Resource> {
    META_OBJECT(PostProcess, SCENE_NS::ClassId::PostProcess, IntroduceInterfaces)
public:
    PostProcess() : Super(ClassId::PostProcessResourceTemplate) {}

    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, ITonemap::Ptr, Tonemap, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IBloom::Ptr, Bloom, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IColorFringe::Ptr, ColorFringe, "")
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IPostProcess, IVignette::Ptr, Vignette, "")
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(ITonemap::Ptr, Tonemap)
    META_IMPLEMENT_READONLY_PROPERTY(IBloom::Ptr, Bloom)
    META_IMPLEMENT_READONLY_PROPERTY(IColorFringe::Ptr, ColorFringe)
    META_IMPLEMENT_READONLY_PROPERTY(IVignette::Ptr, Vignette)

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    IEcsObject::Ptr GetEcsObject() const override;

    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::PostProcessResource.Id().ToUid();
    }

private:
    template<typename T>
    bool InitEffect(const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id);
    META_NS::IObject::Ptr CreateEffect(const META_NS::IProperty::Ptr& p, const META_NS::ClassInfo& id);

private:
    IInternalPostProcess::Ptr pp_;
};

SCENE_END_NAMESPACE()

#endif