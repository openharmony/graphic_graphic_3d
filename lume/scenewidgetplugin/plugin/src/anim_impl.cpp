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
#include <scene_plugin/api/scene_uid.h>

#include <3d/ecs/components/mesh_component.h>

#include <meta/ext/concrete_base_object.h>
#include <meta/interface/animation/modifiers/intf_loop.h>
#include <meta/interface/animation/modifiers/intf_speed.h>

#include "bind_templates.inl"
#include "intf_node_private.h"
#include "intf_submesh_bridge.h"
#include "node_impl.h"
#include "submesh_handler_uid.h"
#include "task_utils.h"

using SCENE_NS::MakeTask;
namespace {
class AnimImpl
    : public META_NS::ConcreteBaseMetaObjectFwd<AnimImpl, NodeImpl, SCENE_NS::ClassId::Animation,
          META_NS::IParallelAnimation, META_NS::IAttachment, META_NS::ITimedAnimation, META_NS::IStartableAnimation> {
    using Super = META_NS::ConcreteBaseMetaObjectFwd<AnimImpl, NodeImpl, SCENE_NS::ClassId::Animation,
        META_NS::IParallelAnimation, META_NS::IAttachment, META_NS::ITimedAnimation, META_NS::IStartableAnimation>;

    // From IEcsAnimation
    // META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IEcsAnimation, bool, ReadOnly, false,
    // META_NS::DefaultPropFlags::R_FLAGS)

    // From INamed
    META_IMPLEMENT_INTERFACE_PROPERTY(META_NS::INamed, BASE_NS::string, Name, {})

    // From IAnimation
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, bool, Enabled, true)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, bool, Valid, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, META_NS::TimeSpan, TotalDuration,
        META_NS::TimeSpan::Milliseconds(500), META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, bool, Running, false, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(IAnimation, float, Progress, 0, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)
    META_IMPLEMENT_INTERFACE_PROPERTY(IAnimation, META_NS::ICurve1D::Ptr, Curve)
    META_IMPLEMENT_INTERFACE_PROPERTY(
        IAnimation, META_NS::IAnimationController::WeakPtr, Controller, {}, META_NS::DEFAULT_PROPERTY_FLAGS_NO_SER)

    // from IAttachment IAnimationWithModifiableDuration
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(META_NS::IAttachment, META_NS::IObject::WeakPtr, DataContext)
    META_IMPLEMENT_INTERFACE_READONLY_PROPERTY(META_NS::IAttachment, META_NS::IAttach::WeakPtr, AttachedTo)

    // from ITimedAnimation
    META_IMPLEMENT_INTERFACE_PROPERTY(
        META_NS::ITimedAnimation, META_NS::TimeSpan, Duration, META_NS::TimeSpan::Milliseconds(500))

    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnFinished)
    META_IMPLEMENT_EVENT(META_NS::IOnChanged, OnStarted)

    // IAttach
    bool Attach(
        const BASE_NS::shared_ptr<META_NS::IObject>& attachment, const META_NS::IObject::Ptr& dataContext) override;
    bool Detach(const META_NS::IObject::Ptr& attachment) override;

    /**
     * @brief Called by the framework when an the attachment is being attached to an IObject. If this
     *        function succeeds, the object is attached to the target.
     * @param object The IObject instance the attachment is attached to.
     * @param dataContext The data context for this attachment.
     * @note The data context can be the same object as the object being attached to, or
     *       something else. It is up to the attachment to decide how to handle them.
     * @return The implementation should return true if the attachment can be attached to target object.
     *         If the attachment cannot be added, the implementation should return false.
     */
    bool Attaching(const IAttach::Ptr& target, const IObject::Ptr& dataContext) override
    {
        META_ACCESS_PROPERTY(AttachedTo)->SetValue(target);
        META_ACCESS_PROPERTY(DataContext)->SetValue(dataContext);
        return true;
    }
    /**
     * @brief Detach the attachment from an object.
     * @param object The object to attach to.
     * @return If the attachment can be detached from the target, the implementation should return true.
     *         If detaching is not possible, the implementation should return false. In such a case the
     *         target may choose to not remove the attachment. During for example object destruction,
     *         the target will ignore the return value.
     */
    bool Detaching(const IAttach::Ptr& target) override
    {
        META_ACCESS_PROPERTY(AttachedTo)->SetValue({});
        META_ACCESS_PROPERTY(DataContext)->SetValue({});

        // TODO: Remove this line of code, once framework automatically removes animations
        // from animation controller when objects are destroyed.

        auto animation = GetSelf<IAnimation>();
        if (animation) {
            auto controller = META_NS::GetValue(animation->Controller()).lock();
            if (controller) {
                controller->RemoveAnimation(animation);
            }
        }

        return true;
    }

    void AddAnimation(const IAnimation::Ptr&) override {}
    void RemoveAnimation(const IAnimation::Ptr&) override {}

    BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() const override
    {
        return {};
    }

    void Step(const META_NS::IClock::ConstPtr& clock) override
    {
        // Nothing to do. Animation is stepped by Engine.
    }

    void Start() override
    {
        // Set ecs animation to pause
	if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
            META_NS::SetValue(animationState_, (uint8_t)CORE3D_NS::AnimationComponent::PlaybackState::PLAY);

            META_ACCESS_PROPERTY(Running)->SetValue(true);
            META_NS::Invoke<META_NS::IOnChanged>(OnStarted());
        }
    }

    void InternalStop()
    {
        // Set ecs animation to pause
        META_NS::SetValue(animationState_, (uint8_t)CORE3D_NS::AnimationComponent::PlaybackState::STOP);
        META_ACCESS_PROPERTY(Running)->SetValue(false);
        SetProgress(0.0f);
    }

    void Stop() override
    {
	if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
            InternalStop();
	}
    }

    void Pause() override
    {
        if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
            META_NS::SetValue(animationState_, (uint8_t)CORE3D_NS::AnimationComponent::PlaybackState::PAUSE);
            META_ACCESS_PROPERTY(Running)->SetValue(false);
        }
    }

    void Restart() override
    {
        Stop();
        Start();
    }

    void Finish() override
    {
        if (META_ACCESS_PROPERTY_VALUE(Enabled)) {
            META_NS::SetValue(animationState_, (uint8_t)CORE3D_NS::AnimationComponent::PlaybackState::STOP);
            META_ACCESS_PROPERTY(Running)->SetValue(false);
            Seek(1.0f);
            META_NS::Invoke<META_NS::IOnChanged>(OnFinished());
        }
    }

    void Seek(float position) override
    {
        if (META_ACCESS_PROPERTY_VALUE(Enabled)) {	
            position = BASE_NS::Math::clamp01(position);
            SetProgress(position);
        }
    }

    void SetProgress(float progress)
    {
        progress = Base::Math::clamp(progress, 0.0f, 1.0f);
        animationStateTime_->SetValue(progress * GetValue(TotalDuration()).ToSecondsFloat());
        META_ACCESS_PROPERTY(Progress)->SetValue(progress);
    }

    // void AddKey(IEcsTrackAnimation::Ptr track, float time) override {}
    // void RemoveKey(IEcsTrackAnimation::Ptr track, uint32_t index) override {}
    // void UpdateKey(IEcsTrackAnimation::Ptr track, uint32_t oldKeyIndex, uint32_t newKeyIndex, float time)
    // override {}

    static constexpr BASE_NS::string_view ANIMATION_COMPONENT_NAME = "AnimationComponent";
    static constexpr size_t ANIMATION_COMPONENT_NAME_LEN = ANIMATION_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view ANIMATION_STATE = "AnimationComponent.state";
    static constexpr BASE_NS::string_view ANIMATION_REPEATS = "AnimationComponent.repeatCount";
    static constexpr BASE_NS::string_view ANIMATION_SOFFSET = "AnimationComponent.startOffset";
    static constexpr BASE_NS::string_view ANIMATION_DURATION = "AnimationComponent.duration";
    static constexpr BASE_NS::string_view ANIMATION_WEIGHT = "AnimationComponent.weight";
    static constexpr BASE_NS::string_view ANIMATION_SPEED = "AnimationComponent.speed";
    static constexpr BASE_NS::string_view ANIMATION_TRACKS = "AnimationComponent.tracks";

    static constexpr BASE_NS::string_view ANIMATION_STATE_COMPONENT_NAME = "AnimationStateComponent";
    static constexpr size_t ANIMATION_STATE_COMPONENT_NAME_LEN = ANIMATION_STATE_COMPONENT_NAME.size() + 1;
    static constexpr BASE_NS::string_view ANIMATION_STATE_TIME = "AnimationStateComponent.time";
    static constexpr BASE_NS::string_view ANIMATION_OPTIONS = "AnimationStateComponent.options";

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = false;
        if (ret = NodeImpl::Build(data); ret) {
            PropertyNameMask()[ANIMATION_COMPONENT_NAME] = { ANIMATION_STATE.substr(ANIMATION_COMPONENT_NAME_LEN),
                ANIMATION_REPEATS.substr(ANIMATION_COMPONENT_NAME_LEN),
                ANIMATION_SOFFSET.substr(ANIMATION_COMPONENT_NAME_LEN),
                ANIMATION_DURATION.substr(ANIMATION_COMPONENT_NAME_LEN),
                ANIMATION_SPEED.substr(ANIMATION_COMPONENT_NAME_LEN),
                ANIMATION_TRACKS.substr(ANIMATION_COMPONENT_NAME_LEN) };
            PropertyNameMask()[ANIMATION_STATE_COMPONENT_NAME] = { ANIMATION_STATE_TIME.substr(
                                                                       ANIMATION_STATE_COMPONENT_NAME_LEN),
                ANIMATION_OPTIONS.substr(ANIMATION_STATE_COMPONENT_NAME_LEN) };
        }
        return ret;
    }

    void SetValid(bool valid)
    {
        META_ACCESS_PROPERTY(Valid)->SetValue(valid);
        if (valid) {
            if (auto ecsScene = EcsScene()) {
                // Animation controller does not add animation unless it is valid (i.e. connected)
                ecsScene->AddApplicationTask(MakeTask(
                                                 [](const auto& attachment, const auto& controller) {
                                                     if (attachment && controller) {
                                                         controller->AddAnimation(attachment);
                                                     }
                                                     return false;
                                                 },
                                                 GetSelf<META_NS::IAnimation>(),
                                                 interface_pointer_cast<META_NS::IAnimationController>(ecsScene)),
                    false);
            }
        }
    }

    void UpdateTotalDuration() const
    {
        const auto speed = animationSpeed_ ? animationSpeed_->GetValue() : 1.f;
        const auto origDuration = animationOrigDuration_ ? animationOrigDuration_->GetValue() : 0.f;

	// By default we will play once.
	float times = 1.0f;

        // In case we have repeats requested, then play also them.
        if (animationRepeatCount_) {
            times += static_cast<float>(animationRepeatCount_->GetValue());
        }
        const auto newTotalDuration =
            speed != 0.f ? META_NS::TimeSpan::Seconds(origDuration * times / speed) : META_NS::TimeSpan::Infinite();

        META_ACCESS_PROPERTY(TotalDuration)->SetValue(newTotalDuration);
    }

    void UpdateProgress() const
    {
        const auto currentTime = animationStateTime_ ? animationStateTime_->GetValue() : 0.f;
        const auto totalDuration = TotalDuration()->GetValue().ToSecondsFloat();
        const auto newProgress = totalDuration != 0.f ? currentTime / totalDuration : 1.f;
        META_ACCESS_PROPERTY(Progress)->SetValue(newProgress);
    }

    bool CompleteInitialization(const BASE_NS::string& path) override
    {
        // At the end we would not even like to call this, to be refactored
        if (!NodeImpl::CompleteInitialization(path)) {
            return false;
        }

        auto meta = interface_pointer_cast<META_NS::IMetadata>(ecsObject_);
        BindChanges(propHandler_, Name(), meta, "Name");
        // Track timestamps do not scale automatically when total duration changes, have engine duration separated
        // from ours BindChanges<float, META_NS::TimeSpan>(

        animationState_ = meta->GetPropertyByName<uint8_t>(ANIMATION_STATE);
        animationOptions_ = meta->GetPropertyByName<uint8_t>(ANIMATION_OPTIONS);
        animationStateTime_ = meta->GetPropertyByName<float>(ANIMATION_STATE_TIME);
        animationSpeed_ = meta->GetPropertyByName<float>(ANIMATION_SPEED);
        animationRepeatCount_ = meta->GetPropertyByName<uint32_t>(ANIMATION_REPEATS);
        animationOrigDuration_ = meta->GetPropertyByName<float>(ANIMATION_DURATION);
        if (animationSpeed_) {
            propHandler_.NewHandler(nullptr, nullptr).Subscribe(animationSpeed_, [this]() { UpdateTotalDuration(); });
        }
        if (animationRepeatCount_) {
            propHandler_.NewHandler(nullptr, nullptr).Subscribe(animationRepeatCount_, [this]() {
                UpdateTotalDuration();
            });
        }
        if (animationOrigDuration_) {
            propHandler_.NewHandler(nullptr, nullptr).Subscribe(animationOrigDuration_, [this]() {
                UpdateTotalDuration();
            });
        }
        if (animationStateTime_) {
            propHandler_.NewHandler(nullptr, nullptr).Subscribe(animationStateTime_, [this]() { UpdateProgress(); });
        }
        propHandler_.NewHandler(nullptr, nullptr).Subscribe(Enabled(), [this] {
            if (!Enabled()->GetValue()) {
                InternalStop();
            }
        });
        TotalDuration()->OnChanged()->AddHandler(
            META_NS::MakeCallback<META_NS::IOnChanged>([this] { UpdateProgress(); }));
        if (animationOptions_) {
            animationOptions_->SetValue(0);
        }

        if (animationStateTime_) {
        }
        if (animationRepeatCount_) {
            animationRepeatCount_->SetValue(0);
        }
        UpdateTotalDuration();

        // properties are suitable for direct value binding, adding the custom handlers for the rest:

        // We interpret the animation valid if it is connected
        SetValid(true);

        propHandler_.NewHandler(nullptr, nullptr)
            .Subscribe(META_ACCESS_PROPERTY(ConnectionStatus), META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
                SetValid(META_ACCESS_PROPERTY(ConnectionStatus)->GetValue() == ECS_OBJ_STATUS_CONNECTED);
            }));

        propHandler_.NewHandler(nullptr, nullptr)
            .Subscribe(META_ACCESS_PROPERTY(Progress), META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
                if (META_NS::GetValue(META_ACCESS_PROPERTY(Progress)) >= 1.0f) {
                    META_ACCESS_PROPERTY(Running)->SetValue(false);
                    META_NS::Invoke<META_NS::IOnChanged>(OnFinished());
                }
            }));
        return true;
    }

    bool BuildChildren(SCENE_NS::INode::BuildBehavior) override
    {
        // in typical cases we should not have children
        if (META_NS::GetValue(META_ACCESS_PROPERTY(Status)) == SCENE_NS::INode::NODE_STATUS_CONNECTED) {
            SetStatus(SCENE_NS::INode::NODE_STATUS_FULLY_CONNECTED);
            META_NS::Invoke<META_NS::IOnChanged>(OnBound());
            bound_ = true;
        }
        return true;
    }

    void AttachModifier(const BASE_NS::shared_ptr<META_NS::IAnimationModifier>& attachment);
    void DetachModifier(const BASE_NS::shared_ptr<META_NS::IAnimationModifier>& attachment);

    mutable META_NS::EventImpl<META_NS::IOnChanged> onFinished_;
    mutable META_NS::EventImpl<META_NS::IOnChanged> onStarted_;

    META_NS::Property<float> animationStateTime_;
    META_NS::Property<float> animationSpeed_;
    META_NS::Property<float> animationOrigDuration_;
    META_NS::Property<uint8_t> animationOptions_;
    META_NS::Property<uint8_t> animationState_;
    META_NS::Property<uint32_t> animationRepeatCount_;
};
struct LoopCountConverter {
    static uint32_t ToEcs(SceneHolder& sceneHolder, const int32_t& v)
    {
        return v < 0 ? CORE3D_NS::AnimationComponent::REPEAT_COUNT_INFINITE : static_cast<uint32_t>(v);
    }
    static int32_t ToUi(SceneHolder& sceneHolder, const uint32_t& v)
    {
        return v == CORE3D_NS::AnimationComponent::REPEAT_COUNT_INFINITE ? -1 : static_cast<int32_t>(v);
    }
};

bool AnimImpl::Attach(const BASE_NS::shared_ptr<META_NS::IObject>& attachment, const IObject::Ptr& dataContext)
{
    if (auto am = interface_pointer_cast<META_NS::IAnimationModifier>(attachment)) {
        AttachModifier(am);
    }
    return Super::Attach(attachment, dataContext);
}

bool AnimImpl::Detach(const META_NS::IObject::Ptr& attachment)
{
    if (auto am = interface_pointer_cast<META_NS::IAnimationModifier>(attachment)) {
        DetachModifier(am);
    }
    return Super::Detach(attachment);
}

void AnimImpl::AttachModifier(const BASE_NS::shared_ptr<META_NS::IAnimationModifier>& attachment)
{
    if (auto loop = interface_pointer_cast<META_NS::AnimationModifiers::ILoop>(attachment)) {
        BindPropChanges<int32_t, uint32_t, LoopCountConverter>(propHandler_, loop->LoopCount(), animationRepeatCount_);
    } else if (auto speed = interface_pointer_cast<META_NS::AnimationModifiers::ISpeed>(attachment)) {
        BindPropChanges<float, float>(propHandler_, speed->SpeedFactor(), animationSpeed_);
    } else {
        CORE_LOG_E("Not implemented!");
    }
}
void AnimImpl::DetachModifier(const BASE_NS::shared_ptr<META_NS::IAnimationModifier>& attachment)
{
    if (auto loop = interface_pointer_cast<META_NS::AnimationModifiers::ILoop>(attachment)) {
        propHandler_.EraseHandler(animationRepeatCount_, loop->LoopCount());
        animationRepeatCount_->SetValue(0);
    } else if (auto speed = interface_pointer_cast<META_NS::AnimationModifiers::ISpeed>(attachment)) {
        propHandler_.EraseHandler(animationSpeed_, speed->SpeedFactor());
        animationSpeed_->SetValue(1.f);
    } else {
        CORE_LOG_E("Not implemented!");
    }
}
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterAnimImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<AnimImpl>();
}
void UnregisterAnimImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<AnimImpl>();
}
SCENE_END_NAMESPACE();
