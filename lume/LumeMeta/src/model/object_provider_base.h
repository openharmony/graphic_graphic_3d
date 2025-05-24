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

#ifndef META_SRC_MODEL_OBJECT_PROVIDER_BASE_H
#define META_SRC_MODEL_OBJECT_PROVIDER_BASE_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/model/intf_object_provider.h>

#include "object.h"

META_BEGIN_NAMESPACE()

META_REGISTER_CLASS(ObjectProviderBase, "13656d27-e228-4fd7-a4ce-5bb051cdaafc", ObjectCategory::NO_CATEGORY)

class ObjectProviderBase : public IntroduceInterfaces<MetaObject, IModelObjectProvider> {
    META_OBJECT(ObjectProviderBase, ClassId::ObjectProviderBase, IntroduceInterfaces)
public:
    ~ObjectProviderBase() override;
    META_NO_COPY_MOVE(ObjectProviderBase)

    ObjectProviderBase() = default;

    IObject::Ptr CreateObject(const DataModelIndex& index) override;
    bool DisposeObject(const META_NS::IObject::Ptr& item) override;
    size_t GetObjectCount(const DataModelIndex& index) const override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_EVENT_DATA(IObjectProvider, IOnDataAdded, OnDataAdded)
    META_STATIC_EVENT_DATA(IObjectProvider, IOnDataRemoved, OnDataRemoved)
    META_STATIC_EVENT_DATA(IObjectProvider, IOnDataMoved, OnDataMoved)
    META_STATIC_PROPERTY_DATA(IObjectProvider, size_t, CacheHint, 10)
    META_END_STATIC_DATA()
    META_IMPLEMENT_EVENT(IOnDataAdded, OnDataAdded)
    META_IMPLEMENT_EVENT(IOnDataRemoved, OnDataRemoved)
    META_IMPLEMENT_EVENT(IOnDataMoved, OnDataMoved)
    META_IMPLEMENT_PROPERTY(size_t, CacheHint)

    bool SetDataModel(const IDataModel::Ptr& model) override;
    IDataModel::Ptr GetDataModel() const override;

protected:
    virtual IObject::Ptr Construct(const IMetadata::Ptr& data);

private:
    void BindProperties(const IObject::Ptr& object, const IMetadata::Ptr& data) const;

private:
    IDataModel::Ptr model_;
    BASE_NS::vector<IObject::Ptr> recyclebin_;
};

META_END_NAMESPACE()

#endif
