
#ifndef SCENE_INTERFACE_POSTPROCESS_ITONEMAP_H
#define SCENE_INTERFACE_POSTPROCESS_ITONEMAP_H

#include <scene/base/types.h>
#include <scene/interface/postprocess/intf_postprocess_effect.h>

SCENE_BEGIN_NAMESPACE()

enum class TonemapType {
    /** Aces */
    ACES = 0,
    /** Aces 2020 */
    ACES_2020 = 1,
    /** Filmic */
    FILMIC = 2,
};

class ITonemap : public IPostProcessEffect {
    META_INTERFACE(IPostProcessEffect, ITonemap, "aec077b9-42bf-49e3-87f4-d3b7e821768f")
public:
    /**
     * @brief Camera postprocessing settings, tonemap type
     * @return
     */
    META_PROPERTY(TonemapType, Type)

    /**
     * @brief Camera postprocessing settings, tonemap exposure
     * @return
     */
    META_PROPERTY(float, Exposure)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::TonemapType)
META_INTERFACE_TYPE(SCENE_NS::ITonemap)

#endif
