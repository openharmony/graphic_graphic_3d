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

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

#include "component/postprocess_component.h"

SCENE_BEGIN_NAMESPACE()

class PostProcess
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IPostProcess, IEcsObjectAccess, ICreateEntity> {
    META_OBJECT(PostProcess, SCENE_NS::ClassId::PostProcess, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IPostProcess, ITonemap::Ptr, Tonemap)
    META_STATIC_PROPERTY_DATA(IPostProcess, IBloom::Ptr, Bloom)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(ITonemap::Ptr, Tonemap)
    META_IMPLEMENT_PROPERTY(IBloom::Ptr, Bloom)

    CORE_NS::Entity CreateEntity(const IInternalScene::Ptr& scene) override;

    bool SetEcsObject(const IEcsObject::Ptr&) override;
    IEcsObject::Ptr GetEcsObject() const override;

private:
    void Init(const IEcsObject::Ptr&);

private:
    IInternalPostProcess::Ptr pp_;
    META_NS::ICallable::Ptr stateChanged_;
};

SCENE_END_NAMESPACE()

#endif