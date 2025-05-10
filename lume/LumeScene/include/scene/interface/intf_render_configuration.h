
#ifndef SCENE_INTERFACE_IRENDER_CONFIGURATION_H
#define SCENE_INTERFACE_IRENDER_CONFIGURATION_H

#include <scene/base/namespace.h>
#include <scene/interface/intf_environment.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IRenderConfiguration : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRenderConfiguration, "cdedd61a-0e33-4eb7-8148-687ddbc2c2da")
public:
    META_PROPERTY(IEnvironment::Ptr, Environment)
    META_PROPERTY(BASE_NS::string, RenderNodeGraphUri)
    META_PROPERTY(BASE_NS::string, PostRenderNodeGraphUri)
};

META_REGISTER_CLASS(
    RenderConfiguration, "fa191fb1-311d-419c-b359-1737496a0305", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IRenderConfiguration)

#endif
