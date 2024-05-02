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

#include "render_data_store_manager.h"

#include <cstddef>

#include <base/util/uid_util.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/device/intf_device.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreManager::RenderDataStoreManager(IRenderContext& renderContext) : renderContext_(renderContext) {}

void RenderDataStoreManager::CommitFrameData()
{
    // only modify the write index when double buffered in use
    if (renderDataStoreFlags_ & DOUBLE_BUFFERED_RENDER_DATA_STORES) {
        frameWriteIndex_ = 1u - frameWriteIndex_;
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    commitDeviceFrameIndex_ = renderContext_.GetDevice().GetFrameCount();
#endif

    decltype(pendingRenderAccess_) pendingRenderAccess;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingRenderAccess = std::move(pendingRenderAccess_);
    }

    // prepare access for render time access
    for (auto& pendingRef : pendingRenderAccess) {
        if (pendingRef.destroy) { // remove deferred destruction stores
            renderAccessStores_.erase(pendingRef.hash);
        } else {
            PLUGIN_ASSERT(pendingRef.renderDataStore);
            renderAccessStores_.insert_or_assign(pendingRef.hash, pendingRef.renderDataStore);
        }
    }

    // all valid stores can be accessed from render access stores without locks
    for (const auto& ref : renderAccessStores_) {
        ref.second->CommitFrameData();
    }
}

void RenderDataStoreManager::PreRender()
{
    // all valid stores can be accessed from render access stores without locks
    for (const auto& ref : renderAccessStores_) {
        ref.second->PreRender();
    }
}

void RenderDataStoreManager::PostRender()
{
    // all valid stores can be accessed from render access stores without locks
    for (const auto& ref : renderAccessStores_) {
        ref.second->PostRender();
    }
}

void RenderDataStoreManager::PreRenderBackend()
{
    // all valid stores can be accessed from render access stores without locks
    for (const auto& ref : renderAccessStores_) {
        ref.second->PreRenderBackend();
    }
}

void RenderDataStoreManager::PostRenderBackend()
{
    // all valid stores can be accessed from render access stores without locks
    for (const auto& ref : renderAccessStores_) {
        ref.second->PostRenderBackend();
    }

    DeferredDestruction();
}

IRenderDataStore* RenderDataStoreManager::GetRenderDataStore(const string_view name) const
{
    if (name.empty()) {
        return nullptr;
    }

    auto const nameHash = hash(name);

    std::lock_guard<std::mutex> lock(mutex_);

    if (const auto iter = stores_.find(nameHash); iter != stores_.cend()) {
        return iter->second.get();
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(
            name + "_RDS_NOT_FOUND__", "RENDER_VALIDATION: render data store: (%s), not found", name.data());
#endif
        return nullptr;
    }
}

IRenderDataStore* RenderDataStoreManager::GetRenderTimeRenderDataStore(const string_view name) const
{
    if (name.empty()) {
        return nullptr;
    }

    // not locked

    auto const nameHash = hash(name);
    if (const auto iter = renderAccessStores_.find(nameHash); iter != renderAccessStores_.cend()) {
        return iter->second;
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(
            name + "_RDS_NOT_FOUND", "RENDER_VALIDATION: render data store: (%s), not found", name.data());
#endif
        return nullptr;
    }
}

IRenderDataStore* RenderDataStoreManager::Create(const Uid& dataStoreTypeUid, char const* dataStoreName)
{
    auto const dataStoreTypeHash = hash(dataStoreTypeUid);
    if (auto const factoryIt = factories_.find(dataStoreTypeHash); factoryIt != factories_.end()) {
        auto const dataStoreNameHash = hash(string_view(dataStoreName));
        IRenderDataStore* dataStore = nullptr;

        std::lock_guard<std::mutex> lock(mutex_);

        if (auto const namedStoreIt = stores_.find(dataStoreNameHash); namedStoreIt != stores_.cend()) {
            PLUGIN_LOG_D("Named data store already exists (type: %s) (name: %s)", to_string(dataStoreTypeUid).data(),
                dataStoreName);
            dataStore = namedStoreIt->second.get();
            if (dataStore->GetUid() != dataStoreTypeUid) {
                PLUGIN_LOG_E("Named data store (type: %s, name: %s) exists with different type (%s)",
                    to_string(dataStoreTypeUid).data(), dataStoreName, dataStore->GetTypeName().data());
                dataStore = nullptr;
            }
        } else {
            auto const dataStoreIt = stores_.insert_or_assign(dataStoreNameHash,
                RenderDataStorePtr { factoryIt->second.createDataStore(renderContext_, dataStoreName),
                    factoryIt->second.destroyDataStore });
            pointerToStoreHash_.insert_or_assign(dataStoreIt.first->second.get(), uint64_t { dataStoreNameHash });
            dataStore = dataStoreIt.first->second.get();
        }

        pendingRenderAccess_.push_back({ dataStoreNameHash, dataStore, false });
        return dataStore;
    } else {
        PLUGIN_LOG_E("render data store type not found (type: %s) (named: %s)", to_string(dataStoreTypeUid).data(),
            dataStoreName);
        PLUGIN_ASSERT(false);
    }
    return nullptr;
}

void RenderDataStoreManager::Destroy(const Uid& dataStoreTypeUid, IRenderDataStore* instance)
{
    if (instance) {
        const uint64_t typeHash = hash(dataStoreTypeUid);

        std::lock_guard<std::mutex> lock(mutex_);

        deferredDestructionDataStores_.push_back({ typeHash, instance });

        PLUGIN_ASSERT(pointerToStoreHash_.contains(instance));
        if (auto const storeIt = pointerToStoreHash_.find(instance); storeIt != pointerToStoreHash_.cend()) {
            pendingRenderAccess_.push_back({ storeIt->second, instance, true });
        }
    }
}

IRenderDataStoreManager::RenderDataStoreFlags RenderDataStoreManager::GetRenderDataStoreFlags() const
{
    return renderDataStoreFlags_;
}

#if (RENDER_VALIDATION_ENABLED == 1)
void RenderDataStoreManager::ValidateCommitFrameData() const
{
    if (renderDataStoreFlags_ & DOUBLE_BUFFERED_RENDER_DATA_STORES) {
        if (commitDeviceFrameIndex_ != renderContext_.GetDevice().GetFrameCount()) {
            PLUGIN_LOG_E("Render data store manager CommitFrameData() needs to be called before rendering when using "
                         "double buffered render data stores.");
        }
    }
}
#endif

IRenderDataStoreManager::FrameIndices RenderDataStoreManager::GetFrameIndices() const
{
    if (renderDataStoreFlags_ & DOUBLE_BUFFERED_RENDER_DATA_STORES) {
        return { frameWriteIndex_, 1u - frameWriteIndex_ };
    } else {
        return { frameWriteIndex_, frameWriteIndex_ };
    }
}

void RenderDataStoreManager::DeferredDestruction()
{
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& destroyRef : deferredDestructionDataStores_) {
        if (auto const pos = factories_.find(destroyRef.dataStoreHash); pos != factories_.end()) {
            if (auto const storeIt = pointerToStoreHash_.find(destroyRef.instance);
                storeIt != pointerToStoreHash_.cend()) {
                stores_.erase(storeIt->second);
                pointerToStoreHash_.erase(storeIt);
            }
        }
    }
    deferredDestructionDataStores_.clear();
}

void RenderDataStoreManager::AddRenderDataStoreFactory(const RenderDataStoreTypeInfo& typeInfo)
{
    // plugin load and factory addition is sequential
    // not locked access

    if (typeInfo.createDataStore && typeInfo.destroyDataStore) {
        auto const dataStoreTypeHash = hash(typeInfo.uid);
        factories_.insert({ dataStoreTypeHash, typeInfo });
    } else {
        PLUGIN_LOG_E("RenderDataStoreTypeInfo must provide non-null function pointers");
        PLUGIN_ASSERT(typeInfo.createDataStore && "createDataStore cannot be null");
        PLUGIN_ASSERT(typeInfo.destroyDataStore && "destroyDataStore cannot be null");
    }
}

void RenderDataStoreManager::RemoveRenderDataStoreFactory(const RenderDataStoreTypeInfo& typeInfo)
{
    for (auto b = pointerToStoreHash_.begin(), e = pointerToStoreHash_.end(); b != e;) {
        if (b->first->GetUid() == typeInfo.uid) {
            stores_.erase(b->second);
            b = pointerToStoreHash_.erase(b);
        } else {
            ++b;
        }
    }
    auto const dataStoreTypeHash = hash(typeInfo.uid);
    factories_.erase(dataStoreTypeHash);
}

RenderNodeRenderDataStoreManager::RenderNodeRenderDataStoreManager(const RenderDataStoreManager& renderDataStoreMgr)
    : renderDataStoreMgr_(renderDataStoreMgr)
{}

IRenderDataStore* RenderNodeRenderDataStoreManager::GetRenderDataStore(const string_view name) const
{
    return renderDataStoreMgr_.GetRenderTimeRenderDataStore(name);
}
RENDER_END_NAMESPACE()
