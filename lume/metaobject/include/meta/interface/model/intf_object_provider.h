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

#ifndef META_INTERFACE_MODEL_IOBJECT_PROVIDER_H
#define META_INTERFACE_MODEL_IOBJECT_PROVIDER_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/loaders/intf_content_loader.h>
#include <meta/interface/model/intf_data_model.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Generic object provider, creates objects based on model data,
 */
class IObjectProvider : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IObjectProvider, "11a75ee4-c661-4a25-816a-b8fbfc4c4497")
public:
    /**
     * @brief Create object from model data at give location or null if not possible,
     */
    virtual IObject::Ptr CreateObject(const DataModelIndex& index) = 0;
    /**
     * @brief Dispose object that was created with CreateObject,
     * Note: This allows the implementation to break bindings and/or recycle objects.
     * @return true if the object is being reused.
     */
    virtual bool DisposeObject(const IObject::Ptr& item) = 0;

    /**
     * @brief Get count of objects in given dimension at given index.
     */
    virtual size_t GetObjectCount(const DataModelIndex& index) const = 0;

    /**
     * @brief Get count of objects in first dimension.
     */
    size_t GetObjectCount() const
    {
        return GetObjectCount(DataModelIndex {});
    }

    /**
     * @brief Event when data is added to the underlying model.
     */
    META_EVENT(IOnDataAdded, OnDataAdded)
    /**
     * @brief Event when data is removed from the underlying model.
     */
    META_EVENT(IOnDataRemoved, OnDataRemoved)
    /**
     * @brief Event when data is moved in the underlying model.
     */
    META_EVENT(IOnDataMoved, OnDataMoved)
    /**
     * @brief Tell a hint to the implementation how many objects it should cache for reuse.
     */
    META_PROPERTY(size_t, CacheHint);
};

/**
 * @brief Interface to set data model which is used for objects.
 */
class IModelObjectProvider : public IObjectProvider {
    META_INTERFACE(IObjectProvider, IModelObjectProvider, "e68b0f51-e66d-4f33-a7a2-b9b6dcf2bfe8")
public:
    /**
     * @brief Set data model from where the data is queried for created object.
     * @return true on success.
     */
    virtual bool SetDataModel(const IDataModel::Ptr& model) = 0;
    /**
     * @brief Get previously set data model.
     */
    virtual IDataModel::Ptr GetDataModel() const = 0;
};

/**
 * @brief Interface to set object's class uid which the IObjectProvider's CreateObject creates.
 *        Uses ObjectRegistry to instantiate the given type when needed.
 */
class IInstantiatingObjectProvider : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInstantiatingObjectProvider, "d6996e21-0f4d-4207-9c71-d7a3f141f566")
public:
    /**
     * @brief Set object's class uid which the IObjectProvider's CreateObject creates.
     * @return true on success.
     */
    virtual bool SetObjectClassId(const ObjectId& id) = 0;
};

/**
 * @brief Interface to set content loader which the IObjectProvider's CreateObject uses.
 */
class IContentLoaderObjectProvider : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContentLoaderObjectProvider, "d5f6507d-c823-436c-b45c-8e30ea9217e9")
public:
    /**
     * @brief Set content loader which the IObjectProvider's CreateObject uses.
     * @return true on success.
     */
    virtual bool SetContentLoader(const IContentLoader::Ptr& loader) = 0;
};

META_END_NAMESPACE()

#endif
