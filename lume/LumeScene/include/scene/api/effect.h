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
#ifndef SCENE_API_EFFECT_H
#define SCENE_API_EFFECT_H

#include <scene/api/resource.h>
#include <scene/interface/intf_effect.h>

#include <meta/api/interface_object.h>
#include <meta/api/object.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief THe Effect class is the base class for all effects.
 */
class Effect : public META_NS::Object {
public:
    META_INTERFACE_OBJECT(Effect, META_NS::Object, IEffect)
    META_INTERFACE_OBJECT_INSTANTIATE(Effect, ::SCENE_NS::ClassId::Effect)
    /// @see IEffect::Enabled
    META_INTERFACE_OBJECT_PROPERTY(bool, Enabled)
    /// @see IEffect::GetEffectClassId
    META_NS::ObjectId GetEffectClassId() const
    {
        return META_INTERFACE_OBJECT_CALL_PTR(GetEffectClassId());
    }
    /**
     * @brief Returns a effect property with given name.
     * @param name Name of the effect property to return.
     */
    template<typename Type>
    auto GetProperty(BASE_NS::string_view name) const
    {
        auto meta = META_NS::Metadata(*this);
        return meta.GetProperty<Type>(name);
    }
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_POST_PROCESS_H
