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

#include "object_container.h"

#include <algorithm>

#include <meta/base/interface_utils.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

bool ObjectContainer::Build(const IMetadata::Ptr& data)
{
    bool ret = Super::Build(data);
    if (ret) {
        parent_ = GetSelf<IContainer>();
        SetImplementingIContainer(GetSelf<IObject>().get(), this);
    }
    return ret;
}

void ObjectContainer::Destroy()
{
    InternalRemoveAll();
    Super::Destroy();
}

META_END_NAMESPACE()
