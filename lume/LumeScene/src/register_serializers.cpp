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

#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/resource/image_info.h>

#include <base/containers/unordered_map.h>

#include <meta/base/meta_types.h>
#include <meta/ext/any_builder.h>
#include <meta/ext/serialization/common_value_serializers.h>
#include <meta/interface/detail/any.h>

SCENE_BEGIN_NAMESPACE()

namespace Internal {

using IntPair = BASE_NS::pair<BASE_NS::string_view, int64_t>;

template<typename Type>
static auto ExportIntPair(const BASE_NS::string& name, const IntPair& i1, const IntPair& i2)
{
    auto obj = META_NS::CreateObjectNode(META_NS::UidFromType<Type>(), name);
    if (obj) {
        auto& r = META_NS::GetObjectRegistry();
        if (auto n = r.Create<META_NS::IMapNode>(META_NS::ClassId::MapNode)) {
            if (auto in = r.Create<META_NS::IIntNode>(META_NS::ClassId::IntNode)) {
                in->SetValue(i1.second);
                n->AddNode(i1.first, in);
            }
            if (auto in = r.Create<META_NS::IIntNode>(META_NS::ClassId::IntNode)) {
                in->SetValue(i2.second);
                n->AddNode(i2.first, in);
            }
            obj->SetMembers(n);
        }
    }
    return obj;
}

static bool ImportIntPair(const META_NS::ISerNode::ConstPtr& node, IntPair& i1, IntPair& i2)
{
    if (auto obj = interface_cast<META_NS::IObjectNode>(node)) {
        if (auto m = interface_cast<META_NS::IMapNode>(obj->GetMembers())) {
            auto rslNode = m->FindNode(i1.first);
            auto rsloNode = m->FindNode(i2.first);
            return META_NS::ExtractInteger(rslNode, i1.second) && META_NS::ExtractInteger(rsloNode, i2.second);
        }
    }
    return false;
}

void RegisterSerializers()
{
    auto& data = META_NS::GetObjectRegistry().GetGlobalSerializationData();

    META_NS::RegisterSerializer<RenderSort>(
        [](auto&, const RenderSort& v) {
            return ExportIntPair<RenderSort>("RenderSort", IntPair { "renderSortLayer", v.renderSortLayer },
                IntPair { "renderSortLayerOrder", v.renderSortLayerOrder });
        },
        [](auto&, const META_NS::ISerNode::ConstPtr& node, RenderSort& out) {
            IntPair i1 { "renderSortLayer", {} };
            IntPair i2 { "renderSortLayerOrder", {} };
            auto res = ImportIntPair(node, i1, i2);
            if (res) {
                out.renderSortLayer = static_cast<uint8_t>(i1.second);
                out.renderSortLayerOrder = static_cast<uint8_t>(i2.second);
            }
            return res;
        });

    META_NS::RegisterSerializer<ColorFormat>(
        [](auto&, const ColorFormat& v) {
            return ExportIntPair<ColorFormat>(
                "ColorFormat", IntPair { "format", v.format }, IntPair { "usageFlags", v.usageFlags });
        },
        [](auto&, const META_NS::ISerNode::ConstPtr& node, ColorFormat& out) {
            IntPair i1 { "format", {} };
            IntPair i2 { "usageFlags", {} };
            auto res = ImportIntPair(node, i1, i2);
            if (res) {
                out.format = static_cast<BASE_NS::Format>(i1.second);
                out.usageFlags = static_cast<uint32_t>(i2.second);
            }
            return res;
        });

    META_NS::RegisterSerializer<ImageLoadInfo>(
        data,
        [](auto&, const ImageLoadInfo& v) {
            auto obj = META_NS::CreateObjectNode(META_NS::UidFromType<ImageLoadInfo>(), "ImageLoadInfo");
            if (obj) {
                auto& r = META_NS::GetObjectRegistry();
                if (auto n = r.Create<META_NS::IMapNode>(META_NS::ClassId::MapNode)) {
                    if (auto in = r.Create<META_NS::IUIntNode>(META_NS::ClassId::UIntNode)) {
                        in->SetValue(static_cast<uint64_t>(v.loadFlags));
                        n->AddNode("LoadFlags", in);
                    }
                    if (auto in = r.Create<META_NS::IUIntNode>(META_NS::ClassId::UIntNode)) {
                        in->SetValue(static_cast<uint64_t>(v.info.usageFlags));
                        n->AddNode("UsageFlags", in);
                    }
                    if (auto in = r.Create<META_NS::IUIntNode>(META_NS::ClassId::UIntNode)) {
                        in->SetValue(static_cast<uint64_t>(v.info.memoryFlags));
                        n->AddNode("MemoryFlags", in);
                    }
                    if (auto in = r.Create<META_NS::IUIntNode>(META_NS::ClassId::UIntNode)) {
                        in->SetValue(static_cast<uint64_t>(v.info.creationFlags));
                        n->AddNode("CreationFlags", in);
                    }
                    obj->SetMembers(n);
                }
            }
            return obj;
        },
        [](auto&, const META_NS::ISerNode::ConstPtr& node, ImageLoadInfo& out) {
            if (auto obj = interface_cast<META_NS::IObjectNode>(node)) {
                if (auto m = interface_cast<META_NS::IMapNode>(obj->GetMembers())) {
                    auto lf = m->FindNode("LoadFlags");
                    auto uf = m->FindNode("UsageFlags");
                    auto mf = m->FindNode("MemoryFlags");
                    auto cf = m->FindNode("CreationFlags");
                    return META_NS::ExtractInteger(lf, out.loadFlags) &&
                           META_NS::ExtractInteger(uf, out.info.usageFlags) &&
                           META_NS::ExtractInteger(mf, out.info.memoryFlags) &&
                           META_NS::ExtractInteger(cf, out.info.creationFlags);
                }
            }
            return false;
        });
}

void UnRegisterSerializers()
{
    auto& data = META_NS::GetObjectRegistry().GetGlobalSerializationData();
    META_NS::UnregisterSerializer<RenderSort>(data);
    META_NS::UnregisterSerializer<ColorFormat>(data);
}

} // namespace Internal
SCENE_END_NAMESPACE()
