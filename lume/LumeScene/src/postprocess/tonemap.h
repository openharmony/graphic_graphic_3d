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

#ifndef SCENE_SRC_POSTPROCESS_TONEMAP_H
#define SCENE_SRC_POSTPROCESS_TONEMAP_H

#include <scene/interface/postprocess/intf_tonemap.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class Tonemap : public META_NS::IntroduceInterfaces<META_NS::ObjectFwd, ITonemap> {
    META_OBJECT(Tonemap, SCENE_NS::ClassId::Tonemap, IntroduceInterfaces)
public:
    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IPostProcessEffect, bool, Enabled)
    META_STATIC_PROPERTY_DATA(ITonemap, TonemapType, Type, TonemapType::ACES)
    META_STATIC_PROPERTY_DATA(ITonemap, float, Exposure, 0.7f)
    META_END_STATIC_DATA()

    META_IMPLEMENT_PROPERTY(bool, Enabled)
    META_IMPLEMENT_PROPERTY(TonemapType, Type)
    META_IMPLEMENT_PROPERTY(float, Exposure)
};

SCENE_END_NAMESPACE()

#endif