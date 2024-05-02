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
#include <scene_plugin/api/material_uid.h>

#include <meta/ext/concrete_base_object.h>

#include "bind_templates.inl"
#include "intf_node_private.h"

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;

namespace {
class ShaderImpl : public META_NS::ObjectFwd<ShaderImpl, SCENE_NS::ClassId::Shader, META_NS::ClassId::Object,
                       SCENE_NS::IShader, ISceneHolderRef> {
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IShader, BASE_NS::string, Uri, "")

    bool Build(const IMetadata::Ptr& data) override
    {
        if (!data) {
            // no parameters, so legacy init
            return true;
        }
        auto rcp = data->GetPropertyByName<IntfPtr>("RenderContext");
        if (!rcp) {
            // render context not given as parameter.
            return true;	
        }
        auto urp = data->GetPropertyByName<BASE_NS::string>("uri");
        if (urp) {
            // uri given as parameter
            if (auto renderContext = interface_cast<RENDER_NS::IRenderContext>(rcp->GetValue())) {
                // we have render context, so we can access ahder manger
                auto& shaderMan = renderContext->GetDevice().GetShaderManager();
                auto uri = urp->GetValue();
                shaderMan.LoadShaderFile(uri);
                renderHandleReference_  = shaderMan.GetShaderHandle(uri);
            }
        }
        return true;
    }

    RENDER_NS::RenderHandleReference GetRenderHandleReference(RENDER_NS::IShaderManager& shaderManager) override
    {
        if (RENDER_NS::RenderHandleUtil::IsValid(renderHandleReference_.GetHandle())) {
            return renderHandleReference_;
        }

        auto uri = GetValue(META_ACCESS_PROPERTY(Uri));

        renderHandleReference_ = shaderManager.GetShaderHandle(uri);
        if (!renderHandleReference_) {
            shaderManager.LoadShaderFile(uri);
            renderHandleReference_ = shaderManager.GetShaderHandle(uri);
        }

        return renderHandleReference_;
    }

    void SetSceneHolder(const SceneHolder::WeakPtr& sh) override
    {
        sh_ = sh;
    }

    SceneHolder::Ptr SceneHolder() override
    {
        return sh_.lock();
    }

    void SetIndex(size_t ix) override
    {
        ix_ = ix;
    }

private:
    RENDER_NS::RenderHandleReference renderHandleReference_;
    SceneHolder::WeakPtr sh_;
    size_t ix_ { 0 };
};
} // namespace
SCENE_BEGIN_NAMESPACE()

void RegisterShaderImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<ShaderImpl>();
}
void UnregisterShaderImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<ShaderImpl>();
}

SCENE_END_NAMESPACE()
