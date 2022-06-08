/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "perf/cpu_timer.h"

#include <chrono>
#include <cstdint>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
using namespace std::chrono;
using Clock = system_clock;

void CpuTimer::Begin() noexcept
{
    begin_ = Clock::now().time_since_epoch().count();
}

void CpuTimer::End() noexcept
{
    end_ = Clock::now().time_since_epoch().count();
}

int64_t CpuTimer::GetMicroseconds() const noexcept
{
    return (int64_t)(duration_cast<microseconds>(Clock::duration(end_) - Clock::duration(begin_)).count());
}
RENDER_END_NAMESPACE()
