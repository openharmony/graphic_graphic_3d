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

#ifndef API_BASE_CONTAINERS_BYTE_ARRAY_H
#define API_BASE_CONTAINERS_BYTE_ARRAY_H

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
/** @ingroup group_containers_bytearray */
/** Byte Array */
class ByteArray {
public:
    ByteArray(const size_t byteSize) : data_(make_unique<uint8_t[]>(byteSize)), byteSize_(byteSize) {}

    array_view<uint8_t> GetData()
    {
        return { data_.get(), byteSize_ };
    }

    array_view<const uint8_t> GetData() const
    {
        return { data_.get(), byteSize_ };
    }

private:
    unique_ptr<uint8_t[]> data_;
    size_t byteSize_ { 0U };
};
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_BYTE_ARRAY_H
