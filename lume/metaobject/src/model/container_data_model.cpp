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
#include "container_data_model.h"

#include <meta/api/make_callback.h>

META_BEGIN_NAMESPACE()

bool ContainerDataModel::Build(const IMetadata::Ptr&)
{
    SetRequiredInterfaces({ IMetadata::UID });
    OnAdded()->AddHandler(MakeCallback<IOnChildChanged>([this](const ChildChangedInfo& info) {
        META_ACCESS_EVENT(OnDataAdded)->Invoke(DataModelIndex(info.index), 1);
    }));
    OnRemoved()->AddHandler(MakeCallback<IOnChildChanged>([this](const ChildChangedInfo& info) {
        META_ACCESS_EVENT(OnDataRemoved)->Invoke(DataModelIndex(info.index), 1);
    }));
    OnMoved()->AddHandler(MakeCallback<IOnChildMoved>([this](const ChildMovedInfo& info) {
        META_ACCESS_EVENT(OnDataMoved)->Invoke(DataModelIndex(info.from), 1, DataModelIndex(info.to));
    }));
    return true;
}

IMetadata::ConstPtr ContainerDataModel::GetModelData(const DataModelIndex& index) const
{
    return GetAt<IMetadata>(index.Index());
}

IMetadata::Ptr ContainerDataModel::GetModelData(const DataModelIndex& index)
{
    return GetAt<IMetadata>(index.Index());
}

size_t ContainerDataModel::GetSize(const DataModelIndex& index) const
{
    return ObjectContainerFwd::GetSize();
}

META_END_NAMESPACE()
