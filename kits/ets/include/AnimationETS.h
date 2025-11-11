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

#ifndef OHOS_3D_ANIMATION_ETS_H
#define OHOS_3D_ANIMATION_ETS_H

#include <meta/api/make_callback.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/animation/modifiers/intf_speed.h>
#include <meta/interface/intf_attach.h>
#include <scene/interface/intf_scene.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <string>
#include "SceneResourceETS.h"

namespace OHOS::Render3D {
class AnimationETS : public SceneResourceETS {
public:
    AnimationETS(const META_NS::IAnimation::Ptr animation, const SCENE_NS::IScene::Ptr scene);
    ~AnimationETS() override;
    void Destroy() override;
    void Cleanup();

    SCENE_NS::IScene::Ptr GetScene() const
    {
        return scene_.lock();
    }

    META_NS::IObject::Ptr GetNativeObj() const override;
    BASE_NS::string GetName();
    void SetName(const BASE_NS::string &name);

    bool GetEnabled();
    void SetEnabled(bool enabled);

    float GetSpeed();
    void SetSpeed(float speed);

    float GetDuration() const;
    bool GetRunning() const;
    float GetProgress() const;

    void OnStarted(const std::function<void()> &onStartedCB);
    void OnFinished(const std::function<void()> &onFinishedCB);

    void Pause();
    void Restart();
    void Seek(float position);
    void Start();
    void Stop();
    void Finish();

private:
    META_NS::IAnimation::WeakPtr animation_{nullptr};
    SCENE_NS::IScene::WeakPtr scene_{nullptr}; // Store a weak ref

    META_NS::IEvent::Token OnFinishedToken_{0};
    META_NS::IEvent::Token OnStartedToken_{0};

    std::function<void()> OnFinishedCB_;
    META_NS::AnimationModifiers::ISpeed::WeakPtr speedModifier_;
#define USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED 0
#if defined(USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED) && (USE_ANIMATION_STATE_COMPONENT_ON_COMPLETED == 1)
    META_NS::IProperty::WeakPtr completed_;
#endif
    META_NS::Event<META_NS::IEvent> OnCompletedEvent_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_ANIMATION_ETS_H
