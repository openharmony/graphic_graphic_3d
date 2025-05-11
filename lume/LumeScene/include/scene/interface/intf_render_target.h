
#ifndef SCENE_INTERFACE_IRENDER_TARGET_H
#define SCENE_INTERFACE_IRENDER_TARGET_H

#include <scene/base/namespace.h>

#include <render/resource_handle.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IRenderTarget : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRenderTarget, "b748e3c0-62f1-4ef8-ba77-a5f46722144b")
public:
    META_READONLY_PROPERTY(BASE_NS::Math::UVec2, Size)

    virtual RENDER_NS::RenderHandleReference GetRenderHandle() const = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IRenderTarget)

#endif
