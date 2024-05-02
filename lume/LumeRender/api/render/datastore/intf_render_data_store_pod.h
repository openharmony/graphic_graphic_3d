/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_IRENDER_DATA_STORE_POD_H
#define API_RENDER_IRENDER_DATA_STORE_POD_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastorepod */
/** RenderDataStorePod interface.
 * Not internally synchronized.
 *
 * This data store is not reset.
 * Allocated POD memory is released when the store is destroyed.
 *
 * Usage:
 * CreatePodStore() with a unique name and size.
 * Set() update named data block.
 * Get() get array_view to named data block.
 *
 * Internally synchronized.
 */
class IRenderDataStorePod : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "b5dbde59-e733-4640-99e7-cc992840a0bd" };

    /** Create a new pod store with a unique name and initial data.
     * If pod store with a same name already created. It is updated with new data.
     * @param typeName A typename for POD data. (e.g. "PostProcess"). This is not the RenderDataStore::TYPE_NAME
     * @param name A unique name of the POD.
     * @param data Data to be stored.
     */
    virtual void CreatePod(const BASE_NS::string_view typeName, const BASE_NS::string_view name,
        const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Destroy a specific pod store.
     * @param typeName A typename for POD data. (e.g. "PostProcess"). This is not the RenderDataStore::TYPE_NAME
     * @param name A unique name of the POD.
     */
    virtual void DestroyPod(const BASE_NS::string_view typeName, const BASE_NS::string_view name) = 0;

    /** Set POD data by unique name.
     * @param name Name
     * @param data Data
     */
    virtual void Set(const BASE_NS::string_view name, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Get view to a POD data by unique name. Cast to own types in code.
     * Do not hold this data. To store this data, copy it somewhere
     * @param name Name
     */
    virtual BASE_NS::array_view<const uint8_t> Get(const BASE_NS::string_view name) const = 0;

    /** Get view to all POD names with given type.
     * @param typeName A name type name.
     */
    virtual BASE_NS::array_view<const BASE_NS::string> GetPodNames(const BASE_NS::string_view typeName) const = 0;

protected:
    IRenderDataStorePod() = default;
    ~IRenderDataStorePod() override = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_POD_H
