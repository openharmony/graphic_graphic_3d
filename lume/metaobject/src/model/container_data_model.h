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

#ifndef META_SRC_MODEL_CONTAINER_DATA_MODEL_H
#define META_SRC_MODEL_CONTAINER_DATA_MODEL_H

#include <meta/ext/object_container.h>
#include <meta/interface/model/intf_data_model.h>

META_BEGIN_NAMESPACE()

class ContainerDataModel : public ObjectContainerFwd<ContainerDataModel, ClassId::ContainerDataModel, IDataModel> {
public:
    bool Build(const IMetadata::Ptr&) override;

    IMetadata::ConstPtr GetModelData(const DataModelIndex& index) const override;
    IMetadata::Ptr GetModelData(const DataModelIndex& index) override;
    size_t GetSize(const DataModelIndex& index) const override;

    META_IMPLEMENT_INTERFACE_EVENT(IDataModel, IOnDataAdded, OnDataAdded)
    META_IMPLEMENT_INTERFACE_EVENT(IDataModel, IOnDataRemoved, OnDataRemoved)
    META_IMPLEMENT_INTERFACE_EVENT(IDataModel, IOnDataMoved, OnDataMoved)
};

META_END_NAMESPACE()

#endif
