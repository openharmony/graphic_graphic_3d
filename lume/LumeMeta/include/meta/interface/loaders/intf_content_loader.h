/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Content loader interface
 * Author: Lauri Jaaskela
 * Create: 2022-05-19
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
