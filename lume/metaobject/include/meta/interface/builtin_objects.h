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

#ifndef META_INTERFACE_BUILTIN_OBJECTS_H
#define META_INTERFACE_BUILTIN_OBJECTS_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

namespace GlobalObjectInstance {
/** Instance id of the default object context instance */
[[maybe_unused]] constexpr static BASE_NS::Uid DEFAULT_OBJECT_CONTEXT { "3fe40d0f-4ec5-455c-a66d-dcd749b08194" };
} // namespace GlobalObjectInstance

/** Base objects */
META_REGISTER_CLASS(BaseObject, "211f0da1-1a09-422c-8dd0-fbc493a72d08", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MetaObject, "10d92222-5b93-4380-9fb9-f839b63d1b4b", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(Object, "b2377edc-27ec-4aa1-b071-636d78db63e2", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ObjectContext, "22b552eb-2caa-4b8a-9983-9817ab9fa92d", ObjectCategoryBits::INTERNAL)
META_REGISTER_CLASS(ProxyObject, "bd2164e1-b1b6-472a-8783-7787c7de9b36", ObjectCategoryBits::INTERNAL)
META_REGISTER_CLASS(ObjectContainer, "e25ec91e-f5cb-4afa-b277-a16c19da2e4a", ObjectCategoryBits::CONTAINER)
META_REGISTER_CLASS(ObjectFlatContainer, "30f501c2-7b62-44a4-911b-9b0ff2c850c4", ObjectCategoryBits::CONTAINER)
META_REGISTER_CLASS(AttachmentContainer, "d03551ab-8ef4-4b0b-98ba-f0edd2db6ba2", ObjectCategoryBits::INTERNAL)
META_REGISTER_CLASS(ContainerObserver, "a13844ca-9830-4dd2-8ecc-625563bcc5b0", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ObjectHierarchyObserver, "48cb1bb4-dd0a-44a5-a530-c2073e3a70df", ObjectCategoryBits::NO_CATEGORY)

/** String pool */
META_REGISTER_CLASS(StringPool, "8fb945d5-a73e-44c5-be0e-c6f518900aef", ObjectCategoryBits::NO_CATEGORY)

/** Json serializer */
META_REGISTER_CLASS(JsonSerializer, "71c930c7-91ad-45a5-8d47-044d70316ea5", ObjectCategoryBits::NO_CATEGORY);

META_REGISTER_CLASS(SystemClock, "96a7dc3c-7532-4a0e-b6b1-3a406947419c", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ManualClock, "7069eb20-4fe8-4c78-8d98-9a5938b09170", ObjectCategoryBits::NO_CATEGORY)

/** Property validators */
META_REGISTER_CLASS(ClampIntegerValidator, "3b8278a3-e50b-4e56-8cc1-990370ed5aa7", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Implementation of data model that implements IContainer interface.
 */
META_REGISTER_CLASS(ContainerDataModel, "84911f22-92ad-47fe-9399-cc0e9222affa", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Implementation that instantiates objects with given class uid and sets properties from the data for this
 * object Note: "Model." is added to the property names of data when added to the created object instance. CreateObject:
 * Creates object by giving metadata from the model as Build argument. If object implements IRecyclable, Rebuild is
 * being called with the metadata. Otherwise matching properties are added to the object and those bind to the original
 * data. DisposeObject: If object implements IRecyclable, Dispose is called, otherwise all properties without NATIVE
 * flag are reset. Recycles objects based on the cache hint.
 */
META_REGISTER_CLASS(
    InstantiatingObjectProvider, "208fc857-2d5c-4eb9-98e4-de45eba5e2ca", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Implements object provider that uses content loader interface to create the objects.
 */
META_REGISTER_CLASS(
    ContentLoaderObjectProvider, "f5ab9348-43cf-4154-bdd6-11d7ed6082d6", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Implementation that provides view over many IObjectProviders as single IObjectProvider. Implements IContainer.
 */
META_REGISTER_CLASS(CompositeObjectProvider, "f0eaec42-f6b2-43f0-b487-52ddbda305bb", ObjectCategoryBits::NO_CATEGORY)

/** Content loaders */
META_REGISTER_CLASS(CsvStringResourceLoader, "2e264ba2-a9a3-4f95-bac6-339b6c3ea033", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(JsonContentLoader, "b9d3860f-2a7e-4ac0-8140-542aea4a5e09", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ClassContentLoader, "5d2bebca-f12c-11ec-8ea0-0242ac120002", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Default Event <-> Meta function connector object.
 */
META_REGISTER_CLASS(Connector, "5b643aaa-c6f2-4839-b145-3afcb04d4125", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Function that refers to IObject's meta-function by name.
 */
META_REGISTER_CLASS(SettableFunction, "9a3c3b9d-406c-487f-87c5-184c4b80724a", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Function that returns value of a property.
 */
META_REGISTER_CLASS(PropertyFunction, "d458e645-c83a-4cb6-be3d-2bc1720b384f", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief Object that contains another object, the searchability and serialisation for the contained object can be
 * controlled.
 */
META_REGISTER_CLASS(ContentObject, "a1e95c16-99df-42d9-bc94-600df414a43b", ObjectCategoryBits::NO_CATEGORY)

/**
 * @brief An object which can be used to control all IStartable instances that are (or become) part of an
 *        object hierarchy.
 */
META_REGISTER_CLASS(StartableObjectController, "8e6748be-3ff3-4b2a-8f7e-7b2d3fa2fc94", ObjectCategoryBits::NO_CATEGORY)

META_REGISTER_CLASS(Exporter, "143ae57a-fb57-4130-ae7c-ae3aa4b56495", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(Importer, "4fbc30f7-2225-4f2a-8dc4-7d23bdb7c782", ObjectCategoryBits::NO_CATEGORY)

META_REGISTER_CLASS(JsonExporter, "fe5d68ca-334a-44e0-b8c0-9d44bf2dc9a6", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(JsonImporter, "80f8cd0b-c5a9-4f19-bc36-5c83eb2926b4", ObjectCategoryBits::NO_CATEGORY)

META_REGISTER_CLASS(DebugOutput, "cfb42445-363f-40d2-a231-1623b8028392", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(JsonOutput, "dd4f3444-bc9d-436b-90c7-312793dc38f2", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(JsonInput, "0b4587d2-af0d-4c67-bd6b-0386fa2de094", ObjectCategoryBits::NO_CATEGORY)

META_REGISTER_CLASS(RootNode, "776bd8ab-a135-4108-bae3-233ce615b900", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(NilNode, "776bd8ab-a135-4108-bae3-233ce615b901", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ObjectNode, "776bd8ab-a135-4108-bae3-233ce615b902", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ArrayNode, "776bd8ab-a135-4108-bae3-233ce615b903", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(MapNode, "776bd8ab-a135-4108-bae3-233ce615b904", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(BoolNode, "776bd8ab-a135-4108-bae3-233ce615b905", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(IntNode, "776bd8ab-a135-4108-bae3-233ce615b906", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(UIntNode, "776bd8ab-a135-4108-bae3-233ce615b907", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(DoubleNode, "776bd8ab-a135-4108-bae3-233ce615b909", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(StringNode, "776bd8ab-a135-4108-bae3-233ce615b90a", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(RefNode, "776bd8ab-a135-4108-bae3-233ce615b90b", ObjectCategoryBits::NO_CATEGORY)

META_REGISTER_CLASS(Bind, "27513ec5-8415-4491-b3f4-51205936cc5d", ObjectCategoryBits::NO_CATEGORY)

// engine support
META_REGISTER_CLASS(EngineValueManager, "a63723d0-7755-4ca3-a9d6-983df6e12cff", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(EngineInputPropertyManager, "10a81469-07c2-4cab-b8a5-3249e415cb5a", ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

#endif
