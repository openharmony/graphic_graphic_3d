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
#ifndef META_API_MODEL_INSTANTIATING_OBJECT_PROVIDER_H
#define META_API_MODEL_INSTANTIATING_OBJECT_PROVIDER_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/model/intf_object_provider.h>

META_BEGIN_NAMESPACE()

/*
 * @brief Wrapper for META_NS::ClassId::InstantiatingObjectProvider which is implementation of object provider
 * that will instantiate objects when needed.
 */
class InstantiatingObjectProvider final
    : public Internal::ObjectInterfaceAPI<InstantiatingObjectProvider, ClassId::InstantiatingObjectProvider> {
    META_API(InstantiatingObjectProvider)
    META_API_OBJECT_CONVERTIBLE(IObjectProvider)

    META_API_CACHE_INTERFACE(IModelObjectProvider, Provider)
    META_API_CACHE_INTERFACE(IInstantiatingObjectProvider, InstProvider)

public:
    META_API_INTERFACE_PROPERTY_CACHED(Provider, CacheHint, size_t)

    /** See IInstantiatingObjectProvider::SetObjectClassUid */
    InstantiatingObjectProvider& ObjectClassUid(const ObjectId& id)
    {
        META_API_CACHED_INTERFACE(InstProvider)->SetObjectClassId(id);
        return *this;
    }

    /** See IModelObjectProvider::SetDataModel */
    InstantiatingObjectProvider& DataModel(const IDataModel::Ptr& model)
    {
        META_API_CACHED_INTERFACE(Provider)->SetDataModel(model);
        return *this;
    }

    /** See IModelObjectProvider::GetDataModel */
    IDataModel::Ptr GetDataModel() const
    {
        return META_API_CACHED_INTERFACE(Provider)->GetDataModel();
    }
};

META_END_NAMESPACE()

#endif
