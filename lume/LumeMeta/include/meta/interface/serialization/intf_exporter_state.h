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

#ifndef META_INTERFACE_EXPORTER_STATE_H
#define META_INTERFACE_EXPORTER_STATE_H

#include <core/resources/intf_resource_manager.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/serialization/intf_exporter.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Interface to access some of the exporter state and auxiliary objects
 */
class IExporterState : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExporterState, "34783a70-1c28-47b9-8a54-b1692f5df886")
public:
    virtual ExportOptions GetOptions() const = 0;
    virtual bool HasBeenExported(const InstanceId& id) const = 0;
    virtual bool IsGlobalObject(const InstanceId& id) const = 0;
    /// Convert current instance id to serialisable instance id
    virtual InstanceId ConvertInstanceId(const InstanceId& id) const = 0;

    virtual CORE_NS::IResourceManager::Ptr GetResourceManager() const = 0;
    virtual META_NS::IObject::Ptr GetUserContext() const = 0;
};

META_INTERFACE_TYPE(META_NS::IExporterState)

META_END_NAMESPACE()

#endif
