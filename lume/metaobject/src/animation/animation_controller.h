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
#ifndef META_SRC_ANIMATION_CONTROLLER_H
#define META_SRC_ANIMATION_CONTROLLER_H

#include <shared_mutex>

#include <base/containers/vector.h>

#include <meta/ext/attachment/attachment.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_clock.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class AnimationController final : public META_NS::AttachmentFwd<AnimationController, ClassId::AnimationController,
                                      IAnimationController, ISerializable, IImportFinalize> {
private:
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimationController, uint32_t, Count)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimationController, uint32_t, RunningCount)

protected:
    bool AttachTo(const META_NS::IAttach::Ptr& target, const META_NS::IObject::Ptr& dataContext) override;
    bool DetachFrom(const META_NS::IAttach::Ptr& target) override;

private: // IAnimationController
    BASE_NS::vector<IAnimation::WeakPtr> GetAnimations() const override;
    BASE_NS::vector<IAnimation::WeakPtr> GetRunning() const override;
    bool AddAnimation(const IAnimation::Ptr& animation) override;
    bool RemoveAnimation(const IAnimation::Ptr& animation) override;
    void Clear() override;
    StepInfo Step(const IClock::ConstPtr& clock) override;
    bool Build(const IMetadata::Ptr& data) override;

    ReturnError Export(IExportContext&) const override;
    ReturnError Import(IImportContext&) override;
    ReturnError Finalize(IImportFunctions&) override;

private:
    void UpdateAnimations();
    void UpdateRunningHandler(const IAnimation::Ptr& animation, bool addHandler);

    IOnChanged::InterfaceTypePtr updateCallback_;             // UpdateAnimations() callback
    mutable BASE_NS::vector<IAnimation::WeakPtr> animations_; // All animations
    mutable BASE_NS::vector<IAnimation::WeakPtr> running_;    // Currently running animations
    mutable std::shared_mutex mutex_;
};

META_END_NAMESPACE()

#endif
