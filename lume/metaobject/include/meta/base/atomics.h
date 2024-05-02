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
#endif

CORE_BEGIN_NAMESPACE();
/*
 * Implementation of InterlockedIncrement/InterlockedDecrement (int32_t atomics)
 * Bare minimum to implement thread safe reference counters.
 **/

#if defined(_MSC_VER) && defined(WIN32)
// On windows and visual studio, we just forward to the matching OS methods.
inline int32_t AtomicIncrement(volatile int32_t* a)
{
    return ::_InterlockedIncrement((long*)a);
}
inline int32_t AtomicDecrement(volatile int32_t* a)
{
    return ::_InterlockedDecrement((long*)a);
}
inline int32_t AtomicRead(const volatile int32_t* a)
{
    return ::_InterlockedExchangeAdd((long*)a, 0);
}
inline int32_t AtomicIncrementIfNotZero(volatile int32_t* a)
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
    void Lock()
    {
        while (_InterlockedCompareExchange(&lock_, taken_, free_) == taken_) {
        }
    }
    void Unlock()
    {
        _InterlockedExchange(&lock_, free_);
    }

private:
    long lock_ = 0;
    static constexpr long taken_ = 1;
    static constexpr long free_ = 0;
};
#elif defined(__has_builtin) && __has_builtin(__atomic_add_fetch) && __has_builtin(__atomic_load_n) && \
    __has_builtin(__atomic_compare_exchange_n)
/* gcc built in atomics, supported on clang also */
inline int32_t AtomicIncrement(volatile int32_t* a)
{
    return __atomic_add_fetch(a, 1, __ATOMIC_ACQ_REL);
}
inline int32_t AtomicDecrement(volatile int32_t* a)
{
    return __atomic_add_fetch(a, -1, __ATOMIC_ACQ_REL);
}
inline int32_t AtomicRead(const volatile int32_t* a)
{
    return __atomic_load_n(a, __ATOMIC_ACQUIRE);
}
inline int32_t AtomicIncrementIfNotZero(volatile int32_t* a)
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
    void Lock()
    {
        long taken = 1;
        long expect = 0;
        while (!__atomic_compare_exchange(&lock_, &expect, &taken, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
            expect = 0;
        }
    }
    void Unlock()
    {
        long free = 0;
        __atomic_store(&lock_, &free, __ATOMIC_SEQ_CST);
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

    explicit ScopedSpinLock(SpinLock& l) : lock_(l)
    {
        lock_.Lock();
    }
    ~ScopedSpinLock()
    {
        lock_.Unlock();
    }

private:
    SpinLock& lock_;
};

CORE_END_NAMESPACE();
#endif
