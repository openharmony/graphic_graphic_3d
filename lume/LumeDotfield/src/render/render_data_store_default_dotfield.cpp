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

#include "render_data_store_default_dotfield.h"

#include <algorithm>
#include <cstdint>
#include <vector>

#include <base/containers/array_view.h>
#include <core/intf_engine.h>
#include <core/namespace.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/resource_handle.h>

#include "app/shaders/common/dotfield_common.h"
#include "app/shaders/common/dotfield_struct_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace Dotfield {
RenderDataStoreDefaultDotfield::RenderDataStoreDefaultDotfield(IRenderContext& context, const string_view name)
    : context_(context), name_(name), gpuResourceMgr_(context_.GetDevice().GetGpuResourceManager())
{}

RenderDataStoreDefaultDotfield::~RenderDataStoreDefaultDotfield() = default;

void RenderDataStoreDefaultDotfield::PostRender()
{
    Clear();
    bufferData_.currFrameIndex = 1u - bufferData_.currFrameIndex;
}

void RenderDataStoreDefaultDotfield::Clear() {}

void RenderDataStoreDefaultDotfield::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreDefaultDotfield::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreDefaultDotfield::GetRefCount()
{
    return refcnt_;
}

void RenderDataStoreDefaultDotfield::AddDotfieldPrimitive(const RenderDataDefaultDotfield::DotfieldPrimitive& primitive)
{
    primitives_.emplace_back(primitive);

    const uint32_t dotfieldCount = primitive.size.x * primitive.size.y;

    // data buffers
    const GpuBufferDesc desc {
        BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            BufferUsageFlagBits::CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS,
        static_cast<uint32_t>(sizeof(float) * 2 * dotfieldCount),
    };
    RenderDataDefaultDotfield::BufferData::Buffer buffer;
    buffer.dataBuffer[0] = gpuResourceMgr_.Create(desc);
    buffer.dataBuffer[1] = gpuResourceMgr_.Create(desc);
    bufferData_.buffers.emplace_back(buffer);
}

array_view<RenderDataDefaultDotfield::DotfieldPrimitive> RenderDataStoreDefaultDotfield::GetDotfieldPrimitives()
{
    return { primitives_.data(), primitives_.size() };
}

array_view<const RenderDataDefaultDotfield::DotfieldPrimitive>
RenderDataStoreDefaultDotfield::GetDotfieldPrimitives() const
{
    return { primitives_.data(), primitives_.size() };
}

void RenderDataStoreDefaultDotfield::RemoveDotfieldPrimitive(const Entity& entity)
{
    for (auto it = primitives_.cbegin(); it != primitives_.cend(); ++it) {
        if (it->entity == entity) {
            const auto index = it - primitives_.cbegin();
            primitives_.erase(it);
            bufferData_.buffers.erase(bufferData_.buffers.cbegin() + index);
            break;
        }
    }
}

const RenderDataDefaultDotfield::BufferData& RenderDataStoreDefaultDotfield::GetBufferData() const
{
    return bufferData_;
}

float RenderDataStoreDefaultDotfield::GetTimeStep() const noexcept
{
    return timeStep_;
}

void RenderDataStoreDefaultDotfield::SetTimeStep(float time) noexcept
{
    timeStep_ = time;
}

float RenderDataStoreDefaultDotfield::GetTime() const noexcept
{
    return time_;
}

void RenderDataStoreDefaultDotfield::SetTime(float time) noexcept
{
    time_ = time;
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreDefaultDotfield::Create(IRenderContext& context, char const* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreDefaultDotfield(context, name));
}
} // namespace Dotfield
