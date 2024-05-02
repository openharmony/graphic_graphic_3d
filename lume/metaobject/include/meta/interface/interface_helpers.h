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

#ifndef META_INTERFACE_EXT_INTERFACE_HELPERS_H
#define META_INTERFACE_EXT_INTERFACE_HELPERS_H

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/atomics.h>
#include <meta/base/namespace.h>
#include <meta/base/type_traits.h>
#include <meta/interface/intf_lifecycle.h>

META_BEGIN_NAMESPACE()

template<typename Type, typename = int>
struct IsIntroduceInterface {
    constexpr static bool VALUE = false;
};

template<typename Type>
struct IsIntroduceInterface<Type, decltype((void)Type::INTRODUCE_INTERFACES_TAG, 0)> {
    constexpr static bool VALUE = true;
};

namespace Internal {

using GIFuncType = CORE_NS::IInterface*(void*);

struct UidInfo {
    BASE_NS::Uid uid;
    GIFuncType* getInterface {};
};

template<size_t Size>
struct UIDArray {
    constexpr UidInfo& operator[](size_t i) noexcept
    {
        return data[i];
    }
    constexpr const UidInfo& operator[](size_t i) const noexcept
    {
        return data[i];
    }
    constexpr static size_t SIZE = Size;
    UidInfo data[Size];
};

constexpr static void SwapUidInfo(UidInfo& l, UidInfo& r) noexcept
{
    UidInfo tmp = BASE_NS::move(l);
    l = BASE_NS::move(r);
    r = BASE_NS::move(tmp);
}

static constexpr bool operator<(const UidInfo& lhs, const UidInfo& rhs) noexcept
{
    return lhs.uid < rhs.uid;
}

template<size_t Size>
constexpr static void SortUidArrayImpl(UIDArray<Size>& array, size_t left, size_t right) noexcept
{
    if (left < right) {
        auto m = left;
        for (auto i = left + 1; i < right; i++) {
            if (array[i] < array[left]) {
                SwapUidInfo(array[++m], array[i]);
            }
        }
        SwapUidInfo(array[left], array[m]);
        SortUidArrayImpl(array, left, m);
        SortUidArrayImpl(array, m + 1, right);
    }
}

template<size_t Size>
constexpr static void SortUidArray(UIDArray<Size>& array) noexcept
{
    SortUidArrayImpl(array, 0, Size);
}

constexpr static CORE_NS::IInterface* BinarySearch(
    const BASE_NS::Uid& uid, void* me, const UidInfo* info, size_t size) noexcept
{
    int32_t lo = 0;
    int32_t hi = static_cast<int32_t>(size);
    auto* p = info;
    while (lo <= hi) {
        const auto mid = lo + (hi - lo) / 2;
        p = info + mid;
        const auto d = uid.compare(p->uid);
        if (!d) {
            // Found match
            return p->getInterface(me);
        }
        if (d > 0) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return {};
}

constexpr static CORE_NS::IInterface* LinearSearch(
    const BASE_NS::Uid& uid, void* me, const UidInfo* info, size_t size) noexcept
{
    auto* p = info;
    while (p < info + size) {
        const auto d = uid.compare(p->uid);
        if (!d) {
            // Found match
            return p->getInterface(me);
        }
        if (d < 0) {
            // Rest of uids are bigger than reference, i.e. no match
            break;
        }
        ++p;
    }
    return {};
}

template<size_t Size>
constexpr static CORE_NS::IInterface* SearchInterface(
    const BASE_NS::Uid& uid, void* me, const UIDArray<Size>& info) noexcept
{
    constexpr auto linearSearchThreshold = 10;
    if constexpr (Size == 0) {
        return {};
    }
    if constexpr (Size < linearSearchThreshold) {
        return LinearSearch(uid, me, info.data, Size);
    }
    return BinarySearch(uid, me, info.data, Size);
}

} // namespace Internal

template<typename... Interfaces>
class IntroduceInterfaces;

template<typename T>
constexpr auto CastImpl(T* p)
{
    return p;
}
template<typename T, typename Current, typename... Path>
constexpr auto CastImpl(T* p)
{
    return CastImpl<Current, Path...>(static_cast<Current*>(p));
}

template<typename...>
struct Deducer {};

template<typename Me, typename... Interfaces>
struct GetInterfacesImpl final {
    constexpr GetInterfacesImpl()
    {
        size_t index = 0;
        (FillArrayDepth<Interfaces, Interfaces>(index), ...);
        Internal::SortUidArray(arr_);
    }

    template<typename I>
    static constexpr size_t CountInterfacesDepth()
    {
        if constexpr (IsIntroduceInterface<I>::VALUE) {
            return I::SIZE;
        } else {
            if constexpr (!BASE_NS::is_same_v<I, CORE_NS::IInterface>) {
                return 1 + CountInterfacesDepth<typename I::base>();
            }
        }
        return 0;
    }

    template<typename I, typename... Intfs>
    static constexpr size_t CountInterfaces()
    {
        return (0 + ... + CountInterfacesDepth<Interfaces>());
    }

    static constexpr size_t SIZE = CountInterfaces<Interfaces...>();

    constexpr Internal::UIDArray<SIZE> Get() const
    {
        return arr_;
    }

    constexpr bool HasUid(const BASE_NS::Uid& uid, size_t size) const
    {
        if (uid != CORE_NS::IInterface::UID) {
            for (size_t i = 0; i != size; ++i) {
                if (arr_.data[i].uid == uid) {
                    return true;
                }
            }
        }
        return false;
    }

    template<typename... Path, typename... Intfs>
    constexpr void FillArrayDepthIntroduce(size_t& index, Deducer<Intfs...>)
    {
        (FillArrayDepth<Intfs, Path..., Intfs>(index), ...);
    }

    template<typename I, typename... Path>
    constexpr void FillArrayDepth(size_t& index)
    {
        if constexpr (IsIntroduceInterface<I>::VALUE) {
            FillArrayDepthIntroduce<Path...>(index, typename I::deducer {});
        } else if (!HasUid(I::UID, index)) {
            if constexpr (!BASE_NS::is_same_v<I, CORE_NS::IInterface>) {
                arr_.data[index].uid = I::UID;
                arr_.data[index].getInterface = [](void* p) constexpr {
                    return static_cast<CORE_NS::IInterface*>(CastImpl<Me, Path...>(static_cast<Me*>(p)));
                };
                ++index;
                FillArrayDepth<typename I::base, Path..., typename I::base>(index);
            }
        }
    }

private:
    Internal::UIDArray<SIZE> arr_ {};
};

template<typename Me, typename... Interfaces>
constexpr auto GetInterfaces()
{
    GetInterfacesImpl<Me, Interfaces...> gi;
    return gi.Get();
}

template<typename... Interfaces>
BASE_NS::vector<BASE_NS::Uid> GetInterfacesVector()
{
    auto arr = GetInterfaces<Interfaces...>();
    return BASE_NS::vector<BASE_NS::Uid>(arr.data, arr.data + arr.SIZE);
}

template<>
inline BASE_NS::vector<BASE_NS::Uid> GetInterfacesVector<>()
{
    return BASE_NS::vector<BASE_NS::Uid> {};
}

/**
 * @brief Check if list of interfaces (or their base classes) contains ILifecycle
 */
template<typename... Interfaces>
constexpr bool HAS_ILIFECYCLE = (false || ... || BASE_NS::is_convertible_v<Interfaces*, ILifecycle*>);

template<typename T, typename...>
struct FirstType {
    using Type = T;
};

template<typename... T>
using FirstTypeT = typename FirstType<T...>::Type;

/**
 * @brief Helper class to derive from which implements reference counting and
 *        GetInterface for you.
 */
template<typename... Interfaces>
class IntroduceInterfaces : public Interfaces... {
public:
    using deducer = Deducer<Interfaces...>;
    static constexpr bool INTRODUCE_INTERFACES_TAG {};
    using IntroduceInterfacesType = IntroduceInterfaces;

    template<typename... Args>
    explicit IntroduceInterfaces(Args&&... args) noexcept : FirstTypeT<Interfaces...>(BASE_NS::forward<Args>(args)...)
    {}

    void Ref() override
    {
        CORE_NS::AtomicIncrement(&refcnt_);
    }
    void Unref() override
    {
        if (CORE_NS::AtomicDecrement(&refcnt_) == 0) {
            if constexpr (HAS_ILIFECYCLE<Interfaces...>) {
                if (auto i = this->GetInterface(ILifecycle::UID)) {
                    static_cast<ILifecycle*>(i)->Destroy();
                }
            }
            delete this;
        }
    }

    constexpr CORE_NS::IInterface* StaticGetInterface(const BASE_NS::Uid& uid)
    {
        if (uid == CORE_NS::IInterface::UID) {
            // CORE_NS::IInterface is implicitly implemented by everyone
            return static_cast<CORE_NS::IInterface*>(static_cast<void*>(this));
        }
        return Internal::SearchInterface(uid, this, IntsImpl());
    }

    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        return StaticGetInterface(uid);
    }
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        auto* me = const_cast<IntroduceInterfaces*>(this);
        return me->IntroduceInterfaces::GetInterface(uid);
    }

    template<size_t... Index>
    static BASE_NS::vector<BASE_NS::Uid> GetInterfacesVectorImpl(IndexSequence<Index...>)
    {
        return { IntsImpl().data[Index].uid... };
    }

    static BASE_NS::vector<BASE_NS::Uid> GetInterfacesVector()
    {
        return GetInterfacesVectorImpl(MakeIndexSequence<SIZE>());
    }

    static auto& IntsImpl()
    {
        // this needs to be in the function to compile with newer vc++ which considers the IntroduceInterfaces
        // to not be defined yet when we instantiate the GetInterfaces causing static_casts to base to fail.
        static constexpr auto interfaces = META_NS::GetInterfaces<IntroduceInterfaces, Interfaces...>();
        return interfaces;
    }
    constexpr static size_t SIZE = GetInterfacesImpl<IntroduceInterfaces, Interfaces...>::SIZE;

    int32_t refcnt_ { 0 };
};

template<>
class IntroduceInterfaces<> {
public:
    using deducer = Deducer<>;
    static constexpr bool INTRODUCE_INTERFACES_TAG {};
    using IntroduceInterfacesType = IntroduceInterfaces;

    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid)
    {
        return nullptr;
    }
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const
    {
        return nullptr;
    }

    static BASE_NS::vector<BASE_NS::Uid> GetInterfacesVector()
    {
        return BASE_NS::vector<BASE_NS::Uid>();
    }
};

#define STATIC_INTERFACES_WITH_CONCRETE_BASE(introduced, baseclass) \
public:                                                             \
    static BASE_NS::vector<BASE_NS::Uid> GetInterfacesVector()      \
    {                                                               \
        return GetStaticInterfaces();                               \
    }                                                               \
    static BASE_NS::vector<BASE_NS::Uid> GetStaticInterfaces()      \
    {                                                               \
        auto v1 = introduced::GetInterfacesVector();                \
        auto v2 = baseclass::GetInterfacesVector();                 \
        v1.insert(v1.end(), v2.begin(), v2.end());                  \
        return v1;                                                  \
    }

META_END_NAMESPACE()

#endif
