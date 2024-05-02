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

#ifndef API_3D_RENDER_IRENDER_DATA_STORE_MORPH_H
#define API_3D_RENDER_IRENDER_DATA_STORE_MORPH_H

#include <cstdint>

#include <3d/namespace.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastoremorph */
/**
RenderDataMorph.
*/
struct RenderDataMorph {
    /** Max vertex buffer count */
    static constexpr uint32_t MAX_VERTEX_BUFFER_COUNT { 3 };

    /** Submesh */
    struct Submesh {
        /** Vertex count */
        uint32_t vertexCount;

        /** {0 = position, 1 = normal, 2 = tangent} */
        RenderVertexBuffer vertexBuffers[MAX_VERTEX_BUFFER_COUNT];

        /** Vertex buffer count */
        uint32_t vertexBufferCount { 0 };

        /** Buffer contains base position/normal/tangent and all morph target deltas (position/normal/tangent) */
        RenderVertexBuffer morphTargetBuffer;

        /** Number of morph targets */
        uint32_t morphTargetCount { 0 };

        struct Target {
            /** Target ID */
            uint32_t id;
            /** Weight value for target (must be non-zero) */
            float weight;
        };
        /** Targets with non-zero weights */
        BASE_NS::vector<Target> activeTargets;
    };
};

/** @ingroup group_render_irenderdatastoremorph */
/**
RenderDataStoreMorph interface.
Not internally syncronized.
*/
class IRenderDataStoreMorph : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "230e8df1-9465-4894-af8f-f47f38413000" };

    /** Reserve size */
    struct ReserveSize {
        /** Submesh count */
        uint32_t submeshCount { 0 };
    };

    ~IRenderDataStoreMorph() override = default;

    /** Add submeshes safely. */
    virtual void AddSubmesh(const RenderDataMorph::Submesh& submesh) = 0;

    /** Get submeshes
     * @return Return array view of morph submeshes
     */
    virtual BASE_NS::array_view<const RenderDataMorph::Submesh> GetSubmeshes() const = 0;

protected:
    IRenderDataStoreMorph() = default;
};
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_IRENDER_DATA_STORE_MORPH_H
