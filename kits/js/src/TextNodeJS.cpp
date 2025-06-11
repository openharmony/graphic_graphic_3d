/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "TextNodeJS.h"

#include <font/implementation_uids.h>
#include <font/intf_font_manager.h>
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_text.h>

#include <render/implementation_uids.h>

#include "SceneJS.h"
#include "ParamParsing.h"

namespace {
void PrintFontsFromNode(const SCENE_NS::INode::Ptr& someNode)
{
    if (auto ecsAccess = interface_pointer_cast<SCENE_NS::IEcsObjectAccess>(someNode)) {
        if (auto engineClassRegister = ecsAccess->GetEcsObject()
                                           ->GetScene()
                                           ->GetEcsContext()
                                           .GetNativeEcs()
                                           ->GetClassFactory()
                                           .GetInterface<CORE_NS::IClassRegister>()) {
            if (auto* renderClassRegister = CORE_NS::GetInstance<CORE_NS::IClassRegister>(
                    *engineClassRegister, RENDER_NS::UID_RENDER_CONTEXT)) {
                auto fontManager =
                    CORE_NS::GetInstance<FONT_NS::IFontManager>(*renderClassRegister, FONT_NS::UID_FONT_MANAGER);

                CORE_LOG_E("Fonts");
                for (auto& face : fontManager->GetTypeFaces()) {
                    CORE_LOG_E("face: \"%s\" family: \"%s\"", BASE_NS::string(face.name).c_str(),
                        BASE_NS::string(face.styleName).c_str());
                }
            }
        }
    }
}

BASE_NS::string GetDefaultFont(const SCENE_NS::INode::Ptr& node)
{
    BASE_NS::string defaultFont = "";
    auto ecsAccess = interface_pointer_cast<SCENE_NS::IEcsObjectAccess>(node);
    if (!ecsAccess) {
        return defaultFont;
    }
    auto engineClassRegister = ecsAccess->GetEcsObject()
                                   ->GetScene()
                                   ->GetEcsContext()
                                   .GetNativeEcs()
                                   ->GetClassFactory()
                                   .GetInterface<CORE_NS::IClassRegister>();
    if (!engineClassRegister) {
        return defaultFont;
    }
    if (auto *renderClassRegister =
            CORE_NS::GetInstance<CORE_NS::IClassRegister>(*engineClassRegister, RENDER_NS::UID_RENDER_CONTEXT)) {
        auto fontManager = CORE_NS::GetInstance<FONT_NS::IFontManager>(*renderClassRegister, FONT_NS::UID_FONT_MANAGER);
        if (fontManager) {
            auto typeFaces = fontManager->GetTypeFaces();
            if (!typeFaces.empty()) {
                defaultFont = typeFaces.begin()->name;
            }
        }
    }
    return defaultFont;
}
} // anonymous namespace

void TextNodeJS::Init(napi_env env, napi_value exports)
{
    using namespace NapiApi;

    BASE_NS::vector<napi_property_descriptor> node_props = {
        GetSetProperty<BASE_NS::string, TextNodeJS, &TextNodeJS::GetText, &TextNodeJS::SetText>("text"),
        GetSetProperty<BASE_NS::string, TextNodeJS, &TextNodeJS::GetFont, &TextNodeJS::SetFont>("font"),
        GetSetProperty<NapiApi::Object, TextNodeJS, &TextNodeJS::GetColor, &TextNodeJS::SetColor>("color"),
        GetSetProperty<float, TextNodeJS, &TextNodeJS::GetFontSize, &TextNodeJS::SetFontSize>("fontSize"),
    };
    NodeImpl::GetPropertyDescs(node_props);

    napi_value func;
    auto status = napi_define_class(env, "TextNode", NAPI_AUTO_LENGTH, BaseObject::ctor<TextNodeJS>(), nullptr,
        node_props.size(), node_props.data(), &func);

    NapiApi::MyInstanceState* mis;
    NapiApi::MyInstanceState::GetInstance(env, (void**)&mis);
    if (mis) {
        mis->StoreCtor("TextNode", func);
    }
}

TextNodeJS::TextNodeJS(napi_env e, napi_callback_info i) : BaseObject(e, i), NodeImpl(NodeImpl::TEXT)
{
    LOG_V("TextNodeJS ++");

    NapiApi::FunctionContext<NapiApi::Object, NapiApi::Object> fromJs(e, i);
    if (!fromJs) {
        // no arguments. so internal create.
        // expecting caller to finish initialization
        return;
    }

    {
        // add the dispose hook to scene. (so that the geometry node is disposed when scene is disposed)
        NapiApi::Object meJs(fromJs.This());
        NapiApi::Object scene = fromJs.Arg<0>();
        if (const auto sceneJS = scene.GetJsWrapper<SceneJS>()) {
            sceneJS->StrongDisposeHook(reinterpret_cast<uintptr_t>(&scene_), meJs);
        }
    }

    // java script call.. with arguments
    scene_ = fromJs.Arg<0>().valueOrDefault();
    auto scn = scene_.GetObject().GetNative<SCENE_NS::IScene>();
    if (scn == nullptr) {
        // hmm..
        LOG_F("Invalid scene for TextNodeJS!");
        return;
    }
    if (!GetNativeObject()) {
        LOG_E("Cannot finish creating a text node: Native text node object missing");
        assert(false);
        return;
    }

    auto sceneNodeParameters = NapiApi::Object { fromJs.Arg<1>() };
    if (const auto name = ExtractName(sceneNodeParameters); !name.empty()) {
        fromJs.This().Set("name", name);
    }
}

TextNodeJS::~TextNodeJS()
{
    LOG_V("TextNodeJS --");
}

void* TextNodeJS::GetInstanceImpl(uint32_t id)
{
    if (id == TextNodeJS::ID)
        return this;
    return NodeImpl::GetInstanceImpl(id);
}

void TextNodeJS::DisposeNative(void* sc)
{
    if (!disposed_) {
        LOG_V("TextNodeJS::DisposeNative");
        disposed_ = true;

        if (auto* sceneJS = static_cast<SceneJS*>(sc)) {
            sceneJS->ReleaseStrongDispose(reinterpret_cast<uintptr_t>(&scene_));
        }
        scene_.Reset();
    }
}

void TextNodeJS::Finalize(napi_env env)
{
    DisposeNative(scene_.GetObject().GetJsWrapper<SceneJS>());
    BaseObject::Finalize(env);
}

napi_value TextNodeJS::GetText(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    BASE_NS::string name;
    auto native = ctx.This().GetNative();
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    auto text = interface_pointer_cast<SCENE_NS::IText>(native);
    if (!node || !text) {
        LOG_E("TextNodeJS::GetText NodeImpl not a text node!");
        return ctx.GetUndefined();
    }

    auto result = META_NS::GetValue(text->Text());

    return ctx.GetString(result);
}

void TextNodeJS::SetText(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    BASE_NS::string text = ctx.Arg<0>();

    if (auto textNode = ctx.This().GetNative<SCENE_NS::IText>()) {
        textNode->Text()->SetValue(text);
        auto node = interface_pointer_cast<SCENE_NS::INode>(textNode);
        auto fontFamily = META_NS::GetValue(textNode->FontFamily());
        if (node) {
            PrintFontsFromNode(node);
            if (fontFamily.empty()) {
                META_NS::SetValue(textNode->FontFamily(), GetDefaultFont(node));
            }
        }
    } else {
        LOG_W("Unable to set text value to TextNode");
    }
}

napi_value TextNodeJS::GetFont(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto native = ctx.This().GetNative();
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    auto text = interface_pointer_cast<SCENE_NS::IText>(native);
    if (!node || !text) {
        LOG_E("TextNodeJS::GetFont NodeImpl not a text node!");
        return ctx.GetUndefined();
    }

    auto result = META_NS::GetValue(text->FontFamily());

    return ctx.GetString(result);
}

void TextNodeJS::SetFont(NapiApi::FunctionContext<BASE_NS::string>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    BASE_NS::string font = ctx.Arg<0>();

    if (auto textNode = ctx.This().GetNative<SCENE_NS::IText>()) {
        META_NS::SetValue(textNode->FontFamily(), font);
    } else {
        LOG_W("Unable to set font to TextNode");
    }
}

napi_value TextNodeJS::GetColor(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto native = ctx.This().GetNative();
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    auto text = interface_pointer_cast<SCENE_NS::IText>(native);
    if (!node || !text) {
        LOG_E("TextNodeJS::GetColor NodeImpl not a text node!");
        return ctx.GetUndefined();
    }

    if (!color_) {
        color_ = BASE_NS::make_unique<ColorProxy>(ctx.Env(), text->TextColor());
    }

    return color_->Value();
}

void TextNodeJS::SetColor(NapiApi::FunctionContext<NapiApi::Object>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    auto native = ctx.This().GetNative();
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    auto text = interface_pointer_cast<SCENE_NS::IText>(native);
    if (!node || !text) {
        LOG_E("TextNodeJS::SetColor NodeImpl not a text node!");
        return;
    }

    if (!color_) {
        color_ = BASE_NS::make_unique<ColorProxy>(ctx.Env(), text->TextColor());
    }

    color_->SetValue(ctx.Arg<0>());
}

napi_value TextNodeJS::GetFontSize(NapiApi::FunctionContext<>& ctx)
{
    if (!validateSceneRef()) {
        return ctx.GetUndefined();
    }

    auto native = ctx.This().GetNative();
    auto node = interface_pointer_cast<SCENE_NS::INode>(native);
    auto text = interface_pointer_cast<SCENE_NS::IText>(native);
    if (!node || !text) {
        LOG_E("TextNodeJS::GetFontSize NodeImpl not a text node!");
        return ctx.GetUndefined();
    }

    auto result = META_NS::GetValue(text->FontSize());

    return ctx.GetNumber(result);
}

void TextNodeJS::SetFontSize(NapiApi::FunctionContext<float>& ctx)
{
    if (!validateSceneRef()) {
        return;
    }

    float fontSize = ctx.Arg<0>();

    if (auto textNode = ctx.This().GetNative<SCENE_NS::IText>()) {
        textNode->FontSize()->SetValue(fontSize);
    } else {
        LOG_W("Unable to set font size to TextNode");
    }
}
