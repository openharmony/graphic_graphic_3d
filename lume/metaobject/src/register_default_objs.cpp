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
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_future.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>

#include "attachment_container.h"
#include "base_object.h"
#include "connector.h"
#include "container/object_container.h"
#include "container/object_flat_container.h"
#include "container_observer.h"
#include "engine/engine_input_property_manager.h"
#include "functions.h"
#include "loaders/class_content_loader.h"
#include "loaders/csv_string_resource_loader.h"
#include "loaders/json_content_loader.h"
#include "meta_object.h"
#include "model/composite_object_provider.h"
#include "model/container_data_model.h"
#include "model/content_loader_object_provider.h"
#include "model/instantiating_object_provider.h"
#include "number.h"
#include "object.h"
#include "object_context.h"
#include "object_hierarchy_observer.h"
#include "property/bind.h"
#include "proxy_object.h"
#include "serialization/backend/debug_output.h"
#include "serialization/backend/json_input.h"
#include "serialization/backend/json_output.h"
#include "serialization/exporter.h"
#include "serialization/importer.h"
#include "serialization/json_exporter.h"
#include "serialization/json_importer.h"
#include "serialization/ser_nodes.h"
#include "startable_object_controller.h"

META_BEGIN_NAMESPACE()

IObjectFactory::Ptr GetManualClockFactory();
IObjectFactory::Ptr GetSystemClockFactory();

namespace Internal {

IObjectFactory::Ptr GetPollingTaskQueueFactory();
IObjectFactory::Ptr GetThreadedTaskQueueFactory();
IObjectFactory::Ptr GetPromiseFactory();
IObjectFactory::Ptr GetContentObjectFactory();

static constexpr ObjectTypeInfo OBJECTS[] = { MetaObject::OBJECT_INFO, Object::OBJECT_INFO,
    ObjectContainer::OBJECT_INFO, ObjectFlatContainer::OBJECT_INFO, ProxyObject::OBJECT_INFO,
    ContainerObserver::OBJECT_INFO, SettableFunction::OBJECT_INFO, PropertyFunction::OBJECT_INFO,
    ObjectContext::OBJECT_INFO, AttachmentContainer::OBJECT_INFO, ContainerDataModel::OBJECT_INFO,
    CompositeObjectProvider::OBJECT_INFO, InstantiatingObjectProvider::OBJECT_INFO,
    ContentLoaderObjectProvider::OBJECT_INFO, CsvStringResourceLoader::OBJECT_INFO, ClassContentLoader::OBJECT_INFO,
    ObjectHierarchyObserver::OBJECT_INFO, StartableObjectController::OBJECT_INFO, Internal::Number::OBJECT_INFO,
    Connector::OBJECT_INFO, JsonContentLoader::OBJECT_INFO, EngineValueManager::OBJECT_INFO,
    EngineInputPropertyManager::OBJECT_INFO };

void RegisterBuiltInObjects(IObjectRegistry& registry)
{
    registry.RegisterObjectType<META_NS::BaseObject>();
    for (auto& t : OBJECTS) {
        registry.RegisterObjectType(t.GetFactory());
    }
    registry.RegisterObjectType(GetManualClockFactory());
    registry.RegisterObjectType(GetSystemClockFactory());
    registry.RegisterObjectType(GetPollingTaskQueueFactory());
    registry.RegisterObjectType(GetThreadedTaskQueueFactory());
    registry.RegisterObjectType(GetPromiseFactory());
    registry.RegisterObjectType(GetContentObjectFactory());
    registry.RegisterObjectType<Serialization::Exporter>();
    registry.RegisterObjectType<Serialization::JsonExporter>();
    registry.RegisterObjectType<Serialization::Importer>();
    registry.RegisterObjectType<Serialization::JsonImporter>();
    registry.RegisterObjectType<Serialization::DebugOutput>();
    registry.RegisterObjectType<Serialization::JsonOutput>();
    registry.RegisterObjectType<Serialization::JsonInput>();
    registry.RegisterObjectType<Serialization::NilNode>();
    registry.RegisterObjectType<Serialization::MapNode>();
    registry.RegisterObjectType<Serialization::ArrayNode>();
    registry.RegisterObjectType<Serialization::ObjectNode>();
    registry.RegisterObjectType<Serialization::RootNode>();
    registry.RegisterObjectType<Serialization::BoolNode>();
    registry.RegisterObjectType<Serialization::IntNode>();
    registry.RegisterObjectType<Serialization::UIntNode>();
    registry.RegisterObjectType<Serialization::DoubleNode>();
    registry.RegisterObjectType<Serialization::StringNode>();
    registry.RegisterObjectType<Serialization::RefNode>();
    registry.RegisterObjectType<Bind>();
}

void UnRegisterBuiltInObjects(IObjectRegistry& registry)
{
    registry.UnregisterObjectType<Bind>();
    registry.UnregisterObjectType<Serialization::NilNode>();
    registry.UnregisterObjectType<Serialization::MapNode>();
    registry.UnregisterObjectType<Serialization::ArrayNode>();
    registry.UnregisterObjectType<Serialization::ObjectNode>();
    registry.UnregisterObjectType<Serialization::RootNode>();
    registry.UnregisterObjectType<Serialization::BoolNode>();
    registry.UnregisterObjectType<Serialization::IntNode>();
    registry.UnregisterObjectType<Serialization::UIntNode>();
    registry.UnregisterObjectType<Serialization::DoubleNode>();
    registry.UnregisterObjectType<Serialization::StringNode>();
    registry.UnregisterObjectType<Serialization::RefNode>();
    registry.UnregisterObjectType<Serialization::NilNode>();
    registry.UnregisterObjectType<Serialization::JsonInput>();
    registry.UnregisterObjectType<Serialization::JsonOutput>();
    registry.UnregisterObjectType<Serialization::DebugOutput>();
    registry.UnregisterObjectType<Serialization::JsonImporter>();
    registry.UnregisterObjectType<Serialization::Importer>();
    registry.UnregisterObjectType<Serialization::JsonExporter>();
    registry.UnregisterObjectType<Serialization::Exporter>();
    registry.UnregisterObjectType(GetContentObjectFactory());
    registry.UnregisterObjectType(GetManualClockFactory());
    registry.UnregisterObjectType(GetSystemClockFactory());
    registry.UnregisterObjectType(GetPollingTaskQueueFactory());
    registry.UnregisterObjectType(GetThreadedTaskQueueFactory());
    registry.UnregisterObjectType(GetPromiseFactory());
    for (auto& t : OBJECTS) {
        registry.UnregisterObjectType(t.GetFactory());
    }
    registry.UnregisterObjectType<META_NS::BaseObject>();
}
} // namespace Internal
META_END_NAMESPACE()
