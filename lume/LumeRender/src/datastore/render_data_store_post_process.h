/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_POST_PROCESS_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_POST_PROCESS_H

#include <atomic>
#include <cstdint>
#include <mutex>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>
#include <render/shaders/common/render_post_process_structs_common.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;

/**
 * RenderDataStorePostProcess implementation.
 */
class RenderDataStorePostProcess final : public IRenderDataStorePostProcess {
public:
    RenderDataStorePostProcess(const IRenderContext& renderContex, BASE_NS::string_view name);
    ~RenderDataStorePostProcess() override = default;

    // IRenderDataStore
    void PreRender() override {}
    void PostRender() override {}
    void PreRenderBackend() override {}
    void PostRenderBackend() override {}
    void Clear() override {}

    uint32_t GetFlags() const override
    {
        return 0;
    }

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStorePostProcess
    void Create(BASE_NS::string_view name) override;
    void Create(BASE_NS::string_view name, BASE_NS::string_view ppName, const RenderHandleReference& shader) override;

    void Destroy(BASE_NS::string_view name) override;
    void Destroy(BASE_NS::string_view name, BASE_NS::string_view ppName) override;

    bool Contains(BASE_NS::string_view name) const override;
    bool Contains(BASE_NS::string_view name, BASE_NS::string_view ppName) const override;

    void Set(BASE_NS::string_view name, BASE_NS::string_view ppName, const PostProcess::Variables& vars) override;
    void Set(BASE_NS::string_view name, BASE_NS::array_view<PostProcess::Variables> vars) override;

    GlobalFactors GetGlobalFactors(BASE_NS::string_view name) const override;
    BASE_NS::vector<PostProcess> Get(BASE_NS::string_view name) const override;
    PostProcess Get(BASE_NS::string_view name, BASE_NS::string_view ppName) const override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStorePostProcess";

    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(IRenderContext& renderContext, const char* name);

private:
    const IRenderContext& renderContext_;
    const BASE_NS::string name_;

    struct PostProcessStack {
        GlobalFactors globalFactors;
        BASE_NS::vector<PostProcess> postProcesses;
    };

    // must be locked when called
    void CreateFromPod(BASE_NS::string_view name);
    static void SetImpl(const PostProcess::Variables& vars, uint32_t ppIndex, PostProcessStack& ppStack);
    static void SetGlobalFactorsImpl(const PostProcess::Variables& vars, uint32_t ppIndex, PostProcessStack& ppStack);
    void GetShaderProperties(const RenderHandleReference& shader, PostProcess::Variables& vars);
    void FillDefaultPostProcessData(const PostProcessConfiguration& ppConfig, PostProcessStack& ppStack);

    BASE_NS::unordered_map<BASE_NS::string, PostProcessStack> allPostProcesses_;

    mutable std::mutex mutex_;

    std::atomic_int32_t refcnt_ { 0 };
};

/**
 * PostProcessConversion helper.
 */
class PostProcessConversionHelper final {
public:
    static inline BASE_NS::Math::Vec4 GetFactorTonemap(const PostProcessConfiguration& input)
    {
        return { input.tonemapConfiguration.exposure, 0.0f, 0.0f,
            static_cast<float>(input.tonemapConfiguration.tonemapType) };
    }

    static inline BASE_NS::Math::Vec4 GetFactorVignette(const PostProcessConfiguration& input)
    {
        return { input.vignetteConfiguration.coefficient, input.vignetteConfiguration.power, 0.0f, 0.0f };
    }

    static inline BASE_NS::Math::Vec4 GetFactorDither(const PostProcessConfiguration& input)
    {
        return { input.ditherConfiguration.amountCoefficient, 0, 0,
            static_cast<float>(input.ditherConfiguration.ditherType) };
    }

    static inline BASE_NS::Math::Vec4 GetFactorColorConversion(const PostProcessConfiguration& input)
    {
        return { 0.0f, 0.0f, 0.0f, static_cast<float>(input.colorConversionConfiguration.conversionFunctionType) };
    }

    static inline BASE_NS::Math::Vec4 GetFactorFringe(const PostProcessConfiguration& input)
    {
        return { input.colorFringeConfiguration.coefficient, input.colorFringeConfiguration.distanceCoefficient, 0.0f,
            0.0f };
    }

    static inline BASE_NS::Math::Vec4 GetFactorBlur(const PostProcessConfiguration& input)
    {
        return { static_cast<float>(input.blurConfiguration.blurType),
            static_cast<float>(input.blurConfiguration.blurQualityType), input.blurConfiguration.filterSize, 0 };
    }

    static inline BASE_NS::Math::Vec4 GetFactorBloom(const PostProcessConfiguration& input)
    {
        // the bloom global factor does not include scatter
        return { input.bloomConfiguration.thresholdHard, input.bloomConfiguration.thresholdSoft,
            input.bloomConfiguration.amountCoefficient, input.bloomConfiguration.dirtMaskCoefficient };
    }

    static inline BASE_NS::Math::Vec4 GetFactorFxaa(const PostProcessConfiguration& input)
    {
        return { static_cast<float>(input.fxaaConfiguration.sharpness),
            static_cast<float>(input.fxaaConfiguration.quality), 0.0f, 0.0f };
    }

    static inline BASE_NS::Math::Vec4 GetFactorTaa(const PostProcessConfiguration& input)
    {
        constexpr float alpha = 0.1f;

        uint32_t varianceClippingBicubic = 0;
        if (input.taaConfiguration.useBicubic) {
            varianceClippingBicubic |= (1 << TAA_USE_BICUBIC_BIT);
        }
        if (input.taaConfiguration.useVarianceClipping) {
            varianceClippingBicubic |= (1 << TAA_USE_VARIANCE_CLIPPING_BIT);
        }
        if (input.taaConfiguration.useyCoCG) {
            varianceClippingBicubic |= (1 << TAA_USE_YCOCG_BIT);
        }
        if (input.taaConfiguration.ignoreBicubicEdges) {
            varianceClippingBicubic |= (1 << TAA_IGNORE_EDGES_BIT);
        }

        const float* vcb = reinterpret_cast<float*>(&varianceClippingBicubic);

        return { static_cast<float>(input.taaConfiguration.sharpness),
            static_cast<float>(input.taaConfiguration.quality), *vcb, alpha };
    }

    static inline BASE_NS::Math::Vec4 GetFactorDof(const PostProcessConfiguration& input)
    {
        const float focusStart = (input.dofConfiguration.focusPoint - (input.dofConfiguration.focusRange / 2));
        const float focusEnd = (input.dofConfiguration.focusPoint + (input.dofConfiguration.focusRange / 2));
        const float nearTransitionStart = (focusStart - input.dofConfiguration.nearTransitionRange);
        const float farTransitionEnd = (focusEnd + input.dofConfiguration.farTransitionRange);

        return { nearTransitionStart, focusStart, focusEnd, farTransitionEnd };
    }

    static inline BASE_NS::Math::Vec4 GetFactorDof2(const PostProcessConfiguration& input)
    {
        return { input.dofConfiguration.nearBlur, input.dofConfiguration.farBlur, input.dofConfiguration.nearPlane,
            input.dofConfiguration.farPlane };
    }

    static inline BASE_NS::Math::Vec4 GetFactorMotionBlur(const PostProcessConfiguration& input)
    {
        return {
            static_cast<float>(input.motionBlurConfiguration.sharpness),
            static_cast<float>(input.motionBlurConfiguration.quality),
            input.motionBlurConfiguration.alpha,
            input.motionBlurConfiguration.velocityCoefficient,
        };
    }
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_POST_PROCESS_H
