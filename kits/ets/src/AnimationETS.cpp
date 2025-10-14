/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "AnimationETS.h"
#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

namespace OHOS::Render3D {
AnimationETS::AnimationETS(const META_NS::IAnimation::Ptr animation, const SCENE_NS::IScene::Ptr scene)
    : SceneResourceETS(SceneResourceETS::SceneResourceType::ANIMATION), animation_(animation), scene_(scene)
{
    using namespace META_NS;
    auto attach = interface_cast<META_NS::IAttach>(animation);
    if (animation && attach) {
        // check if there is a speed controller already.(and use that)
        auto attachments = attach->GetAttachments();
        for (auto at : attachments) {
            if (auto modifier = interface_pointer_cast<AnimationModifiers::ISpeed>(at)) {
                // yes.. (expect at most one)
                speedModifier_ = modifier;
                break;
            }
        }
#if defined(USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED) && (USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED == 1)
        using namespace SCENE_NS;
        auto acc = interface_cast<IEcsObjectAccess>(animation);
        IEcsObject::Ptr ecsObj;
        if ((acc) && (ecsObj = acc->GetEcsObject())) {
            auto completedProp = ecsObj->CreateProperty("AnimationStateComponent.completed").GetResult();
            if (completedProp) {
                OnCompletedEvent_ = completedProp->OnChanged();
            }
            completed_ = completedProp;
        }
#else
        // Use IAnimation OnFinished to trigger the animation ends (has a bug)
        OnCompletedEvent_ = animation->OnFinished();
#endif
    }
}

AnimationETS::~AnimationETS()
{
    if (auto modifier = speedModifier_.lock()) {
        if (auto attach = interface_cast<META_NS::IAttach>(animation_.lock())) {
            attach->Detach(modifier);
        }
    }
    if (auto anim = animation_.lock(); (anim != nullptr) && (OnStartedToken_ != 0)) {
        anim->OnStarted()->RemoveHandler(OnStartedToken_);
    }
    OnStartedToken_ = 0;

    if (OnCompletedEvent_ && OnFinishedToken_ != 0) {
        OnCompletedEvent_->RemoveHandler(OnFinishedToken_);
    }
    OnFinishedToken_ = 0;
    OnFinishedCB_ = {};
}

META_NS::IObject::Ptr AnimationETS::GetNativeObj() const
{
    return interface_pointer_cast<META_NS::IObject>(animation_);
}

BASE_NS::string AnimationETS::GetName()
{
    if (auto named = interface_cast<META_NS::INamed>(animation_.lock())) {
        return META_NS::GetValue(named->Name());
    } else {
        return "";
    }
}
void AnimationETS::SetName(const BASE_NS::string &name)
{
    if (auto named = interface_cast<META_NS::INamed>(animation_.lock())) {
        named->Name()->SetValue(name);
    }
}

bool AnimationETS::GetEnabled()
{
    bool enabled{false};
    if (auto a = animation_.lock()) {
        enabled = a->Enabled()->GetValue();
    }
    return enabled;
}
void AnimationETS::SetEnabled(bool enabled)
{
    if (auto a = animation_.lock()) {
        a->Enabled()->SetValue(enabled);
    }
}

float AnimationETS::GetSpeed()
{
    float speed = 1.0;
    if (auto modifier = speedModifier_.lock()) {
        speed = modifier->SpeedFactor()->GetValue();
    }
    return speed;
}
void AnimationETS::SetSpeed(float speed)
{
    if (auto a = animation_.lock()) {
        using namespace META_NS;
        META_NS::AnimationModifiers::ISpeed::Ptr modifier = speedModifier_.lock();
        if (!modifier) {
            modifier = GetObjectRegistry().Create<AnimationModifiers::ISpeed>(ClassId::SpeedAnimationModifier);
            interface_cast<IAttach>(a)->Attach(modifier);
            speedModifier_ = modifier;
        }
        modifier->SpeedFactor()->SetValue(speed);
    }
}

float AnimationETS::GetDuration() const
{
    float duration = 0.0;
    if (auto a = animation_.lock()) {
        duration = a->TotalDuration()->GetValue().ToSecondsFloat();
    }
    return duration;
}

bool AnimationETS::GetRunning() const
{
    bool running{false};
    if (auto a = animation_.lock()) {
        running = a->Running()->GetValue();
    }
    return running;
}

float AnimationETS::GetProgress() const
{
    float progress = 0.0;
    if (auto a = animation_.lock()) {
        progress = a->Progress()->GetValue();
    }
    return progress;
}

void AnimationETS::OnStarted(const std::function<void()> &onStartedCB)
{
    META_NS::IAnimation::Ptr anim = animation_.lock();
    if (!anim) {
        CORE_LOG_E("Invalid animation");
        return;
    }
    if (OnStartedToken_ != 0) {
        anim->OnStarted()->RemoveHandler(OnStartedToken_);
        OnStartedToken_ = 0;
    }
    OnStartedToken_ = anim->OnStarted()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(onStartedCB));
}

void AnimationETS::OnFinished(const std::function<void()> &onFinishedCB)
{
    if (!OnCompletedEvent_) {
        CORE_LOG_E("There is no completed event for animation");
        return;
    }
    if (OnFinishedToken_ != 0) {
        OnCompletedEvent_->RemoveHandler(OnFinishedToken_);
        OnFinishedToken_ = 0;
        OnFinishedCB_ = {};
    }
    OnFinishedCB_ = onFinishedCB;
#if defined(USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED) && (USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED == 1)
    auto cb = META_NS::MakeCallback<META_NS::IOnChanged>([this]() {
        bool completion = false;
        auto completedProp = completed_.lock();
        if (!completedProp) {
            return;
        }
        completedProp->GetValue().GetValue(completion);
        if (completion && OnFinishedCB_) {
            OnFinishedCB_();
        }
    });
#else
    auto cb = META_NS::MakeCallback<META_NS::IOnChanged>(onFinishedCB);
#endif
    OnFinishedToken_ = OnCompletedEvent_->AddHandler(cb);
}

void AnimationETS::Pause()
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(animation_.lock())) {
        a->Pause();
    }
}

void AnimationETS::Restart()
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(animation_.lock())) {
        a->Restart();
    }
}

void AnimationETS::Seek(float position)
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(animation_.lock())) {
        a->Seek(position);
    }
}

void AnimationETS::Start()
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(animation_.lock())) {
        a->Start();
    }
}

void AnimationETS::Stop()
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(animation_.lock())) {
        a->Stop();
    }
}

void AnimationETS::Finish()
{
    if (auto a = interface_cast<META_NS::IStartableAnimation>(animation_.lock())) {
        a->Finish();
    }
}
}  // namespace OHOS::Render3D