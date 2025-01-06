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

#ifndef META_INTERFACE_RESOURCE_IRESOURCE_H
#define META_INTERFACE_RESOURCE_IRESOURCE_H

#include <core/plugin/intf_interface.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Tag interface for resources
 */
class IResource : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IResource, "1d2aa2c6-7a08-49bf-a360-aecf201a8681")
public:
};

META_INTERFACE_TYPE(META_NS::IResource)

META_END_NAMESPACE()

#endif
