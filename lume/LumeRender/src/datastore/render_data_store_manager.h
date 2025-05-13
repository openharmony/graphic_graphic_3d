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

#ifndef RENDER_DATA_STORE_RENDER_DATA_STORE_MANAGER_H
#define RENDER_DATA_STORE_RENDER_DATA_STORE_MANAGER_H

#include <mutex>

#include <base/containers/flat_map.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/intf_plugin.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;

/**
RenderDataManager class.
Implementation for render data manager class.
*/
class RenderDataStoreManager final : public IRenderDataStoreManager {
public:
    explicit RenderDataStoreManager(IRenderContext& renderContext);
    ~RenderDataStoreManager() override = default;

    RenderDataStoreManager(const RenderDataStoreManager&) = delete;
    RenderDataStoreManager& operator=(const RenderDataStoreManager&) = delete;

    void CommitFrameData();
    void PreRender();
    void PostRender();
    void PreRenderBackend();
    void PostRenderBackend();

    // uses locking mechanism
    BASE_NS::refcnt_ptr<IRenderDataStore> GetRenderDataStore(BASE_NS::string_view name) const override;
    // Does not use lock. Call only from render nodes.
    // This is only safe during PreRender() PostRender() scope.
    IRenderDataStore* GetRenderTimeRenderDataStore(BASE_NS::string_view name) const;

    BASE_NS::refcnt_ptr<IRenderDataStore> Create(
        const BASE_NS::Uid& dataStoreTypeUid, const char* dataStoreName) override;

    RenderDataStoreFlags GetRenderDataStoreFlags() const override;

    // Not synchronized and not in API
    void AddRenderDataStoreFactory(const RenderDataStoreTypeInfo& typeInfo);
    // Not synchronized and not in API
    void RemoveRenderDataStoreFactory(const RenderDataStoreTypeInfo& typeInfo);

private:
    IRenderContext& renderContext_;

    struct PendingRenderAccessStore {
        uint64_t hash { 0u };
        BASE_NS::refcnt_ptr<IRenderDataStore> renderDataStore { nullptr };
    };

    mutable std::mutex mutex_;

    // the following needs be locked with mutex
    BASE_NS::unordered_map<uint64_t, BASE_NS::refcnt_ptr<IRenderDataStore>> stores_;
    BASE_NS::unordered_map<IRenderDataStore*, uint64_t> pointerToStoreHash_;
    BASE_NS::vector<PendingRenderAccessStore> pendingRenderAccess_;

    // lock not needed for access
    BASE_NS::flat_map<uint64_t, BASE_NS::refcnt_ptr<IRenderDataStore>> renderAccessStores_;
    BASE_NS::unordered_map<uint64_t, RenderDataStoreTypeInfo> factories_;

    RenderDataStoreFlags renderDataStoreFlags_ { 0u };
};

class RenderNodeRenderDataStoreManager final : public IRenderNodeRenderDataStoreManager {
public:
    explicit RenderNodeRenderDataStoreManager(const RenderDataStoreManager& renderDataStoreMgr);
    ~RenderNodeRenderDataStoreManager() override = default;

    RenderNodeRenderDataStoreManager(const RenderNodeRenderDataStoreManager&) = delete;
    RenderNodeRenderDataStoreManager& operator=(const RenderNodeRenderDataStoreManager&) = delete;

    // calls the unsafe method from render data store
    IRenderDataStore* GetRenderDataStore(BASE_NS::string_view name) const override;

private:
    const RenderDataStoreManager& renderDataStoreMgr_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATA_STORE_RENDER_DATA_STORE_MANAGER_H
