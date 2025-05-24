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

#ifndef SCENE_SRC_POSTPROCESS_UTIL_H
#define SCENE_SRC_POSTPROCESS_UTIL_H

#include "converting_value.h"

SCENE_BEGIN_NAMESPACE()

template<uint32_t Bit>
struct PPEffectEnabledConverter {
    PPEffectEnabledConverter(META_NS::ConstProperty<uint32_t> flags) : flags_(flags) {}

    using SourceType = bool;
    using TargetType = uint32_t;

    SourceType ConvertToSource(META_NS::IAny& any, const TargetType& v) const
    {
        return v & Bit;
    }
    TargetType ConvertToTarget(const SourceType& v) const
    {
        return v ? flags_->GetValue() | Bit : flags_->GetValue() & ~Bit;
    }

private:
    META_NS::ConstProperty<uint32_t> flags_;
};

class IPPEffectInit : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPPEffectInit, "ee72a777-45ff-42ff-b816-5f169b4453b0")
public:
    virtual bool Init(const META_NS::IProperty::Ptr& flags) = 0;
};

SCENE_END_NAMESPACE()

#endif