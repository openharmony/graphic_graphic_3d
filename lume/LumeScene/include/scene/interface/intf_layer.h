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

#ifndef SCENE_INTERFACE_ILAYER_H
#define SCENE_INTERFACE_ILAYER_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

class ILayer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ILayer, "08b90de2-ae9f-4d53-a93a-3a31393d83c8")
public:
    META_PROPERTY(uint64_t, LayerMask)
};

constexpr uint64_t LayerMask(uint16_t bit)
{
    return 1ULL << bit;
}

constexpr uint64_t DEFAULT_LAYER_MASK { LayerMask(0) };
constexpr uint64_t ALL_LAYER_MASK { 0xFFFFFFFFffffffffULL };
constexpr uint64_t NONE_LAYER_MASK { 0ULL };

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::ILayer)

#endif
