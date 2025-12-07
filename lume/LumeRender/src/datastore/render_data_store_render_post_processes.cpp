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

#include "render_data_store_render_post_processes.h"

#include <cstdint>

#include <base/containers/fixed_string.h>
#include <render/intf_render_context.h>

#include "datastore/render_data_store_manager.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreRenderPostProcesses::RenderDataStoreRenderPostProcesses(
    const IRenderContext& /*renderContext*/, const string_view name)
    : name_(name)
{}

RenderDataStoreRenderPostProcesses::~RenderDataStoreRenderPostProcesses() = default;

void RenderDataStoreRenderPostProcesses::PostRender()
{
    Clear();
}

void RenderDataStoreRenderPostProcesses::Clear()
{
    const auto lock = std::lock_guard(mutex_);

    nameToPipeline_.clear();
    pipelines_.clear();
}

void RenderDataStoreRenderPostProcesses::AddData(
    const string_view name, array_view<const IRenderDataStoreRenderPostProcesses::PostProcessData> data)
{
    if ((!name.empty()) && (!data.empty())) {
        const auto lock = std::lock_guard(mutex_);

        size_t index = 0;
        if (auto iter = nameToPipeline_.find(name); iter != nameToPipeline_.end()) {
            index = static_cast<size_t>(iter->second);
        } else {
            index = pipelines_.size();
            nameToPipeline_.insert_or_assign(name, uint32_t(index));
            pipelines_.push_back({});
        }
        // clear and add
        if (index < pipelines_.size()) {
            auto& pp = pipelines_[index].postProcesses;
            pp.clear();
            pp.append(data.begin(), data.end());
        }
    }
}

IRenderDataStoreRenderPostProcesses::PostProcessPipeline RenderDataStoreRenderPostProcesses::GetData(
    const string_view name) const
{
    const auto lock = std::lock_guard(mutex_);

    if (const auto iter = nameToPipeline_.find(name); iter != nameToPipeline_.cend()) {
        if (iter->second < pipelines_.size()) {
            return pipelines_[iter->second];
        }
    }
    return {};
}

void RenderDataStoreRenderPostProcesses::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreRenderPostProcesses::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreRenderPostProcesses::GetRefCount()
{
    return refcnt_;
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreRenderPostProcesses::Create(IRenderContext& renderContext, const char* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreRenderPostProcesses(renderContext, name));
}
RENDER_END_NAMESPACE()
