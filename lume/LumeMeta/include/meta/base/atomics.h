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
#ifndef META_BASE_ATOMICS_H
#define META_BASE_ATOMICS_H

#include <stdint.h>

#include <core/namespace.h>
#if defined(_MSC_VER) && defined(WIN32)
#include <intrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_acle.h>
#endif

CORE_BEGIN_NAMESPACE();
/*
 * Implementation of InterlockedIncrement/InterlockedDecrement (int32_t atomics)
 * Bare minimum to implement thread safe reference counters.
 **/

#if defined(_MSC_VER) && defined(WIN32)
// On windows and visual studio, we just forward to the matching OS methods.
inline int32_t AtomicIncrement(volatile int32_t* a) noexcept
{
    return ::_InterlockedIncrement((long*)a);
}
inline int32_t AtomicDecrement(volatile int32_t* a) noexcept
{
    return ::_InterlockedDecrement((long*)a);
}
inline int32_t AtomicRead(const volatile int32_t* a) noexcept
{
    return ::_InterlockedExchangeAdd((long*)a, 0);
}
inline int32_t AtomicIncrementIfNotZero(volatile int32_t* a) noexcept
{
    int32_t v = AtomicRead(a);
    while (v) {
        int32_t temp = v;
        v = ::_InterlockedCompareExchange((long*)a, v + 1, v);
        if (v == temp) {
            return temp;
        }
    }
    return v;
}

// Trivial spinlock implemented with atomics.
// NOTE: this does NOT yield while waiting, so use this ONLY in places where lock contention times are expected to be
// trivial. Also does not ensure fairness. but most likely enough for our reference counting purposes.
// and of course is non-recursive, so you can only lock once in a thread.
class SpinLock {
public:
    SpinLock() noexcept = default;
    ~SpinLock() noexcept = default;
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;
    void Lock() noexcept
    {
        while (_InterlockedCompareExchange(&lock_, 1, 0) == 1) {
            _mm_pause();
        }
    }
    void Unlock() noexcept
    {
        _InterlockedExchange(&lock_, 0);
    }

private:
    long lock_ = 0;
};
#elif defined(__has_builtin) && __has_builtin(__atomic_add_fetch) && __has_builtin(__atomic_load_n) && \
    __has_builtin(__atomic_compare_exchange_n)
/* gcc built in atomics, supported on clang also */
inline int32_t AtomicIncrement(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, 1, __ATOMIC_ACQ_REL);
}
inline int32_t AtomicDecrement(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, -1, __ATOMIC_ACQ_REL);
}
inline int32_t AtomicRead(const volatile int32_t* a) noexcept
{
    return __atomic_load_n(a, __ATOMIC_ACQUIRE);
}
inline int32_t AtomicIncrementIfNotZero(volatile int32_t* a) noexcept
{
    int32_t v = AtomicRead(a);
    while (v) {
        int32_t temp = v;
        if (__atomic_compare_exchange_n(a, &v, temp + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
            return temp;
        }
    }
    return v;
}
// Trivial spinlock implemented with atomics.
// NOTE: this does NOT yield while waiting, so use this ONLY in places where lock contention times are expected to be
// trivial. Also does not ensure fairness. but most likely enough for our reference counting purposes.
// and of course is non-recursive, so you can only lock once in a thread.
class SpinLock {
public:
    SpinLock() noexcept = default;
    ~SpinLock() noexcept = default;
    SpinLock(const SpinLock&) = delete;
    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;
    void Lock() noexcept
    {
        long expected = 0;
#if defined(__aarch64__)
        __sevl();
#endif
        while (!__atomic_compare_exchange_n(&lock_, &expected, 1, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
            expected = 0;
#if __has_builtin(__builtin_ia32_pause)
            __builtin_ia32_pause();
#elif defined(__arm__) || defined(__aarch64__)
            __wfe();
#endif
        }
    }
    void Unlock() noexcept
    {
        __atomic_store_n(&lock_, 0, __ATOMIC_RELEASE);
#if defined(__arm__)
        __sev();
#endif
    }

private:
    long lock_ = 0;
};

#else
#error Compiler / Platform specific atomic methods not implemented !
#endif

/**
 * @brief Scoped helper to lock and unlock spin locks.
 */
class ScopedSpinLock {
public:
    ScopedSpinLock(const ScopedSpinLock&) = delete;
    ScopedSpinLock& operator=(const ScopedSpinLock&) = delete;
    ScopedSpinLock(ScopedSpinLock&&) = delete;
    ScopedSpinLock& operator=(ScopedSpinLock&&) = delete;

    explicit ScopedSpinLock(SpinLock& l) noexcept : lock_(l)
    {
        lock_.Lock();
    }
    ~ScopedSpinLock() noexcept
    {
        lock_.Unlock();
    }

private:
    SpinLock& lock_;
};

CORE_END_NAMESPACE();
#endif
