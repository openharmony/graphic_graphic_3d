
#ifndef SCENE_INTERFACE_POSTPROCESS_IPOSTPROCESS_EFFECT_H
#define SCENE_INTERFACE_POSTPROCESS_IPOSTPROCESS_EFFECT_H

#include <scene/base/types.h>

SCENE_BEGIN_NAMESPACE()

class IPostProcessEffect : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPostProcessEffect, "7e8243b4-b6c8-482d-9ef4-0ad2828618e2")
public:
    META_PROPERTY(bool, Enabled)
};

enum class EffectQualityType {
    /** Low quality */
    LOW = 0,
    /** Normal quality */
    NORMAL = 1,
    /** High quality */
    HIGH = 2,
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::EffectQualityType)
META_INTERFACE_TYPE(SCENE_NS::IPostProcessEffect)

#endif
