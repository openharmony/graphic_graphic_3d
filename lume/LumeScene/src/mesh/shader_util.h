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

#ifndef SCENE_SRC_MESH_SHADER_UTIL_H
#define SCENE_SRC_MESH_SHADER_UTIL_H

#include <scene/ext/named_scene_object.h>
#include <scene/interface/intf_shader_util.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class ShaderUtil : public META_NS::IntroduceInterfaces<META_NS::BaseObject, IShaderUtil> {
    META_OBJECT(ShaderUtil, ClassId::ShaderUtil, IntroduceInterfaces)
public:
    Future<IShader::Ptr> CreateDefaultShader() const override;

    bool Build(const META_NS::IMetadata::Ptr&) override;

private:
    IInternalScene::WeakPtr scene_;
};

SCENE_END_NAMESPACE()

#endif
