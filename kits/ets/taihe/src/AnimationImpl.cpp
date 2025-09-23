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

namespace OHOS::Render3D::KITETS {
::SceneResources::Animation AnimationImpl::createAnimationFromETS(std::shared_ptr<AnimationETS> animationETS)
{
    return taihe::make_holder<AnimationImpl, ::SceneResources::Animation>(animationETS);
}

AnimationImpl::AnimationImpl(const std::shared_ptr<AnimationETS> animationETS)
    : SceneResourceImpl(SceneResources::SceneResourceType::key_t::ANIMATION, animationETS), animationETS_(animationETS)
{}

AnimationImpl::~AnimationImpl()
{
    animationETS_.reset();
}

bool AnimationImpl::getEnabled()
{
    return animationETS_->GetEnabled();
}

void AnimationImpl::setEnabled(bool enabled)
{
    animationETS_->SetEnabled(enabled);
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

void AnimationImpl::onStarted(::taihe::callback_view<void(SceneResources::CallbackUndefinedType const &)> callback)
{
    animationETS_->OnStarted(
        [callback = ::taihe::callback<void(SceneResources::CallbackUndefinedType const &)>(callback)]() {
            if (callback.is_error()) {
                WIDGET_LOGE("Animation onStarted callback error");
            } else {
                auto result = SceneResources::CallbackUndefinedType::make_undefined();
                callback(result);
            }
        });
}

void AnimationImpl::onFinishedInner(::taihe::callback_view<void()> callback)
{
    animationETS_->OnFinished([callback = ::taihe::callback<void()>(callback)]() {
        if (callback.is_error()) {
            WIDGET_LOGE("Animation onFinished callback error");
        } else {
            callback();
        }
    });
}

void AnimationImpl::pause()
{
    animationETS_->Pause();
}

void AnimationImpl::restart()
{
    animationETS_->Restart();
}

void AnimationImpl::seek(double position)
{
    animationETS_->Seek(static_cast<float>(position));
}

void AnimationImpl::start()
{
    animationETS_->Start();
}

void AnimationImpl::stop()
{
    animationETS_->Stop();
}

void AnimationImpl::finish()
{
    animationETS_->Finish();
}

int64_t AnimationImpl::getAnimationImpl()
{
    return reinterpret_cast<uintptr_t>(this);
}

std::shared_ptr<AnimationETS> AnimationImpl::getAnimationETS() const
{
    return animationETS_;
}

::SceneResources::Animation animationTransferStaticImpl(uintptr_t input)
{
    ani_object esValue = reinterpret_cast<ani_object>(input);
    void *nativePtr = nullptr;
    if (!arkts_esvalue_unwrap(taihe::get_env(), esValue, &nativePtr) || nativePtr == nullptr) {
        TH_THROW(std::runtime_error, "animationTransferStaticImpl failed during arkts_esvalue_unwrap.");
    }

    AnimationJS *tro = reinterpret_cast<AnimationJS *>(nativePtr);
    META_NS::IObject::Ptr anim = tro->GetNativeObject();
    if (anim == nullptr) {
        TH_THROW(std::runtime_error, "animationTransferStaticImpl failed during GetNativeObject.");
    }

    NapiApi::Object sceneJs = tro->GetSceneWeakRef().GetObject();
    if (!sceneJs) {
        TH_THROW(std::runtime_error, "animationTransferStaticImpl failed during GetSceneWeakRef.");
    }
    SCENE_NS::IScene::Ptr scene = sceneJs.GetNative<SCENE_NS::IScene>();
    if (!scene) {
        TH_THROW(std::runtime_error, "animationTransferStaticImpl Invalid scene.");
    }

    return taihe::make_holder<AnimationImpl, ::SceneResources::Animation>(std::make_shared<AnimationETS>(anim, scene));
}

uintptr_t animationTransferDynamicImpl(::SceneResources::Animation input)
{
    if (input.is_error()) {
        WIDGET_LOGE("null input animation vtbl_ptr");
        return 0;
    }
    int64_t implRawPtr = input->getAnimationImpl();
    AnimationImpl *implPtr = reinterpret_cast<AnimationImpl *>(implRawPtr);

    std::shared_ptr<AnimationETS> animationETS = implPtr->getAnimationETS();
    if (!animationETS) {
        WIDGET_LOGE("get AnimationETS failed");
        return 0;
    }

    META_NS::IObject::Ptr animationRef = animationETS->GetNativeObj();
    META_NS::IAnimation::Ptr anim = interface_pointer_cast<META_NS::IAnimation>(animationRef);
    if (!anim) {
        WIDGET_LOGE("can't get scene from animation");
        return 0;
    }

    napi_env jsenv;
    if (!arkts_napi_scope_open(taihe::get_env(), &jsenv)) {
        WIDGET_LOGE("arkts_napi_scope_open failed");
        return 0;
    }
    if (!CheckNapiEnv(jsenv)) {
        WIDGET_LOGE("CheckNapiEnv failed");
        // An error has occurred, ignoring the function call result.
        arkts_napi_scope_close_n(jsenv, 0, nullptr, nullptr);
        return 0;
    }

    SCENE_NS::IScene::Ptr scene = animationETS->GetScene();
    if (!scene) {
        WIDGET_LOGE("INVALID SCENE!");
        return 0;
    }

    NapiApi::Object sceneJs = CreateFromNativeInstance(jsenv, scene, PtrType::STRONG, {});
    napi_value args[] = { sceneJs.ToNapiValue(), NapiApi::Object(jsenv).ToNapiValue() };
    NapiApi::Object animationJs = CreateFromNativeInstance(jsenv, anim, PtrType::STRONG, args);

    napi_value animValue = animationJs.ToNapiValue();
    ani_ref resAny;
    if (!arkts_napi_scope_close_n(jsenv, 1, &animValue, &resAny)) {
        WIDGET_LOGE("arkts_napi_scope_close_n failed");
        return 0;
    }
    return reinterpret_cast<uintptr_t>(resAny);
}
} // namespace OHOS::Render3D::KITETS

using namespace OHOS::Render3D::KITETS;
// Since these macros are auto-generate, lint will cause false positive.
// NOLINTBEGIN
TH_EXPORT_CPP_API_animationTransferStaticImpl(animationTransferStaticImpl);
TH_EXPORT_CPP_API_animationTransferDynamicImpl(animationTransferDynamicImpl);
// NOLINTEND