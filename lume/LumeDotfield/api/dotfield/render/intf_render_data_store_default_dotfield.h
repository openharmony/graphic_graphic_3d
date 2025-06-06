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

#ifndef PLUGIN_API_RENDER_DATA_STORE_DEFAULT_DOTFIELD_H
#define PLUGIN_API_RENDER_DATA_STORE_DEFAULT_DOTFIELD_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/ecs/entity.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/namespace.h>
#include <render/render_data_structures.h>

namespace RENDER_NS {
class IGpuResourceManager;
class IShaderManager;
} // namespace RENDER_NS

namespace Dotfield {
struct RenderDataDefaultDotfield {
    /* Buffering count, for easier management. */
    static constexpr uint32_t DOTFIELD_BUFFERING_COUNT { 2u };

    /** Dotfield
     */
    struct DotfieldPrimitive {
        CORE_NS::Entity entity {};

        BASE_NS::Math::Mat4X4 matrix { 1.f };
        BASE_NS::Math::UVec2 size { 64u, 64u };
        BASE_NS::Math::Vec2 touch { 0.0f, 0.0f };
        BASE_NS::Math::Vec3 touchDirection { 0.0f, 0.0f, 0.0f };
        BASE_NS::Math::UVec4 colors { 0u, 0u, 0u, 0u };
        float pointScale { 60.f };
        float touchRadius { 0.f };
    };

    struct BufferData {
        uint32_t currFrameIndex { 0u }; // 0 or 1 for double buffering

        struct Buffer {
            // double buffering
            RENDER_NS::RenderHandleReference dataBuffer[DOTFIELD_BUFFERING_COUNT]; // (packed) data
        };
        BASE_NS::vector<Buffer> buffers;
    };
};

/**
RenderDataStoreDefaultDotfield

- Handles per frame particle primitives.
- Handles/Managers particle buffers and effects.
*/
class IRenderDataStoreDefaultDotfield : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "f01a1d5f-2bfa-4f46-892f-33eb0397a654" };

    virtual void AddDotfieldPrimitive(const RenderDataDefaultDotfield::DotfieldPrimitive& dotfieldPrimitive) = 0;
    virtual void RemoveDotfieldPrimitive(const CORE_NS::Entity& entity) = 0;
    virtual BASE_NS::array_view<RenderDataDefaultDotfield::DotfieldPrimitive> GetDotfieldPrimitives() = 0;
    virtual BASE_NS::array_view<const RenderDataDefaultDotfield::DotfieldPrimitive> GetDotfieldPrimitives() const = 0;
    virtual const RenderDataDefaultDotfield::BufferData& GetBufferData() const = 0;

    virtual float GetTimeStep() const noexcept = 0;
    virtual void SetTimeStep(float time) noexcept = 0;
    virtual float GetTime() const noexcept = 0;
    virtual void SetTime(float time) noexcept = 0;
};
} // namespace Dotfield

#endif // PLUGIN_API_RENDER_DATA_STORE_DEFAULT_DOTFIELD_H
