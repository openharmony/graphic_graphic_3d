/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "builtin.h"

#include <meta/base/meta_types.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/resource/intf_resource.h>

#include "../import_helpers.h"
#include "../resolve_object.h"

SCENE_IMP_BEGIN_NAMESPACE()

template <typename Type>
class BuiltinContainerImpl : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, IBuiltinContainer> {
    META_NS::ObjectId GetClassId() const override
    {
        return META_NS::MakeUid<Type>("ImpBuilt");
    }
    BASE_NS::string_view GetClassName() const override
    {
        return "BuiltinContainerImpl";
    }
    BASE_NS::vector<BASE_NS::Uid> GetInterfaces() const override
    {
        return GetInterfacesVector();
    }

public:
    BuiltinContainerImpl(Type obj) : object_(BASE_NS::move(obj))
    {}

    bool SetToAny(META_NS::IAny& out) override
    {
        return out.SetValue(object_);
    }

private:
    Type object_;
};

ImportResult ImportResourceId::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "resourceId")) {
        return ImportResult{err};
    }

    auto name = context.GetOptString("name");
    auto group = context.GetOptString("group");

    CORE_NS::ResourceId rid(name, group);
    if (!rid.IsValid()) {
        return ImportResult{context.CreateDiagnostics("Invalid resourceId")};
    }
    return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<CORE_NS::ResourceId>>(BASE_NS::move(rid))};
}

ImportResult ImportVec2::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "vec2")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto val = GetOptVec2(context, "value"); h.HandleOptValue(val)) {
        if (val.error) {
            return ImportResult{val.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Math::Vec2>>(*val.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid vec2, value missing")};
}

ImportResult ImportVec3::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "vec3")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto pos = GetOptVec3(context, "value"); h.HandleOptValue(pos)) {
        if (pos.error) {
            return ImportResult{pos.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Math::Vec3>>(*pos.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid vec3, value missing")};
}

ImportResult ImportVec4::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "vec4")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto val = GetOptVec4(context, "value"); h.HandleOptValue(val)) {
        if (val.error) {
            return ImportResult{val.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Math::Vec4>>(*val.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid vec4, value missing")};
}

ImportResult ImportColor::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "color")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto val = GetOptColor(context, "value"); h.HandleOptValue(val)) {
        if (val.error) {
            return ImportResult{val.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Color>>(*val.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid color, value missing")};
}

ImportResult ImportQuat::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "quat")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto val = GetOptQuat(context, "value"); h.HandleOptValue(val)) {
        if (val.error) {
            return ImportResult{val.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Math::Quat>>(*val.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid quat, value missing")};
}

ImportResult ImportUVec2::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "uvec2")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto val = GetOptUVec2(context, "value"); h.HandleOptValue(val)) {
        if (val.error) {
            return ImportResult{val.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Math::UVec2>>(*val.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid uvec2, value missing")};
}

ImportResult ImportMat4x4::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "mat4x4")) {
        return ImportResult{err};
    }

    ErrorHandler h(context);
    if (auto val = GetOptMat4x4(context, "value"); h.HandleOptValue(val)) {
        if (val.error) {
            return ImportResult{val.error};
        }
        return ImportResult{BASE_NS::make_shared<BuiltinContainerImpl<BASE_NS::Math::Mat4X4>>(*val.value)};
    }
    return ImportResult{context.CreateDiagnostics("Invalid mat4x4, value missing")};
}

ImportResult ImportObjectRef::Import(ImportContext& context)
{
    if (auto err = context.RequireString("type", "objectRef")) {
        return ImportResult{err};
    }
    auto path = context.GetOptString("path");
    if (path.empty()) {
        return ImportResult{context.CreateDiagnostics("objectRef missing path")};
    }
    return ResolveObject(context, context.GetImportParameters().object, path);
}

SCENE_IMP_END_NAMESPACE()