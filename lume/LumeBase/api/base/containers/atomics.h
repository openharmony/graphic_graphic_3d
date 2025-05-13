/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef API_BASE_CONTAINERS_ATOMICS_H
#define API_BASE_CONTAINERS_ATOMICS_H

#include <stdint.h>

#include <base/namespace.h>
#if defined(_MSC_VER) && defined(WIN32)
#include <intrin.h>
#elif defined(__arm__) || defined(__aarch64__)
#include <arm_acle.h>
#endif

BASE_BEGIN_NAMESPACE()
/*
 * Implementation of InterlockedIncrement/InterlockedDecrement (int32_t atomics)
 * Bare minimum to implement thread safe reference counters.
 */

#if defined(_MSC_VER) && defined(WIN32)
// On windows and visual studio, we just forward to the matching OS methods.
inline int32_t AtomicIncrement(volatile int32_t* a) noexcept
{
    return ::_InterlockedIncrement(reinterpret_cast<volatile long*>(a));
}

inline int32_t AtomicIncrementRelaxed(volatile int32_t* a) noexcept
{
    return ::_InterlockedIncrement(reinterpret_cast<volatile long*>(a));
}
inline int32_t AtomicIncrementAcquire(volatile int32_t* a) noexcept
{
    return ::_InterlockedIncrement(reinterpret_cast<volatile long*>(a));
}
inline int32_t AtomicIncrementRelease(volatile int32_t* a) noexcept
{
    return ::_InterlockedIncrement(reinterpret_cast<volatile long*>(a));
}

inline int32_t AtomicDecrement(volatile int32_t* a) noexcept
{
    return ::_InterlockedDecrement(reinterpret_cast<volatile long*>(a));
}

inline int32_t AtomicDecrementRelaxed(volatile int32_t* a) noexcept
{
    return ::_InterlockedDecrement(reinterpret_cast<volatile long*>(a));
}
inline int32_t AtomicDecrementAcquire(volatile int32_t* a) noexcept
{
    return ::_InterlockedDecrement(reinterpret_cast<volatile long*>(a));
}
inline int32_t AtomicDecrementRelease(volatile int32_t* a) noexcept
{
    return ::_InterlockedDecrement(reinterpret_cast<volatile long*>(a));
}

inline int32_t AtomicRead(const volatile int32_t* a) noexcept
{
    return ::_InterlockedExchangeAdd(reinterpret_cast<volatile long*>(const_cast<volatile int32_t*>(a)), 0);
}
inline int32_t AtomicReadRelaxed(const volatile int32_t* a) noexcept
{
    return *a;
}
inline int32_t AtomicReadAcquire(const volatile int32_t* a) noexcept
{
    auto ret = *a;
    _ReadWriteBarrier();
    return ret;
}

inline int32_t AtomicIncrementIfNotZero(volatile int32_t* a) noexcept
{
    int32_t v = AtomicReadRelaxed(a);
    while (v) {
        int32_t temp = v;
        v = ::_InterlockedCompareExchange(reinterpret_cast<volatile long*>(a), v + 1, v);
        if (v == temp) {
            return temp;
        }
    }
    return v;
}

inline void AtomicFenceRelaxed() noexcept
{
    // no op
}
inline void AtomicFenceAcquire() noexcept
{
    _ReadWriteBarrier();
}
inline void AtomicFenceRelease() noexcept
{
    _ReadWriteBarrier();
}

/**
 * @brief Simple spin lock with pause instruction
 */
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

inline int32_t AtomicIncrementRelaxed(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, 1, __ATOMIC_RELAXED);
}
inline int32_t AtomicIncrementAcquire(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, 1, __ATOMIC_ACQUIRE);
}
inline int32_t AtomicIncrementRelease(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, 1, __ATOMIC_RELEASE);
}

inline int32_t AtomicDecrement(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, -1, __ATOMIC_ACQ_REL);
}

inline int32_t AtomicDecrementRelaxed(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, -1, __ATOMIC_RELAXED);
}
inline int32_t AtomicDecrementAcquire(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, -1, __ATOMIC_ACQUIRE);
}
inline int32_t AtomicDecrementRelease(volatile int32_t* a) noexcept
{
    return __atomic_add_fetch(a, -1, __ATOMIC_RELEASE);
}

inline int32_t AtomicRead(const volatile int32_t* a) noexcept
{
    return __atomic_load_n(a, __ATOMIC_ACQUIRE);
}
inline int32_t AtomicReadRelaxed(const volatile int32_t* a) noexcept
{
    return __atomic_load_n(a, __ATOMIC_RELAXED);
}
inline int32_t AtomicReadAcquire(const volatile int32_t* a) noexcept
{
    return __atomic_load_n(a, __ATOMIC_ACQUIRE);
}

inline int32_t AtomicIncrementIfNotZero(volatile int32_t* a) noexcept
{
    int32_t v = AtomicReadRelaxed(a);
    while (v) {
        int32_t temp = v;
        if (__atomic_compare_exchange_n(a, &v, temp + 1, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
            return temp;
        }
    }
    return v;
}

inline void AtomicFenceRelaxed() noexcept
{
    __atomic_thread_fence(__ATOMIC_RELAXED);
}
inline void AtomicFenceAcquire() noexcept
{
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
}
inline void AtomicFenceRelease() noexcept
{
    __atomic_thread_fence(__ATOMIC_RELEASE);
}
/**
 * @brief Implements simple TTAS spin lock with pause instruction
 */
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

BASE_END_NAMESPACE()
#endif // API_BASE_CONTAINERS_ATOMICS_H
