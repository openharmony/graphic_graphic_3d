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

#include "PostProcessETS.h"
#include "Utils.h"

namespace OHOS::Render3D {
PostProcessETS::PostProcessETS(const SCENE_NS::ICamera::Ptr camera, const SCENE_NS::IPostProcess::Ptr pp)
    : camera_(camera), postProc_(pp)
{}

PostProcessETS::PostProcessETS(const std::shared_ptr<TonemapETS> tonemap, const std::shared_ptr<BloomETS> bloom)
{
    tonemap_ = tonemap;
    bloom_ = bloom;
}

PostProcessETS::~PostProcessETS()
{
    tonemap_.reset();
    bloom_.reset();
    postProc_.reset();
    camera_.reset();
}

std::shared_ptr<TonemapETS> PostProcessETS::GetToneMapping()
{
    auto pp = postProc_.lock();
    if (!pp) {
        // The configuration data from ETS is stored in tonemap_.
        return tonemap_;
    }
    if (!tonemap_) {
        SCENE_NS::ITonemap::Ptr toneValue = META_NS::GetValue(pp->Tonemap());
        tonemap_ = std::make_shared<TonemapETS>(pp, toneValue);
    }

    if (tonemap_->IsEnabled()) {
        return tonemap_;
    } else {
        // tonemap disabled.
        return nullptr;
    }
}

void PostProcessETS::SetToneMapping(const std::shared_ptr<TonemapETS> tonemap)
{
    auto pp = postProc_.lock();
    if (!pp) {
        return;
    }

    if (!tonemap_) {
        SCENE_NS::ITonemap::Ptr toneValue = META_NS::GetValue(pp->Tonemap());
        tonemap_ = std::make_shared<TonemapETS>(pp, toneValue);
    }

    if (tonemap) {
        if (tonemap_->StrictEqual(tonemap)) {
            // setting the exactly the same tonemap setting. do nothing.
            return;
        }
        if (tonemap_->IsMatch(tonemap)) {
            ExecSyncTask([this, tonemap]() {
                tonemap_->SetEnabled(true);
                tonemap_->SetType(tonemap->GetType());
                tonemap_->SetExposure(tonemap->GetExposure());
                return META_NS::IAny::Ptr{};
            });
        } else {
            CORE_LOG_F("Invalid state. Can't change the post process of a tonemap if it already has one");
        }
    } else {
        tonemap_->SetEnabled(false);
    }
}

std::shared_ptr<BloomETS> PostProcessETS::GetBloom()
{
    auto pp = postProc_.lock();
    if (!pp) {
        // The configuration data from ETS is stored in bloom_.
        return bloom_;
    }
    if (!bloom_) {
        SCENE_NS::IBloom::Ptr bloomValue = META_NS::GetValue(pp->Bloom());
        bloom_ = std::make_shared<BloomETS>(pp, bloomValue);
    }

    if (bloom_->IsEnabled()) {
        return bloom_;
    } else {
        // bloom disabled.
        return nullptr;
    }
}

void PostProcessETS::SetBloom(const std::shared_ptr<BloomETS> bloom)
{
    auto pp = postProc_.lock();
    if (!pp) {
        return;
    }
    if (!bloom_) {
        SCENE_NS::IBloom::Ptr bloomValue = META_NS::GetValue(pp->Bloom());
        bloom_ = std::make_shared<BloomETS>(pp, bloomValue);
    }
    if (bloom) {
        if (bloom_->StrictEqual(bloom)) {
            // setting the exactly the same bloom setting. do nothing.
            return;
        }
        if (bloom_->IsMatch(bloom)) {
            ExecSyncTask([this, bloom]() {
                bloom_->SetEnabled(true);
                bloom_->SetType(bloom->GetType());
                bloom_->SetQuality(bloom->GetQuality());
                bloom_->SetThresholdHard(bloom->GetThresholdHard());
                bloom_->SetThresholdSoft(bloom->GetThresholdSoft());
                bloom_->SetScatter(bloom->GetScatter());
                bloom_->SetScaleFactor(bloom->GetScaleFactor());
                bloom_->SetAmountCoefficient(bloom->GetAmountCoefficient());
                return META_NS::IAny::Ptr{};
            });
        } else {
            CORE_LOG_F("Invalid state. Can't change the post process of a bloom if it already has one");
        }
    } else {
        bloom_->SetEnabled(false);
    }
}
}  // namespace OHOS::Render3D