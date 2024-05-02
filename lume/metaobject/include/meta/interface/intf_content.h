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

#ifndef META_INTERFACE_ICONTENT_H
#define META_INTERFACE_ICONTENT_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/loaders/intf_content_loader.h>

META_BEGIN_NAMESPACE()

REGISTER_INTERFACE(IContent, "837cb3bf-df9d-4585-9293-82e782778db3")

/**
 * @brief The IContent interface defines an interface which can be implemented by objects
 *        that contain an internal object hierarchy which should not be visible in the
 *        application object hierarchy.
 */
class IContent : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IContent)
public:
    /**
     * @brief The object content. The implementer forwards all appropriate calls
     *        to content, which means that in practise the object set to Content will
     *        behave as it would be a child of the object implementing IContent.
     *
     *        The difference is that the object hierarchy under Content is not available
     *        as a child of the object.
     * @note  The implementer shall use IObjectFlags::SERIALIZE_HIERARCHY flag to determine
     *        if the Content property should be serialized when exporting the object. By default
     *        IContent implementations should not serialize their content.
     */
    META_READONLY_PROPERTY(IObject::Ptr, Content)
    /**
     * @brief Set value of Content property. This will also reset the ContentLoader property to null.
     * @param content The object to set as content.
     * @return True if content was successfully set, false otherwise.
     * @note This function can fail e.g. if the implementer also implements IRequiredInterfaces
     *       and the content does not fulfil interface requirements.
     */
    virtual bool SetContent(const IObject::Ptr& content) = 0;
    /**
     * @brief If true, the object hierarchy contained within the Content property should be
     *        searched when finding items from a object hierarchy (see IContainer::Find* methods).
     *        When false, the content hierarchy should be considered internal and not traversed
     *        by external users.
     *
     *        The default value is true.
     */
    META_PROPERTY(bool, ContentSearchable)
    /**
     * @brief If set, the Content property should be automatically populated by using the
     *        specified IContentLoader.
     */
    META_PROPERTY(META_NS::IContentLoader::Ptr, ContentLoader)
};

META_END_NAMESPACE()

#endif
