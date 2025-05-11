/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Container Data Model Implementation
 * Author: Mikael Kilpeläinen
 * Create: 2023-05-25
 */

#ifndef META_SRC_MODEL_CONTAINER_DATA_MODEL_H
#define META_SRC_MODEL_CONTAINER_DATA_MODEL_H

#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_container.h>
#include <meta/interface/model/intf_data_model.h>

META_BEGIN_NAMESPACE()

class ContainerDataModel : public IntroduceInterfaces<CommonObjectContainerFwd, IDataModel> {
    META_OBJECT(ContainerDataModel, ClassId::ContainerDataModel, IntroduceInterfaces, ClassId::ObjectContainer)
public:
    bool Build(const IMetadata::Ptr&) override;

    IMetadata::ConstPtr GetModelData(const DataModelIndex& index) const override;
    IMetadata::Ptr GetModelData(const DataModelIndex& index) override;
    size_t GetSize(const DataModelIndex& index) const override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_EVENT_DATA(IDataModel, IOnDataAdded, OnDataAdded)
    META_STATIC_EVENT_DATA(IDataModel, IOnDataRemoved, OnDataRemoved)
    META_STATIC_EVENT_DATA(IDataModel, IOnDataMoved, OnDataMoved)
    META_END_STATIC_DATA()

    META_IMPLEMENT_EVENT(IOnDataAdded, OnDataAdded)
    META_IMPLEMENT_EVENT(IOnDataRemoved, OnDataRemoved)
    META_IMPLEMENT_EVENT(IOnDataMoved, OnDataMoved)
};

META_END_NAMESPACE()

#endif
