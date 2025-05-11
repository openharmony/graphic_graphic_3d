
#include <ComponentTools/base_manager.h>
#include <ComponentTools/base_manager.inl>

#include "entity_owner_component.h"

#define IMPLEMENT_MANAGER
#include <core/property_tools/property_macros.h>

SCENE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::countof;

using CORE_NS::BaseManager;
using CORE_NS::IComponentManager;
using CORE_NS::IEcs;
using CORE_NS::Property;

class EntityOwnerComponentManager final : public BaseManager<EntityOwnerComponent, IEntityOwnerComponentManager> {
    BEGIN_PROPERTY(EntityOwnerComponent, componentMetaData_)
#include "entity_owner_component.h"
    END_PROPERTY();

public:
    explicit EntityOwnerComponentManager(IEcs& ecs)
        : BaseManager<EntityOwnerComponent, IEntityOwnerComponentManager>(ecs, CORE_NS::GetName<EntityOwnerComponent>())
    {}

    ~EntityOwnerComponentManager() = default;

    size_t PropertyCount() const override
    {
        return BASE_NS::countof(componentMetaData_);
    }

    const Property* MetaData(size_t index) const override
    {
        if (index < BASE_NS::countof(componentMetaData_)) {
            return &componentMetaData_[index];
        }
        return nullptr;
    }

    array_view<const Property> MetaData() const override
    {
        return componentMetaData_;
    }
};

IComponentManager* IEntityOwnerComponentManagerInstance(IEcs& ecs)
{
    return new EntityOwnerComponentManager(ecs);
}

void IEntityOwnerComponentManagerDestroy(IComponentManager* instance)
{
    delete static_cast<EntityOwnerComponentManager*>(instance);
}
SCENE_END_NAMESPACE()
