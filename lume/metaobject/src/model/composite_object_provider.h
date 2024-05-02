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
#include <meta/ext/object_container.h>
#include <meta/interface/model/intf_object_provider.h>

META_BEGIN_NAMESPACE()

class CompositeObjectProvider
    : public ObjectFlatContainerFwd<CompositeObjectProvider, ClassId::CompositeObjectProvider, IObjectProvider> {
public:
    ~CompositeObjectProvider() override;
    CompositeObjectProvider();
    META_NO_COPY_MOVE(CompositeObjectProvider)

    bool Build(const IMetadata::Ptr&) override;

    IObject::Ptr CreateObject(const DataModelIndex& index) override;
    bool DisposeObject(const META_NS::IObject::Ptr& item) override;
    size_t GetObjectCount(const DataModelIndex& index) const override;

    META_IMPLEMENT_INTERFACE_EVENT(IObjectProvider, IOnDataAdded, OnDataAdded)
    META_IMPLEMENT_INTERFACE_EVENT(IObjectProvider, IOnDataRemoved, OnDataRemoved)
    META_IMPLEMENT_INTERFACE_EVENT(IObjectProvider, IOnDataMoved, OnDataMoved)

    META_IMPLEMENT_INTERFACE_PROPERTY(IObjectProvider, size_t, CacheHint, 10);

private:
    void OnAddedProviderData(const IObjectProvider::Ptr& provider, DataModelIndex base, size_t count);
    void OnRemovedProviderData(const IObjectProvider::Ptr& provider, DataModelIndex base, size_t count);
    void OnMovedProviderData(
        const IObjectProvider::Ptr& provider, DataModelIndex from, size_t count, DataModelIndex to);

    IObjectProvider* FindProvider(size_t& index) const;
    size_t CalculateIndex(const IObjectProvider::Ptr& provider, size_t localIndex) const;
    size_t CalculateIndexBase(size_t provider) const;

    void OnProviderAdded(const ChildChangedInfo& info);
    void OnProviderRemoved(const ChildChangedInfo& info);
    void OnProviderMoved(const ChildMovedInfo& info);

private:
    BASE_NS::unordered_map<const IObject*, IObjectProvider*> objects_;
};

META_END_NAMESPACE()

#endif
