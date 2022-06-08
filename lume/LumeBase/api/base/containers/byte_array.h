/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_CONTAINERS_BYTE_ARRAY_H
#define API_BASE_CONTAINERS_BYTE_ARRAY_H

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
