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

#ifndef API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_SCENE_H
#define API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_SCENE_H

#include <3d/render/render_data_defines_3d.h>
#include <base/containers/string_view.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastoredefaultscene */
/** RenderDataStoreDefaultScene interface.
 * Not internally syncronized.
 */
class IRenderDataStoreDefaultScene : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "1f43a445-1e26-417b-878a-d3044fde6e2c" };

    ~IRenderDataStoreDefaultScene() override = default;

    /** Set scene
     * @param scene Default scene to set
     */
    virtual void SetScene(const RenderScene& scene) = 0;

    /** Get scene.
     * @param name Name of the scene.
     * @return render scene.
     */
    virtual RenderScene GetScene(const BASE_NS::string_view name) const = 0;

    /** Get (default) scene. Returns the first scene.
     * @return render scene.
     */
    virtual RenderScene GetScene() const = 0;

protected:
    IRenderDataStoreDefaultScene() = default;
};
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_SCENE_H
