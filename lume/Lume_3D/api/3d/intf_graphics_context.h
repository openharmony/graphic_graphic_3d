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

#ifndef API_3D_IGRAPHICS_CONTEXT_H
#define API_3D_IGRAPHICS_CONTEXT_H

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>
#include <render/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEngine;
class IEcs;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class RenderHandleReference;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class ISceneUtil;
class IRenderUtil;
class IMeshUtil;
class IGltf2;

/**
 * IGraphicsContext.
 * Graphics context interface for 3D rendering use cases.
 */
class IGraphicsContext : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid { "c6eb95b1-8b32-40f7-8acd-7969792e574b" };

    using Ptr = BASE_NS::refcnt_ptr<IGraphicsContext>;

    /** Initialize the context.
     */
    virtual void Init() = 0;

    /** Get rendering context.
     * @return Reference to context.
     */
    virtual RENDER_NS::IRenderContext& GetRenderContext() const = 0;

    /** Get render node graphs from ECS instance.
     * @return Array view of ECS instance render node graphs.
     */
    virtual BASE_NS::array_view<const RENDER_NS::RenderHandleReference> GetRenderNodeGraphs(
        const CORE_NS::IEcs& ecs) const = 0;

    /** Get SceneUtil interface. Uses graphics context resource creator and engine internally.
     * @return Reference to scene util interface.
     */
    virtual ISceneUtil& GetSceneUtil() const = 0;

    /** Get MeshUtil interface. Uses graphics context resource creator and engine internally.
     * @return Reference to mesh util interface.
     */
    virtual IMeshUtil& GetMeshUtil() const = 0;

    /** Get Gltf2 interface. Uses graphics context resource creator and engine internally.
     * @return Reference to gltf2 interface.
     */
    virtual IGltf2& GetGltf() const = 0;

    /** Get render util.
     * @return Reference to render util.
     */
    virtual IRenderUtil& GetRenderUtil() const = 0;

protected:
    IGraphicsContext() = default;
    virtual ~IGraphicsContext() = default;
};
CORE3D_END_NAMESPACE()

#endif // API_3D_IGRAPHICS_CONTEXT_H
