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
#include "node_impl.h"

namespace {
class GraphicsStateImpl : public META_NS::ObjectFwd<GraphicsStateImpl, SCENE_NS::ClassId::GraphicsState,
                              META_NS::ClassId::Object, SCENE_NS::IGraphicsState, ISceneHolderRef> {
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IGraphicsState, BASE_NS::string, Uri, "")
    META_IMPLEMENT_INTERFACE_PROPERTY(SCENE_NS::IGraphicsState, BASE_NS::string, Variant, "")

    bool Build(const IMetadata::Ptr& data) override
    {
        return true;
    }

    RENDER_NS::RenderHandleReference GetRenderHandleReference(RENDER_NS::IShaderManager& shaderManager) override
    {
        if (RENDER_NS::RenderHandleUtil::IsValid(renderHandleReference_.GetHandle())) {
            return renderHandleReference_;
        }

        auto uri = GetValue(META_ACCESS_PROPERTY(Uri));
        auto variant = GetValue(META_ACCESS_PROPERTY(Variant));

        renderHandleReference_ = shaderManager.GetGraphicsStateHandle(uri, variant);
        if (!renderHandleReference_) {
            // presumably we should resolve the state from shader instead trying to create a default our self
            // the problem is, we don't actually know to which shader we are attached to
            // basically the shader should have set its default during the material component set-up, though
            shaderManager.CreateGraphicsState({ uri, {} }, { variant, {} });
            renderHandleReference_ = shaderManager.GetGraphicsStateHandle(uri, variant);
        }

        return renderHandleReference_;
    }

    void SetGraphicsState(const RENDER_NS::GraphicsState& state, SCENE_NS::IMaterial::Ptr mat) override
    {
        if (auto sh = SceneHolder()) {
            sh->QueueApplicationTask(META_NS::MakeCallback<META_NS::ITaskQueueTask>(
                                         [st = state, sh = sh_, material = BASE_NS::weak_ptr(mat), type = ix_]() {
                                             if (auto sceneHolder = sh.lock()) {
                                                 if (auto mat = interface_cast<SCENE_NS::IEcsObject>(material.lock())) {
                                                     sceneHolder->SetGraphicsState(
                                                         mat->GetEntity(), (SceneHolder::ShaderType)type, st);
                                                 }
                                             }
                                             return false;
                                         }),
                false);
        }
    }

    SCENE_NS::IShaderGraphicsState::Ptr GetGraphicsState(SCENE_NS::IMaterial::Ptr mat) override
    {
        auto ret =
            META_NS::GetObjectRegistry().Create<SCENE_NS::IShaderGraphicsState>(SCENE_NS::ClassId::GraphicsState);
        if (auto sh = SceneHolder()) {
            sh->QueueApplicationTask(
                META_NS::MakeCallback<META_NS::ITaskQueueTask>([sh = sh_, material = BASE_NS::weak_ptr(mat), type = ix_,
                                                               ret_w = BASE_NS::weak_ptr(ret)]() {
                    if (auto sceneHolder = sh.lock()) {
                        if (auto mat = interface_cast<SCENE_NS::IEcsObject>(material.lock())) {
                            if (auto ret = ret_w.lock()) {
                                sceneHolder->GetGraphicsState(mat->GetEntity(), (SceneHolder::ShaderType)type, ret);
                                if (auto typedRet =
                                        interface_cast<SCENE_NS::IPendingRequestData<RENDER_NS::GraphicsState>>(ret)) {
                                    typedRet->MarkReady();
                                }
                            }
                        }
                    }
                    return false;
                }),
                false);
        }

        return ret;
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
void RegisterGraphicsStateImpl()
{
    META_NS::GetObjectRegistry().RegisterObjectType<GraphicsStateImpl>();
}
void UnregisterGraphicsStateImpl()
{
    META_NS::GetObjectRegistry().UnregisterObjectType<GraphicsStateImpl>();
}
SCENE_END_NAMESPACE()