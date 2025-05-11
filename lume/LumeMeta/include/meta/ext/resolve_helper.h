/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Helper function for implementing Resolve functions
 * Author: Mikael Kilpeläinen
 * Create: 2022-11-22
 */

#ifndef META_EXT_RESOLVE_HELPER_H
#define META_EXT_RESOLVE_HELPER_H

#include <meta/base/ref_uri.h>
#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

inline bool CheckValidResolve(const IObjectInstance::Ptr& base, const RefUri& uri)
{
    if (!base || !uri.IsValid()) {
        return false;
    }
    if (uri.BaseObjectUid() != BASE_NS::Uid {} && InstanceId(uri.BaseObjectUid()) != base->GetInstanceId()) {
        return false;
    }
    return true;
}

META_END_NAMESPACE()

#endif
