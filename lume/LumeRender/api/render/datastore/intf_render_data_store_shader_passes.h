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

#ifndef API_RENDER_IRENDER_DATA_STORE_SHADER_PASSES_H
#define API_RENDER_IRENDER_DATA_STORE_SHADER_PASSES_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/device/intf_shader_pipeline_binder.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_IrenderDataStoreShaderPasses */
/** IRenderDataStoreShaderPasses interface.
 * Internally synchronized.
 * The data should not be added during render front-end.
 *
 * Usage:
 * AddRenderData() or AddComputeData() every frame to specific block name
 * Fetch data in render node(s) with GetRenderData() or GetComputeData()
 */

class IRenderDataStoreShaderPasses : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "1e85fc6d-09e0-4d2d-9422-3165f486c92d" };

    /** Compute pass data */
    struct ComputePassData {
        BASE_NS::vector<IShaderPipelineBinder::Ptr> shaderBinders;
    };
    /** Render pass data */
    struct RenderPassData {
        RenderPassWithHandleReference renderPass;
        BASE_NS::vector<IShaderPipelineBinder::Ptr> shaderBinders;
    };
    /** Property binding data info */
    struct PropertyBindingDataInfo {
        /** Aligned byte size needed for binding data which is not in local GPU buffers
         * Alignment is based on minimun UBO dynamic binding offsets.
         */
        uint32_t alignedByteSize { 0U };
    };

    /** Add data to named block for fetching.
     * @param name Name of the data block. One can add data to same named block multiple times during a frame.
     * @param data Render pass data.
     */
    virtual void AddRenderData(BASE_NS::string_view name, BASE_NS::array_view<const RenderPassData> data) = 0;

    /** Add data to named block for fetching.
     * @param name Name of the data block. One can add data to same named block multiple times during a frame.
     * @param data Render pass data.
     */
    virtual void AddComputeData(BASE_NS::string_view name, BASE_NS::array_view<const ComputePassData> data) = 0;

    /** Get compute data per block (name).
     * @param name Name of the data block. One can add data to same named block multiple times during a frame.
     * @return Render pass data.
     */
    virtual BASE_NS::vector<RenderPassData> GetRenderData(BASE_NS::string_view name) const = 0;

    /** Get compute data per block (name).
     * @param name Name of the data block. One can add data to same named block multiple times during a frame.
     * @return Compute pass data.
     */
    virtual BASE_NS::vector<ComputePassData> GetComputeData(BASE_NS::string_view name) const = 0;

    /** Get all rendering data.
     * @return Returns all RenderPassData.
     */
     virtual BASE_NS::vector<RenderPassData> GetRenderData() const = 0;

    /** Get all rendering data.
     * @return Returns all RenderPassData.
     */
     virtual BASE_NS::vector<ComputePassData> GetComputeData() const = 0;

    /** Get render property binding info.
     * @param name Name of the data block. One can add data to named block multiple times during a frame.
     * @return PropertyBindingDataInfo Returns all render related.
     */
     virtual PropertyBindingDataInfo GetRenderPropertyBindingInfo(BASE_NS::string_view name) const = 0;

    /** Get render property binding info.
     * @param name Name of the data block. One can add data to named block multiple times during a frame.
     * @return PropertyBindingDataInfo Returns all compute related.
     */
     virtual PropertyBindingDataInfo GetComputePropertyBindingInfo(BASE_NS::string_view name) const = 0;

    /** Get all render property binding info.
     * @return PropertyBindingDataInfo Returns all render related.
     */
     virtual PropertyBindingDataInfo GetRenderPropertyBindingInfo() const = 0;

    /** Get all compute property binding info.
     * @return PropertyBindingDataInfo Returns all compute related.
     */
     virtual PropertyBindingDataInfo GetComputePropertyBindingInfo() const = 0;

protected:
    virtual ~IRenderDataStoreShaderPasses() override = default;
    IRenderDataStoreShaderPasses() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_SHADER_PASSES_H
