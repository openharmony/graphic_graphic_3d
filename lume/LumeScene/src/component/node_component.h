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

#ifndef SCENE_SRC_COMPONENT_NODE_COMPONENT_H
#define SCENE_SRC_COMPONENT_NODE_COMPONENT_H

#include <scene/ext/component_fwd.h>

#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class IInternalNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalNode, "7ce14037-9137-4cbd-877a-8ea8862e9fb2")
public:
    META_PROPERTY(bool, Enabled)
};

META_REGISTER_CLASS(NodeComponent, "0a222395-21cc-4518-a1ca-0cdca384ba9e", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeComponent : public META_NS::IntroduceInterfaces<ComponentFwd, IInternalNode> {
    META_OBJECT(NodeComponent, ClassId::NodeComponent, IntroduceInterfaces)

public:
    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_PROPERTY_DATA(IInternalNode, bool, Enabled, "NodeComponent.enabled")
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)

public:
    BASE_NS::string GetName() const override;
};

SCENE_END_NAMESPACE()

#endif