/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Serialization reference uri util
 * Author: Mikael Kilpeläinen
 * Create: 2022-11-22
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
