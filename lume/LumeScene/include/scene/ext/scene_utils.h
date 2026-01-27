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

#ifndef SCENE_EXT_SCENE_UTILS_H
#define SCENE_EXT_SCENE_UTILS_H

#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_external_node.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

#include <meta/interface/intf_attach.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Return the resource manager associated with an IInternalScene instance.
 */
inline CORE_NS::IResourceManager::Ptr GetResourceManager(const IInternalScene* is)
{
    if (!is) {
        return {};
    }
    const auto context = is->GetContext();
    if (!context) {
        return {};
    }
    return context->GetResources();
}

/**
 * @brief Return the resource manager associated with an IInternalScene instance.
 */
inline CORE_NS::IResourceManager::Ptr GetResourceManager(const IInternalScene::ConstPtr& is)
{
    return GetResourceManager(is.get());
}

/**
 * @brief Return the resource manager associated with an IInternalScene instance.
 */
inline CORE_NS::IResourceManager::Ptr GetResourceManager(const IScene* scene)
{
    return scene ? GetResourceManager(scene->GetInternalScene()) : nullptr;
}

/**
 * @brief Return the resource manager associated with an IScene instance.
 */
inline CORE_NS::IResourceManager::Ptr GetResourceManager(const IScene::ConstPtr& scene)
{
    return GetResourceManager(scene.get());
}

/**
 * @brief Returns the first attachment of the node which implements IExternalNode,
 *        or nullptr in case such an attachment does not exist.
 */
inline IExternalNode::Ptr GetExternalNodeAttachment(const INode::ConstPtr& node)
{
    const auto att = interface_cast<META_NS::IAttach>(node);
    if (att) {
        const auto ext = att->GetAttachments<IExternalNode>();
        if (!ext.empty()) {
            return ext.front();
        }
    }
    return nullptr;
}

SCENE_END_NAMESPACE()

#endif
