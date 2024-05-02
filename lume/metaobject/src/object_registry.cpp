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

#include "object_registry.h"

#include <chrono>

#include <base/containers/fixed_string.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/uid_util.h>

#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/intf_derived.h>

#include "any.h"
#include "call_context.h"
#include "metadata.h"
#include "property/bind.h"
#include "property/stack_property.h"
#include "random.h"

#define OBJ_REG_LOG(...)

/*
    Notes:
      * Currently Unregistering object and creating it at the same time can cause the pointer ObjectTypeInfo still
            be used after the Unregister function returns.
      * Same issue applies to creating and unregistering property.
*/

META_BEGIN_NAMESPACE()

const size_t DISPOSAL_THRESHOLD = 100;

static BASE_NS::Uid GenerateInstanceId(uint64_t random)
{
    // NOTE: instance uid:s are generated from 64 bit random number and 64 bit timestamp
    auto elapsed = std::chrono::high_resolution_clock::now();
    auto high = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed.time_since_epoch()).count();

    BASE_NS::Uid uid;
    uid.data[0] = high;
    uid.data[1] = random;
    return uid;
}

ObjectRegistry::ObjectRegistry() : random_(CreateXoroshiro128(BASE_NS::FNV1aHash("ToolKitObjectRegistry"))) {}

ObjectRegistry::~ObjectRegistry()
{
    queues_.clear();
    defaultContext_.reset();
    classRegistry_.Clear();
    // Just for sanity.
    GC();
    //  And to make sure all "objects" are unregistered. (we have some that never un-register)
    //  doing it one at a time, because unregister will modify the list..
    bool first = true;
    auto& pluginRegistry = CORE_NS::GetPluginRegister();
    for (;;) {
        const auto& types = pluginRegistry.GetTypeInfos(ObjectTypeInfo::UID);
        if (types.empty()) {
            break;
        }
        auto object = static_cast<const ObjectTypeInfo* const>(types[0]);
        if (first) {
            CORE_LOG_F("Object classes not unregistered before object registry death. (force unregister)");
            first = false;
        }
        auto& classInfo = object->GetFactory()->GetClassInfo();
        CORE_LOG_F(
            "Name: [%s] ClassId [%s]", BASE_NS::string(classInfo.Name()).c_str(), classInfo.Id().ToString().c_str());
        pluginRegistry.UnregisterTypeInfo(*types[0]);
    }
}

IClassRegistry& ObjectRegistry::GetClassRegistry()
{
    return classRegistry_;
}

static void RegisterToPluginRegistry(const ObjectTypeInfo& info)
{
    auto& pluginRegistry = CORE_NS::GetPluginRegister();
    const auto& types = pluginRegistry.GetTypeInfos(ObjectTypeInfo::UID);
    for (auto it = types.begin(); it != types.end(); it++) {
        auto object = static_cast<const ObjectTypeInfo* const>(*it);
        if (*object == info) {
            return;
        }
    }
    pluginRegistry.RegisterTypeInfo(info);
}

static void UnregisterFromPluginRegistry(const BASE_NS::Uid& uid)
{
    auto& pluginRegistry = CORE_NS::GetPluginRegister();
    const auto& types = pluginRegistry.GetTypeInfos(ObjectTypeInfo::UID);
    for (auto it = types.begin(); it != types.end(); it++) {
        auto object = static_cast<const ObjectTypeInfo* const>(*it);
        if (object->GetFactory()->GetClassInfo().Id() == ObjectId(uid)) {
            pluginRegistry.UnregisterTypeInfo(**it);
            break;
        }
    }
}

bool ObjectRegistry::RegisterObjectType(const IClassInfo::Ptr& classInfo)
{
    if (!classInfo) {
        return false;
    }
    const auto factory = interface_pointer_cast<IObjectFactory>(classInfo);
    if (!factory) {
        CORE_LOG_E("ObjectRegistry: The class (%s) being registered does not provide object factory",
            classInfo->GetClassInfo().Name().data());
    }
    if (!classRegistry_.Register(factory)) {
        return false;
    }

    // Construct dummy object of that type to get the static metadata initialised
    BASE_NS::vector<IObject::Ptr> classes;
    auto res = CreateInternal(factory->GetClassInfo().Id().ToUid(), classes);
    if (res.successful && !classes.empty()) {
        // for now we need to set the super classes to safely destroy some objects, using dummy instance id
        SetObjectInstanceIds(classes, BASE_NS::Uid {});
    } else {
        CORE_LOG_W("Failed to create object when generating static metadata [uid=%s]",
            BASE_NS::to_string(factory->GetClassInfo().Id().ToUid()).c_str());
    }
    return true;
}

bool ObjectRegistry::UnregisterObjectType(const IClassInfo::Ptr& classInfo)
{
    if (const auto factory = interface_pointer_cast<IObjectFactory>(classInfo)) {
        return classRegistry_.Unregister(factory);
    }
    return false;
}

BASE_NS::string ObjectRegistry::GetClassName(BASE_NS::Uid uid) const
{
    return classRegistry_.GetClassName(uid);
}

ObjectRegistry::CreateResult ObjectRegistry::CreateInternal(
    BASE_NS::Uid uid, BASE_NS::vector<IObject::Ptr>& classes) const
{
    IObject::Ptr obj;
    ClassInfo info;
    if (auto fac = classRegistry_.GetObjectFactory(uid)) {
        obj = fac->CreateInstance();
        if (obj) {
            info = fac->GetClassInfo();
            return CreateResult { ConstructObjectInternal(obj, classes), info.category, info.IsSingleton() };
        }
    }
    return { false, 0, false };
}

bool ObjectRegistry::ConstructObjectInternal(const IObject::Ptr& obj, BASE_NS::vector<IObject::Ptr>& classes) const
{
    classes.push_back(obj);
    if (auto agr = obj->GetInterface<IDerived>()) {
        auto superUid = agr->GetSuperClassUid();
        if (superUid != BASE_NS::Uid {}) {
            OBJ_REG_LOG("\tCreate super of %s", GetClassName(superUid).c_str());
            auto super = CreateInternal(superUid, classes);
            if (!super.successful) {
                // failed to create super class.
                CORE_LOG_F("Could not create the super class [uid=%s]", BASE_NS::to_string(superUid).c_str());
                return false;
            }
        }
    }
    return true;
}

void ObjectRegistry::SetObjectInstanceIds(const BASE_NS::vector<IObject::Ptr>& classes, InstanceId instid) const
{
    IObject::Ptr obj = classes.front();
    IObject::Ptr base;

    // Prepare object hierarchy for building by setting instance ids and super objects
    for (auto it = classes.rbegin(); it != classes.rend(); ++it) {
        if (auto o = (*it)->GetInterface<ILifecycle>()) {
            o->SetInstanceId(instid);
        }
        if (auto der = (*it)->GetInterface<IDerived>()) {
            if (base) {
                OBJ_REG_LOG("\tAssigning instance of %s as super to %s", BASE_NS::string(base->GetClassName()).c_str(),
                    BASE_NS::string((*it)->GetClassName()).c_str());
            }
            der->SetSuperInstance(obj, base);
        }
        base = *it;
    }
}

bool ObjectRegistry::BuildObject(const BASE_NS::vector<IObject::Ptr>& classes, const IMetadata::Ptr& data) const
{
    if (classes.empty()) {
        return false;
    }
    IObject::Ptr obj = classes.front();
    IMetadata::Ptr meta = ConstructMetadata();

    // Set metadata first so that one can use IMetadata via GetSelf in the non-top classes' Build
    for (auto it = classes.rbegin(); it != classes.rend(); ++it) {
        if (auto i = (*it)->GetInterface<IMetadataInternal>()) {
            i->SetMetadata(meta);
        }
    }
    // call build for the object hierarchy
    for (auto it = classes.rbegin(); it != classes.rend(); ++it) {
        if (auto ctor = (*it)->GetInterface<ILifecycle>()) {
            OBJ_REG_LOG("\tBuilding %s", BASE_NS::string((*it)->GetClassName()).c_str());
            if (!ctor->Build(data)) {
                return false;
            }
        }
    }
    return true;
}

IObject::Ptr ObjectRegistry::Create(ObjectId uid, const CreateInfo& createInfo, const IMetadata::Ptr& data) const
{
    CheckGC();

    auto instid = createInfo.instanceId;

    if (instid == BASE_NS::Uid {}) {
        std::unique_lock lock { mutex_ };
        if (auto so = FindSingleton(uid.ToUid())) {
            return so;
        }
        instid = GenerateInstanceId(random_->GetRandom());
    } else {
        std::shared_lock lock { mutex_ };
        if (auto so = FindSingleton(uid.ToUid())) {
            return so;
        }
        // make sure that an object with specified instanceid does not exist already.
        auto it = instancesByUid_.find(instid);
        if (it != instancesByUid_.end() && !it->second.ptr.expired()) {
            CORE_LOG_F("Object with instance id %s already exists.", instid.ToString().c_str());
            return {};
        }
    }
    OBJ_REG_LOG("Create instance of %s {instance id %s}", GetClassName(uid).c_str(), instid.ToString().c_str());
    BASE_NS::vector<IObject::Ptr> classes;
    auto t = CreateInternal(uid.ToUid(), classes);
    if (t.successful && !classes.empty()) {
        if (PostCreate(uid.ToUid(), instid.ToUid(), t, createInfo, classes, data)) {
            return classes.front();
        }
    }

    CORE_LOG_F("Could not create instance of %s", GetClassName(uid.ToUid()).c_str());
    return nullptr;
}

bool ObjectRegistry::PostCreate(const BASE_NS::Uid& uid, InstanceId instid, const CreateResult& t,
    const CreateInfo& createInfo, const BASE_NS::vector<IObject::Ptr>& classes, const IMetadata::Ptr& data) const
{
    SetObjectInstanceIds(classes, instid);

    if (!BuildObject(classes, data)) {
        CORE_LOG_F("Failed to build object (%s).", GetClassName(uid).c_str());
        return false;
    }

    std::unique_lock lock { mutex_ };
    auto& i = instancesByUid_[instid];
    if (!i.ptr.expired()) {
        // seems someone beat us to it
        CORE_LOG_F("Object with instance id %s already exists.", instid.ToString().c_str());
        return false;
    }
    i = ObjectInstance { classes.front(), t.category };

    if (t.singleton) {
        singletons_[uid] = classes.front(); // Store singleton weakref
    }
    if (createInfo.isGloballyAvailable) {
        CORE_LOG_D("Registering global object: %s [%s]", GetClassName(uid).c_str(), instid.ToString().c_str());
        globalObjects_[instid] = classes.front();
    }
    return true;
}

IObject::Ptr ObjectRegistry::Create(ObjectId uid, const CreateInfo& createInfo) const
{
    return Create(uid, createInfo, nullptr);
}

IObject::Ptr ObjectRegistry::Create(const META_NS::ClassInfo& info, const CreateInfo& createInfo) const
{
    return Create(info.Id(), createInfo);
}

BASE_NS::vector<ObjectCategoryItem> ObjectRegistry::GetAllCategories() const
{
    static const BASE_NS::vector<ObjectCategoryItem> items = { { ObjectCategoryBits::WIDGET, "Widgets" },
        { ObjectCategoryBits::ANIMATION, "Animations" }, { ObjectCategoryBits::LAYOUT, "Layouts" },
        { ObjectCategoryBits::CURVE, "Curves" }, { ObjectCategoryBits::SHAPE, "Shapes" },
        { ObjectCategoryBits::CONTAINER, "Containers" }, { ObjectCategoryBits::INTERNAL, "Internals" },
        { ObjectCategoryBits::APPLICATION, "Application specifics" },
        { ObjectCategoryBits::ANIMATION_MODIFIER, "Animation modifier" },
        { ObjectCategoryBits::NO_CATEGORY, "Not categorized" } };
    return items;
}

IObjectFactory::ConstPtr ObjectRegistry::GetObjectFactory(const ObjectId& uid) const
{
    std::shared_lock lock { mutex_ };
    return classRegistry_.GetObjectFactory(uid.ToUid());
}

BASE_NS::vector<IClassInfo::ConstPtr> ObjectRegistry::GetAllTypes(
    ObjectCategoryBits category, bool strict, bool excludeDeprecated) const
{
    std::shared_lock lock { mutex_ };
    return classRegistry_.GetAllTypes(category, strict, excludeDeprecated);
}

void ObjectRegistry::CheckGC() const
{
    if (purgeCounter_ > DISPOSAL_THRESHOLD && disposalInProgress_.test_and_set()) {
        {
            std::unique_lock lock { disposalMutex_ };
            disposalsStorage_.swap(disposals_);
            purgeCounter_ = 0;
        }
        DoDisposal(disposalsStorage_);
        disposalsStorage_.clear();
        disposalInProgress_.clear();
    }
}

void ObjectRegistry::GC() const
{
    for (auto it = instancesByUid_.begin(); it != instancesByUid_.end();) {
        if (it->second.ptr.expired()) {
            it = instancesByUid_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = singletons_.begin(); it != singletons_.end();) {
        if (it->second.expired()) {
            it = singletons_.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = globalObjects_.begin(); it != globalObjects_.end();) {
        if (it->second.expired()) {
            it = globalObjects_.erase(it);
        } else {
            ++it;
        }
    }
}

void ObjectRegistry::Purge()
{
    std::unique_lock lock { mutex_ };
    GC();
}

void ObjectRegistry::DoDisposal(const BASE_NS::vector<InstanceId>& uids) const
{
    std::unique_lock lock { mutex_ };
    for (auto&& v : uids) {
        auto it = instancesByUid_.find(v);
        if (it != instancesByUid_.end()) {
            instancesByUid_.erase(it);
            auto it = singletons_.find(v);
            if (it != singletons_.end()) {
                singletons_.erase(it);
            }
        }
    }
}

void ObjectRegistry::DisposeObject(const InstanceId& uid) const
{
    std::unique_lock lock { disposalMutex_ };
    disposals_.push_back(uid);
    ++purgeCounter_;
}

IMetadata::Ptr ObjectRegistry::ConstructMetadata() const
{
    return IMetadata::Ptr { new Internal::Metadata };
}

ICallContext::Ptr ObjectRegistry::ConstructDefaultCallContext() const
{
    return ICallContext::Ptr { new DefaultCallContext };
}

BASE_NS::vector<IObject::Ptr> ObjectRegistry::GetAllObjectInstances() const
{
    CheckGC();
    BASE_NS::vector<IObject::Ptr> result;
    std::shared_lock lock { mutex_ };
    result.reserve(instancesByUid_.size());
    for (const auto& v : instancesByUid_) {
        if (auto strong = v.second.ptr.lock()) {
            result.emplace_back(strong);
        }
    }
    return result;
}

BASE_NS::vector<IObject::Ptr> ObjectRegistry::GetAllSingletonObjectInstances() const
{
    CheckGC();
    BASE_NS::vector<IObject::Ptr> result;
    std::shared_lock lock { mutex_ };
    if (!singletons_.empty()) {
        result.reserve(singletons_.size());
        for (const auto& s : singletons_) {
            if (auto strong = s.second.lock()) {
                result.push_back(strong);
            }
        }
    }
    return result;
}

BASE_NS::vector<IObject::Ptr> ObjectRegistry::GetObjectInstancesByCategory(
    ObjectCategoryBits category, bool strict) const
{
    CheckGC();
    BASE_NS::vector<IObject::Ptr> result;
    std::shared_lock lock { mutex_ };
    for (const auto& i : instancesByUid_) {
        if (CheckCategoryBits(static_cast<ObjectCategoryBits>(i.second.category), category, strict)) {
            if (auto strong = i.second.ptr.lock()) {
                result.emplace_back(strong);
            }
        }
    }
    return result;
}

IObject::Ptr ObjectRegistry::FindSingleton(const BASE_NS::Uid uid) const
{
    auto it = singletons_.find(uid);
    return it != singletons_.end() ? it->second.lock() : nullptr;
}

IObject::Ptr ObjectRegistry::GetObjectInstanceByInstanceId(InstanceId uid) const
{
    if (uid == BASE_NS::Uid()) {
        // invalid/zero/empty UID.
        return nullptr;
    }

    CheckGC();

    std::shared_lock lock { mutex_ };

    // See if it's an singleton.
    auto sing = FindSingleton(uid.ToUid());
    if (sing) {
        return sing;
    }

    // Non singletons then
    auto it2 = instancesByUid_.find(uid);
    if (it2 != instancesByUid_.end()) {
        if (auto strong = it2->second.ptr.lock()) {
            return strong;
        }
    }

    // No such instance then
    return nullptr;
}

BASE_NS::string ObjectRegistry::ExportToString(const IObjectRegistryExporter::Ptr& exporter) const
{
    return exporter ? exporter->ExportRegistry(this) : "";
}

IObjectContext::Ptr ObjectRegistry::GetDefaultObjectContext() const
{
    {
        std::shared_lock lock { mutex_ };
        if (defaultContext_) {
            return defaultContext_;
        }
    }

    IObjectContext::Ptr context = interface_pointer_cast<IObjectContext>(
        Create(ClassId::ObjectContext, { GlobalObjectInstance::DEFAULT_OBJECT_CONTEXT, true }));

    std::unique_lock lock { mutex_ };
    // still not set?
    if (!defaultContext_) {
        defaultContext_ = context;
    }
    CORE_ASSERT(defaultContext_);
    return defaultContext_;
}

ITaskQueue::Ptr ObjectRegistry::GetTaskQueue(const BASE_NS::Uid& queueId) const
{
    std::shared_lock lock { mutex_ };
    if (auto queue = queues_.find(queueId); queue != queues_.end()) {
        return queue->second;
    }
    CORE_LOG_W("Cannot get task queue, task queue not registered: %s", BASE_NS::to_string(queueId).data());
    return {};
}

bool ObjectRegistry::RegisterTaskQueue(const ITaskQueue::Ptr& queue, const BASE_NS::Uid& queueId)
{
    std::unique_lock lock { mutex_ };
    if (!queue) {
        if (auto existing = queues_.find(queueId); existing != queues_.end()) {
            queues_.erase(existing);
            return true;
        }
        // Null queue but no existing queue found
        return false;
    }
    queues_[queueId] = queue;
    return true;
}

bool ObjectRegistry::UnregisterTaskQueue(const BASE_NS::Uid& queueId)
{
    std::unique_lock lock { mutex_ };
    if (auto existing = queues_.find(queueId); existing != queues_.end()) {
        queues_.erase(existing);
        return true;
    }
    return false;
}

bool ObjectRegistry::HasTaskQueue(const BASE_NS::Uid& queueId) const
{
    std::shared_lock lock { mutex_ };
    return queues_.find(queueId) != queues_.end();
}

bool ObjectRegistry::UnregisterAllTaskQueues()
{
    std::unique_lock lock { mutex_ };
    queues_.clear();
    return true;
}

static ITaskQueue::WeakPtr& GetCurrentTaskQueueImpl()
{
    static thread_local ITaskQueue::WeakPtr q;
    return q;
}

ITaskQueue::Ptr ObjectRegistry::GetCurrentTaskQueue() const
{
    return GetCurrentTaskQueueImpl().lock();
}
ITaskQueue::WeakPtr ObjectRegistry::SetCurrentTaskQueue(ITaskQueue::WeakPtr q)
{
    auto& impl = GetCurrentTaskQueueImpl();
    auto res = impl;
    impl = q;
    return res;
}

void ObjectRegistry::RegisterInterpolator(TypeId propertyTypeUid, BASE_NS::Uid interpolatorClassUid)
{
    std::unique_lock lock { mutex_ };
    interpolatorConstructors_[propertyTypeUid] = interpolatorClassUid;
}

void ObjectRegistry::UnregisterInterpolator(TypeId propertyTypeUid)
{
    std::unique_lock lock { mutex_ };
    interpolatorConstructors_.erase(propertyTypeUid);
}

bool ObjectRegistry::HasInterpolator(TypeId propertyTypeUid) const
{
    std::shared_lock lock { mutex_ };
    return interpolatorConstructors_.contains(propertyTypeUid);
}

IInterpolator::Ptr ObjectRegistry::CreateInterpolator(TypeId propertyTypeUid)
{
    TypeId uid;
    {
        std::shared_lock lock { mutex_ };
        if (auto it = interpolatorConstructors_.find(propertyTypeUid); it != interpolatorConstructors_.end()) {
            uid = it->second;
        }
    }
    if (uid != TypeId {}) {
        return interface_pointer_cast<IInterpolator>(Create(uid.ToUid(), CreateInfo {}));
    }
    // We don't have an interpolator for the given property type, return the default interpolator (which just steps the
    // value)
    CORE_LOG_D("No interpolator for property type %s, falling back to default interpolator",
        propertyTypeUid.ToString().c_str());
    return interface_pointer_cast<IInterpolator>(IObjectRegistry::Create(ClassId::DefaultInterpolator));
}

const CORE_NS::IInterface* ObjectRegistry::GetInterface(const BASE_NS::Uid& uid) const
{
    const CORE_NS::IInterface* result = nullptr;
    if (uid == CORE_NS::IInterface::UID) {
        const IObjectRegistry* obj = static_cast<const IObjectRegistry*>(this);
        result = static_cast<const IInterface*>(obj);
    }
    if (uid == IObjectRegistry::UID) {
        result = static_cast<const IObjectRegistry*>(this);
    }
    if (uid == ITaskQueueRegistry::UID) {
        result = static_cast<const ITaskQueueRegistry*>(this);
    }
    return result;
}
CORE_NS::IInterface* ObjectRegistry::GetInterface(const BASE_NS::Uid& uid)
{
    CORE_NS::IInterface* result = nullptr;
    if (uid == CORE_NS::IInterface::UID) {
        IObjectRegistry* obj = static_cast<IObjectRegistry*>(this);
        result = static_cast<IInterface*>(obj);
    }
    if (uid == IObjectRegistry::UID) {
        result = static_cast<IObjectRegistry*>(this);
    }
    if (uid == ITaskQueueRegistry::UID) {
        result = static_cast<ITaskQueueRegistry*>(this);
    }
    return result;
}
void ObjectRegistry::Ref() {}
void ObjectRegistry::Unref() {}

META_NS::IPropertyRegister& ObjectRegistry::GetPropertyRegister()
{
    return *this;
}

META_NS::IProperty::Ptr ObjectRegistry::Create(const ObjectId& object, BASE_NS::string_view name) const
{
    if (object == ClassId::StackProperty) {
        auto p = META_NS::IProperty::Ptr(new META_NS::Internal::StackProperty(BASE_NS::string(name)));
        if (auto i = interface_cast<IPropertyInternal>(p)) {
            i->SetSelf(p);
        }
        return p;
    }
    return nullptr;
}
IBind::Ptr ObjectRegistry::CreateBind() const
{
    return interface_pointer_cast<IBind>(Create(ClassId::Bind, CreateInfo {}));
}
IAny& ObjectRegistry::InvalidAny() const
{
    static DummyAny any;
    return any;
}
IAny::Ptr ObjectRegistry::ConstructAny(const ObjectId& id) const
{
    std::shared_lock lock { mutex_ };
    auto it = anyBuilders_.find(id);
    return it != anyBuilders_.end() ? it->second->Construct() : nullptr;
}
bool ObjectRegistry::IsAnyRegistered(const ObjectId& id) const
{
    std::shared_lock lock { mutex_ };
    return anyBuilders_.find(id) != anyBuilders_.end();
}
void ObjectRegistry::RegisterAny(BASE_NS::shared_ptr<AnyBuilder> builder)
{
    std::unique_lock lock { mutex_ };
    if (anyBuilders_.find(builder->GetObjectId()) != anyBuilders_.end()) {
        CORE_LOG_W("Any already registered [id=%s]", builder->GetObjectId().ToString().c_str());
    }
    anyBuilders_[builder->GetObjectId()] = builder;
}
void ObjectRegistry::UnregisterAny(const ObjectId& id)
{
    std::unique_lock lock { mutex_ };
    anyBuilders_.erase(id);
}
IGlobalSerializationData& ObjectRegistry::GetGlobalSerializationData()
{
    return *this;
}
SerializationSettings ObjectRegistry::GetDefaultSettings() const
{
    std::shared_lock lock { mutex_ };
    return defaultSettings_;
}
void ObjectRegistry::SetDefaultSettings(const SerializationSettings& settings)
{
    std::unique_lock lock { mutex_ };
    defaultSettings_ = settings;
}
void ObjectRegistry::RegisterGlobalObject(const IObject::Ptr& object)
{
    std::unique_lock lock { mutex_ };
    if (auto p = interface_cast<IObjectInstance>(object)) {
        globalObjects_[p->GetInstanceId()] = object;
    }
}
void ObjectRegistry::UnregisterGlobalObject(const IObject::Ptr& object)
{
    std::unique_lock lock { mutex_ };
    if (auto p = interface_cast<IObjectInstance>(object)) {
        globalObjects_.erase(p->GetInstanceId());
    }
}
IObject::Ptr ObjectRegistry::GetGlobalObject(const InstanceId& id) const
{
    std::shared_lock lock { mutex_ };
    auto it = globalObjects_.find(id);
    return it != globalObjects_.end() ? it->second.lock() : nullptr;
}
void ObjectRegistry::RegisterValueSerializer(const IValueSerializer::Ptr& s)
{
    std::unique_lock lock { mutex_ };
    valueSerializers_[s->GetTypeId()] = s;
}
void ObjectRegistry::UnregisterValueSerializer(const TypeId& id)
{
    std::unique_lock lock { mutex_ };
    valueSerializers_.erase(id);
}
IValueSerializer::Ptr ObjectRegistry::GetValueSerializer(const TypeId& id) const
{
    std::shared_lock lock { mutex_ };
    auto it = valueSerializers_.find(id);
    return it != valueSerializers_.end() ? it->second : nullptr;
}

IEngineInternalValueAccess::Ptr ObjectRegistry::GetInternalValueAccess(const CORE_NS::PropertyTypeDecl& type) const
{
    std::shared_lock lock { mutex_ };
    auto it = engineInternalAccess_.find(type);
    return it != engineInternalAccess_.end() ? it->second : nullptr;
}
void ObjectRegistry::RegisterInternalValueAccess(
    const CORE_NS::PropertyTypeDecl& type, IEngineInternalValueAccess::Ptr ptr)
{
    std::unique_lock lock { mutex_ };
    engineInternalAccess_[type] = BASE_NS::move(ptr);
}
void ObjectRegistry::UnregisterInternalValueAccess(const CORE_NS::PropertyTypeDecl& type)
{
    std::unique_lock lock { mutex_ };
    engineInternalAccess_.erase(type);
}
IEngineData& ObjectRegistry::GetEngineData()
{
    return *this;
}
META_END_NAMESPACE()
