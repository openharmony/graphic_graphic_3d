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

#ifndef META_INTERFACE_IOWNER_H
#define META_INTERFACE_IOWNER_H

#include <meta/base/interface_macros.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

class IProperty;
struct StaticMetadata;

/// Generic owner concept interface
class IOwner : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IOwner, "ee3ddb5d-e749-4d4b-b3ae-3463cf5c492e")
public:
};

class IMetadataOwner : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IMetadataOwner, "66ce57ab-d234-47c0-b358-536609ac7e78")
public:
    virtual void OnMetadataConstructed(const StaticMetadata&, CORE_NS::IInterface&) = 0;
};

/// Property owner interface
class IPropertyOwner : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyOwner, "59f14ea1-972c-4d3b-81f1-3d26633fa864")
public:
    /// Called directly when owned property changes without need to register handlers
    virtual void OnPropertyChanged(const IProperty&) = 0;
};

META_END_NAMESPACE()

#endif
