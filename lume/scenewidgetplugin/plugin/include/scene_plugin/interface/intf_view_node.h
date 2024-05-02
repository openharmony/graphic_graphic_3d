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
#ifndef SCENEPLUGIN_INTF_VIEWNODE_H
#define SCENEPLUGIN_INTF_VIEWNODE_H

#include <scene_plugin/interface/intf_nodes.h>

#include <ui/base/intf_view.h>

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::ViewNode
REGISTER_INTERFACE(IViewNode, "ad95f37b-1e41-4017-b9e5-9af1faca84e0")
class IViewNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IViewNode, InterfaceId::IViewNode)
public:
    /**
     * @Brief Controls underlaying canvas view. Scaling applied to the whole design.
     */
    META_PROPERTY(BASE_NS::Math::Vec2, DesignScale)

    /**
     * @Brief Controls underlaying canvas view. Scale factor for QuadScale (how to map pixels to 3d units).
     */
    META_PROPERTY(float, PixelToUnitScale)

    /**
     * @Brief Controls underlaying canvas view. Scale the quad to x:size.x y:size.y (ie. make the pixel size match 3d
     * unit sizes).
     */
    META_PROPERTY(bool, QuadScale)

    /**
     * @Brief Controls underlaying canvas view. if true then canvas size is set to match the designSize.
     *        If false the canvas size can be freely specified.
     */
    META_PROPERTY(bool, SizeFromDesignSize)

    /**
     * @Brief Pixel size of the color buffer.
     */
    META_PROPERTY(BASE_NS::Math::UVec2, Size)

    /**
     * @Brief Clear color.
     */
    META_PROPERTY(BASE_NS::Color, ClearColor)

    /**
     * @Brief Uri of the ui file, e.g."project://assets/ui/test.ui"
     */
    META_PROPERTY(BASE_NS::string, Filename)

    /**
     * @Brief Returns root view for this canvas
     */
};

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IViewNode::WeakPtr);
META_TYPE(SCENE_NS::IViewNode::Ptr);

#endif // SCENEPLUGIN_INTF_VIEWNODE_H
