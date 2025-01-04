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

#ifndef META_SRC_MODEL_COMPOSITE_OBJECT_PROVIDER_H
#define META_SRC_MODEL_COMPOSITE_OBJECT_PROVIDER_H

#include <base/containers/unordered_map.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/object_container.h>
#include <meta/interface/model/intf_object_provider.h>

META_BEGIN_NAMESPACE()

class CompositeObjectProvider : public IntroduceInterfaces<CommonObjectContainerFwd, IObjectProvider> {
    META_OBJECT(
        CompositeObjectProvider, ClassId::CompositeObjectProvider, IntroduceInterfaces, ClassId::ObjectFlatContainer)
public:
    ~CompositeObjectProvider() override;
    CompositeObjectProvider();
    META_NO_COPY_MOVE(CompositeObjectProvider)

    bool Build(const IMetadata::Ptr&) override;

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

private:
    void OnAddedProviderData(const IObjectProvider::Ptr& provider, DataModelIndex base, size_t count);
    void OnRemovedProviderData(const IObjectProvider::Ptr& provider, DataModelIndex base, size_t count);
    void OnMovedProviderData(
        const IObjectProvider::Ptr& provider, DataModelIndex from, size_t count, DataModelIndex to);

    IObjectProvider* FindProvider(size_t& index) const;
    size_t CalculateIndex(const IObjectProvider::Ptr& provider, size_t localIndex) const;
    size_t CalculateIndexBase(size_t provider) const;

    void OnProviderChanged(const ChildChangedInfo& info);
    void OnProviderAdded(const ChildChangedInfo& info);
    void OnProviderRemoved(const ChildChangedInfo& info);
    void OnProviderMoved(const ChildChangedInfo& info);

private:
    BASE_NS::unordered_map<const IObject*, IObjectProvider*> objects_;
};

META_END_NAMESPACE()

#endif
