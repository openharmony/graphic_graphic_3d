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
