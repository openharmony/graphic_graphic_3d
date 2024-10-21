/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef META_TEST_DATA_HELPERS_H
#define META_TEST_DATA_HELPERS_H

#include <tuple>
#include <type_traits>

#include <gtest/gtest.h>

#include <base/containers/type_traits.h>
#include <base/containers/vector.h>

namespace META_NS {

template<typename T>
struct TType {
    using Type = T;
    BASE_NS::vector<std::function<Type()>> values;

    template<typename... Args, typename = BASE_NS::enable_if_t<std::conjunction_v<std::is_convertible<Args, Type>...>>>
    TType(Args... args) : values {  [args] { return static_cast<T>(args); }... }
    {}

    TType(std::initializer_list<std::function<Type()>> values) : values { values } {}
};

template<typename... T>
struct TestTypes {
    using Types = ::testing::Types<T...>;
    using Tuple = std::tuple<TType<T>...>;

    TestTypes(TType<T>... v) : values { v... } {}

    template<typename Type>
    auto GetValue(size_t index) const
    {
        auto v = std::get<TType<Type>>(values);
        CORE_ASSERT_MSG(index < v.values.size(), "invalid test values");
        return v.values[index]();
    }

    Tuple values;
};

}

#endif