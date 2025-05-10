
#ifndef SCENE_INTERFACE_IPOSTPROCESS_H
#define SCENE_INTERFACE_IPOSTPROCESS_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_bloom.h>
#include <scene/interface/postprocess/intf_tonemap.h>

SCENE_BEGIN_NAMESPACE()

class IPostProcess : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPostProcess, "a0b91095-0383-40f8-a211-5735ca5440f9")
public:
    /**
     * @brief Camera postprocessing settings, tonemap
     * @return
     */
    META_READONLY_PROPERTY(ITonemap::Ptr, Tonemap)
    /**
     * @brief Camera postprocessing settings, bloom
     * @return
     */
    META_READONLY_PROPERTY(IBloom::Ptr, Bloom)
};

META_REGISTER_CLASS(PostProcess, "2df6c964-5a32-4270-86e6-755e3f5697ff", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IPostProcess)

#endif
