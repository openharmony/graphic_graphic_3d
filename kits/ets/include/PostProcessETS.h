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

#ifndef OHOS_3D_POSTPROCESS_ETS_H
#define OHOS_3D_POSTPROCESS_ETS_H

#include <meta/api/util.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_camera.h>

#include "TonemapETS.h"
#include "BloomETS.h"

namespace OHOS::Render3D {
class PostProcessETS {
public:
    static std::shared_ptr<PostProcessETS> FromJS(
        const std::shared_ptr<TonemapETS> tonemap, const std::shared_ptr<BloomETS> bloom);
    PostProcessETS(const SCENE_NS::ICamera::Ptr camera, const SCENE_NS::IPostProcess::Ptr pp);
    ~PostProcessETS();

    std::shared_ptr<TonemapETS> GetToneMapping();
    void SetToneMapping(const std::shared_ptr<TonemapETS> tonemap);

    std::shared_ptr<BloomETS> GetBloom();
    void SetBloom(const std::shared_ptr<BloomETS> bloom);

    // SCENE_NS::ICamera::Ptr GetCamera();
    // void SetCamera(const SCENE_NS::ICamera::Ptr camera);

    bool StrictEqual(const std::shared_ptr<PostProcessETS> other) const
    {
        if (!other) {
            return false;
        }
        return (camera_.lock() == other->camera_.lock() && postProc_ == other->postProc_);
    }

    bool IsMatch(const std::shared_ptr<PostProcessETS> other) const
    {
        // one postprocess cannot be assigned to different camera.
        if (!other) {
            return true;
        }
        auto otherCamera = other->camera_.lock();
        if (!otherCamera) {
            return true;
        }
        return camera_.lock() == otherCamera;
    }

private:
    PostProcessETS();
    SCENE_NS::ICamera::WeakPtr camera_{nullptr};     // weak ref to owning camera.
    SCENE_NS::IPostProcess::Ptr postProc_{nullptr};  // keep a strong ref..

    std::shared_ptr<TonemapETS> tonemap_;
    std::shared_ptr<BloomETS> bloom_;
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_POSTPROCESS_ETS_H