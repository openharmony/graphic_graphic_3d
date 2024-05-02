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

#ifndef META_INTERFACE_MODEL_DATA_MODEL_INDEX_H
#define META_INTERFACE_MODEL_DATA_MODEL_INDEX_H

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Multi-dimensional index for Data Models.
 * These indices are not meant to be stored as such because they can contain pointers to other dimensions,
 * so either construct new index for each call or make sure all the dimensions are stored.
 * Example:
 *     struct Index2D {
 *         Index2D(size_t x, size_t y) : secondDim_(x), firstDim_(y, &secondDim_) {}
 *         operator DataModelIndex() const {
 *             return firstDim_;
 *         }
 *         DataModelIndex secondDim_;
 *         DataModelIndex firstDim_;
 *     };
 */
class DataModelIndex {
public:
    /**
     * @brief Default construction creates invalid index.
     */
    DataModelIndex() = default;
    DataModelIndex(size_t index, const DataModelIndex* dim = nullptr) : index_(index), dimension_(dim) {}

    /**
     * @brief Returns true if this is valid index.
     */
    bool IsValid() const
    {
        return index_ != -1;
    }

    /**
     * @brief Get the plain index value for current dimension.
     */
    size_t Index() const
    {
        return index_;
    }

    /**
     * @brief Get next dimension
     */
    DataModelIndex Dimension() const
    {
        return dimension_ ? *dimension_ : DataModelIndex {};
    }

    bool operator==(const DataModelIndex& other) const
    {
        return index_ == other.index_ && dimension_ == other.dimension_;
    }
    bool operator!=(const DataModelIndex& other) const
    {
        return !(*this == other);
    }

    const DataModelIndex* GetDimensionPointer() const
    {
        return dimension_;
    }

private:
    size_t index_ = -1;
    const DataModelIndex* dimension_ {};
};

META_END_NAMESPACE()

META_TYPE(META_NS::DataModelIndex)

#endif
