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

#include "render_data_store_shader_passes.h"

#include <cinttypes>
#include <cstdint>

#include <base/containers/fixed_string.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };

constexpr uint32_t Align(uint32_t value, uint32_t align)
{
    if (value == 0) {
        return 0;
    }
    return ((value + align) / align) * align;
}
}// namespace

RenderDataStoreShaderPasses::RenderDataStoreShaderPasses(const IRenderContext& renderContext, const string_view name)
    : renderContext_(renderContext), name_(name)
{}

RenderDataStoreShaderPasses::~RenderDataStoreShaderPasses() {}

void RenderDataStoreShaderPasses::PostRender()
{
    Clear();
}

void RenderDataStoreShaderPasses::Clear()
{
    const auto lock = std::lock_guard(mutex_);

    nameToRenderObjects_.clear();
    nameToComputeObjects_.clear();
}

void RenderDataStoreShaderPasses::AddRenderData(
    const BASE_NS::string_view name, const BASE_NS::array_view<const RenderPassData> data)
{
    if ((!name.empty()) && (!data.empty())) {
        const auto lock = std::lock_guard(mutex_);

        auto& ref = nameToRenderObjects_[name];
        ref.rpData.append(data.begin(), data.end());
        for (const auto& rpRef : ref.rpData) {
            for (const auto& shaderRef : rpRef.shaderBinders) {
                if (shaderRef) {
                    ref.alignedPropertyByteSize = Align(shaderRef->GetPropertyBindingByteSize(), OFFSET_ALIGNMENT);
                }
            }
        }
    }
}

void RenderDataStoreShaderPasses::AddComputeData(
    const BASE_NS::string_view name, const BASE_NS::array_view<const ComputePassData> data)
{
    if ((!name.empty()) && (!data.empty())) {
        const auto lock = std::lock_guard(mutex_);

        auto& ref = nameToComputeObjects_[name];
        ref.cpData.append(data.begin(), data.end());
        for (const auto& rpRef : ref.cpData) {
            for (const auto& shaderRef : rpRef.shaderBinders) {
                if (shaderRef) {
                    ref.alignedPropertyByteSize = Align(shaderRef->GetPropertyBindingByteSize(), OFFSET_ALIGNMENT);
                }
            }
        }
    }
}

vector<IRenderDataStoreShaderPasses::RenderPassData> RenderDataStoreShaderPasses::GetRenderData(
    const BASE_NS::string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToRenderObjects_.find(name); iter != nameToRenderObjects_.cend()) {
        return iter->second.rpData;
    }
    return {};
}

vector<IRenderDataStoreShaderPasses::ComputePassData> RenderDataStoreShaderPasses::GetComputeData(
    const BASE_NS::string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToComputeObjects_.find(name); iter != nameToComputeObjects_.cend()) {
        return iter->second.cpData;
    }
    return {};
}

vector<IRenderDataStoreShaderPasses::RenderPassData> RenderDataStoreShaderPasses::GetRenderData() const
{
    const auto lock = std::lock_guard(mutex_);

    BASE_NS::vector<RenderPassData> data;
    data.reserve(nameToRenderObjects_.size());
    for (const auto& ref : nameToRenderObjects_) {
        data.append(ref.second.rpData.begin(), ref.second.rpData.end());
    }
    return data;
}

vector<IRenderDataStoreShaderPasses::ComputePassData> RenderDataStoreShaderPasses::GetComputeData() const
{
    const auto lock = std::lock_guard(mutex_);

    BASE_NS::vector<ComputePassData> data;
    data.reserve(nameToComputeObjects_.size());
    for (const auto& ref : nameToComputeObjects_) {
        data.append(ref.second.cpData.begin(), ref.second.cpData.end());
    }
    return data;
}

RenderDataStoreShaderPasses::PropertyBindingDataInfo RenderDataStoreShaderPasses::GetRenderPropertyBindingInfo(
    BASE_NS::string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToRenderObjects_.find(name); iter != nameToRenderObjects_.cend()) {
        return { iter->second.alignedPropertyByteSize };
    }
    return {};
}

RenderDataStoreShaderPasses::PropertyBindingDataInfo RenderDataStoreShaderPasses::GetComputePropertyBindingInfo(
    BASE_NS::string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToComputeObjects_.find(name); iter != nameToComputeObjects_.cend()) {
        return { iter->second.alignedPropertyByteSize };
    }
    return {};
}

RenderDataStoreShaderPasses::PropertyBindingDataInfo RenderDataStoreShaderPasses::GetRenderPropertyBindingInfo() const
{
    const auto lock = std::lock_guard(mutex_);

    PropertyBindingDataInfo info;
    for (const auto& ref : nameToRenderObjects_) {
        info.alignedByteSize = ref.second.alignedPropertyByteSize;
    }
    return info;
}

RenderDataStoreShaderPasses::PropertyBindingDataInfo RenderDataStoreShaderPasses::GetComputePropertyBindingInfo() const
{
    const auto lock = std::lock_guard(mutex_);

    PropertyBindingDataInfo info;
    for (const auto& ref : nameToComputeObjects_) {
        info.alignedByteSize = ref.second.alignedPropertyByteSize;
    }
    return info;
}

// for plugin / factory interface
IRenderDataStore* RenderDataStoreShaderPasses::Create(IRenderContext& renderContext, char const* name)
{
    // engine not used
    return new RenderDataStoreShaderPasses(renderContext, name);
}

void RenderDataStoreShaderPasses::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreShaderPasses*>(instance);
}
RENDER_END_NAMESPACE()
