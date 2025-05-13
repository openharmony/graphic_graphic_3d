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

#ifndef META_INTERFACE_ISTATIC_METADATA_H
#define META_INTERFACE_ISTATIC_METADATA_H

#include <meta/base/interface_macros.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

/// Interface to get registered static metadata for object type
class IStaticMetadata : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStaticMetadata, "de9eee18-b2cc-4060-8bd8-5fabd3457e59")
public:
    /// Get static metadata
    virtual const StaticObjectMetadata* GetStaticMetadata() const = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IStaticMetadata)

#endif
