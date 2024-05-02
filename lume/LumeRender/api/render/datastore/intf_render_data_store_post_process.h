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

#ifndef API_RENDER_IRENDER_DATA_STORE_POST_PROCESS_H
#define API_RENDER_IRENDER_DATA_STORE_POST_PROCESS_H

#include <cstdint>

#include <base/containers/fixed_string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastorepostprocess */
/** IRenderDataStorePostProcess interface.
 * Internally synchronized.
 *
 * Post process mapper for low-level post process PODs
 * One can add names for custom post process and add their local data
 *
 * Usage:
 * Create() with a unique name.
 * Get() get (preferrably only a single post process data).
 * Destroy() destroy from render data store.
 */
class IRenderDataStorePostProcess : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "329a8427-4c0f-48ab-b9fa-3f9bc6dfbcbd" };

    /** Values map to render_data_store_render_pods.h */
    struct GlobalFactors {
        /** Enabled flags */
        uint32_t enableFlags { 0u };

        /** User post process factors which are automatically mapped and can be used easily anywhere in the pipeline */
        BASE_NS::Math::Vec4 factors[PostProcessConstants::GLOBAL_FACTOR_COUNT];

        /** User post process factors which are automatically mapped and can be used easily anywhere in the pipeline */
        BASE_NS::Math::Vec4 userFactors[PostProcessConstants::USER_GLOBAL_FACTOR_COUNT];
    };

    struct PostProcess {
        /* Name for the post process (flags and data fetched with this) e.g. "custom_tonemap" */
        BASE_NS::fixed_string<64u> name;
        /*  Id, created when post process created */
        uint32_t id { ~0u };
        /* Factor index to fetch user factors from GlobalPostProcess struct */
        uint32_t factorIndex { ~0u };
        /* Default shader handle for the post process */
        RenderHandleReference shader {};

        struct Variables {
            /* Factor index to fetch user factors from GlobalPostProcess struct */
            uint32_t userFactorIndex { ~0u };
            /* Factor which can be mapped to various parts of the pipeline with GlobalPostProcess struct */
            BASE_NS::Math::Vec4 factor { 0.0f, 0.0f, 0.0f, 0.0f };
            /* Factors which are mapped with LocalPostProcess struct. Byte data to pass data without conversions */
            uint8_t customPropertyData[PostProcessConstants::USER_LOCAL_FACTOR_BYTE_SIZE] {};
            /* Is the post process enabled */
            bool enabled { false };
            /* Additional flags */
            uint32_t flags { 0u };
        };
        Variables variables;
    };

    /** Create a new post process with POD or use existing. (Unique names with render data store pods)
     * If the name is already the already created object is returned.
     * @param name A unique name for the post process stack.
     */
    virtual void Create(const BASE_NS::string_view name) = 0;

    /** Create a new single post process to already created post process
     * If the name is already the already created the object is not created again.
     * NOTE: if "materialMetaData" is found from the shader the localFactorData is fetch from it.
     * One needs to do Set() to override this default built-in data.
     * @param name A unique name of the post process.
     * @param ppName A unique name for the single post process.
     * @param shader Optional shader handle to be used as post process default shader.
     */
    virtual void Create(
        const BASE_NS::string_view name, const BASE_NS::string_view ppName, const RenderHandleReference& shader) = 0;

    /** Destroy. The related render data store post process pod is destroyed as well
     * @param name Name of the post process.
     */
    virtual void Destroy(const BASE_NS::string_view name) = 0;

    /** Destroy a single post process.
     * @param name Name of the post process.
     * @param ppName Name of the single post process effect.
     */
    virtual void Destroy(const BASE_NS::string_view name, const BASE_NS::string_view ppName) = 0;

    /** Checks that the post process is already created.
     * @param name Name of the post process.
     * @return Boolean value.
     */
    virtual bool Contains(const BASE_NS::string_view name) const = 0;

    /** Checks that the post process and its single post process is already created.
     * @param name Name of the post process.
     * @param ppName Name of the single post process.
     * @return Boolean value.
     */
    virtual bool Contains(const BASE_NS::string_view name, const BASE_NS::string_view ppName) const = 0;

    /** Set variables to a single post process.
     * @param name Name of the post process stack
     * @param ppName Name of the single post process effect.
     * @param vars Variables to set for the single post process
     */
    virtual void Set(
        const BASE_NS::string_view name, const BASE_NS::string_view ppName, const PostProcess::Variables& vars) = 0;

    /** Set multiple variables to a post process. They are filled in order.
     * @param name Name of the post process stack
     * @param vars Variables to set for the single post processes
     */
    virtual void Set(const BASE_NS::string_view name, const BASE_NS::array_view<PostProcess::Variables> vars) = 0;

    /** Get global post process data.
     * @param name Name of the post process stack
     * @return Global factors
     */
    virtual GlobalFactors GetGlobalFactors(const BASE_NS::string_view name) const = 0;

    /** Get all post processes by name. Copies all data should not be used every frame.
     * @param name Name of the post process stack
     */
    virtual BASE_NS::vector<PostProcess> Get(const BASE_NS::string_view name) const = 0;

    /** Get a single post process data.
     * @param name Name of the post process stack
     * @param ppName Name of the single post process
     * @return Single post process
     */
    virtual PostProcess Get(const BASE_NS::string_view name, const BASE_NS::string_view ppName) const = 0;

protected:
    virtual ~IRenderDataStorePostProcess() override = default;
    IRenderDataStorePostProcess() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_POST_PROCESS_H
