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
    META_PROPERTY(ObjectId, ClassId);
};

META_END_NAMESPACE()

#endif
