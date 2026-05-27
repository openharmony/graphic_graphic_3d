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

#ifndef SCENE_SRC_RESOURCE_RENDER_RESOURCE_MANAGER_H
#define SCENE_SRC_RESOURCE_RENDER_RESOURCE_MANAGER_H

#include <scene/interface/intf_render_context.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class RenderResourceManager : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IRenderResourceManager> {
    META_OBJECT(RenderResourceManager, ClassId::RenderResourceManager, IntroduceInterfaces)
public:
    bool Build(const META_NS::IMetadata::Ptr&) override;

    Future<IImage::Ptr> CreateImage(const ImageCreateInfo& info, BASE_NS::vector<uint8_t> data) override;
    Future<IImage::Ptr> LoadImage(BASE_NS::string_view uri, const ImageLoadInfo&) override;
    IImage::Ptr LoadImageDeferred(BASE_NS::string_view uri, const ImageLoadInfo&) override;
    void WaitAllPendingLoads() override;
    Future<IShader::Ptr> LoadShader(BASE_NS::string_view uri) override;

    /**
     * @brief Drain the process-wide deferred-load queue and tear down the
     * shared decode thread pool. Must be called while the LumeEngine plugin
     * (which owns the ITaskQueueFactory used to construct the pool) is still
     * loaded; otherwise the pool's destructor joins worker threads via code
     * that has already been unmapped, causing a segfault at process exit.
     *
     * Safe to call multiple times — second and later calls are no-ops.
     * Called from the LumeScene plugin's UnregisterInterfaces.
     */
    static void Shutdown();

private:
    IRenderContext::Ptr context_;
};

SCENE_END_NAMESPACE()

#endif