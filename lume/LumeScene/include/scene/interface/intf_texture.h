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

#ifndef SCENE_INTERFACE_ITEXTURE_H
#define SCENE_INTERFACE_ITEXTURE_H

#include <scene/base/namespace.h>
#include <scene/interface/intf_bitmap.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Predefined sampler types
 */
enum class TextureSampler {
    UNKNOWN,
    /** Default sampler, nearest repeat */
    NEAREST_REPEAT,
    /** Default sampler, nearest clamp */
    NEAREST_CLAMP,
    /** Default sampler, linear repeat */
    LINEAR_REPEAT,
    /** Default sampler, linear clamp */
    LINEAR_CLAMP,
    /** Default sampler, linear mipmap repeat */
    LINEAR_MIPMAP_REPEAT,
    /** Default sampler, linear mipmap clamp */
    LINEAR_MIPMAP_CLAMP,
    END_OF_LIST
};

class ITexture : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITexture, "38c66860-d325-4256-becd-ffd2f94fbd8a")
public:
    /**
     * @brief Texture Image
     */
    META_PROPERTY(IBitmap::Ptr, Image)
    /**
     * @brief Sampler type to be used with texture.
     */
    META_PROPERTY(TextureSampler, Sampler)
    /**
     * @brief Factor
     */
    META_PROPERTY(BASE_NS::Math::Vec4, Factor)
    /**
     * @brief Translation of the texture.
     */
    META_PROPERTY(BASE_NS::Math::Vec2, Translation)
    /**
     * @brief Rotation of the texture.
     */
    META_PROPERTY(float, Rotation)
    /**
     * @brief Scale of the texture.
     */
    META_PROPERTY(BASE_NS::Math::Vec2, Scale)
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::TextureSampler)
META_INTERFACE_TYPE(SCENE_NS::ITexture)

#endif
