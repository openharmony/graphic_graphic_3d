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

#ifndef META_INTERFACE_LOADERS_CONTENT_LOADER_H
#define META_INTERFACE_LOADERS_CONTENT_LOADER_H

#include <meta/base/meta_types.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IContentLoader, "ad908b91-aab9-4962-a426-6fcecbf21261")

/**
 * @brief The IContentLoader interface defines an interface for objects which
 *        can be used to create other objects.
 *
 *        The most common use case is to use an IContentLoader object to automatically
 *        create ContentWidget content by setting the IContent::ContentLoader property.
 */
class IContentLoader : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContentLoader)
public:
    /**
     * @brief Creates a new instance of the content this content loader can create.
     * @param params Loader-specific parameters used for object creation.
     */
    virtual IObject::Ptr Create(const IObject::Ptr& params) = 0;
};

META_END_NAMESPACE()

META_TYPE(META_NS::IContentLoader::Ptr)

#endif
