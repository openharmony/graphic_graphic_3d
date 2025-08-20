/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_BLOOM_ETS_H
#define OHOS_3D_BLOOM_ETS_H

#include <memory>

#include <scene/interface/intf_scene.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/intf_postprocess.h>

namespace OHOS::Render3D {
class BloomETS {
public:
    static std::shared_ptr<BloomETS> FromJS(
        const float thresholdHard, const float thresholdSoft, const float scaleFactor, const float scatter);

    enum class Quality : uint32_t { LOW = 1, NORMAL = 2, HIGH = 3 };
    enum class Type : uint32_t { NORMAL = 1, HORIZONTAL = 2, VERTICAL = 3, BILATERAL = 4 };

    BloomETS(const SCENE_NS::IPostProcess::Ptr postProc, const SCENE_NS::IBloom::Ptr bloom);
    ~BloomETS();

    float GetAmountCoefficient();
    void SetAmountCoefficient(float amount);

    float GetThresholdSoft();
    void SetThresholdSoft(float thresholdSoft);

    float GetThresholdHard();
    void SetThresholdHard(float thresholdHard);

    float GetScatter();
    void SetScatter(float scatter);

    float GetScaleFactor();
    void SetScaleFactor(float scaleFactor);

    BloomETS::Type GetType();
    void SetType(const BloomETS::Type &type);

    BloomETS::Quality GetQuality();
    void SetQuality(const BloomETS::Quality &quality);

    bool IsEnabled();
    void SetEnabled(const bool enable);

    bool StrictEqual(const std::shared_ptr<BloomETS> other) const
    {
        if (!other) {
            return false;
        }
        return (postProc_.lock() == other->postProc_.lock() && bloom_ == other->bloom_);
    }

    bool IsMatch(const std::shared_ptr<BloomETS> other) const
    {
        // one bloom cannot be assigned to different post-processes.
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
    BloomETS();
    inline SCENE_NS::BloomType ToInternalType(const BloomETS::Type &type);
    inline BloomETS::Type FromInternalType(const SCENE_NS::BloomType &type);

    inline SCENE_NS::EffectQualityType ToInternalQuality(const BloomETS::Quality &quality);
    inline BloomETS::Quality FromInternalQuality(const SCENE_NS::EffectQualityType &quality);

    float thresholdHard_{1.0f};
    float thresholdSoft_{2.0f};
    float amountCoefficient_{0.25f};
    float scatter_{1.0f};
    float scaleFactor_{1.0f};
    BloomETS::Type type_{BloomETS::Type::NORMAL};
    BloomETS::Quality quality_{BloomETS::Quality::NORMAL};

    SCENE_NS::IPostProcess::WeakPtr postProc_;
    SCENE_NS::IBloom::Ptr bloom_;  // bloom object from postproc_
};
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_BLOOM_ETS_H