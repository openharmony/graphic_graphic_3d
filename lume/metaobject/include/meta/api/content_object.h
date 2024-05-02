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

#ifndef META_API_CONTENT_OBJECT_H
#define META_API_CONTENT_OBJECT_H

#include <meta/api/property/binding.h>
#include <meta/interface/intf_content.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_required_interfaces.h>

#include "object.h"

META_BEGIN_NAMESPACE()

class ContentObject final : public Internal::ObjectInterfaceAPI<ContentObject, ClassId::ContentObject> {
    META_API(ContentObject)
    META_API_OBJECT_CONVERTIBLE(IContent)
    META_API_CACHE_INTERFACE(IContent, Content)
    META_API_OBJECT_CONVERTIBLE(IRequiredInterfaces)
    META_API_CACHE_INTERFACE(IRequiredInterfaces, RequiredInterfaces)
public:
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Content, Content, IObject::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(Content, ContentSearchable, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Content, ContentLoader, IContentLoader::Ptr)

    ContentObject& SerializeContent(bool serialize)
    {
        SetObjectFlags(ObjectRef(), ObjectFlagBits::SERIALIZE_HIERARCHY, serialize);
        return *this;
    }
    bool SetContent(const IObject::Ptr& content)
    {
        return META_API_CACHED_INTERFACE(Content)->SetContent(content);
    }
    bool SetRequiredInterfaces(const BASE_NS::vector<TypeId>& interfaces)
    {
        return META_API_CACHED_INTERFACE(RequiredInterfaces)->SetRequiredInterfaces(interfaces);
    }
    BASE_NS::vector<TypeId> GetRequiredInterfaces() const
    {
        return META_API_CACHED_INTERFACE(RequiredInterfaces)->GetRequiredInterfaces();
    }
};

META_END_NAMESPACE()

#endif
