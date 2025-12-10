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

#include "render_data_store_manager.h"

#include <cstddef>

#include <base/util/uid_util.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreManager::RenderDataStoreManager(IRenderContext& renderContext) : renderContext_(renderContext) {}

void RenderDataStoreManager::CommitFrameData()
{
    decltype(pendingRenderAccess_) pendingRenderAccess;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingRenderAccess = std::move(pendingRenderAccess_);
    }

    // prepare access for render time access
    for (auto& pendingRef : pendingRenderAccess) {
        renderAccessStores_.insert_or_assign(pendingRef.hash, BASE_NS::move(pendingRef.renderDataStore));
    }
    pendingRenderAccess.clear();

    // all valid stores can be accessed from render access stores without locks
    // remove unused data stores and gather their hashes so that stores_ can be cleaned up as well.
    for (auto it = renderAccessStores_.begin(); it != renderAccessStores_.end();) {
        if (it->second->GetRefCount() > 2) { // in stores_, renderAccessStores_ and user, 2: min ref count
            ++it;
        } else {
            pendingRenderAccess.push_back({ it->first, BASE_NS::move(it->second) });
            it = renderAccessStores_.erase(it);
        }
    }
    if (!pendingRenderAccess.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& ref : pendingRenderAccess) {
            pointerToStoreHash_.erase(ref.renderDataStore.get());
            stores_.erase(ref.hash);
        }
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
}

BASE_NS::refcnt_ptr<IRenderDataStore> RenderDataStoreManager::GetRenderDataStore(const string_view name) const
{
    if (name.empty()) {
        return {};
    }

    auto const nameHash = hash(name);

    std::lock_guard<std::mutex> lock(mutex_);

    if (const auto iter = stores_.find(nameHash); iter != stores_.cend()) {
        return iter->second;
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(
            name + "_RDS_NOT_FOUND__", "RENDER_VALIDATION: render data store: (%s), not found", name.data());
#endif
        return {};
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
        return iter->second.get();
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(
            name + "_RDS_NOT_FOUND", "RENDER_VALIDATION: render data store: (%s), not found", name.data());
#endif
        return nullptr;
    }
}

BASE_NS::refcnt_ptr<IRenderDataStore> RenderDataStoreManager::Create(
    const Uid& dataStoreTypeUid, const char* dataStoreName)
{
    auto const dataStoreTypeHash = hash(dataStoreTypeUid);
    if (auto const factoryIt = factories_.find(dataStoreTypeHash); factoryIt != factories_.end()) {
        auto const dataStoreNameHash = hash(string_view(dataStoreName));
        refcnt_ptr<IRenderDataStore> dataStore;

        std::lock_guard<std::mutex> lock(mutex_);

        if (auto const namedStoreIt = stores_.find(dataStoreNameHash); namedStoreIt != stores_.cend()) {
            PLUGIN_LOG_D("Named data store already exists (type: %s) (name: %s)", to_string(dataStoreTypeUid).data(),
                dataStoreName);
            dataStore = namedStoreIt->second;
            if (dataStore->GetUid() != dataStoreTypeUid) {
                PLUGIN_LOG_E("Named data store (type: %s, name: %s) exists with different type (%s)",
                    to_string(dataStoreTypeUid).data(), dataStoreName, dataStore->GetTypeName().data());
                dataStore = nullptr;
            }
        } else {
            auto const dataStoreIt = stores_.insert_or_assign(dataStoreNameHash,
                refcnt_ptr<IRenderDataStore>(factoryIt->second.createDataStore(renderContext_, dataStoreName)));
            pointerToStoreHash_.insert_or_assign(dataStoreIt.first->second.get(), uint64_t { dataStoreNameHash });
            dataStore = dataStoreIt.first->second;
        }

        pendingRenderAccess_.push_back({ dataStoreNameHash, dataStore });
        return dataStore;
    } else {
        PLUGIN_LOG_E("render data store type not found (type: %s) (named: %s)", to_string(dataStoreTypeUid).data(),
            dataStoreName);
    }
    return nullptr;
}

IRenderDataStoreManager::RenderDataStoreFlags RenderDataStoreManager::GetRenderDataStoreFlags() const
{
    return renderDataStoreFlags_;
}

void RenderDataStoreManager::AddRenderDataStoreFactory(const RenderDataStoreTypeInfo& typeInfo)
{
    // plugin load and factory addition is sequential
    // not locked access

    if (typeInfo.createDataStore) {
        auto const dataStoreTypeHash = hash(typeInfo.uid);
        factories_.insert({ dataStoreTypeHash, typeInfo });
    } else {
        PLUGIN_LOG_E("RenderDataStoreTypeInfo must provide non-null function pointers");
        PLUGIN_ASSERT(typeInfo.createDataStore && "createDataStore cannot be null");
    }
}

void RenderDataStoreManager::RemoveRenderDataStoreFactory(const RenderDataStoreTypeInfo& typeInfo)
{
    // stores_ and pointerToStoreHash_ are modified under lock. renderAccessStores_ is assumed to be touched only from
    // RenderFrame, so this doesn't help. This implies that plugins should be unloaded only while not rendering.
    std::lock_guard lock(mutex_);

    for (auto b = pointerToStoreHash_.begin(), e = pointerToStoreHash_.end(); b != e;) {
        if (b->first->GetUid() == typeInfo.uid) {
            stores_.erase(b->second);
            renderAccessStores_.erase(b->second);
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
