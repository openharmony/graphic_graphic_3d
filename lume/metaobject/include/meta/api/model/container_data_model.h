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

#ifndef META_API_MODEL_CONTAINER_DATA_MODEL_H
#define META_API_MODEL_CONTAINER_DATA_MODEL_H

#include <meta/api/internal/object_api.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/model/intf_data_model.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Wrapper for META_NS::ClassId::ContainerDataModel which provides data model interface for container.
 */
class ContainerDataModel : public Internal::ObjectInterfaceAPI<ContainerDataModel, ClassId::ContainerDataModel> {
    META_API(ContainerDataModel)

    META_API_OBJECT_CONVERTIBLE(IDataModel)
    META_API_OBJECT_CONVERTIBLE(IContainer)

    META_API_CACHE_INTERFACE(IDataModel, DataModel)
    META_API_CACHE_INTERFACE(IContainer, Container)

public:
    /** See IContainer::Add */
    ContainerDataModel& Record(IMetadata::Ptr record)
    {
        META_API_CACHED_INTERFACE(Container)->Add(BASE_NS::move(record));
        return *this;
    }

    /** See IContainer::Remove */
    ContainerDataModel& Remove(size_t index)
    {
        META_API_CACHED_INTERFACE(Container)->Remove(index);
        return *this;
    }

    /** See IContainer::Insert */
    ContainerDataModel& Insert(size_t index, IMetadata::Ptr record)
    {
        META_API_CACHED_INTERFACE(Container)->Insert(index, BASE_NS::move(record));
        return *this;
    }
};

META_END_NAMESPACE()

#endif
