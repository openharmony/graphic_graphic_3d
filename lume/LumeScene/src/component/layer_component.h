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

#ifndef SCENE_SRC_COMPONENT_LAYER_COMPONENT_H
#define SCENE_SRC_COMPONENT_LAYER_COMPONENT_H

#include <scene/ext/component.h>
#include <scene/interface/intf_layer.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(LayerComponent, "87a0e450-94d8-4bd2-bc33-c37d8772d819", META_NS::ObjectCategoryBits::NO_CATEGORY)

class LayerComponent : public META_NS::IntroduceInterfaces<Component, ILayer> {
    META_OBJECT(LayerComponent, ClassId::LayerComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(ILayer, uint64_t, LayerMask, "LayerComponent.layerMask")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(uint64_t, LayerMask)

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif
