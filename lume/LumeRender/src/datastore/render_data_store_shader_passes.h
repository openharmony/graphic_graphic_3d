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

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_FULLSCREEN_SHADER_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_FULLSCREEN_SHADER_H

#include <cstdint>
#include <mutex>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_shader_passes.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
/**
 * RenderDataStoreFullscreenShader implementation.
 */
class RenderDataStoreShaderPasses final : public IRenderDataStoreShaderPasses {
public:
    RenderDataStoreShaderPasses(const IRenderContext& renderContex, const BASE_NS::string_view name);
    ~RenderDataStoreShaderPasses() override;

    void AddRenderData(const BASE_NS::string_view name, const BASE_NS::array_view<const RenderPassData> data) override;
    void AddComputeData(
        const BASE_NS::string_view name, const BASE_NS::array_view<const ComputePassData> data) override;
    
    BASE_NS::vector<RenderPassData> GetRenderData(const BASE_NS::string_view name) const override;
    BASE_NS::vector<ComputePassData> GetComputeData(const BASE_NS::string_view name) const override;
    BASE_NS::vector<RenderPassData> GetRenderData() const override;
    BASE_NS::vector<ComputePassData> GetComputeData() const override;

    PropertyBindingDataInfo GetRenderPropertyBindingInfo(BASE_NS::string_view name) const override;
    PropertyBindingDataInfo GetComputePropertyBindingInfo(BASE_NS::string_view name) const override;
    PropertyBindingDataInfo GetRenderPropertyBindingInfo() const override;
    PropertyBindingDataInfo GetComputePropertyBindingInfo() const override;

    void CommitFrameData() override {}
    void PreRender() override {}
    void PostRender() override;
    void PreRenderBackend() override {}
    void PostRenderBackend() override {}
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreShaderPasses";

    static IRenderDataStore* Create(IRenderContext& renderContext, char const* name);
    static void Destroy(IRenderDataStore* instance);

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

private:
    const IRenderContext& renderContext_;
    BASE_NS::string name_;

    struct RenderPassDataInfo {
        BASE_NS::vector<RenderPassData> rpData;
        uint32_t alignedPropertyByteSize { 0U };
    };
    struct ComputePassDataInfo {
        BASE_NS::vector<ComputePassData> cpData;
        uint32_t alignedPropertyByteSize { 0U };
    };

    BASE_NS::unordered_map<BASE_NS::string, RenderPassDataInfo> nameToRenderObjects_;
    BASE_NS::unordered_map<BASE_NS::string, ComputePassDataInfo> nameToComputeObjects_;

    mutable std::mutex mutex_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_FULLSCREEN_SHADER_H
