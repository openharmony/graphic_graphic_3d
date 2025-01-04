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

#include "animation_controller.h"

#include <meta/api/make_callback.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

bool AnimationController::Build(const IMetadata::Ptr& data)
{
    updateCallback_ = MakeCallback<IOnChanged>(this, &AnimationController::UpdateAnimations);
    return true;
}

bool AnimationController::AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext)
{
    return true;
}

bool AnimationController::DetachFrom(const META_NS::IAttach::Ptr& target)
{
    return true;
}

void AnimationController::UpdateAnimations()
{
    uint32_t count = 0;
    uint32_t running = 0;
    {
        std::unique_lock lock(mutex_);
        auto it = animations_.begin();
        running_.clear();
        while (it != animations_.end()) {
            auto& weak = *it;
            if (auto animation = weak.lock()) {
                // Assuming here that getting the running property value cannot cause a recursive call back
                // to the animation controller
                if (GetValue(animation->Running())) {
                    running_.push_back(weak);
                }
                it++;
            } else {
                // Animation has died, clean up weak_ptr from our list
                it = animations_.erase(it);
            }
        }
        count = animations_.size();
        running = running_.size();
    }

    SetValue(META_ACCESS_PROPERTY(Count), count);
    SetValue(META_ACCESS_PROPERTY(RunningCount), running);
}

void AnimationController::UpdateRunningHandler(const IAnimation::Ptr& animation, bool addHandler)
{
    if (!animation) {
        return;
    }
    if (auto running = animation->Running()) {
        if (addHandler) {
            running->OnChanged()->AddHandler(updateCallback_, uintptr_t(this));
        } else {
            running->OnChanged()->RemoveHandler(uintptr_t(this));
        }
    }
}

bool AnimationController::AddAnimation(const META_NS::IAnimation::Ptr& animation)
{
    if (!animation) {
        return false;
    }
    {
        std::unique_lock lock(mutex_);
        for (auto& weak : animations_) {
            if (weak.lock() == animation) {
                // Already there
                return true;
            }
        }
        animations_.emplace_back(animation);
    }

    UpdateRunningHandler(animation, true);
    SetValue(animation->Controller(), GetSelf<IAnimationController>());
    UpdateAnimations();
    return true;
}

bool AnimationController::RemoveAnimation(const META_NS::IAnimation::Ptr& animation)
{
    bool removed = false;
    if (animation) {
        std::unique_lock lock(mutex_);
        for (auto it = animations_.begin(); it != animations_.end(); it++) {
            if ((*it).lock() == animation) {
                animations_.erase(it);
                removed = true;
                break;
            }
        }
    }
    if (removed) {
        UpdateRunningHandler(animation, false);
        UpdateAnimations();
    }
    return removed;
}

void AnimationController::Clear()
{
    {
        BASE_NS::vector<IAnimation::WeakPtr> all;
        {
            std::unique_lock lock(mutex_);
            all.swap(animations_);
            running_.clear();
        }
        for (auto& weak : all) {
            UpdateRunningHandler(weak.lock(), false);
        }
    }
    UpdateAnimations();
}

IAnimationController::StepInfo AnimationController::Step(const IClock::ConstPtr& clock)
{
    StepInfo info { 0, 0 };

    for (auto& anim : GetRunning()) {
        if (auto animation = anim.lock()) {
            animation->Step(clock);
            info.stepped_++;
        }
    }

    // Step may end up starting/stopping some animations
    std::shared_lock lock(mutex_);
    info.running_ = running_.size();
    return info;
}

BASE_NS::vector<IAnimation::WeakPtr> AnimationController::GetAnimations() const
{
    std::shared_lock lock(mutex_);
    return animations_;
}

BASE_NS::vector<IAnimation::WeakPtr> AnimationController::GetRunning() const
{
    std::shared_lock lock(mutex_);
    return running_;
}

ReturnError AnimationController::Export(IExportContext& c) const
{
    return Serializer(c) & NamedValue("__animations", animations_);
}
ReturnError AnimationController::Import(IImportContext& c)
{
    return Serializer(c) & NamedValue("__animations", animations_);
}
ReturnError AnimationController::Finalize(IImportFunctions&)
{
    // take the animations out and re-add by setting the animation controller
    BASE_NS::vector<IAnimation::WeakPtr> vec = BASE_NS::move(animations_);
    for (auto&& anim : vec) {
        if (auto p = anim.lock()) {
            p->Controller()->SetValue(GetSelf<IAnimationController>());
        }
    }
    return GenericError::SUCCESS;
}

META_END_NAMESPACE()
