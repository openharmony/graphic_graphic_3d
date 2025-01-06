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

#ifndef SCENE_SRC_COMPONENT_GENERIC_COMPONENT_H
#define SCENE_SRC_COMPONENT_GENERIC_COMPONENT_H

#include <scene/ext/component_fwd.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(GenericComponent, "1eb22ecd-b8dd-4a1c-a491-1d4ed83c5179", META_NS::ObjectCategoryBits::NO_CATEGORY)

class GenericComponent : public META_NS::IntroduceInterfaces<ComponentFwd, IGenericComponent> {
    META_OBJECT(GenericComponent, ClassId::GenericComponent, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    BASE_NS::string GetName() const override;

private:
    BASE_NS::string component_;
};

SCENE_END_NAMESPACE()

#endif