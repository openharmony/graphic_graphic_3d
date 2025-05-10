
#include "object_data_container.h"

#include <algorithm>

META_BEGIN_NAMESPACE()
namespace Internal {

bool ObjectDataContainer::AddFunction(const IFunction::Ptr& p)
{
    return Attach(p);
}
bool ObjectDataContainer::RemoveFunction(const IFunction::Ptr& p)
{
    return Detach(p);
}
bool ObjectDataContainer::AddProperty(const IProperty::Ptr& p)
{
    return Attach(p);
}
bool ObjectDataContainer::RemoveProperty(const IProperty::Ptr& p)
{
    return Detach(p);
}
bool ObjectDataContainer::AddEvent(const IEvent::Ptr& p)
{
    return Attach(p);
}
bool ObjectDataContainer::RemoveEvent(const IEvent::Ptr& p)
{
    return Detach(p);
}
BASE_NS::vector<IProperty::Ptr> ObjectDataContainer::GetProperties()
{
    return PtrArrayCast<IProperty>(FindAll({ "", TraversalType::NO_HIERARCHY, { TypeId(IProperty::UID) } }));
}
BASE_NS::vector<IProperty::ConstPtr> ObjectDataContainer::GetProperties() const
{
    auto vec = const_cast<ObjectDataContainer&>(*this).GetProperties();
    BASE_NS::vector<IProperty::ConstPtr> res;
    res.insert(res.end(), vec.begin(), vec.end());
    return res;
}
BASE_NS::vector<IFunction::Ptr> ObjectDataContainer::GetFunctions()
{
    return PtrArrayCast<IFunction>(FindAll({ "", TraversalType::NO_HIERARCHY, { TypeId(IFunction::UID) } }));
}
BASE_NS::vector<IFunction::ConstPtr> ObjectDataContainer::GetFunctions() const
{
    auto vec = const_cast<ObjectDataContainer&>(*this).GetFunctions();
    BASE_NS::vector<IFunction::ConstPtr> res;
    res.insert(res.end(), vec.begin(), vec.end());
    return res;
}
BASE_NS::vector<IEvent::Ptr> ObjectDataContainer::GetEvents()
{
    return PtrArrayCast<IEvent>(FindAll({ "", TraversalType::NO_HIERARCHY, { TypeId(IEvent::UID) } }));
}
BASE_NS::vector<IEvent::ConstPtr> ObjectDataContainer::GetEvents() const
{
    auto vec = const_cast<ObjectDataContainer&>(*this).GetEvents();
    BASE_NS::vector<IEvent::ConstPtr> res;
    res.insert(res.end(), vec.begin(), vec.end());
    return res;
}
static MetadataType GetMetadataType(const IObject::Ptr& p)
{
    if (interface_cast<IProperty>(p)) {
        return MetadataType::PROPERTY;
    }
    if (interface_cast<IEvent>(p)) {
        return MetadataType::EVENT;
    }
    if (interface_cast<IFunction>(p)) {
        return MetadataType::FUNCTION;
    }
    return MetadataType::UNKNOWN;
}

static BASE_NS::vector<TypeId> ConvertToTypeIds(MetadataType types)
{
    BASE_NS::vector<TypeId> ids;
    if (uint8_t(types) & uint8_t(MetadataType::PROPERTY)) {
        ids.push_back(TypeId(IProperty::UID));
    }
    if (uint8_t(types) & uint8_t(MetadataType::EVENT)) {
        ids.push_back(TypeId(IEvent::UID));
    }
    if (uint8_t(types) & uint8_t(MetadataType::FUNCTION)) {
        ids.push_back(TypeId(IFunction::UID));
    }
    return ids;
}

BASE_NS::vector<MetadataInfo> ObjectDataContainer::GetAllMetadatas(MetadataType types) const
{
    BASE_NS::vector<MetadataInfo> res;
    if (auto s = GetOwner<IStaticMetadata>()) {
        if (auto pm = s->GetStaticMetadata()) {
            auto r = GetAllStaticMetadata(*pm, types);
            res.insert(res.end(), r.begin(), r.end());
        }
    }
    BASE_NS::vector<TypeId> ids = ConvertToTypeIds(types);
    for (auto&& v : FindAll({ "", TraversalType::NO_HIERARCHY, ids })) {
        // only compare name and type
        if (std::find_if(res.begin(), res.end(), [name = v->GetName(), type = GetMetadataType(v)](const auto& e) {
                return e.name == name && e.type == type;
            }) == res.end()) {
            MetadataInfo info { GetMetadataType(v), v->GetName() };
            if (auto prop = interface_cast<IProperty>(v)) {
                info.propertyType = prop->GetTypeId();
            }
            res.push_back(info);
        }
    }
    return res;
}

MetadataInfo ObjectDataContainer::GetMetadata(MetadataType type, BASE_NS::string_view name) const
{
    if (auto s = GetOwner<IStaticMetadata>()) {
        if (auto pm = s->GetStaticMetadata()) {
            if (auto p = FindStaticMetadata(*pm, name, type)) {
                MetadataInfo info { p->type, p->name, p->interfaceInfo };
                if (p->type == MetadataType::PROPERTY) {
                    info.propertyType = GetMetaPropertyType(*p);
                    info.readOnly = p->flags & static_cast<uint8_t>(Internal::PropertyFlag::READONLY);
                }
                info.data = p;
                return info;
            }
        }
    }
    BASE_NS::vector<TypeId> ids = ConvertToTypeIds(type);
    if (auto v = FindAny({ BASE_NS::string(name), TraversalType::NO_HIERARCHY, ids })) {
        MetadataInfo info { GetMetadataType(v), v->GetName() };
        if (auto prop = interface_cast<IProperty>(v)) {
            info.propertyType = prop->GetTypeId();
        }
        return info;
    }
    return {};
}

IProperty::Ptr ObjectDataContainer::GetProperty(BASE_NS::string_view name, MetadataQuery q)
{
    IProperty::Ptr p = interface_pointer_cast<IProperty>(
        FindAny({ BASE_NS::string(name), TraversalType::NO_HIERARCHY, { TypeId(IProperty::UID) } }));

    if (!p && q == MetadataQuery::CONSTRUCT_ON_REQUEST) {
        auto res = ConstructFromMetadata<IProperty>(GetOwner<IOwner>(), name, MetadataType::PROPERTY);
        p = res.object;
        if (p && !res.isForward) {
            Attach(p);
        }
    }
    return p;
}
IProperty::ConstPtr ObjectDataContainer::GetProperty(BASE_NS::string_view name, MetadataQuery q) const
{
    return const_cast<ObjectDataContainer&>(*this).GetProperty(name, q);
}
IFunction::Ptr ObjectDataContainer::GetFunction(BASE_NS::string_view name, MetadataQuery q)
{
    IFunction::Ptr p = interface_pointer_cast<IFunction>(
        FindAny({ BASE_NS::string(name), TraversalType::NO_HIERARCHY, { TypeId(IFunction::UID) } }));

    if (!p && q == MetadataQuery::CONSTRUCT_ON_REQUEST) {
        auto res = ConstructFromMetadata<IFunction>(GetOwner<IOwner>(), name, MetadataType::FUNCTION);
        p = res.object;
        if (p && !res.isForward) {
            Attach(p);
        }
    }
    return p;
}
IFunction::ConstPtr ObjectDataContainer::GetFunction(BASE_NS::string_view name, MetadataQuery q) const
{
    return const_cast<ObjectDataContainer&>(*this).GetFunction(name, q);
}
IEvent::Ptr ObjectDataContainer::GetEvent(BASE_NS::string_view name, MetadataQuery q)
{
    IEvent::Ptr p = interface_pointer_cast<IEvent>(
        FindAny({ BASE_NS::string(name), TraversalType::NO_HIERARCHY, { TypeId(IEvent::UID) } }));

    if (!p && q == MetadataQuery::CONSTRUCT_ON_REQUEST) {
        auto res = ConstructFromMetadata<IEvent>(GetOwner<IOwner>(), name, MetadataType::EVENT);
        p = res.object;
        if (p && !res.isForward) {
            Attach(p);
        }
    }
    return p;
}
IEvent::ConstPtr ObjectDataContainer::GetEvent(BASE_NS::string_view name, MetadataQuery q) const
{
    return const_cast<ObjectDataContainer&>(*this).GetEvent(name, q);
}

} // namespace Internal

META_END_NAMESPACE()
