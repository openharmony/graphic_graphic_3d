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

#ifndef OHOS_3D_TONEMAP_ETS_H
#define OHOS_3D_TONEMAP_ETS_H

#include <memory>

#include <scene/interface/postprocess/intf_tonemap.h>
#include <scene/interface/intf_postprocess.h>

namespace OHOS::Render3D {
class TonemapETS {
public:
    enum ToneMappingType {
        ACES = 0,
        ACES_2020 = 1,
        FILMIC = 2,
    };
    static constexpr ToneMappingType DEFAULT_TYPE = ToneMappingType::ACES;
    static constexpr float DEFAULT_EXPOSURE = 0.7;

    TonemapETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::ITonemap::Ptr tonemap);
    // store tonemap settings from ETS
    TonemapETS(const ToneMappingType &type, const float exposure);
    ~TonemapETS();

    TonemapETS::ToneMappingType GetType();
    void SetType(const ToneMappingType type);

    float GetExposure();
    void SetExposure(const float exposure);

    bool IsEnabled();
    void SetEnabled(const bool enable);

    bool StrictEqual(const std::shared_ptr<TonemapETS> other) const
    {
        if (!other) {
            return false;
        }
        return (postProc_.lock() == other->postProc_.lock() && tonemap_.lock() == other->tonemap_.lock());
    }

    bool IsMatch(const std::shared_ptr<TonemapETS> other) const
    {
        // one tonemap cannot be assigned to different post-processes.
        if (!other) {
            return true;
        }
        auto otherPostProcess = other->postProc_.lock();
        if (!otherPostProcess) {
            return true;
        }
        return postProc_.lock() == otherPostProcess;
    }

private:
    inline SCENE_NS::TonemapType ToInternalType(const TonemapETS::ToneMappingType &type);
    inline TonemapETS::ToneMappingType FromInternalType(const SCENE_NS::TonemapType &type);

    ToneMappingType type_;
    float exposure_;

    SCENE_NS::IPostProcess::WeakPtr postProc_;
    SCENE_NS::ITonemap::WeakPtr tonemap_;  // tonemap object from postproc_
};
}  // namespace OHOS::Render3D
#endif // OHOS_3D_TONEMAP_ETS_H