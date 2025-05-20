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

#include "animation_type.h"

#include <scene/ext/util.h>

SCENE_BEGIN_NAMESPACE()

bool AnimationResourceType::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        auto context = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        if (!context) {
            CORE_LOG_E("Invalid arguments to construct AnimationResourceType");
            return false;
        }
        context_ = context;
    }
    return res;
}
BASE_NS::string AnimationResourceType::GetResourceName() const
{
    return "AnimationResource";
}
BASE_NS::Uid AnimationResourceType::GetResourceType() const
{
    return ClassId::AnimationResource.Id().ToUid();
}
CORE_NS::IResource::Ptr AnimationResourceType::LoadResource(const StorageInfo& s) const
{
    return nullptr;
}
bool AnimationResourceType::SaveResource(const CORE_NS::IResource::ConstPtr& p, const StorageInfo& s) const
{
    return true;
}
bool AnimationResourceType::ReloadResource(const StorageInfo& s, const CORE_NS::IResource::Ptr&) const
{
    return false;
}

SCENE_END_NAMESPACE()