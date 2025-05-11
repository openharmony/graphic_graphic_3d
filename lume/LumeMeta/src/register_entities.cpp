#include <meta/base/namespace.h>
#include <meta/interface/intf_object_registry.h>

META_BEGIN_NAMESPACE()

namespace Internal {

void RegisterBuiltInAnimations(IObjectRegistry&);
void RegisterBuiltInObjects(IObjectRegistry&);
void UnRegisterBuiltInObjects(IObjectRegistry&);
void UnRegisterBuiltInProperties(IObjectRegistry&);
void UnRegisterBuiltInSerializers(IObjectRegistry&);
void UnRegisterBuiltInAnimations(IObjectRegistry&);

void RegisterEntities(IObjectRegistry& registry)
{
    Internal::RegisterBuiltInObjects(registry);
    Internal::RegisterBuiltInAnimations(registry);
}

void UnRegisterEntities(IObjectRegistry& registry)
{
    Internal::UnRegisterBuiltInAnimations(registry);
    Internal::UnRegisterBuiltInObjects(registry);
}

} // namespace Internal
META_END_NAMESPACE()
