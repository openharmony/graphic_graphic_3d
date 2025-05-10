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

#ifndef API_RENDER_IRENDER_DATA_STORE_RENDER_POST_PROCESSES_H
#define API_RENDER_IRENDER_DATA_STORE_RENDER_POST_PROCESSES_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_post_process.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastorerenderpostprocesses */
/** IRenderDataStoreRenderPostProcesses interface.
 * Internally synchronized.
 *
 * Post process mapper for rendering
 * One needs to Implement IRenderPostProcess interface
 *
 * Usage:
 * AddData() from client side.
 * GetData() in render node.
 * Data needs to be added every frame (cleared automatically)
 */
class IRenderDataStoreRenderPostProcesses : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "ae33858f-31f9-4c8b-9386-5d542a9ee018" };

    struct PostProcessData {
        /** Unique id for the post process effect
         * Use the same every frame
         * There can multiple post processes with the same post process with different id
         * There can be multiple post processes with the same post process with the same id
         */
        uint64_t id { ~0ULL };
        /** Post process effect interface pointer
         */
        IRenderPostProcess::Ptr postProcess;
    };
    /** Container for post process pipeline
     */
    struct PostProcessPipeline {
        /** All post processes
         */
        BASE_NS::vector<PostProcessData> postProcesses;
    };

    /** Add data for rendering
     * @param data All render post processes.
     */
    virtual void AddData(BASE_NS::string_view name, BASE_NS::array_view<const PostProcessData> data) = 0;

    /** Get all rendering data.
     * @param name Name of the post process pipeline
     * @return Returns post process pipeline for the given name
     */
    virtual PostProcessPipeline GetData(BASE_NS::string_view name) const = 0;

protected:
    virtual ~IRenderDataStoreRenderPostProcesses() override = default;
    IRenderDataStoreRenderPostProcesses() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_RENDER_POST_PROCESSES_H
