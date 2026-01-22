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

#include "render_data_store_default_scene.h"

#include <cstdint>

#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/log.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

RenderDataStoreDefaultScene::RenderDataStoreDefaultScene(const string_view name) : name_(name) {}

void RenderDataStoreDefaultScene::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultScene::Clear()
{
    scenes_.clear();
    nameToScene_.clear();
    nextId = 0;
}

void RenderDataStoreDefaultScene::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreDefaultScene::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreDefaultScene::GetRefCount()
{
    return refcnt_;
}

void RenderDataStoreDefaultScene::SetScene(const RenderScene& renderScene)
{
    const size_t arrIdx = scenes_.size();
    scenes_.push_back(renderScene);
    if (renderScene.name.empty()) {
        nameToScene_[to_string(nextId++)] = arrIdx;
    } else {
        nameToScene_[renderScene.name] = arrIdx;
    }
}

RenderScene RenderDataStoreDefaultScene::GetScene(const string_view name) const
{
    if (const auto iter = nameToScene_.find(name); iter != nameToScene_.cend()) {
        CORE_ASSERT(iter->second < scenes_.size());
        return scenes_[iter->second];
    } else {
        return {};
    }
}

RenderScene RenderDataStoreDefaultScene::GetScene() const
{
    if (!scenes_.empty()) {
        return scenes_[0];
    } else {
        return {};
    }
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreDefaultScene::Create(IRenderContext&, char const* name)
{
    // device not needed
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreDefaultScene(name));
}
CORE3D_END_NAMESPACE()
