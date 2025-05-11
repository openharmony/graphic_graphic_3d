
#include "exporter.h"

#include <meta/api/util.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_object_context.h>
#include <meta/interface/serialization/intf_serializable.h>

#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

bool Exporter::ShouldSerialize(const IObject::ConstPtr& object) const
{
    return IsFlagSet(object, ObjectFlagBits::SERIALIZE);
}
bool Exporter::ShouldSerialize(const IAny& any) const
{
    SharedPtrConstIInterface p;
    if (any.GetValue(p)) {
        if (auto flags = interface_cast<IObjectFlags>(p)) {
            return flags->GetObjectFlags().IsSet(ObjectFlagBits::SERIALIZE);
        }
    }
    return true;
}

void Exporter::SetInstanceIdMapping(BASE_NS::unordered_map<InstanceId, InstanceId> map)
{
    mapInstanceIds_ = BASE_NS::move(map);
}
void Exporter::SetResourceManager(CORE_NS::IResourceManager::Ptr p)
{
    resources_ = BASE_NS::move(p);
}
void Exporter::SetUserContext(IObject::Ptr p)
{
    userContext_ = BASE_NS::move(p);
}
void Exporter::SetMetadata(SerMetadata m)
{
    metadata_ = BASE_NS::move(m);
}

InstanceId Exporter::ConvertInstanceId(const InstanceId& id) const
{
    auto it = mapInstanceIds_.find(id);
    return it != mapInstanceIds_.end() ? it->second : id;
}

bool Exporter::MarkExported(const IObject::ConstPtr& object)
{
    bool res = false;
    if (auto i = interface_cast<IObjectInstance>(object)) {
        auto id = i->GetInstanceId();
        auto it = exported_.find(id);
        res = it != exported_.end();
        if (!res) {
            exported_[id] = object;
        }
    }
    return res;
}

bool Exporter::HasBeenExported(const InstanceId& id) const
{
    return exported_.find(id) != exported_.end();
}

ISerNode::Ptr Exporter::Export(const IObject::ConstPtr& object)
{
    BASE_NS::shared_ptr<RootNode> res;
    if (object) {
        ISerNode::Ptr node;
        auto r = ExportObject(object, node);
        if (r) {
            // resolve deferred weak ptr exports
            for (auto&& d : deferred_) {
                if (!ResolveDeferredWeakPtr(d)) {
                    CORE_LOG_E("Failed to resolve deferred weak ptr export");
                }
            }
            res.reset(new RootNode { BASE_NS::move(node), metadata_ });
        }
    }
    return res;
}

ISerNode::Ptr Exporter::CreateObjectNode(const IObject::ConstPtr& object, BASE_NS::shared_ptr<MapNode> node)
{
    ISerNode::Ptr res;
    InstanceId iid;
    BASE_NS::string name = object->GetName();
    if (auto i = interface_cast<IObjectInstance>(object)) {
        iid = ConvertInstanceId(i->GetInstanceId());
    }
    if (name == iid.ToString()) {
        name = "";
    }
    return ISerNode::Ptr(new ObjectNode(
        BASE_NS::string(object->GetClassName()), BASE_NS::move(name), object->GetClassId(), iid, BASE_NS::move(node)));
}

ISerNode::Ptr Exporter::CreateObjectRefNode(const RefUri& ref)
{
    RefUri uri(ref);
    uri.SetBaseObjectUid(ConvertInstanceId(uri.BaseObjectUid()).ToUid());
    return ISerNode::Ptr(new RefNode { uri });
}

ISerNode::Ptr Exporter::CreateObjectRefNode(const IObject::ConstPtr& object)
{
    ISerNode::Ptr res;
    if (auto i = interface_cast<IObjectInstance>(object)) {
        RefUri ref(i->GetInstanceId().ToUid());
        res = CreateObjectRefNode(ref);
    }
    return res;
}

ReturnError Exporter::ExportObject(const IObject::ConstPtr& object, ISerNode::Ptr& res)
{
    ReturnError err = GenericError::SUCCESS;
    if (ShouldSerialize(object)) {
        if (MarkExported(object)) {
            res = CreateObjectRefNode(object);
        } else if (auto ser = interface_cast<ISerializable>(object)) {
            ExportContext context(*this, object);
            err = ser->Export(context);
            if (err) {
                res = context.GetSubstitution();
                if (!res) {
                    res = CreateObjectNode(object, context.ExtractNode());
                }
            }
        } else {
            res = AutoExportObject(object);
        }
    }
    return err;
}

ISerNode::Ptr Exporter::AutoExportObject(const IObject::ConstPtr& object)
{
    BASE_NS::vector<NamedNode> members;
    AutoExportObjectMembers(object, members);
    return CreateObjectNode(object, BASE_NS::shared_ptr<MapNode>(new MapNode(members)));
}

ReturnError Exporter::AutoExportObjectMembers(const IObject::ConstPtr& object, BASE_NS::vector<NamedNode>& members)
{
    if (auto flags = interface_cast<IObjectFlags>(object)) {
        if (flags->GetObjectDefaultFlags() != flags->GetObjectFlags()) {
            ISerNode::Ptr node;
            auto res = ExportValue(Any<uint64_t>(flags->GetObjectFlags().GetValue()), node);
            if (res && node) {
                members.push_back(NamedNode { "__flags", node });
            }
        }
    }
    if (auto attach = interface_cast<IAttach>(object)) {
        if (IsFlagSet(object, ObjectFlagBits::SERIALIZE_ATTACHMENTS)) {
            if (auto cont = attach->GetAttachmentContainer()) {
                if (auto node = ExportIContainer(*cont)) {
                    members.push_back(NamedNode { "__attachments", node });
                }
            }
        }
    }
    if (auto cont = interface_cast<IContainer>(object)) {
        if (IsFlagSet(object, ObjectFlagBits::SERIALIZE_HIERARCHY)) {
            if (auto node = ExportIContainer(*cont)) {
                members.push_back(NamedNode { "__children", node });
            }
        }
    }
    return GenericError::SUCCESS;
}

ISerNode::Ptr Exporter::ExportIContainer(const IContainer& cont)
{
    BASE_NS::vector<ISerNode::Ptr> elements;
    for (size_t i = 0; i != cont.GetSize(); ++i) {
        ISerNode::Ptr node;
        if (ExportObject(cont.GetAt(i), node) && node) {
            elements.push_back(BASE_NS::move(node));
        }
    }
    return ISerNode::Ptr(new ArrayNode(BASE_NS::move(elements)));
}

template<typename... Builtins>
static ISerNode::Ptr ExportSingleBuiltinValue(TypeList<Builtins...>, const IAny& value)
{
    ISerNode::Ptr res;
    [[maybe_unused]] bool r =
        ((META_NS::IsCompatible(value, Builtins::ID) ? (res = Builtins::CreateNode(value), true) : false) || ...);
    return res;
}

ISerNode::Ptr Exporter::ExportArray(const IArrayAny& array)
{
    ISerNode::Ptr res;
    auto any = array.Clone(AnyCloneOptions { CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM });
    if (any) {
        BASE_NS::vector<ISerNode::Ptr> elements;
        for (size_t i = 0; i != array.GetSize(); ++i) {
            if (!array.GetAnyAt(i, *any)) {
                return nullptr;
            }
            ISerNode::Ptr node;
            if (ExportValue(*any, node) && node) {
                elements.push_back(BASE_NS::move(node));
            }
        }
        res.reset(new ArrayNode(BASE_NS::move(elements)));
    }
    return res;
}

ISerNode::Ptr Exporter::ExportBuiltinValue(const IAny& value)
{
    ISerNode::Ptr res;
    if (auto arr = interface_cast<IArrayAny>(&value)) {
        res = ExportArray(*arr);
    } else {
        if (value.GetTypeId() == UidFromType<float>()) {
            // handle as double
            res = ExportSingleBuiltinValue(SupportedBuiltins {}, Any<double>(GetValue<float>(value)));
        } else {
            res = ExportSingleBuiltinValue(SupportedBuiltins {}, value);
        }
    }
    return res;
}

ReturnError Exporter::ExportPointer(const IAny& entity, ISerNode::Ptr& res)
{
    // first see if it is a weak pointer
    BASE_NS::weak_ptr<const CORE_NS::IInterface> weak;
    bool isWeak = entity.GetValue(weak);
    if (isWeak) {
        if (auto obj = interface_pointer_cast<IObject>(weak)) {
            return ExportWeakPtr(obj, res);
        }
    }

    BASE_NS::shared_ptr<const CORE_NS::IInterface> intf;
    if (entity.GetValue(intf)) {
        // see if it is null pointer
        if (!intf) {
            res = ISerNode::Ptr(new NilNode);
            return GenericError::SUCCESS;
        }
        // finally handle normal pointer case
        if (!isWeak) {
            if (auto obj = interface_pointer_cast<IObject>(intf)) {
                return ExportObject(obj, res);
            }
            if (auto any = interface_pointer_cast<IAny>(intf)) {
                return ExportAny(any, res);
            }
        }
    }
    return GenericError::FAIL;
}

ReturnError Exporter::ExportValue(const IAny& entity, ISerNode::Ptr& res)
{
    if (!ShouldSerialize(entity)) {
        return GenericError::SUCCESS;
    }
    if (auto exp = globalData_.GetValueSerializer(entity.GetTypeId())) {
        res = exp->Export(*this, entity);
        if (res) {
            return GenericError::SUCCESS;
        }
        CORE_LOG_W("Value export registered for type [%s, %s] but it failed", entity.GetTypeIdString().c_str(),
            entity.GetTypeId().ToString().c_str());
    }
    res = ExportBuiltinValue(entity);
    if (!res) {
        ExportPointer(entity, res);
    }
    if (!res) {
        CORE_LOG_F(
            "Failed to export type [%s, %s]", entity.GetTypeIdString().c_str(), entity.GetTypeId().ToString().c_str());
        return GenericError::FAIL;
    }
    return GenericError::SUCCESS;
}

ReturnError Exporter::ExportAny(const IAny::ConstPtr& any, ISerNode::Ptr& res)
{
    ReturnError err = GenericError::SUCCESS;
    if (!registry_.GetPropertyRegister().IsAnyRegistered(any->GetClassId())) {
        CORE_LOG_W("Exporting any that is not registered [class id=%s, type=%s, type id=%s]",
            any->GetClassId().ToString().c_str(), any->GetTypeIdString().c_str(), any->GetTypeId().ToString().c_str());
    }
    if (!any) {
        res = ISerNode::Ptr(new NilNode);
    } else if (auto ser = interface_cast<ISerializable>(any)) {
        ExportContext context(*this, interface_pointer_cast<IObject>(any));
        err = ser->Export(context);
        if (err) {
            res = context.GetSubstitution();
            if (!res) {
                res = ISerNode::Ptr(
                    new ObjectNode(BASE_NS::string("Any"), {}, any->GetClassId(), {}, context.ExtractNode()));
            }
        }
    } else {
        ISerNode::Ptr node;
        err = ExportValue(*any, node);
        if (err && node) {
            auto members = CreateShared<MapNode>(BASE_NS::vector<NamedNode> { NamedNode { "value", node } });
            res = ISerNode::Ptr(
                new ObjectNode(BASE_NS::string("Any"), {}, any->GetClassId(), {}, BASE_NS::move(members)));
        }
    }
    return err;
}

IObject::Ptr Exporter::ResolveUriSegment(const IObject::ConstPtr& ptr, RefUri& uri) const
{
    if (auto instance = interface_cast<IObjectInstance>(ptr)) {
        if (auto context = interface_cast<IObjectContext>(ptr)) {
            uri.PushObjectContextSegment();
        } else {
            uri.PushObjectSegment(instance->GetName());
        }
        return ptr->Resolve(RefUri::ParentUri());
    }
    if (auto property = interface_cast<IProperty>(ptr)) {
        uri.PushPropertySegment(property->GetName());
        auto owner = interface_pointer_cast<IObject>(property->GetOwner());
        if (!owner) {
            CORE_LOG_E("No Owner for property '%s' when exporting weak ptr", property->GetName().c_str());
        }
        return owner;
    }
    return nullptr;
}

ReturnError Exporter::ResolveDeferredWeakPtr(const DeferredWeakPtrResolve& d)
{
    if (auto p = d.ptr.lock()) {
        auto original = p;
        RefUri uri;
        while (p) {
            if (auto obj = interface_cast<IObjectInstance>(p)) {
                auto iid = ConvertInstanceId(obj->GetInstanceId());
                if (HasBeenExported(iid) || globalData_.GetGlobalObject(obj->GetInstanceId())) {
                    uri.SetBaseObjectUid(iid.ToUid());
                    d.node->SetValue(uri);
                    return GenericError::SUCCESS;
                }
            }
            p = ResolveUriSegment(p, uri);
        }
        CORE_LOG_E("Could not find suitable anchor object when exporting weak ptr [%s, %s, %s]",
            BASE_NS::string(original->GetClassName()).c_str(), original->GetName().c_str(),
            original->GetClassId().ToString().c_str());
    } else {
        CORE_LOG_E("Weak ptr is null when doing deferred resolve");
    }
    return GenericError::FAIL;
}

ReturnError Exporter::ExportWeakPtr(const IObject::ConstWeakPtr& ptr, ISerNode::Ptr& res)
{
    if (auto p = ptr.lock()) {
        auto node = CreateShared<RefNode>(RefUri {});
        res = ISerNode::Ptr(node);
        deferred_.push_back(DeferredWeakPtrResolve { node, ptr });
    } else {
        res = ISerNode::Ptr(new NilNode);
    }
    return GenericError::SUCCESS;
}

ReturnError Exporter::ExportToNode(const IAny& entity, ISerNode::Ptr& res)
{
    return ExportValue(entity, res);
}

ReturnError ExportContext::Export(BASE_NS::string_view name, const IAny& entity)
{
    ISerNode::Ptr node;
    auto res = exporter_.ExportValue(entity, node);
    if (res && node) {
        elements_.push_back(NamedNode { BASE_NS::string(name), BASE_NS::move(node) });
    }
    if (!res) {
        CORE_LOG_W("Failed to export member with name '%s'", BASE_NS::string(name).c_str());
    }
    return res;
}

ReturnError ExportContext::ExportAny(BASE_NS::string_view name, const IAny::Ptr& any)
{
    ISerNode::Ptr node;
    auto res = exporter_.ExportAny(any, node);
    if (res && node) {
        elements_.push_back(NamedNode { BASE_NS::string(name), BASE_NS::move(node) });
    }
    if (!res) {
        CORE_LOG_W("Failed to export member with name '%s'", BASE_NS::string(name).c_str());
    }
    return res;
}

ReturnError ExportContext::ExportWeakPtr(BASE_NS::string_view name, const IObject::ConstWeakPtr& ptr)
{
    ISerNode::Ptr node;
    auto res = exporter_.ExportWeakPtr(ptr, node);
    if (res && node) {
        elements_.push_back(NamedNode { BASE_NS::string(name), BASE_NS::move(node) });
    }
    if (!res) {
        CORE_LOG_W("Failed to export member with name '%s'", BASE_NS::string(name).c_str());
    }
    return res;
}

ReturnError ExportContext::AutoExport()
{
    if (object_) {
        BASE_NS::vector<NamedNode> vec;
        auto res = exporter_.AutoExportObjectMembers(object_, vec);
        if (res) {
            elements_.insert(elements_.end(), vec.begin(), vec.end());
        }
        return res;
    }
    CORE_LOG_W("Failed to auto export, exported type is not IObject");
    return GenericError::FAIL;
}

BASE_NS::shared_ptr<MapNode> ExportContext::ExtractNode()
{
    return BASE_NS::shared_ptr<MapNode>(new MapNode { BASE_NS::move(elements_) });
}

ReturnError ExportContext::ExportToNode(const IAny& entity, ISerNode::Ptr& res)
{
    return exporter_.ExportToNode(entity, res);
}

CORE_NS::IInterface* ExportContext::Context() const
{
    return exporter_.GetInterface(CORE_NS::IInterface::UID);
}
META_NS::IObject::Ptr ExportContext::UserContext() const
{
    return exporter_.GetUserContext();
}
ReturnError ExportContext::SubstituteThis(ISerNode::Ptr n)
{
    ReturnError res = GenericError::FAIL;
    if (n) {
        substNode_ = BASE_NS::move(n);
        // do instance mapping
        if (auto node = interface_cast<IObjectNode>(substNode_)) {
            InstanceId iid;
            if (auto i = interface_cast<IObjectInstance>(object_)) {
                iid = exporter_.ConvertInstanceId(i->GetInstanceId());
            }
            node->SetInstanceId(iid);
        }
        res = GenericError::SUCCESS;
    }
    return res;
}
SerMetadata ExportContext::GetMetadata() const
{
    return exporter_.GetMetadata();
}

} // namespace Serialization
META_END_NAMESPACE()
