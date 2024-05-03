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

#ifndef API_3D_IPLUGIN_H
#define API_3D_IPLUGIN_H

#include <cstdint>

#include <3d/namespace.h>
#include <base/util/uid.h>
#include <core/plugin/intf_plugin.h>

CORE3D_BEGIN_NAMESPACE()
class IGraphicsContext;

/** A plugin which adds 3D related interfaces. */
struct I3DPlugin : public CORE_NS::ITypeInfo {
    /** TypeInfo UID for 3D plugin. */
    static constexpr BASE_NS::Uid UID { "2835609d-04c8-4e0a-820a-0571232c3134" };

    /*
    Plugin lifecycle.
    1. createPlugin  (*as many times as contexts instantiated)   (initialize IGraphicsContext specific state here.)
    2. destroyPlugin  (*as many times as contexts instantiated)  (deinitialize IGraphicsContext specific state here.)
    */

    using CreatePluginFn = CORE_NS::PluginToken (*)(IGraphicsContext&);
    using DestroyPluginFn = void (*)(CORE_NS::PluginToken);

    constexpr I3DPlugin(CreatePluginFn create, DestroyPluginFn destroy)
        : ITypeInfo { UID }, createPlugin { create }, destroyPlugin { destroy }
    {}

    /** Initialize function for 3D plugin.
     * Called when plugin is initially loaded by context.
     * Is expected to register its own named interfaces (IInterface) which are tied to the context instance.
     * Called when attaching to graphics context.
     */
    const CreatePluginFn createPlugin { nullptr };

    /** Deinitialize function for 3D plugin.
     * Called when plugin is about to be unloaded by context.
     * Called when detaching from graphics context.
     */
    const DestroyPluginFn destroyPlugin { nullptr };
};
/** @} */
CORE3D_END_NAMESPACE()

#endif // API_3D_IPLUGIN_H
