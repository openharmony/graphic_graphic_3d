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
#include "composite_object_provider.h"

#include <meta/api/iteration.h>
#include <meta/api/make_callback.h>

META_BEGIN_NAMESPACE()

CompositeObjectProvider::CompositeObjectProvider() = default;

CompositeObjectProvider::~CompositeObjectProvider()
{
    ForEachShared(GetSelf(), [&](const IObject::Ptr& obj) {
        if (auto provider = interface_cast<IObjectProvider>(obj)) {
            provider->OnDataAdded()->RemoveHandler(uintptr_t(this));
            provider->OnDataRemoved()->RemoveHandler(uintptr_t(this));
            provider->OnDataMoved()->RemoveHandler(uintptr_t(this));
        }
    });
}

bool CompositeObjectProvider::Build(const IMetadata::Ptr&)
{
    // todo: the iterates work only for flat container, we don't want to iterate inside children
    //  set such option for container when implemented,
    SetRequiredInterfaces({ IObjectProvider::UID });
    OnAdded()->AddHandler(MakeCallback<IOnChildChanged>(this, &CompositeObjectProvider::OnProviderAdded));
    OnRemoved()->AddHandler(MakeCallback<IOnChildChanged>(this, &CompositeObjectProvider::OnProviderRemoved));
    OnMoved()->AddHandler(MakeCallback<IOnChildMoved>(this, &CompositeObjectProvider::OnProviderMoved));

    META_ACCESS_PROPERTY(CacheHint)->OnChanged()->AddHandler(MakeCallback<IOnChanged>([&] {
        ForEachShared(GetSelf(), [&](const IObject::Ptr& obj) {
            if (auto provider = interface_cast<IObjectProvider>(obj)) {
                provider->CacheHint()->SetValue(CacheHint()->GetValue());
            }
        });
    }));
    return true;
}

IObject::Ptr CompositeObjectProvider::CreateObject(const DataModelIndex& index)
{
    size_t ni = index.Index();
    if (auto p = FindProvider(ni)) {
        if (auto o = p->CreateObject(DataModelIndex { ni, index.GetDimensionPointer() })) {
            objects_[o.get()] = p;
            return o;
        }
    }
    return nullptr;
}

bool CompositeObjectProvider::DisposeObject(const META_NS::IObject::Ptr& item)
{
    auto it = objects_.find(item.get());
    if (it == objects_.end()) {
        return false;
    }

    auto p = it->second;
    objects_.erase(it);
    return p->DisposeObject(item);
}

IObjectProvider* CompositeObjectProvider::FindProvider(size_t& index) const
{
    size_t curSize = 0;
    IObjectProvider* res {};

    IterateShared(GetSelf(), [&](const IObject::Ptr& obj) {
        if (auto provider = interface_cast<IObjectProvider>(obj)) {
            auto prevSize = curSize;
            curSize += provider->GetObjectCount();
            if (index < curSize) {
                index = index - prevSize;
                res = provider;
                return false;
            }
        }
        return true;
    });

    return res;
}

size_t CompositeObjectProvider::GetObjectCount(const DataModelIndex& index) const
{
    // is the size asked for other than first dimension?
    if (index.IsValid()) {
        size_t ni = index.Index();
        if (auto p = FindProvider(ni)) {
            return p->GetObjectCount(DataModelIndex { ni, index.GetDimensionPointer() });
        }
    }

    // calculate size for first dimension
    size_t res = 0;
    IterateShared(GetSelf(), [&](const IObject::Ptr& obj) {
        if (auto provider = interface_cast<IObjectProvider>(obj)) {
            res += provider->GetObjectCount();
        }
        return true;
    });
    return res;
}

size_t CompositeObjectProvider::CalculateIndex(const IObjectProvider::Ptr& provider, size_t localIndex) const
{
    size_t base = 0;
    bool ret = IterateShared(GetSelf(), [&](const IObject::Ptr& obj) {
        if (auto prov = interface_cast<IObjectProvider>(obj)) {
            if (prov == provider.get()) {
                return false;
            }
            base += prov->GetObjectCount();
        }
        return true;
    });
    return ret ? base + localIndex : -1;
}

void CompositeObjectProvider::OnAddedProviderData(
    const IObjectProvider::Ptr& provider, DataModelIndex base, size_t count)
{
    if (provider && base.IsValid()) {
        size_t index = CalculateIndex(provider, base.Index());
        if (index != -1) {
            META_ACCESS_EVENT(OnDataAdded)->Invoke(DataModelIndex { index, base.GetDimensionPointer() }, count);
        }
    }
}

void CompositeObjectProvider::OnRemovedProviderData(
    const IObjectProvider::Ptr& provider, DataModelIndex base, size_t count)
{
    if (provider && base.IsValid()) {
        size_t index = CalculateIndex(provider, base.Index());
        if (index != -1) {
            META_ACCESS_EVENT(OnDataRemoved)->Invoke(DataModelIndex { index, base.GetDimensionPointer() }, count);
        }
    }
}

void CompositeObjectProvider::OnMovedProviderData(
    const IObjectProvider::Ptr& provider, DataModelIndex from, size_t count, DataModelIndex to)
{
    if (provider && from.IsValid() && to.IsValid()) {
        size_t fromIndex = CalculateIndex(provider, from.Index());
        size_t toIndex = CalculateIndex(provider, to.Index());
        if (fromIndex != -1 && toIndex != -1) {
            META_ACCESS_EVENT(OnDataMoved)
                ->Invoke(DataModelIndex { fromIndex, from.GetDimensionPointer() }, count,
                DataModelIndex { toIndex, to.GetDimensionPointer() });
        }
    }
}

void CompositeObjectProvider::OnProviderAdded(const ChildChangedInfo& info)
{
    auto provider = interface_pointer_cast<IObjectProvider>(info.object);
    if (!provider) {
        return;
    }

    auto added = provider->OnDataAdded()->AddHandler(
        MakeCallback<IOnDataAdded>(
            [this](auto provider, DataModelIndex base, size_t count) { OnAddedProviderData(provider, base, count); },
            provider),
        uintptr_t(this));
    auto removed = provider->OnDataRemoved()->AddHandler(
        MakeCallback<IOnDataRemoved>(
            [this](auto provider, DataModelIndex base, size_t count) { OnRemovedProviderData(provider, base, count); },
            provider),
        uintptr_t(this));
    auto moved = provider->OnDataMoved()->AddHandler(
        MakeCallback<IOnDataMoved>([this](auto provider, DataModelIndex from, size_t count,
                                       DataModelIndex to) { OnMovedProviderData(provider, from, count, to); },
            provider),
        uintptr_t(this));

    provider->CacheHint()->SetValue(CacheHint()->GetValue());

    if (provider->GetObjectCount() > 0) {
        size_t index = CalculateIndex(provider, 0);
        if (index != -1) {
            META_ACCESS_EVENT(OnDataAdded)->Invoke(DataModelIndex { index }, provider->GetObjectCount());
        }
    }
}

size_t CompositeObjectProvider::CalculateIndexBase(size_t provider) const
{
    size_t index = 0;
    size_t base = 0;
    IterateShared(GetSelf(), [&](const IObject::Ptr& obj) {
        if (auto prov = interface_cast<IObjectProvider>(obj)) {
            if (++index > provider) {
                return false;
            }
            base += prov->GetObjectCount();
        }
        return true;
    });
    return base;
}

void CompositeObjectProvider::OnProviderRemoved(const ChildChangedInfo& info)
{
    auto provider = interface_pointer_cast<IObjectProvider>(info.object);
    if (!provider) {
        return;
    }

    provider->OnDataAdded()->RemoveHandler(uintptr_t(this));
    provider->OnDataRemoved()->RemoveHandler(uintptr_t(this));
    provider->OnDataMoved()->RemoveHandler(uintptr_t(this));

    if (provider->GetObjectCount() > 0) {
        size_t index = CalculateIndexBase(info.index);
        META_ACCESS_EVENT(OnDataRemoved)->Invoke(DataModelIndex { index }, provider->GetObjectCount());
    }
}

void CompositeObjectProvider::OnProviderMoved(const ChildMovedInfo& info)
{
    auto provider = interface_pointer_cast<IObjectProvider>(info.object);
    if (!provider || provider->GetObjectCount() == 0) {
        return;
    }

    size_t index = 0;
    size_t fromAdjust = info.to < info.from;
    size_t from = CalculateIndexBase(info.from + fromAdjust);
    if (fromAdjust) {
        // in case we moved it already before to the from-location, we need to remove it to get the previous state
        from -= provider->GetObjectCount();
    }

    auto to = CalculateIndex(provider, 0);
    if (to != -1) {
        // if the provider was before where it is currently, we need to add its content to get the previous state
        if (info.from < info.to) {
            to += provider->GetObjectCount();
        }
        META_ACCESS_EVENT(OnDataMoved)
            ->Invoke(DataModelIndex { from }, provider->GetObjectCount(), DataModelIndex { to });
    }
}

META_END_NAMESPACE()