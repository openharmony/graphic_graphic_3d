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
#include <meta/api/make_callback.h>
#include <meta/interface/intf_recyclable.h>

#include "instantiating_object_provider.h"

META_BEGIN_NAMESPACE()

ObjectProviderBase::~ObjectProviderBase()
{
    SetDataModel(nullptr);
}

IObject::Ptr ObjectProviderBase::CreateObject(const DataModelIndex& index)
{
    if (!index.IsValid() || !model_) {
        return nullptr;
    }

    IObject::Ptr obj;
    if (auto data = model_->GetModelData(index)) {
        if (!recyclebin_.empty()) {
            obj = recyclebin_.back();
            recyclebin_.pop_back();
        } else {
            obj = Construct(data);
        }
        if (auto i = interface_cast<IRecyclable>(obj)) {
            i->ReBuild(data);
        } else {
            BindProperties(obj, data);
        }
    }
    return obj;
}

bool ObjectProviderBase::DisposeObject(const META_NS::IObject::Ptr& item)
{
    if (auto i = interface_cast<IRecyclable>(item)) {
        i->Dispose();
    } else if (auto d = interface_cast<IMetadata>(item)) {
        for (auto&& p : d->GetAllProperties()) {
            if (!IsFlagSet(p, ObjectFlagBits::NATIVE)) {
                PropertyLock l { p };
                l->ResetBind();
            }
        }
    }
    bool recycle = recyclebin_.size() < CacheHint()->GetValue();
    if (recycle) {
        recyclebin_.push_back(item);
    }
    return recycle;
}

size_t ObjectProviderBase::GetObjectCount(const DataModelIndex& index) const
{
    return model_ ? model_->GetSize() : 0;
}

bool ObjectProviderBase::SetDataModel(const IDataModel::Ptr& model)
{
    if (model_) {
        model_->OnDataAdded()->RemoveHandler(uintptr_t(this));
        model_->OnDataRemoved()->RemoveHandler(uintptr_t(this));
        model_->OnDataMoved()->RemoveHandler(uintptr_t(this));
    }
    model_ = model;
    if (model_) {
        model_->OnDataAdded()->AddHandler(MakeCallback<IOnDataAdded>([this](DataModelIndex base, size_t count) {
            META_ACCESS_EVENT(OnDataAdded)->Invoke(base, count);
        }),
            uintptr_t(this));
        model_->OnDataRemoved()->AddHandler(MakeCallback<IOnDataRemoved>([this](DataModelIndex base, size_t count) {
            META_ACCESS_EVENT(OnDataRemoved)->Invoke(base, count);
        }),
            uintptr_t(this));
        model_->OnDataMoved()->AddHandler(
            MakeCallback<IOnDataMoved>([this](DataModelIndex from, size_t count, DataModelIndex to) {
                META_ACCESS_EVENT(OnDataMoved)->Invoke(from, count, to);
            }),
            uintptr_t(this));
    }
    return true;
}

IDataModel::Ptr ObjectProviderBase::GetDataModel() const
{
    return model_;
}

void ObjectProviderBase::BindProperties(const IObject::Ptr& object, const IMetadata::Ptr& data) const
{
    if (auto odata = interface_cast<IMetadata>(object)) {
        for (auto&& p : data->GetAllProperties()) {
            BASE_NS::string name = "Model." + p->GetName();
            IProperty::Ptr prop = odata->GetPropertyByName(name);
            if (!prop) {
                prop = DuplicatePropertyType(META_NS::GetObjectRegistry(), p, name);
                if (prop) {
                    odata->AddProperty(prop);
                }
            }
            if (prop) {
                PropertyLock l { prop };
                l->SetBind(p);
            }
        }
    }
}

IObject::Ptr ObjectProviderBase::Construct(const IMetadata::Ptr& data)
{
    return nullptr;
}

META_END_NAMESPACE()
