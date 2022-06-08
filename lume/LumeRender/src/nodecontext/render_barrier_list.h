/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_RENDER_RENDER_BARRIER_LIST_H
#define CORE_RENDER_RENDER_BARRIER_LIST_H

#include <cstddef>
#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "nodecontext/render_command_list.h"
#include "util/linear_allocator.h"

RENDER_BEGIN_NAMESPACE()
/** RenderBarrierList.
 * Stores barriers per single RenderCommandList.
 * Barriers are created with RenderGraph.
 */
class RenderBarrierList final {
public:
    struct BarrierPointBarrierList {
        uint32_t count { 0u };
        CommandBarrier* commandBarriers { nullptr };

        // linked when going through all barriers
        BarrierPointBarrierList* nextBarrierPointBarrierList { nullptr };
    };
    struct BarrierPointBarriers {
        // indirection, usually has only one BarrierPointBarrierList
        uint32_t barrierListCount { 0u };
        uint32_t fullCommandBarrierCount { 0u };

        // pointer to first BarrierPointBarrierList
        BarrierPointBarrierList* firstBarrierList { nullptr };
        // pointer to last BarrierPointBarrierList
        BarrierPointBarrierList* lastBarrierList { nullptr };
    };

    struct LinearAllocatorStruct {
        uint32_t currentIndex { 0 };
        BASE_NS::vector<BASE_NS::unique_ptr<LinearAllocator>> allocators;
    };

    explicit RenderBarrierList(const uint32_t reserveBarrierCountHint);
    ~RenderBarrierList() = default;

    RenderBarrierList(const RenderBarrierList&) = delete;
    RenderBarrierList operator=(const RenderBarrierList&) = delete;

    // reset buffers and data
    void BeginFrame();

    void AddBarriersToBarrierPoint(const uint32_t barrierPointIndex, const BASE_NS::vector<CommandBarrier>& barriers);
    bool HasBarriers(const uint32_t barrierPointIndex) const;
    const BarrierPointBarriers* GetBarrierPointBarriers(const uint32_t barrierPointIndex) const;

private:
    BASE_NS::unordered_map<uint32_t, size_t> barrierPointIndextoIndex_;

    BASE_NS::vector<BarrierPointBarriers> barrierPointBarriers_;
    LinearAllocatorStruct linearAllocator_;
};
RENDER_END_NAMESPACE()

#endif // CORE_RENDER_RENDER_BARRIER_LIST_H
