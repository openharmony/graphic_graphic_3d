/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_MIMP_SRC_TYPES_NODE_TEMPLATE_TYPE_H
#define SCENE_MIMP_SRC_TYPES_NODE_TEMPLATE_TYPE_H

#include <scene/base/namespace.h>

#include <meta/interface/resource/intf_object_template.h>

#include "resource_type_base.h"

SCENE_BEGIN_NAMESPACE()

class NodeTemplateType : public META_NS::IntroduceInterfaces<ResourceTypeBase> {
    META_OBJECT(NodeTemplateType, ClassId::NodeTemplateResource, IntroduceInterfaces)
public:
    NodeTemplateType() : Super(ClassId::NodeTemplateResource)
    {}

    CORE_NS::IResource::Ptr LoadResource(const StorageInfo&) const override;
    bool SaveResource(const CORE_NS::IResource::ConstPtr&, const StorageInfo&) const override;
    bool ReloadResource(const StorageInfo&, const CORE_NS::IResource::Ptr&) const override;

private:
    void LoadNodeTemplateResources(const META_NS::IObjectTemplate& ot, const StorageInfo& s) const;
    void SaveNodeTemplateResources(const META_NS::IObjectTemplate& ot, const StorageInfo& s) const;
};

SCENE_END_NAMESPACE()

#endif
