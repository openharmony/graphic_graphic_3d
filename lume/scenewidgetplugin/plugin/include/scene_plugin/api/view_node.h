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

#ifndef SCENEPLUGINAPI_VIEWNODE_H
#define SCENEPLUGINAPI_VIEWNODE_H

#include <scene_plugin/api/view_node_uid.h>
#include <scene_plugin/interface/intf_view_node.h>

#include <meta/api/internal/object_api.h>
SCENE_BEGIN_NAMESPACE()

/**
 * @brief ViewNode class wraps IViewNode interface. It keeps the referenced object alive using strong ref.
 *        The construction of the object is asynchronous, the properties of the engine may not be available
 *        right after the object instantiation, but OnLoaded() event can be used to observe the state changes.
 *        ViewNode object may not implement Node interfaces on Engine side, so container operations and
 *        transform operations are not applicable as such.
 **/
class ViewNode final : public META_NS::Internal::ObjectInterfaceAPI<ViewNode, ClassId::ViewNode> {
    META_API(ViewNode)
    META_API_OBJECT_CONVERTIBLE(IViewNode)
    META_API_CACHE_INTERFACE(IViewNode, ViewNode)
    META_API_OBJECT_CONVERTIBLE(INode)
    META_API_CACHE_INTERFACE(INode, Node)
public:
    // From Node
    META_API_INTERFACE_PROPERTY_CACHED(Node, Name, BASE_NS::string)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Position, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Scale, BASE_NS::Math::Vec3)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Rotation, BASE_NS::Math::Quat)
    META_API_INTERFACE_PROPERTY_CACHED(Node, Visible, bool)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, DesignScale, BASE_NS::Math::Vec2)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, PixelToUnitScale, float)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, QuadScale, bool)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, SizeFromDesignSize, bool)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, Size, BASE_NS::Math::UVec2)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, ClearColor, BASE_NS::Color)
    META_API_INTERFACE_PROPERTY_CACHED(ViewNode, Filename, BASE_NS::string)

    /**
     * @Brief Returns root view for this canvas
     */
    /*UI_NS::IView::Ptr GetView()
    {
        return META_API_CACHED_INTERFACE(ViewNode)->GetView();
    }*/

public:
    /**
     * @brief Construct ViewNode instance from INode strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    ViewNode(const INode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Construct ViewNode instance from IViewNode strong pointer.
     * @param node the object pointed by interface is kept alive
     */
    ViewNode(const IViewNode::Ptr& node)
    {
        Initialize(interface_pointer_cast<META_NS::IObject>(node));
    }

    /**
     * @brief Gets OnLoaded event reference from INode-interface
     * @return INode::OnLoaded
     */
    auto OnLoaded()
    {
        return META_API_CACHED_INTERFACE(Node)->OnLoaded();
    }

    /**
     * @brief Runs a callback once the material is loaded. If mesh is already initialized, callback will not run.
     * @param callback Code to run, if strong reference is passed, it will keep the instance alive
     *                 causing engine to report memory leak on application exit.
     * @return reference to this instance of ViewNode.
     */
    template<class Callback>
    auto OnLoaded(Callback&& callback)
    {
        OnLoaded()->AddHandler(META_NS::MakeCallback<META_NS::IOnChanged>(callback));
        return *this;
    }
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGINAPI_VIEWNODE_H
