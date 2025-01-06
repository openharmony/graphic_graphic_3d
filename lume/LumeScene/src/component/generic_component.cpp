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

#include "generic_component.h"

#include <scene/ext/util.h>

SCENE_BEGIN_NAMESPACE()

bool GenericComponent::Build(const META_NS::IMetadata::Ptr& d)
{
    if (Super::Build(d)) {
        component_ = GetBuildArg<BASE_NS::string>(d, "Component");
    }
    return !component_.empty();
}

BASE_NS::string GenericComponent::GetName() const
{
    return component_;
}

SCENE_END_NAMESPACE()