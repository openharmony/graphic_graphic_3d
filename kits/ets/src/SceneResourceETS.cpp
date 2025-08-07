/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "SceneResourceETS.h"

#include <meta/api/util.h>

namespace OHOS::Render3D {
SceneResourceETS::SceneResourceETS(SceneResourceType type) : type_(type)
{
    CORE_LOG_D("SceneResourceETS ++");
}

SceneResourceETS::~SceneResourceETS()
{
    CORE_LOG_D("SceneResourceETS --");
}

SceneResourceETS::SceneResourceType SceneResourceETS::GetResourceType()
{
    return type_;
}

std::string SceneResourceETS::GetName() const
{
    META_NS::IObject::Ptr native = GetNativeObj();
    if (!native) {
        return name_;
    }
    BASE_NS::string name;
    if (auto named = interface_cast<META_NS::INamed>(native)) {
        name = META_NS::GetValue(named->Name());
    } else if (native) {
        name = native->GetName();
    }
    if (name.empty()) {
        return name_;  // Use cached if we didn't get anything from underlying object
    } else {
        return name.c_str();
    }
}

void SceneResourceETS::SetName(const std::string &name)
{
    META_NS::IObject::Ptr native = GetNativeObj();
    if (auto named = interface_cast<META_NS::INamed>(native)) {
        META_NS::SetValue(named->Name(), name.c_str());
    } else if (auto objectname = interface_cast<META_NS::IObjectName>(native)) {
        objectname->SetName(name.c_str());
    } else {
        // Object does not support naming, store name locally
        name_ = name;
    }
}

void SceneResourceETS::SetUri(const std::string &uri)
{
    uri_ = uri;
}

std::string SceneResourceETS::GetUri() const
{
    return uri_;
}
}  // namespace OHOS::Render3D