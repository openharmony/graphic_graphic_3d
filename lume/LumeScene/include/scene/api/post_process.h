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
#ifndef SCENE_API_POST_PROCESS_H
#define SCENE_API_POST_PROCESS_H

#include <scene/api/resource.h>
#include <scene/interface/intf_postprocess.h>

SCENE_BEGIN_NAMESPACE()

class Tonemap : public META_NS::InterfaceObject<ITonemap> {
public:
    META_INTERFACE_OBJECT(Tonemap, META_NS::InterfaceObject<ITonemap>, ITonemap)
    /// @see ITonemap::Type
    META_INTERFACE_OBJECT_PROPERTY(TonemapType, Type)
    /// @see ITonemap::Exposure
    META_INTERFACE_OBJECT_PROPERTY(float, Exposure)
};

class Bloom : public META_NS::InterfaceObject<IBloom> {
public:
    META_INTERFACE_OBJECT(Bloom, META_NS::InterfaceObject<IBloom>, IBloom)
    /// @see IBloom::Type
    META_INTERFACE_OBJECT_PROPERTY(BloomType, Type)
    /// @see IBloom::Quality
    META_INTERFACE_OBJECT_PROPERTY(EffectQualityType, Quality)
    /// @see IBloom::ThresholdHard
    META_INTERFACE_OBJECT_PROPERTY(float, ThresholdHard)
    /// @see IBloom::ThresholdSoft
    META_INTERFACE_OBJECT_PROPERTY(float, ThresholdSoft)
    /// @see IBloom::AmountCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, AmountCoefficient)
    /// @see IBloom::DirtMaskCoefficient
    META_INTERFACE_OBJECT_PROPERTY(float, DirtMaskCoefficient)
    /// @see IBloom::DirtMaskImage
    META_INTERFACE_OBJECT_PROPERTY(Image, DirtMaskImage)
    /// @see IBloom::UseCompute
    META_INTERFACE_OBJECT_PROPERTY(bool, UseCompute)
    /// @see IBloom::Scatter
    META_INTERFACE_OBJECT_PROPERTY(float, Scatter)
    /// @see IBloom::ScaleFactor
    META_INTERFACE_OBJECT_PROPERTY(float, ScaleFactor)
};

class PostProcess : META_NS::InterfaceObject<IPostProcess> {
public:
    META_INTERFACE_OBJECT(PostProcess, META_NS::InterfaceObject<IPostProcess>, IPostProcess)
    META_INTERFACE_OBJECT_INSTANTIATE(PostProcess, ClassId::PostProcess)
    /// @see IPostProcess::Tonemap
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Tonemap, Tonemap)
    /// @see IPostProcess::Bloom
    META_INTERFACE_OBJECT_READONLY_PROPERTY(SCENE_NS::Bloom, Bloom)
};

SCENE_END_NAMESPACE()

#endif // SCENE_API_POST_PROCESS_H
