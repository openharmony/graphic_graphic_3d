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

#include "startable_handler.h"

SCENE_BEGIN_NAMESPACE()

IInternalScene::Ptr StartableHandler::GetScene() const
{
    if (auto node = node_.lock()) {
        if (auto scene = node->GetScene()) {
            return scene->GetInternalScene();
        }
    }
    return {};
}

bool StartableHandler::Start(const META_NS::IObject::Ptr& attachment, StartType type)
{
    if (auto startable = interface_cast<META_NS::IStartable>(attachment)) {
        const auto state = META_NS::GetValue(startable->StartableState());
        if (state == META_NS::StartableState::ATTACHED) {
            const auto mode = GetValue(startable->StartableMode());
            if (mode == META_NS::StartBehavior::AUTOMATIC) {
                if (auto scene = GetScene()) {
                    if (type == StartType::DEFERRED) {
                        scene->PostUserNotification(
                            [s = interface_pointer_cast<META_NS::IStartable>(attachment)]() { s->Start(); });
                        return true;
                    }
                    return startable->Start();
                }
            }
        }
    }
    return false;
}

bool StartableHandler::Stop(const META_NS::IObject::Ptr& attachment)
{
    if (auto startable = interface_cast<META_NS::IStartable>(attachment)) {
        const auto state = META_NS::GetValue(startable->StartableState());
        if (state == META_NS::StartableState::STARTED) {
            const auto mode = GetValue(startable->StartableMode());
            if (mode == META_NS::StartBehavior::AUTOMATIC) {
                return startable->Stop();
            }
        }
    }
    return false;
}

bool StartableHandler::StartAll(StartType type)
{
    auto startAll = [this, n = INode::WeakPtr { node_ }]() {
        if (auto attach = interface_pointer_cast<META_NS::IAttach>(n)) {
            if (auto container = attach->GetAttachmentContainer(false)) {
                META_NS::IterateShared(
                    container,
                    [this](const META_NS::IObject::Ptr& attachment) {
                        Start(attachment, StartableHandler::StartType::DIRECT);
                        return true;
                    },
                    META_NS::TraversalType::NO_HIERARCHY);
                return true;
            }
        }
        return false;
    };
    if (auto scene = GetScene()) {
        if (type == StartType::DEFERRED) {
            scene->PostUserNotification(startAll);
        } else {
            startAll();
        }
        return true;
    }
    return false;
}

bool StartableHandler::StopAll()
{
    if (auto attach = interface_pointer_cast<META_NS::IAttach>(node_)) {
        if (auto c = attach->GetAttachmentContainer(false)) {
            META_NS::IterateShared(c, [this](const META_NS::IObject::Ptr& attachment) {
                Stop(attachment);
                return true;
            });
            return true;
        }
    }
    return false;
}

SCENE_END_NAMESPACE()
