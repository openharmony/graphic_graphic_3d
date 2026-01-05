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

#ifndef OHOS_3D_ENTITY_PROXY_H
#define OHOS_3D_ENTITY_PROXY_H

#include <scene/ext/intf_ecs_object_access.h>

#include "property_proxy/PropertyProxy.h"

namespace OHOS::Render3D {
class EntityProxy : public PropertyProxy<CORE_NS::Entity> {
public:
    explicit EntityProxy(const META_NS::Property<CORE_NS::Entity> &prop) : PropertyProxy(prop)
    {
    }

    ~EntityProxy() override
    {
    }
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_ENTITY_PROXY_H