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

#ifndef API_RENDER_IPLUGIN_H
#define API_RENDER_IPLUGIN_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/uid.h>
#include <core/plugin/intf_plugin.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IRenderDataStore;
class IRenderNode;

/** \addtogroup group_plugin_iplugin
 *  @{
 */
/** Information needed from the plugin for managing RenderDataStores. */
struct RenderDataStoreTypeInfo : public CORE_NS::ITypeInfo {
    static constexpr BASE_NS::Uid UID { "79dd23ac-db4f-476e-85cd-a285a3aa4fb0" };

    using CreateRenderDataStoreFn = IRenderDataStore* (*)(IRenderContext& renderContext, char const* instanceName);
    using DestroyRenderDataStoreFn = void (*)(IRenderDataStore* instance);

    /** Unique ID of the rander data store. */
    const BASE_NS::Uid uid;
    /** Name used during data store creation to identify the type of the data store. */
    char const* const typeName { "" };
    /** Pointer to function which is used to create data store instances. */
    const CreateRenderDataStoreFn createDataStore;
    /** Pointer to function which is used to destory data store instances. */
    const DestroyRenderDataStoreFn destroyDataStore;
};

/** Information needed from the plugin for managing RenderNodes. */
struct RenderNodeTypeInfo : public CORE_NS::ITypeInfo {
    /** TypeInfo UID for render node. */
    static constexpr BASE_NS::Uid UID { "92085439-2cf7-4762-8769-28b552f4c5a4" };

    using CreateRenderNodeFn = IRenderNode* (*)();
    using DestroyRenderNodeFn = void (*)(IRenderNode* instance);
    using PluginRenderNodeClassType = uint32_t;
    using PluginRenderNodeBackendFlags = uint32_t;

    /** Unique ID of the rander node. */
    const BASE_NS::Uid uid;
    /** Name used during node creation to identify the type of the node. */
    char const* const typeName { "" };
    /** Pointer to function which is used to create node instances. */
    const CreateRenderNodeFn createNode;
    /** Pointer to function which is used to destroy node instances. */
    const DestroyRenderNodeFn destroyNode;

    /** Render node backend flags (see IRenderNode) */
    PluginRenderNodeBackendFlags renderNodeBackendFlags { 0u };
    /** Render node class type (see IRenderNode) */
    PluginRenderNodeClassType renderNodeClassType { 0u };
};

/** A plugin which adds new render data store and render node types. */
struct IRenderPlugin : public CORE_NS::ITypeInfo {
    /** TypeInfo UID for render plugin. */
    static constexpr BASE_NS::Uid UID { "303e3ffe-36fd-4e1b-82f3-349844fab2eb" };

    /*
    Plugin lifecycle.
    1. createPlugin  (*as many times as contexts instantiated)   (initialize IRenderContext specific state here.)
    2. destroyPlugin  (*as many times as contexts instantiated)  (deinitialize IRenderContext specific state here.)
    */

    using CreatePluginFn = CORE_NS::PluginToken (*)(IRenderContext&);
    using DestroyPluginFn = void (*)(CORE_NS::PluginToken);

    constexpr IRenderPlugin(CreatePluginFn create, DestroyPluginFn destroy)
        : ITypeInfo { UID }, createPlugin { create }, destroyPlugin { destroy }
    {}

    /** Initialize function for render plugin.
     * Called when plugin is initially loaded by context. Used to register paths etc.
     * Is expected to register its own named interfaces (IInterface) which are tied to the context instance.
     * Called when attaching to engine.
     */
    const CreatePluginFn createPlugin { nullptr };

    /** Deinitialize function for render plugin.
     * Called when plugin is about to be unloaded by context.
     * Called when detaching from context.
     */
    const DestroyPluginFn destroyPlugin { nullptr };
};
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_IPLUGIN_H
