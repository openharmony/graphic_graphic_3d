
#ifndef SCENE_EXT_IINTERNAL_CAMERA_H
#define SCENE_EXT_IINTERNAL_CAMERA_H

#include <scene/base/namespace.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IInternalCamera : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalCamera, "20c6fe7d-a520-4ad9-aaba-0121646a9531")
public:
    virtual void NotifyRenderTargetChanged() = 0;
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IInternalCamera)

#endif