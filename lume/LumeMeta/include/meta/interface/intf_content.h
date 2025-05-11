/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Content interface
 * Author: Lauri Jaaskela
 * Create: 2022-05-16
 */

#ifndef META_INTERFACE_ICONTENT_H
#define META_INTERFACE_ICONTENT_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/loaders/intf_content_loader.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IContent, "837cb3bf-df9d-4585-9293-82e782778db3")

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
};

META_END_NAMESPACE()

#endif
