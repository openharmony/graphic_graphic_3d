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

#ifndef SCENEPLUGINAPI_SCENE_H
#define SCENEPLUGINAPI_SCENE_H

#include <scene_plugin/api/material.h>
#include <scene_plugin/api/mesh.h>
#include <scene_plugin/api/node.h>
#include <scene_plugin/interface/intf_scene.h>

#include <meta/api/internal/object_api.h>

#include "scene_uid.h"
SCENE_BEGIN_NAMESPACE()
class Animation final : public META_NS::Internal::ObjectInterfaceAPI<Animation, ClassId::Animation> {
    META_API(Animation)
    META_API_OBJECT_CONVERTIBLE(META_NS::IAnimation)
    META_API_OBJECT_CONVERTIBLE(META_NS::IStartableAnimation)
    META_API_CACHE_INTERFACE(META_NS::IAnimation, Animation)
    META_API_CACHE_INTERFACE(META_NS::IStartableAnimation, StartableAnimation)
    META_API_OBJECT_CONVERTIBLE(INode)
    META_API_CACHE_INTERFACE(INode, Node)

public:
    META_API_INTERFACE_PROPERTY_CACHED(Animation, Enabled, bool)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, Valid, bool)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, TotalDuration, META_NS::TimeSpan)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, Running, bool)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Animation, Progress, float)

    void Start()
    {
        if (auto api = META_API_CACHED_INTERFACE(StartableAnimation)) {
            api->Start();
        }
    }

    void Stop()
    {
        if (auto api = META_API_CACHED_INTERFACE(StartableAnimation)) {
            api->Stop();
        }
    }

    void Pause()
    {
        if (auto api = META_API_CACHED_INTERFACE(StartableAnimation)) {
            api->Pause();
        }
    }

    void Restart()
    {
        if (auto api = META_API_CACHED_INTERFACE(StartableAnimation)) {
            api->Restart();
        }
    }

    void Reset()
    {
        if (auto api = META_API_CACHED_INTERFACE(StartableAnimation)) {
            api->Stop();
        }
    }
};

/**
 * @brief The Scene class is implementing a proxy to access engine elements from META::Object framework.
 */
class Scene final : public META_NS::Internal::ObjectInterfaceAPI<Scene, ClassId::Scene> {
    META_API(Scene)
    META_API_OBJECT_CONVERTIBLE(IScene)
    META_API_CACHE_INTERFACE(IScene, Scene)
public:
    META_API_INTERFACE_PROPERTY_CACHED(Scene, Name, BASE_NS::string)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Scene, Status, uint32_t)
    META_API_INTERFACE_READONLY_PROPERTY_CACHED(Scene, RootNode, INode::Ptr)
    META_API_INTERFACE_PROPERTY_CACHED(Scene, Uri, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(Scene, Asynchronous, bool)
    META_API_INTERFACE_PROPERTY_CACHED(Scene, DefaultCamera, ICamera::Ptr)

public:
    /**
     * @brief Instantiates new Scene instance using a strong ref to IScene::Ptr to object implementing the scene.
     * @param scene strong ref to scene. This may keep the instance alive even the engine has been already purged.
     */
    explicit Scene(const IScene::Ptr& scene)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(scene));
    }

    /**
     * @brief Creates and initializes an empty scene.
     * @return reference to this instance.
     */
    Scene& CreateEmpty()
    {
        if (auto scene = GetSceneInterface()) {
            scene->CreateEmpty();
        }
        return *this;
    }

    /**
     * @brief Loads a scene from file
     * @param uri Defines the scene file to be used.
     * @return reference to this instance.
     */
    Scene& Load(const BASE_NS::string_view& uri)
    {
        if (auto scene = GetSceneInterface()) {
            scene->Load(uri);
        }
        return *this;
    }

    /**
     * @brief Get new Node instance. Returns always an object. Uses flat cache to existing nodes.
     * @param path The full path including the node name on engine scene
     * @return Node instance
     */
    Node GetNode(const BASE_NS::string_view path)
    {
        return Node(GetSceneInterface()->GetNode<INode>(path));
    }

    /**
     * @brief Get existing material proxy. Returns always an object. Uses flat cache to existing materials.
     * @param name The material name on engine scene
     * @return Material instance
     */
    Material GetMaterial(const BASE_NS::string_view name)
    {
        return Material(GetSceneInterface()->GetMaterial(name));
    }

    /**
     * @brief Get existing mesh proxy. Returns always an object. Uses flat cache to existing meshes.
     * @param name The mesh name on engine scene
     * @return Material instance
     */
    Mesh GetMesh(const BASE_NS::string_view path)
    {
        return Mesh(GetSceneInterface()->GetMesh(path));
    }

    /**
     * @brief Get pointer to node interface. Returns always pointer to an object. Uses flat cache to existing nodes.
     * @param path The full path including the node name on engine scene
     * @return pointer to node interface
     */
    template<class T>
    typename T::Ptr GetNode(const BASE_NS::string_view path)
    {
        return GetSceneInterface()->GetNode<T>(path);
    }

    /**
     * @brief Create new node to engine scene
     * @param path The full path including the node name on engine scene
     * @return pointer to new node interface.
     */
    template<class T>
    typename T::Ptr CreateNode(const BASE_NS::string_view path)
    {
        return GetSceneInterface()->CreateNode<T>(path);
    }

    /**
     * @brief Create new material to engine scene.
     * @param name The material name on engine scene
     * @return Material instance
     */
    Material CreateMaterial(const BASE_NS::string_view name)
    {
        return Material(GetSceneInterface()->CreateMaterial(name));
    }

    /**
     * @brief Gets OnLoaded event reference from IScene-interface
     * @return IScene::OnLoaded
     */
    auto OnLoaded()
    {
        return META_API_CACHED_INTERFACE(Scene)->OnLoaded();
    }

    /**
     * @brief Runs a callback once the scene is loaded on engine. If the scene is already initialized,
     * callback will not run, unless the scene is changed and reloaded.
     * @param callback Code to run, if strong reference is passed, it will keep the instance alive
     *                 causing engine to report memory leak on application exit.
     * @return reference to this instance of Scene.
     */
    template<class Callback>
    auto OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }

    /**
     * @brief Get attached scene for the node.
     * @return New Scene instance. Can be uninitialized.
     */
    template<class nodeClass>
    static Scene GetScene(nodeClass& node)
    {
        if (auto nodeInterface = interface_pointer_cast<INode>(node.GetIObject())) {
            return Scene(nodeInterface->GetScene());
        }

        return Scene();
    }

    /**
     *  Load all material proxies on the current scene.
     */
    void InstantiateMaterialProxies()
    {
        META_API_CACHED_INTERFACE(Scene)->InstantiateMaterialProxies();
    }

    /**
     *  Load all mesh proxies on the current scene.
     */
    void InstantiateMeshProxies()
    {
        META_API_CACHED_INTERFACE(Scene)->InstantiateMeshProxies();
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_SCENE_H
