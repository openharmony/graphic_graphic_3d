/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Animation controller implementation
 * Author: Lauri Jaaskela
 * Create: 2021-12-01
 */
#ifndef META_SRC_ANIMATION_CONTROLLER_H
#define META_SRC_ANIMATION_CONTROLLER_H

#include <mutex>
#include <shared_mutex>

#include <base/containers/vector.h>

#include <meta/base/namespace.h>
#include <meta/ext/attachment/attachment.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_clock.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class AnimationController final
    : public IntroduceInterfaces<AttachmentFwd, IAnimationController, ISerializable, IImportFinalize> {
    META_OBJECT(AnimationController, ClassId::AnimationController, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IAnimationController, uint32_t, Count)
    META_STATIC_PROPERTY_DATA(IAnimationController, uint32_t, RunningCount)
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(uint32_t, Count)
    META_IMPLEMENT_READONLY_PROPERTY(uint32_t, RunningCount)

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
