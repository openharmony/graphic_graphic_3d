/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Class content loader interface
 * Author: Lauri Jaaskela
 * Create: 2022-06-21
 */

#ifndef META_INTERFACE_LOADERS_CLASS_CONTENT_LOADER_H
#define META_INTERFACE_LOADERS_CLASS_CONTENT_LOADER_H

#include <meta/interface/interface_macros.h>

#include "intf_dynamic_content_loader.h"

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IClassContentLoader, "1a48a02c-f12d-11ec-8ea0-0242ac120002")

/**
 * @brief The IClassContentLoader interface defines an interface for content loaders
 *        which load their content based on a class id.
 */
class IClassContentLoader : public IDynamicContentLoader {
    META_INTERFACE(IDynamicContentLoader, IClassContentLoader)
public:
    /**
     * @brief Class id of the object to load.
     */
    META_PROPERTY(ObjectId, ClassId)
};

META_END_NAMESPACE()

#endif
