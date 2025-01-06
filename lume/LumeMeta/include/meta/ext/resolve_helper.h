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
