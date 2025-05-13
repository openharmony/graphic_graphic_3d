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

#ifndef API_RENDER_IRENDER_DATA_STORE_MANAGER_H
#define API_RENDER_IRENDER_DATA_STORE_MANAGER_H

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string_view.h>
#include <render/namespace.h>

BASE_BEGIN_NAMESPACE()
struct Uid;
BASE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IRenderDataStore;
/** @ingroup group_render_irenderdatastoremanager */
/** IRenderDataStoreManager interface.
 * Interface class to hold all render data stores.

 * Internally synchronized
 */
class IRenderDataStoreManager {
public:
    IRenderDataStoreManager(const IRenderDataStoreManager&) = delete;
    IRenderDataStoreManager& operator=(const IRenderDataStoreManager&) = delete;

    /** Get a previously created render data store
     * Uses lock, prefer fetching once if you're certain that the data store won't be destroyed
     * @param name Name of the render data store instance
     * @return Pointer to the instance or nullptr if instance doesn't exist.
     */
    virtual BASE_NS::refcnt_ptr<IRenderDataStore> GetRenderDataStore(const BASE_NS::string_view name) const = 0;

    /** Creates a new render data store.
     * @param dataStoreTypeUid Type UID of the render data store instance
     * @param dataStoreName Name of the render data store instance
     * @return Pointer to a new instance or nullptr if creation failed.
     */
    virtual BASE_NS::refcnt_ptr<IRenderDataStore> Create(
        const BASE_NS::Uid& dataStoreTypeUid, const char* dataStoreName) = 0;

    /** Render data store flag bits, which data store implemented should check.
     */
    enum RenderDataStoreFlagBits : uint32_t {};
    /** Container for render data store flag bits */
    using RenderDataStoreFlags = uint32_t;

    /** Get render data store flags. Render data store implementer should evaluate these.
     * @return RenderDataStoreFlags.
     */
    virtual RenderDataStoreFlags GetRenderDataStoreFlags() const = 0;

protected:
    IRenderDataStoreManager() = default;
    virtual ~IRenderDataStoreManager() = default;
};

/** IRenderNodeRenderDataStoreManager interface.
 * Interface class to access all render data stores.

 * Safe usage without locking in render nodes.
 */
class IRenderNodeRenderDataStoreManager {
public:
    IRenderNodeRenderDataStoreManager(const IRenderNodeRenderDataStoreManager&) = delete;
    IRenderNodeRenderDataStoreManager& operator=(const IRenderNodeRenderDataStoreManager&) = delete;

    /** Get a previously created render data store
     * @param name Name of the render data store instance
     * @return Pointer to the instance or nullptr if instance doesn't exist.
     */
    virtual IRenderDataStore* GetRenderDataStore(const BASE_NS::string_view name) const = 0;

protected:
    IRenderNodeRenderDataStoreManager() = default;
    virtual ~IRenderNodeRenderDataStoreManager() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_MANAGER_H
