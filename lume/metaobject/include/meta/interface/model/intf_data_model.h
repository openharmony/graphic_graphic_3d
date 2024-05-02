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

#ifndef META_INTERFACE_MODEL_IDATA_MODEL_H
#define META_INTERFACE_MODEL_IDATA_MODEL_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/model/data_model_index.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE()

struct IOnDataAddedInfo {
    constexpr static BASE_NS::Uid UID { "9de720f7-8b7f-4b64-92ae-c0a3d9a13807" };
    constexpr static char const* NAME { "OnDataAdded" };
};

/**
 * @brief Event which is triggered when data is added to the model.
 * Note: One has to connect directly (no posting to other thread or so) to these events for the indices to be valid.
 * @param base Base index of the added data.
 * 1-D: the base is simply the start index where data was added and count of data entities added.
 * N-D: the base can point to any dimension and identify count entities in that dimension.
 *      Example: Two dimensions, let first dimension be row and second column.
 *               Let 2dIndex(y, x) mean DataModelIndex(y, &col), where DataModelIndex col(x);
 *               1. DataModelIndex(0) with count 1 means row was added to beginning of the model.
 *               2. 2dIndex(0, 0) with count 5 means 5 columns were added to first row.
 */
using IOnDataAdded = META_NS::SimpleEvent<IOnDataAddedInfo, void(DataModelIndex base, size_t count)>;

struct IOnDataRemovedInfo {
    constexpr static BASE_NS::Uid UID { "3440f5d6-b874-40e6-999c-eee3a4049b94" };
    constexpr static char const *NAME { "OnDataRemoved" };
};

/**
 * @brief Event which is triggered when data is removed from the model.
 * See above how indices work.
 */
using IOnDataRemoved = META_NS::SimpleEvent<IOnDataRemovedInfo, void(DataModelIndex base, size_t count)>;

struct IOnDataMovedInfo {
    constexpr static BASE_NS::Uid UID { "7407dcc5-c903-4ceb-a337-23a1ff6f1d76" };
    constexpr static char const *NAME { "OnDataMoved" };
};

/**
 * @brief Event which is triggered when data is removed from the model.
 * See above how indices work.
 */
using IOnDataMoved = META_NS::SimpleEvent<IOnDataMovedInfo, void(DataModelIndex from, size_t count, DataModelIndex to)>;

/**
 * @brief Generic data model which can be used to query data per index.
 */
class IDataModel : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDataModel, "4811e11d-39bb-46d7-8f6e-b70f43a7a9ff")
public:
    /**
     * @brief Get data at given index, null if no such index exists.
     */
    virtual IMetadata::ConstPtr GetModelData(const DataModelIndex& index) const = 0;
    virtual IMetadata::Ptr GetModelData(const DataModelIndex& index) = 0;

    /**
     * @brief Get size of given dimension at given index.
     */
    virtual size_t GetSize(const DataModelIndex& index) const = 0;

    /**
     * @brief Get size of first dimension.
     */
    size_t GetSize() const
    {
        return GetSize(DataModelIndex {});
    }

    /**
     * @brief Event when data is added to the model.
     */
    META_EVENT(IOnDataAdded, OnDataAdded)
    /**
     * @brief Event when data is removed from the model.
     */
    META_EVENT(IOnDataRemoved, OnDataRemoved)
    /**
     * @brief Event when data is moved in the model.
     */
    META_EVENT(IOnDataMoved, OnDataMoved)
};

META_END_NAMESPACE()

#endif
