/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_RENDER_IRENDER_DATA_STORE_FULLSCREEN_SHADER_H
#define API_RENDER_IRENDER_DATA_STORE_FULLSCREEN_SHADER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/device/intf_shader_pipeline_binder.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastorefullscreenshader */
/** IRenderDataStoreFullscreenShader interface.
 * Internally synchronized.
 *
 * One must call Destroy() to destroy IShaderPipelineBinder::Ptr reference from render data store.
 *
 * The render data store could be used for fullscreen effect rendering.
 * One should create a IShaderPipelineBinder from shader,
 * and bind buffers or uniform data in code, or use materialMetadata from shader
 * to initialize pipeline binder uniform data
 *
 * Usage:
 * CreateFullscreenShader() with a unique name/id and a valid shader.
 * Get() get reference counted shader binder object.
 * Destroy() destroy IShaderPipelineBinder::Ptr from render data store.
 */
class IRenderDataStoreFullscreenShader : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "82bd1502-2bd1-4a20-8c99-74f134f11af8" };

    /** Create a new fullscreen material shader and store it to render data store.
     * If the name is already the already created object is returned.
     * @param name A unique name for the fullscreen shader material.
     * @param shader Shader handle.
     * @return Reference counted shader pipeline binder.
     */
    virtual IShaderPipelineBinder::Ptr Create(const BASE_NS::string_view name, const RenderHandleReference& shader) = 0;

    /** Create a new fullscreen material shader and store it to render data store.
     * If the id is already found the already created object is returned.
     * @param id A unique id for the fullscreen shader material.
     * @param shader Shader handle.
     * @return Reference counted shader pipeline binder.
     */
    virtual IShaderPipelineBinder::Ptr Create(const uint64_t id, const RenderHandleReference& shader) = 0;

    /** Destroy the fullscreen shader IShaderPipelineBinder::Ptr from the render data store.
     * It cannot be accessed after destroy from render data store. It can still be alive if someone is keeping the
     * reference.
     * @param name Name of the the stored fullscreen shader.
     */
    virtual void Destroy(const BASE_NS::string_view name) = 0;

    /** Destroy the fullscreen shader IShaderPipelineBinder::Ptr from the render data store.
     * It cannot be accessed after destroy from render data store. It can still be alive if someone is keeping the
     * reference.
     * @param name id of the the stored fullscreen shader.
     */
    virtual void Destroy(const uint64_t id) = 0;

    /** Get reference counted shader pipeline binder object
     * @param name Name
     * @return Reference counted shader pipeline binder. Null if not found.
     */
    virtual IShaderPipelineBinder::Ptr Get(const BASE_NS::string_view name) const = 0;

    /** Get reference counted shader pipeline binder object
     * @param id ID
     * @return Reference counted shader pipeline binder. Null if not found.
     */
    virtual IShaderPipelineBinder::Ptr Get(const uint64_t id) const = 0;

protected:
    virtual ~IRenderDataStoreFullscreenShader() override = default;
    IRenderDataStoreFullscreenShader() = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_FULLSCREEN_SHADER_H
