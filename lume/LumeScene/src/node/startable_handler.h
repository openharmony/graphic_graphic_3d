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

#ifndef SCENE_SRC_STARTABLE_HANDLER_H
#define SCENE_SRC_STARTABLE_HANDLER_H

#include <scene/ext/intf_internal_scene.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>

SCENE_BEGIN_NAMESPACE()

class StartableHandler {
public:
    enum class StartType : uint8_t {
        DEFERRED = 0,
        DIRECT = 1,
    };

    StartableHandler() = delete;
    StartableHandler(const INode::Ptr& node) : node_(node) {}
    virtual ~StartableHandler() = default;
    bool Start(const META_NS::IObject::Ptr& attachment, StartType type);
    bool Stop(const META_NS::IObject::Ptr& attachment);
    bool StartAll(StartType type);
    bool StopAll();
    bool StopAll(const META_NS::IContainer::Ptr& container);

private:
    IInternalScene::Ptr GetScene() const;
    INode::WeakPtr node_;
};

namespace Internal {

bool StartStartable(
    const IInternalScene::Ptr& scene, StartableHandler::StartType type, const META_NS::IObject::Ptr& object);
bool StopStartable(const META_NS::IObject::Ptr& object);

bool StartAllStartables(
    const IInternalScene::Ptr& scene, StartableHandler::StartType type, const META_NS::IObject::Ptr& object);
bool StopAllStartables(const META_NS::IObject::Ptr& object);

} // namespace Internal

SCENE_END_NAMESPACE()

#endif // SCENE_SRC_STARTABLE_HANDLER_H
