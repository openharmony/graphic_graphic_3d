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

#ifndef SCENE_MIMP_SRC_NODE_TEMPLATE_REWRITE_H
#define SCENE_MIMP_SRC_NODE_TEMPLATE_REWRITE_H

#include <scene/interface/resource/intf_exported_node.h>
#include <scene/interface/resource/types.h>

#include <base/containers/unordered_map.h>
#include <core/resources/intf_resource.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/object_fwd.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/resource/intf_object_template.h>
#include <meta/interface/serialization/intf_ser_transformation.h>
#include <meta/interface/serialization/intf_serializable.h>

SCENE_BEGIN_NAMESPACE()

struct NodeTemplateGroupRewrite {
    NodeTemplateGroupRewrite(BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup)
        : oldGroup(oldGroup), newGroup(newGroup)
    {}
    META_NS::ISerNode::Ptr Process(META_NS::ISerNode::ConstPtr);

    BASE_NS::string_view oldGroup;
    BASE_NS::string_view newGroup;
    bool changed{};
};

SCENE_END_NAMESPACE()

#endif