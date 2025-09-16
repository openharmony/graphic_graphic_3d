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

#ifndef OHOS_3D_ANIMATION_IMPL_H
#define OHOS_3D_ANIMATION_IMPL_H

#include "stdexcept"
#include "taihe/optional.hpp"
#include "taihe/runtime.hpp"

#include "SceneTH.user.hpp"
#include "SceneResources.user.hpp"
#include "SceneResources.Transfer.proj.hpp"
#include "SceneResources.Transfer.impl.hpp"

#include "AnimationJS.h"
#include <napi_api.h>
#include "interop_js/arkts_interop_js_api.h"
#include "interop_js/arkts_esvalue.h"
#include "napi/native_api.h"
#include "CheckNapiEnv.h"

#include "AnimationETS.h"
#include "SceneResourceImpl.h"

namespace OHOS::Render3D::KITETS {
class AnimationImpl : public SceneResourceImpl {
public:
    static SceneResources::Animation createAnimationFromETS(std::shared_ptr<AnimationETS> animationETS);
    explicit AnimationImpl(const std::shared_ptr<AnimationETS> animationETS_);
    ~AnimationImpl();

    bool getEnabled();
    void setEnabled(bool enabled);

    ::taihe::optional<double> getSpeed();
    void setSpeed(::taihe::optional_view<double> speed);

    double getDuration();
    bool getRunning();
    double getProgress();

    void onStarted(::taihe::callback_view<void(SceneResources::CallbackUndefinedType const&)> callback);
    void onFinishedInner(::taihe::callback_view<void()> callback);

    void pause();
    void restart();
    void seek(double position);
    void start();
    void stop();
    void finish();

    int64_t getAnimationImpl();
    std::shared_ptr<AnimationETS> getAnimationETS() const;

private:
    std::shared_ptr<AnimationETS> animationETS_{nullptr};
    taihe::callback<void(SceneResources::CallbackUndefinedType const&)> onStartedCB_{{nullptr, nullptr}};
    taihe::callback<void()> onFinishedCB_{{nullptr, nullptr}};
};
} // namespace OHOS::Render3D::KITETS
#endif // OHOS_3D_ANIMATION_IMPL_H