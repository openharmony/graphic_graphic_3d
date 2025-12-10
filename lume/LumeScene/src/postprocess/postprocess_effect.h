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

#ifndef SCENE_SRC_POSTPROCESS_EFFECT_H
#define SCENE_SRC_POSTPROCESS_EFFECT_H

#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/scene_property.h>
#include <scene/ext/util.h>

#include <3d/ecs/components/post_process_component.h>

#include <meta/interface/intf_containable.h>

#include "util.h"

SCENE_BEGIN_NAMESPACE()

namespace Internal {

template<typename PPInterface, uint32_t PPBit>
class PostProcessEffect : public META_NS::IntroduceInterfaces<EcsLazyProperty, IPPEffectInit, META_NS::IContainable,
                              META_NS::IMutableContainable, PPInterface> {
public:
    bool Init(const META_NS::IProperty::Ptr& flags) override
    {
        flags_ = flags;
        return static_cast<bool>(flags_);
    }

    void SetParent(const META_NS::IObject::Ptr& parent) override
    {
        parent_ = parent;
    }

    META_NS::IObject::Ptr GetParent() const override
    {
        return parent_.lock();
    }

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override
    {
        if (!p) {
            return false;
        }
        if (p->GetName() == "Enabled") {
            return flags_ &&
                   PushPropertyValue(p, META_NS::IValue::Ptr {
                                            new ConvertingValue<PPEffectEnabledConverter<PPBit>>(flags_, { flags_ }) });
        }
        return this->AttachEngineProperty(p, GetPropertyPath(path));
    }
    BASE_NS::string GetPropertyPath(BASE_NS::string_view path) const
    {
        return GetComponentPath() + path;
    }

    virtual BASE_NS::string_view GetComponentPath() const = 0;

private:
    META_NS::IObject::WeakPtr parent_;
    META_NS::Property<uint32_t> flags_;
};

} // namespace Internal

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_POSTPROCESS_EFFECT_H
