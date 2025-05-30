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
#include "class_content_loader.h"

#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

bool ClassContentLoader::Build(const IMetadata::Ptr& data)
{
    classIdChangedHandler_.Subscribe(
        META_ACCESS_PROPERTY(ClassId), MakeCallback<IOnChanged>(this, &ClassContentLoader::Reload));
    return true;
}

void ClassContentLoader::Reload()
{
    Invoke<IOnChanged>(META_ACCESS_EVENT(ContentChanged));
}

IObject::Ptr ClassContentLoader::Create(const IObject::Ptr&)
{
    const auto& id = META_ACCESS_PROPERTY(ClassId)->GetValue();

    if (id != BASE_NS::Uid {}) {
        return META_NS::GetObjectRegistry().Create(id);
    }

    return {};
}

META_END_NAMESPACE()
