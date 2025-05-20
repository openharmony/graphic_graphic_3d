/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#include <base/containers/refcnt_ptr.h>
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
    static constexpr BASE_NS::Uid UID { "946e0b9c-5619-46a9-a087-cd0b34807179" };

    using CreateRenderDataStoreFn = BASE_NS::refcnt_ptr<IRenderDataStore> (*)(
        IRenderContext& renderContext, const char* instanceName);

    /** Unique ID of the render data store. */
    const BASE_NS::Uid uid;
    /** Name used during data store creation to identify the type of the data store. */
    const char* const typeName { "" };
    /** Pointer to function which is used to create data store instances. */
    const CreateRenderDataStoreFn createDataStore;
};

/** Information needed from the plugin for managing RenderNodes. */
struct RenderNodeTypeInfo : public CORE_NS::ITypeInfo {
    /** TypeInfo UID for render node. */
    static constexpr BASE_NS::Uid UID { "d54d3800-7378-43e6-bde8-8095660dd7f1" };

    using CreateRenderNodeFn = IRenderNode* (*)();
    using DestroyRenderNodeFn = void (*)(IRenderNode* instance);
    using PluginRenderNodeClassType = uint32_t;
    using PluginRenderNodeBackendFlags = uint32_t;

    /** Unique ID of the render node. */
    const BASE_NS::Uid uid;
    /** Name used during node creation to identify the type of the node. */
    const char* const typeName { "" };
    /** Pointer to function which is used to create node instances. */
    const CreateRenderNodeFn createNode;
    /** Pointer to function which is used to destroy node instances. */
    const DestroyRenderNodeFn destroyNode;

    /** Render node backend flags (see IRenderNode) */
    PluginRenderNodeBackendFlags renderNodeBackendFlags { 0u };
    /** Render node class type (see IRenderNode) */
    PluginRenderNodeClassType renderNodeClassType { 0u };

    /** Optional unique ID of a render node, which should be before this node. */
    const BASE_NS::Uid afterNode;
    /** Optional unique ID of a render node, which should be after this node. */
    const BASE_NS::Uid beforeNode;
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
