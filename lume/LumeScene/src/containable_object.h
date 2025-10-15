/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_CONTAINABLE_OBJECT_H
#define SCENE_SRC_CONTAINABLE_OBJECT_H

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_containable.h>

#include "util_interfaces.h"

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ContainableObject, "289b7887-0f9e-44c2-9f6f-07af9c138b7e", META_NS::ObjectCategoryBits::NO_CATEGORY)

class ContainableObject
    : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::IContainable, META_NS::IMutableContainable> {
    META_OBJECT(ContainableObject, ClassId::ContainableObject, IntroduceInterfaces)

public:
    void SetParent(const META_NS::IObject::Ptr& parent) override
    {
        parent_ = parent;
    }

    META_NS::IObject::Ptr GetParent() const override
    {
        return parent_.lock();
    }

private:
    META_NS::IObject::WeakPtr parent_;
};

SCENE_END_NAMESPACE()

#endif