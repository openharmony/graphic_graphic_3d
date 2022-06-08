/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

    /** Called automatically by the engine (renderer) before render node front-end processing.
     * Needs to be overwritten by the inherit classes.
     * Here one could e.g. allocate GPU resources for current counts or do some specific management
     * for all this frame's data.
     */
    virtual void PreRender() = 0;

    /** Called automatically by the engine (renderer) before back-end processing.
     * Needs to be overwritten by the inherit classes.
     * Here (and only here with render data stores) one can access low level / back-end GPU resources.
     * Process only backend related work here.
     */
    virtual void PreRenderBackend() = 0;

    /** Called automatically by the engine (renderer) after all rendering (front-end / back-end) has been completed.
     * Needs to be overwritten by the inherit classes.
     * Here one can place resets for buffers and data.
     */
    virtual void PostRender() = 0;

    /** Should be called when one starts to fill up the render data store.
     * Needs to be overwritten by the inherit classes.
     * This should clear all the buffers which are filled once a frame.
     */
    virtual void Clear() = 0;

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
