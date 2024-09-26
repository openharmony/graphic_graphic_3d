/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef RENDER_DATA_STORE_DEFAULT_DOTFIELD_H
#define RENDER_DATA_STORE_DEFAULT_DOTFIELD_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>

#include "dotfield/render/intf_render_data_store_default_dotfield.h"

namespace RENDER_NS {
class IRenderContext;
class IGpuResourceManager;
class IShaderManager;
class IRenderDataStoreManager;

} // namespace RENDER_NS

namespace Dotfield {
/**
RenderDataStoreDefaultDotfield

 - Handles/Managers dotfield buffers and effects.
*/
class RenderDataStoreDefaultDotfield final : public IRenderDataStoreDefaultDotfield {
public:
    RenderDataStoreDefaultDotfield(RENDER_NS::IRenderContext& engine, const BASE_NS::string_view name);
    ~RenderDataStoreDefaultDotfield() override;

    void AddDotfieldPrimitive(const RenderDataDefaultDotfield::DotfieldPrimitive& dotfieldPrimitive) override;
    void RemoveDotfieldPrimitive(const CORE_NS::Entity& entity) override;

    BASE_NS::array_view<RenderDataDefaultDotfield::DotfieldPrimitive> GetDotfieldPrimitives() override;
    BASE_NS::array_view<const RenderDataDefaultDotfield::DotfieldPrimitive> GetDotfieldPrimitives() const override;
    const RenderDataDefaultDotfield::BufferData& GetBufferData() const override;

    float GetTimeStep() const noexcept override;
    void SetTimeStep(float time) noexcept override;

    float GetTime() const noexcept override;
    void SetTime(float time) noexcept override;

    void CommitFrameData() override {};
    void PreRender() override {};
    // Reset and start indexing from the beginning. i.e. frame boundary reset.
    void PostRender() override;
    void PreRenderBackend() override {};
    void PostRenderBackend() override {};
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    };

    // for plugin / factory interface
    static constexpr char const* TYPE_NAME = "RenderDataStoreDefaultDotfield";
    static RENDER_NS::IRenderDataStore* Create(RENDER_NS::IRenderContext& context, char const* name);
    static void Destroy(RENDER_NS::IRenderDataStore* instance);

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
    RENDER_NS::IRenderContext& context_;
    BASE_NS::string name_;
    RENDER_NS::IGpuResourceManager& gpuResourceMgr_;

    BASE_NS::vector<RenderDataDefaultDotfield::DotfieldPrimitive> primitives_;

    // store all dotfield gpu buffers here
    RenderDataDefaultDotfield::BufferData bufferData_;
    float timeStep_ { 1.f };
    float time_ { 1.f };
};
} // namespace Dotfield

#endif // RENDER_DATA_STORE_DEFAULT_DOTFIELD_H
