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

#ifndef API_RENDER_IRENDER_DATA_STORE_H
#define API_RENDER_IRENDER_DATA_STORE_H

#include <base/containers/string_view.h>
#include <render/namespace.h>

BASE_BEGIN_NAMESPACE()
struct Uid;
BASE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastore */
/** Base class for render data management.
 * Inherit to create a new data manager.
 */
class IRenderDataStore {
public:
    IRenderDataStore(const IRenderDataStore&) = delete;
    IRenderDataStore& operator=(const IRenderDataStore&) = delete;

    /** Called automatically by the renderer when render data store manager CommitFrameData is called.
     * This is called automatically every frame.
     * Needs to be overwritten by the inherit classes.
     * Here one could e.g. flip double buffers etc.
     */
    virtual void CommitFrameData() = 0;

    /** Called automatically by the engine (renderer) before render node front-end processing.
     * Needs to be overwritten by the inherit classes.
     * Here one could e.g. allocate GPU resources for current counts or do some specific management
     * for all this frame's data.
     */
    virtual void PreRender() = 0;

    /** Called automatically by the engine (renderer) after rendering front-end has been completed.
     * This happens before PreRenderBackend. For example, clear render data store data which has been processed in render
     * nodes. Needs to be overwritten by the inherit classes. Here one can place resets for buffers and data.
     */
    virtual void PostRender() = 0;

    /** Called automatically by the engine (renderer) before back-end processing.
     * Here (and only here with render data stores) one can access low level / back-end GPU resources.
     * Needs to be overwritten by the inherit classes.
     * Process only backend related work here.
     */
    virtual void PreRenderBackend() = 0;

    /** Called automatically by the engine (renderer) after all rendering back-end processing (full rendering)
     * Needs to be overwritten by the inherit classes.
     * Process only post full render here. (Clear data store content already in PostRender())
     */
    virtual void PostRenderBackend() = 0;

    /** Should be called when one starts to fill up the render data store.
     * Needs to be overwritten by the inherit classes.
     * This should clear all the buffers which are filled once a frame.
     */
    virtual void Clear() = 0;

    /** Should return IRenderDataStoreManager::RenderDataStoreFlags which are implemented by the data store.
     * Do not print flags which are not supported by the data store.
     * Needs to be overwritten by the inherit classes.
     * Used by the renderer for e.g. validation. (Can print warnings when correct flags are not returned.)
     */
    virtual uint32_t GetFlags() const = 0;

    /** Get the type name of the render data store.
     * @return Type name of the render data store.
     */
    virtual BASE_NS::string_view GetTypeName() const = 0;

    /** Get the unique instance name of the render data store.
     * @return Unique name of the render data store.
     */
    virtual BASE_NS::string_view GetName() const = 0;

    /** Get the type UID of the render data store.
     * @return Type UID of the render data store.
     */
    virtual const BASE_NS::Uid& GetUid() const = 0;

protected:
    IRenderDataStore() = default;
    virtual ~IRenderDataStore() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_H
