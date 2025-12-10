/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef META_TEST_TEST_UTILS_H
#define META_TEST_TEST_UTILS_H

#include <algorithm>
#include <limits>
#include <ostream>

#include <base/math/mathf.h>
#include <base/math/vector.h>
#include <base/util/uid.h>

#include <meta/base/time_span.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_metadata.h>

#include "helpers/testing_objects.h"

// Tests if all components of a vector are within epsilon of each other
bool AreEqual(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs, float epsilon = BASE_NS::Math::EPSILON);
bool AreNear(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs);
bool AreEqual(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs, float epsilon = BASE_NS::Math::EPSILON);
bool AreNear(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs);

namespace BASE_NS {
std::ostream& operator<<(std::ostream& os, const BASE_NS::Uid& uid);
std::ostream& operator<<(std::ostream& os, const BASE_NS::string& s);
std::ostream& operator<<(std::ostream& os, const BASE_NS::string_view& s);

namespace Math {
std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec2& vec);
std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec3& vec);
// Operators for EXPECT_LT/GT/LE/GE macros to work
// Note: These are not proper comparison operators, they only verify if
// both x and y of lhs is > or < than rhs.
bool operator>(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs);
bool operator<(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs);
bool operator>(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs);
bool operator<(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs);

bool operator>=(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs);
bool operator<=(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs);
bool operator>=(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs);
bool operator<=(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs);
} // namespace Math
} // namespace BASE_NS

namespace META_NS {
std::ostream& operator<<(std::ostream& os, const META_NS::TimeSpan& vec);

template<typename Type>
bool ContainsObjectWithName(const BASE_NS::vector<Type>& vec, const BASE_NS::string& name)
{
    for (auto c : vec) {
        if (auto i = interface_cast<IObject>(c)) {
            if (i->GetName() == name) {
                return true;
            }
        }
    }
    return false;
}

namespace UTest {

template<typename T>
bool IsEqual(const BASE_NS::vector<T>& a, const BASE_NS::vector<T>& b);

template<class A>
inline bool IsEqual(const A& a, const A& b)
{
    if constexpr (BASE_NS::is_floating_point_v<A>) {
        double mul = std::max({ A(1.0), std::fabs(a), std::fabs(b) });
        return std::fabs(a - b) <= std::numeric_limits<A>::epsilon() * mul;
    } else {
        return a == b;
    }
}

inline bool IsEqual(const ITestType::ConstPtr& a, const ITestType::ConstPtr& b)
{
    auto lobj = interface_cast<IObject>(a);
    auto robj = interface_cast<IObject>(b);
    return lobj && robj && lobj->GetName() == robj->GetName();
}

inline bool IsEqual(const ITestType::Ptr& a, const ITestType::Ptr& b)
{
    return IsEqual(ITestType::ConstPtr(a), ITestType::ConstPtr(b));
}

inline bool IsEqual(const IAny& a, const IAny& b)
{
    SharedPtrConstIInterface pa;
    SharedPtrConstIInterface pb;
    if (a.GetValue(pa) && b.GetValue(pb)) {
        return (pa != nullptr) == (pb != nullptr);
    }
    auto copy = a.Clone(true);
    return copy->CopyFrom(b) == AnyReturn::NOTHING_TO_DO;
}

inline bool IsEqual(const IAny::Ptr& a, const IAny::Ptr& b)
{
    return (!a && !b) || (a && b && IsEqual(*a, *b));
}

inline bool IsEqual(const IValue::Ptr& a, const IValue::Ptr& b)
{
    return IsEqual(a->GetValue(), b->GetValue());
}

inline bool IsEqual(const IArrayAny::Ptr& a, const IArrayAny::Ptr& b)
{
    if (a->GetSize() != b->GetSize()) {
        return false;
    }
    for (size_t i = 0; i != a->GetSize(); ++i) {
        auto anya = a->Clone(AnyCloneOptions { CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM });
        auto anyb = b->Clone(AnyCloneOptions { CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM });
        if (!a->GetAnyAt(i, *anya) || !b->GetAnyAt(i, *anyb)) {
            return false;
        }
        if (!IsEqual(anya, anyb)) {
            return false;
        }
    }
    return true;
}

inline bool IsEqual(IObject::Ptr a, IObject::Ptr b)
{
    if ((a != nullptr) != (b != nullptr)) {
        return false;
    }
    if (a && b && a->GetClassId() != b->GetClassId()) {
        return false;
    }
    auto ta = interface_pointer_cast<ITestType>(a);
    auto tb = interface_pointer_cast<ITestType>(b);
    return (!ta && !tb) || IsEqual(ta, tb);
}

inline bool IsEqual(IContainer::Ptr a, IContainer::Ptr b)
{
    if (a->GetSize() != b->GetSize()) {
        return false;
    }
    for (size_t i = 0; i != a->GetSize(); ++i) {
        if (!IsEqual(a->GetAt(i), b->GetAt(i))) {
            return false;
        }
    }
    return true;
}

inline bool IsEqual(IAttach::Ptr a, IAttach::Ptr b)
{
    return IsEqual(a->GetAttachments(), b->GetAttachments());
}

inline bool IsEqual(IAttachment::Ptr a, IAttachment::Ptr b)
{
    auto ao = interface_cast<IObject>(a);
    auto bo = interface_cast<IObject>(b);
    return (!ao && !bo) || (ao && bo && ao->GetName() == bo->GetName());
}

inline bool IsEqual(IProperty::Ptr a, IProperty::Ptr b)
{
    if (a->GetName() != b->GetName()) {
        return false;
    }
    if ((a->GetOwner().lock() != nullptr) != (b->GetOwner().lock() != nullptr)) {
        return false;
    }
    if (a->IsDefaultValue() != b->IsDefaultValue()) {
        return false;
    }
    auto as = interface_cast<IStackProperty>(a);
    auto bs = interface_cast<IStackProperty>(b);
    if (!as || !bs) {
        return false;
    }
    return IsEqual(as->GetDefaultValue(), bs->GetDefaultValue()) &&
           IsEqual(as->GetValues({}, false), bs->GetValues({}, false)) &&
           as->GetModifiers({}, false).size() == bs->GetModifiers({}, false).size();
}

template<typename T>
inline bool IsEqual(const BASE_NS::vector<T>& a, const BASE_NS::vector<T>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i != a.size(); ++i) {
        if (!IsEqual(a[i], b[i])) {
            return false;
        }
    }
    return true;
}

inline BASE_NS::vector<META_NS::IProperty::Ptr> SortByName(BASE_NS::vector<META_NS::IProperty::Ptr> vec)
{
    std::stable_sort(vec.begin(), vec.end(), [](auto a, auto b) { return a->GetName() < b->GetName(); });
    return vec;
}

inline bool IsEqual(IMetadata::Ptr a, IMetadata::Ptr b)
{
    return IsEqual(SortByName(a->GetProperties()), SortByName(b->GetProperties()));
}

} // namespace UTest
} // namespace META_NS

#endif // META_TEST_TEST_UTILS_H
