/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Dynamic content loader interface
 * Author: Lauri Jaaskela
 * Create: 2022-05-30
 */

#ifndef META_INTERFACE_LOADERS_DYNAMIC_CONTENT_LOADER_H
#define META_INTERFACE_LOADERS_DYNAMIC_CONTENT_LOADER_H

#include <meta/interface/interface_macros.h>
#include <meta/interface/property/property_events.h>

#include "intf_content_loader.h"

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IDynamicContentLoader, "273e318f-6edc-423c-b27d-f842c48cac8d")

/**
 * @brief The IDynamicContentLoader interface defines an interface for content loaders
 *        whose content can change at runtime.
 *
 *        For example, a content loader which loads its content from a file can implement
 *        IDynamicContentLoader to inform its users of a change in the content returned
 *        by Create() call.
 *
 */
class IDynamicContentLoader : public IContentLoader {
    META_INTERFACE(IContentLoader, IDynamicContentLoader)
public:
    /**
     * @brief ContentChanged event is invoked when the content created by this content loader
     *        has changed.
     *
     *        If the users want to stay up to date, they should call Create() again after
     *        ContentChanged event has been invoked.
     */
    META_EVENT(IOnChanged, ContentChanged)

    /**
     * @brief Reload content loaded by this content loader. Causes ContentChanged event to
     *        be invoked.
     */
    virtual void Reload() = 0;
};

META_END_NAMESPACE()

#endif
