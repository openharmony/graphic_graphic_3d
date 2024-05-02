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

#include "render_barrier_list.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include <render/namespace.h>

#include "nodecontext/render_command_list.h"
#include "util/linear_allocator.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr size_t MEMORY_ALIGNMENT { 16 };

void* AllocateBarrierListMemory(RenderBarrierList::LinearAllocatorStruct& allocator, const size_t byteSize)
{
    void* rc = allocator.allocators[allocator.currentIndex]->Allocate(byteSize);
    if (rc) {
        return rc;
    } else { // current allocator is out of memory
        PLUGIN_ASSERT_MSG(allocator.allocators[allocator.currentIndex]->GetByteSize() > 0,
            "do not create render barrier list with zero size initial allocation");

        const size_t alignment = allocator.allocators[allocator.currentIndex]->GetAlignment();

        allocator.allocators.push_back(make_unique<LinearAllocator>(byteSize, alignment));
        allocator.currentIndex = (uint32_t)(allocator.allocators.size() - 1);

        rc = allocator.allocators[allocator.currentIndex]->Allocate(byteSize);
        PLUGIN_ASSERT_MSG(rc, "render barrier list allocation : out of memory");
        return rc;
    }
}

CommandBarrier* AllocateCommandBarriers(RenderBarrierList::LinearAllocatorStruct& allocator, const size_t count)
{
    const size_t byteSize = count * sizeof(CommandBarrier);
    return static_cast<CommandBarrier*>(AllocateBarrierListMemory(allocator, byteSize));
}
} // namespace

RenderBarrierList::RenderBarrierList(const uint32_t reserveBarrierCountHint)
{
    const size_t memAllocationByteSize = sizeof(CommandBarrier) * reserveBarrierCountHint;
    if (memAllocationByteSize > 0) {
        linearAllocator_.allocators.push_back(make_unique<LinearAllocator>(memAllocationByteSize, MEMORY_ALIGNMENT));
    }
}

void RenderBarrierList::BeginFrame()
{
    const auto clearAndReserve = [](auto& vec) {
        const size_t count = vec.size();
        vec.clear();
        vec.reserve(count);
    };

    clearAndReserve(barrierPointBarriers_);
    clearAndReserve(barrierPointIndextoIndex_);

    if (!linearAllocator_.allocators.empty()) {
        linearAllocator_.currentIndex = 0;
        if (linearAllocator_.allocators.size() == 1) { // size is good for this frame
            linearAllocator_.allocators[linearAllocator_.currentIndex]->Reset();
        } else if (linearAllocator_.allocators.size() > 1) {
            size_t fullByteSize = 0;
            size_t alignment = 0;
            for (auto& ref : linearAllocator_.allocators) {
                fullByteSize += ref->GetCurrentByteSize();
                alignment = std::max(alignment, (size_t)ref->GetAlignment());
                ref.reset();
            }
            linearAllocator_.allocators.clear();

            // create new single allocation for combined previous size and some extra bytes
            linearAllocator_.allocators.push_back(make_unique<LinearAllocator>(fullByteSize, alignment));
        }
    }
}

void RenderBarrierList::AddBarriersToBarrierPoint(
    const uint32_t barrierPointIndex, const vector<CommandBarrier>& barriers)
{
    if (!barriers.empty()) {
        BarrierPointBarrierList* structData = static_cast<BarrierPointBarrierList*>(
            AllocateBarrierListMemory(linearAllocator_, sizeof(BarrierPointBarrierList)));
        CommandBarrier* commandBarrierData = AllocateCommandBarriers(linearAllocator_, barriers.size());
        if (structData && commandBarrierData) {
            const size_t barriersByteSize = barriers.size() * sizeof(CommandBarrier);
            const bool val = CloneData(commandBarrierData, barriersByteSize, barriers.data(), barriersByteSize);
            PLUGIN_UNUSED(val);
            PLUGIN_ASSERT(val);
            size_t barrierIndex = 0;
            if (auto iter = barrierPointIndextoIndex_.find(barrierPointIndex);
                iter != barrierPointIndextoIndex_.cend()) {
                barrierIndex = iter->second;
            } else { // first barrier to this barrier point index
                barrierPointBarriers_.push_back(BarrierPointBarriers {});
                barrierIndex = barrierPointBarriers_.size() - 1;
                barrierPointIndextoIndex_[barrierPointIndex] = barrierIndex;
            }

            const uint32_t barrierCount = (uint32_t)barriers.size();
            BarrierPointBarriers& ref = barrierPointBarriers_[barrierIndex];
            ref.fullCommandBarrierCount += barrierCount;
            ref.barrierListCount += 1u;

            structData->count = barrierCount;
            structData->commandBarriers = commandBarrierData;
            if (ref.barrierListCount == 1) { // first barrier point barrier list
                ref.firstBarrierList = structData;
            } else {
                // patch next pointer for previous BarrierPointBarrierList
                ref.lastBarrierList->nextBarrierPointBarrierList = structData;
            }
            ref.lastBarrierList = structData;

            PLUGIN_ASSERT(ref.firstBarrierList);
            PLUGIN_ASSERT(ref.lastBarrierList);
        }
    }
}

bool RenderBarrierList::HasBarriers(const uint32_t barrierPointIndex) const
{
    return (barrierPointIndextoIndex_.count(barrierPointIndex) == 1);
}

const RenderBarrierList::BarrierPointBarriers* RenderBarrierList::GetBarrierPointBarriers(
    const uint32_t barrierPointIndex) const
{
    const BarrierPointBarriers* barriers = nullptr;

    if (const auto iter = barrierPointIndextoIndex_.find(barrierPointIndex); iter != barrierPointIndextoIndex_.cend()) {
        const size_t index = iter->second;
        PLUGIN_ASSERT(index < barrierPointBarriers_.size());
        if (index < barrierPointBarriers_.size()) {
            barriers = &barrierPointBarriers_[index];
        }
    }

    return barriers;
}
RENDER_END_NAMESPACE()
