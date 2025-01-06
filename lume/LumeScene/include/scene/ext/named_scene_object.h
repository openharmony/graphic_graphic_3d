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

#ifndef SCENE_EXT_NAMED_SCENE_OBJECT_H
#define SCENE_EXT_NAMED_SCENE_OBJECT_H

#include <scene/base/namespace.h>
#include <scene/ext/ecs_lazy_property_fwd.h>

#include <meta/api/make_callback.h>

SCENE_BEGIN_NAMESPACE()

class NamedSceneObject : public META_NS::IntroduceInterfaces<EcsLazyPropertyFwd, META_NS::INamed> {
    META_OBJECT_NO_CLASSINFO(NamedSceneObject, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

    bool SetEcsObject(const IEcsObject::Ptr& obj) override
    {
        auto ret = Super::SetEcsObject(obj);
        if (ret) {
            Name()->SetValue(GetName());
            Name()->OnChanged()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>([&] {
                if (this->object_) {
                    this->object_->SetName(Name()->GetValue());
                }
            }));
        }
        return ret;
    }

    BASE_NS::string GetName() const override
    {
        return this->object_ ? this->object_->GetName() : "";
    }
};

SCENE_END_NAMESPACE()

#endif