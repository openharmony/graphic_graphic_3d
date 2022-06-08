/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_CORE_PERF_CPU_TIMER_H
#define API_CORE_PERF_CPU_TIMER_H

#include <cstdint>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/**
 * CpuTimer.
 * Simple cpu timer class.
 */
class CpuTimer {
public:
    void Begin() noexcept;
    void End() noexcept;

    int64_t GetMicroseconds() const noexcept;

private:
    int64_t begin_ {};
    int64_t end_ {};
};
RENDER_END_NAMESPACE()

#endif // API_CORE_PERF_CPU_TIMER_H
