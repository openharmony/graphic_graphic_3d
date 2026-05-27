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

#ifndef SCENE_INTERFACE_INODE_IMPORT_H
#define SCENE_INTERFACE_INODE_IMPORT_H

#include <scene/base/types.h>
#include <scene/interface/intf_scene.h>

#include <meta/interface/resource/intf_object_template.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Interface to support import another scene or nodes from another scene under a node
 */
class INodeImport : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, INodeImport, "7fb14660-382e-4d84-a55c-b040c89a04f4")
public:
    /**
     * @brief Adds child from another scene as child of this node.
     *        The scenes must share the same context.
     *        This will make a deep copy of the node and all children/resources it references.
     * @param node Node in another scene
     * @return The child node added or null if failed
     */
    virtual Future<INode::Ptr> ImportChild(const INode::ConstPtr& node) = 0;
    /**
     * @brief Options for ImportChildScene.
     */
    struct ImportOptions {
        /// Name for the scene's root that is copied.
        BASE_NS::string_view nodeName;
        /// Associated resource group for the resources imported from the scene (empty will generate one if needed)
        BASE_NS::string_view resourceGroup;
        enum class NamingBehavior : uint8_t {
            /// Make sure nodeName is unique. Generate a new unique name if the a child with nodeName already exists.
            ENFORCE_UNIQUE_NAME = 0,
            /// remove existing child node with the same name (if any)
            REMOVE_EXISTING,
            /// Do nothing for nodeName, may lead to duplicate child names.
            NAME_AS_IS
        };
        NamingBehavior namingBehavior{NamingBehavior::REMOVE_EXISTING};
        /// Share resources if same scene is imported multiple times.
        bool shareResources{true};
    };
    /**
     * @brief Adds whole scene node hierarchy as child of this node.
     *        The scenes must share the same context.
     *        This will make a deep copy of the node and all children/resources it references.
     *        This will also copy animations
     * @param scene Scene to import
     * @param options Import options.
     * @return The child node added (which was the root in given scene) or null if failed
     */
    virtual Future<INode::Ptr> ImportChildScene(const IScene::ConstPtr& scene, const ImportOptions& options) = 0;
    /// Helper for ImportChildScene.
    Future<INode::Ptr> ImportChildScene(
        const IScene::ConstPtr& scene, BASE_NS::string_view nodeName, BASE_NS::string_view resourceGroup)
    {
        return ImportChildScene(scene, {nodeName, resourceGroup});
    }
    /// Helper for ImportChildScene.
    Future<INode::Ptr> ImportChildScene(const IScene::ConstPtr& scene, BASE_NS::string_view nodeName)
    {
        return ImportChildScene(scene, {nodeName, ""});
    }
    /**
     * @brief Adds whole scene node hierarchy as child of this node.
     * @param uri Uri of the scene resource to load.
     * @param nodeName Name for the scene's root that is copied (the name is usually empty so you can give better name
     * here)
     * @return The child node added (which was the root in given scene) or null if failed
     */
    virtual Future<INode::Ptr> ImportChildScene(BASE_NS::string_view uri, BASE_NS::string_view nodeName) = 0;

    /**
     * @brief Instantiate object template as part of this scene using this node as parent
     * @param templ Object template containing node hierarchy
     */
    virtual Future<INode::Ptr> ImportTemplate(
        const META_NS::IObjectTemplate::ConstPtr& templ, BASE_NS::string_view resourceGroup) = 0;
    Future<INode::Ptr> ImportTemplate(const META_NS::IObjectTemplate::ConstPtr& templ)
    {
        return ImportTemplate(templ, "");
    }
};

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::INodeImport)

#endif
