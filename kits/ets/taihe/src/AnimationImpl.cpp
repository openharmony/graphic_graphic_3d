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

#include "AnimationImpl.h"

#ifdef __SCENE_ADAPTER__
#include "3d_widget_adapter_log.h"
#endif

::SceneResources::Animation AnimationImpl::createAnimationFromETS(std::shared_ptr<AnimationETS> animationETS)
{
    return taihe::make_holder<AnimationImpl, ::SceneResources::Animation>(animationETS);
}

AnimationImpl::AnimationImpl(const std::shared_ptr<AnimationETS> animationETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::ANIMATION), animationETS_(animationETS)
{
    WIDGET_LOGE("AnimationImpl ++");
}

AnimationImpl::~AnimationImpl()
{
    WIDGET_LOGE("AnimationImpl --");
    animationETS_.reset();
}

bool AnimationImpl::getEnabled()
{
    return animationETS_->GetEnabled();
}
void AnimationImpl::setEnabled(bool enabled)
{
    return animationETS_->SetEnabled(enabled);
}

::taihe::optional<double> AnimationImpl::getSpeed()
{
    return taihe::optional<double>(std::in_place, animationETS_->GetSpeed());
}

void AnimationImpl::setSpeed(::taihe::optional_view<double> speed)
{
    if (speed.has_value()) {
        animationETS_->SetSpeed(speed.value());
    } else {
        animationETS_->SetSpeed(1.0F);
    }
}

double AnimationImpl::getDuration()
{
    return static_cast<double>(animationETS_->GetDuration());
}

bool AnimationImpl::getRunning()
{
    return animationETS_->GetRunning();
}

double AnimationImpl::getProgress()
{
    return static_cast<double>(animationETS_->GetProgress());
}

void AnimationImpl::onStarted(::taihe::callback_view<void()> callback)
{
    onStartedCB_ = callback;
    animationETS_->OnStarted([this]() {
        if (!onStartedCB_.is_error()) {
            onStartedCB_();
        }
    });
}

void AnimationImpl::onFinished(::taihe::callback_view<void()> callback)
{
    onFinishedCB_ = callback;
    animationETS_->OnFinished([this]() {
        if (!onFinishedCB_.is_error()) {
            onFinishedCB_();
        }
    });
}

void AnimationImpl::pause()
{
    return animationETS_->Pause();
}

void AnimationImpl::restart()
{
    return animationETS_->Restart();
}

void AnimationImpl::seek(double position)
{
    return animationETS_->Seek(static_cast<float>(position));
}

void AnimationImpl::start()
{
    return animationETS_->Start();
}

void AnimationImpl::stop()
{
    return animationETS_->Stop();
}

void AnimationImpl::finish()
{
    return animationETS_->Finish();
}
