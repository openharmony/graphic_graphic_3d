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

#ifndef META_SRC_REF_URI_UTIL_H
#define META_SRC_REF_URI_UTIL_H

#include <base/util/uid_util.h>

#include <meta/base/ref_uri.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

IObject::Ptr DefaultResolveObject(const IObjectInstance::Ptr& base, const RefUri& uri);

META_END_NAMESPACE()

#endif
