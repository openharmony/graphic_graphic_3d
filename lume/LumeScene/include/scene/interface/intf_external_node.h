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

#ifndef SCENE_INTERFACE_INTF_EXTERNAL_NODE_H
#define SCENE_INTERFACE_INTF_EXTERNAL_NODE_H

#include <scene/base/types.h>

#include <core/resources/intf_resource.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Attachment to mark that current node is imported from external scene
 */
class IExternalNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExternalNode, "33b69e97-4ffa-465d-8126-d7c54165aef6")
public:
    virtual CORE_NS::ResourceId GetResourceId() const = 0;
    virtual void SetResourceId(CORE_NS::ResourceId id) = 0;
};

META_REGISTER_CLASS(ExternalNode, "b26e734e-c4ec-48bc-b845-d9feb6dd5de8", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IExternalNode)

#endif
