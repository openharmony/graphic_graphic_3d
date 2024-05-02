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

#ifndef META_SRC_OBJECT_REGISTRY_H
#define META_SRC_OBJECT_REGISTRY_H

#include <atomic>
#include <shared_mutex>

#include <base/containers/unordered_map.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/intf_interpolator.h>
#include <meta/interface/engine/intf_engine_data.h>
#include <meta/interface/intf_call_context.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "class_registry.h"

META_BEGIN_NAMESPACE()

class IRandom {
public:
    virtual ~IRandom() = default;
    virtual void Initialize(uint64_t seed) = 0;
    virtual uint64_t GetRandom() = 0;
};

class ObjectRegistry final : public IObjectRegistry,
                             public ITaskQueueRegistry,
                             public IPropertyRegister,
                             public IGlobalSerializationData,
                             public IEngineData {
public:
    META_NO_COPY_MOVE(ObjectRegistry)

    ObjectRegistry();
    ~ObjectRegistry() override;

    IClassRegistry& GetClassRegistry() override;

    bool RegisterObjectType(const IClassInfo::Ptr& classInfo) override;
    bool UnregisterObjectType(const IClassInfo::Ptr& classInfo) override;

    IObject::Ptr Create(ObjectId uid, const CreateInfo& createInfo, const IMetadata::Ptr& data) const override;
    IObject::Ptr Create(ObjectId uid, const CreateInfo& createInfo) const override;
    IObject::Ptr Create(const ClassInfo& info, const CreateInfo& createInfo) const override;

    IObjectFactory::ConstPtr GetObjectFactory(const ObjectId& uid) const override;
    BASE_NS::vector<ObjectCategoryItem> GetAllCategories() const override;
    BASE_NS::vector<IClassInfo::ConstPtr> GetAllTypes(
        ObjectCategoryBits category, bool strict, bool excludeDeprecated) const override;

    IObject::Ptr GetObjectInstanceByInstanceId(InstanceId uid) const override;
    BASE_NS::vector<IObject::Ptr> GetAllObjectInstances() const override;
    BASE_NS::vector<IObject::Ptr> GetAllSingletonObjectInstances() const override;
    BASE_NS::vector<IObject::Ptr> GetObjectInstancesByCategory(ObjectCategoryBits category, bool strict) const override;

    BASE_NS::string ExportToString(const IObjectRegistryExporter::Ptr& exporter) const override;
    IObjectContext::Ptr GetDefaultObjectContext() const override;

    void Purge() override;
    void DisposeObject(const InstanceId&) const override;
    IMetadata::Ptr ConstructMetadata() const override;
    ICallContext::Ptr ConstructDefaultCallContext() const override;

    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    IPropertyRegister& GetPropertyRegister() override;
    IProperty::Ptr Create(const ObjectId& object, BASE_NS::string_view name) const override;
    IBind::Ptr CreateBind() const override;
    IAny& InvalidAny() const override;
    IAny::Ptr ConstructAny(const ObjectId& id) const override;
    bool IsAnyRegistered(const ObjectId& id) const override;
    void RegisterAny(BASE_NS::shared_ptr<AnyBuilder> builder) override;
    void UnregisterAny(const ObjectId& id) override;

    // Interpolators
    void RegisterInterpolator(TypeId propertyTypeUid, BASE_NS::Uid interpolatorClassUid) override;
    void UnregisterInterpolator(TypeId propertyTypeUid) override;
    bool HasInterpolator(TypeId propertyTypeUid) const override;
    IInterpolator::Ptr CreateInterpolator(TypeId propertyTypeUid) override;

    IGlobalSerializationData& GetGlobalSerializationData() override;
    SerializationSettings GetDefaultSettings() const override;
    void SetDefaultSettings(const SerializationSettings& settings) override;
    void RegisterGlobalObject(const IObject::Ptr& object) override;
    void UnregisterGlobalObject(const IObject::Ptr& object) override;
    IObject::Ptr GetGlobalObject(const InstanceId& id) const override;
    void RegisterValueSerializer(const IValueSerializer::Ptr&) override;
    void UnregisterValueSerializer(const TypeId& id) override;
    IValueSerializer::Ptr GetValueSerializer(const TypeId& id) const override;

    IEngineInternalValueAccess::Ptr GetInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) const override;
    void RegisterInternalValueAccess(const CORE_NS::PropertyTypeDecl& type, IEngineInternalValueAccess::Ptr) override;
    void UnregisterInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) override;
    IEngineData& GetEngineData() override;

protected:
    void Ref() override;
    void Unref() override;

protected: // ITaskQueueRegistry
    ITaskQueue::Ptr GetTaskQueue(const BASE_NS::Uid& queueId) const override;
    bool RegisterTaskQueue(const ITaskQueue::Ptr& queue, const BASE_NS::Uid& queueId) override;
    bool UnregisterTaskQueue(const BASE_NS::Uid& queueId) override;
    bool HasTaskQueue(const BASE_NS::Uid& queueId) const override;
    bool UnregisterAllTaskQueues() override;
    ITaskQueue::Ptr GetCurrentTaskQueue() const override;
    ITaskQueue::WeakPtr SetCurrentTaskQueue(ITaskQueue::WeakPtr) override;

private:
    struct CreateResult {
        bool successful;
        uint64_t category;
        bool singleton = false;
    };

    CreateResult CreateInternal(BASE_NS::Uid uid, BASE_NS::vector<IObject::Ptr>& classes) const;
    bool ConstructObjectInternal(const IObject::Ptr& obj, BASE_NS::vector<IObject::Ptr>& classes) const;
    void SetObjectInstanceIds(const BASE_NS::vector<IObject::Ptr>& classes, InstanceId instid) const;
    bool BuildObject(const BASE_NS::vector<IObject::Ptr>& classes, const IMetadata::Ptr& data) const;
    bool PostCreate(const BASE_NS::Uid& uid, InstanceId instid, const CreateResult& t, const CreateInfo& createInfo,
        const BASE_NS::vector<IObject::Ptr>& classes, const IMetadata::Ptr& data) const;

    BASE_NS::string GetClassName(BASE_NS::Uid uid) const;
    IObject::Ptr FindSingleton(const BASE_NS::Uid uid) const;
    void CheckGC() const;
    void GC() const;
    void DoDisposal(const BASE_NS::vector<InstanceId>& uids) const;

    struct ObjectInstance {
        IObject::WeakPtr ptr;
        uint64_t category {};
    };

    mutable std::shared_mutex mutex_;
    mutable std::shared_mutex disposalMutex_;

    BASE_NS::unique_ptr<IRandom> random_;

    // mutable so GC can clean up null objects. (GC is called from const methods)
    mutable BASE_NS::unordered_map<InstanceId, IObject::WeakPtr> singletons_;
    mutable BASE_NS::unordered_map<InstanceId, ObjectInstance> instancesByUid_;
    mutable IObjectContext::Ptr defaultContext_;

    mutable std::atomic_flag disposalInProgress_ = ATOMIC_FLAG_INIT;
    mutable std::atomic<size_t> purgeCounter_ {};
    mutable BASE_NS::vector<InstanceId> disposals_;
    mutable BASE_NS::vector<InstanceId> disposalsStorage_;

    BASE_NS::unordered_map<BASE_NS::Uid, ITaskQueue::Ptr> queues_;

    ClassRegistry classRegistry_;

    // Interpolator constructors.
    BASE_NS::unordered_map<TypeId, BASE_NS::Uid> interpolatorConstructors_;

    SerializationSettings defaultSettings_;
    mutable BASE_NS::unordered_map<InstanceId, IObject::WeakPtr> globalObjects_;
    BASE_NS::unordered_map<TypeId, IValueSerializer::Ptr> valueSerializers_;

    BASE_NS::unordered_map<ObjectId, BASE_NS::shared_ptr<AnyBuilder>> anyBuilders_;
    BASE_NS::unordered_map<CORE_NS::PropertyTypeDecl, IEngineInternalValueAccess::Ptr> engineInternalAccess_;
};

META_END_NAMESPACE()

#endif
