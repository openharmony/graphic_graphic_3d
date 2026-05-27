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

#include "../perf/cpu_perf_scope.h"

SCENE_BEGIN_NAMESPACE()

namespace Internal {

bool StartStartable(
    const IInternalScene::Ptr& scene, StartableHandler::StartType type, const META_NS::IObject::Ptr& object)
{
    SCENE_CPU_PERF_SCOPE("Startable", object ? object->GetName() : BASE_NS::string("<null>"));
    if (auto startable = interface_cast<META_NS::IStartable>(object); startable && scene) {
        const auto state = META_NS::GetValue(startable->StartableState());
        if (state == META_NS::StartableState::ATTACHED) {
            const auto mode = GetValue(startable->StartableMode());
            if (mode == META_NS::StartBehavior::AUTOMATIC) {
                if (type == StartableHandler::StartType::DEFERRED) {
                    scene->PostUserNotification(
                        [s = interface_pointer_cast<META_NS::IStartable>(object)]() { s->Start(); });
                    return true;
                }
                return startable->Start();
            }
        }
    }
    return false;
}

bool StopStartable(const META_NS::IObject::Ptr& object)
{
    if (auto startable = interface_cast<META_NS::IStartable>(object)) {
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

BASE_NS::vector<META_NS::IObject::Ptr> GetStartables(const META_NS::IContainer::Ptr& container)
{
    META_NS::IContainer::FindOptions opts;
    opts.uids = {META_NS::IStartable::UID};
    opts.strict = false;
    return container ? container->FindAll(opts) : BASE_NS::vector<META_NS::IObject::Ptr>{};
}

bool StartAllStartables(
    const IInternalScene::Ptr& scene, StartableHandler::StartType type, const META_NS::IObject::Ptr& object)
{
    auto startAll = [s = IInternalScene::WeakPtr{scene}, n = META_NS::IObject::WeakPtr{object}]() {
        SCENE_CPU_PERF_SCOPE("Startable", "StartAllStartablesRun");
        auto is = s.lock();
        if (auto attach = interface_pointer_cast<META_NS::IAttach>(n); is && attach) {
            if (auto container = attach->GetAttachmentContainer(false)) {
                // Snapshot attachments before starting to avoid holding a shared lock on the
                // container during Start(). OnStart callbacks may modify the container which
                // requires a unique lock, causing a deadlock if the shared lock is still held.
                for (auto& startable : GetStartables(container)) {
                    StartStartable(is, StartableHandler::StartType::DIRECT, startable);
                }
                return true;
            }
        }
        return false;
    };
    if (scene) {
        if (type == StartableHandler::StartType::DEFERRED) {
            scene->PostUserNotification(startAll);
        } else {
            startAll();
        }
        return true;
    }
    return false;
}

bool StopAllStartables(const META_NS::IObject::Ptr& object)
{
    if (auto attach = interface_pointer_cast<META_NS::IAttach>(object)) {
        if (auto container = attach->GetAttachmentContainer(false)) {
            for (auto& startable : GetStartables(container)) {
                StopStartable(startable);
            }
            return true;
        }
    }
    return false;
}

}  // namespace Internal

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
    return Internal::StartStartable(GetScene(), type, attachment);
}

bool StartableHandler::Stop(const META_NS::IObject::Ptr& attachment)
{
    return Internal::StopStartable(attachment);
}

bool StartableHandler::StartAll(StartType type)
{
    return Internal::StartAllStartables(GetScene(), type, interface_pointer_cast<META_NS::IObject>(node_));
}

bool StartableHandler::StopAll(const META_NS::IContainer::Ptr& container)
{
    if (container) {
        for (auto& startable : Internal::GetStartables(container)) {
            Stop(startable);
        }
        return true;
    }
    return false;
}

bool StartableHandler::StopAll()
{
    return Internal::StopAllStartables(interface_pointer_cast<META_NS::IObject>(node_));
}

SCENE_END_NAMESPACE()
