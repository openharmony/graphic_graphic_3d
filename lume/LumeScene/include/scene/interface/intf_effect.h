/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SCENE_INTERFACE_IEFFECT_H
#define SCENE_INTERFACE_IEFFECT_H

#include <scene/base/namespace.h>
#include <scene/base/types.h>

#include <render/implementation_uids.h>
#include <render/nodecontext/intf_render_post_process.h>

#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

class IScene;

/**
 * @brief The IEffect inferface is the common base interface for all effects.
 */
class IEffect : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEffect, "f468044a-54db-4d06-aa17-d211909beb8a")
public:
    /**
     * @brief Effect enabled flag.
     */
    META_PROPERTY(bool, Enabled)
    /**
     * @brief Initialize the effect.
     * @param scene The scene this effect is associated with.
     * @param effectClassId The class id of the underlying rendering effect. See SCENE_NS::DefaultEffects.
     * @return True if effect is successfully initialized, false if initialization failed or if the effect was already
     *         initialized with a different class id.
     */
    virtual Future<bool> InitializeEffect(
        const BASE_NS::shared_ptr<IScene>& scene, META_NS::ObjectId effectClassId) = 0;
    /**
     * @brief Returns the class id of the underlying effect.
     */
    virtual META_NS::ObjectId GetEffectClassId() const = 0;
    /**
     * @brief Returns the underlying render effect.
     * @note InitializeEffect() must have been successfully called earlier.
     */
    virtual RENDER_NS::IRenderPostProcess::Ptr GetEffect() const = 0;
};

/**
 * @brief A set of default effects that are always available. The available effects and their parameters are the same as
 *        available through PostProcessComponent defined in 3d/ecs/components/post_process_component.h.
 */
namespace DefaultEffects {
/// @brief Bloom effect
static constexpr META_NS::ObjectId BLOOM_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_BLOOM };
/// @brief Lens flare effect
static constexpr META_NS::ObjectId LENS_FLARE_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_FLARE };
/// @brief Blur effect
static constexpr META_NS::ObjectId BLUR_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_BLUR };
/// @brief Depth of field effect
static constexpr META_NS::ObjectId DEPTH_OF_FIELD_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_DOF };
/// @brief FXAA effect
static constexpr META_NS::ObjectId FXAA_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_FXAA };
/// @brief Motion blur effect
static constexpr META_NS::ObjectId MOTION_BLUR_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_MOTION_BLUR };
/// @brief TAA effect
static constexpr META_NS::ObjectId TAA_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_TAA };
/// @brief Upscale effect
static constexpr META_NS::ObjectId UPSCALE_EFFECT_ID { RENDER_NS::UID_RENDER_POST_PROCESS_UPSCALE };
} // namespace DefaultEffects

/// A default class implementing IEffect.
META_REGISTER_CLASS(Effect, "bdcf8bbd-df33-4ac1-8ae9-18e8ab18072c", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IEffect)

#endif // SCENE_INTERFACE_IEFFECT_H
