/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

// clang-format off
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
// clang-format on

#include <scene/api/node.h>
#include <scene/api/scene.h>
#include <scene/interface/intf_node_import.h>
#include <scene_importer/interface/intf_importer.h>

#include <meta/api/metadata_util.h>
#include <meta/api/object.h>
#include <meta/interface/intf_attach.h>

#include "import_test_helpers.h"
#include "scene/scene_test.h"
#include "test_attachment.h"

SCENE_BEGIN_NAMESPACE()
namespace UTest {

class API_ObjectRefImportTest : public ScenePluginTest {
public:
    void SetUp() override
    {
        META_NS::RegisterObjectType<TestAttachment>();
        ScenePluginTest::SetUp();
        imp = META_NS::GetObjectRegistry().Create<SCENE_IMP_NS::IImporter>(SCENE_IMP_NS::ClassId::Importer);
        ASSERT_TRUE(imp);
        ASSERT_TRUE(imp->Initialize(context, {}));
    }
    void TearDown() override
    {
        META_NS::UnregisterObjectType<TestAttachment>();
        ScenePluginTest::TearDown();
    }

    IScene::Ptr LoadTestScene(BASE_NS::string_view path)
    {
        auto res = imp->Import(path, {});
        EXPECT_TRUE(res);
        if (!res) {
            return nullptr;
        }
        return interface_pointer_cast<IScene>(res.object);
    }

public:
    SCENE_IMP_NS::IImporter::Ptr imp;
};

/**
 * @tc.name: AttachmentObjectRefViaRootPath
 * @tc.desc: objectRef with a root-relative path resolves to a sibling node in the scene.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefViaRootPath, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_att_objref_root_path.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    auto target = scene->FindNode("//target").GetResult();
    ASSERT_TRUE(target);

    // Find the TestAttachment on the ext node
    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            attObj = a;
        }
    }
    ASSERT_TRUE(attObj);

    auto nodeRefProp = META_NS::Metadata(attObj).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);

    auto locked = nodeRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked, target);
}

/**
 * @tc.name: AttachmentObjectRefViaResourceId
 * @tc.desc: objectRef with a resource-id path resolves to an object from the resource index.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefViaResourceId, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_att_objref_resource.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    // Find the TestAttachment on the ext node
    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            attObj = a;
        }
    }
    ASSERT_TRUE(attObj);

    auto objRefProp = META_NS::Metadata(attObj).GetProperty<META_NS::IObject::WeakPtr>("ObjectRef");
    ASSERT_TRUE(objRefProp);

    auto locked = objRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
}

/**
 * @tc.name: AttachmentObjectRefForwardReference
 * @tc.desc: objectRef referencing a node that appears later in the JSON (forward reference).
 *           This tests that forward references are resolved correctly.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefForwardReference, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_att_objref_forward_ref.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    auto target = scene->FindNode("//target").GetResult();
    ASSERT_TRUE(target);

    // Find the TestAttachment on the ext node
    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            attObj = a;
        }
    }
    ASSERT_TRUE(attObj);

    auto nodeRefProp = META_NS::Metadata(attObj).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);

    // Forward reference: target appears after ext in JSON, so this should resolve
    // once deferred resolution is implemented
    auto locked = nodeRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked, target);
}

/**
 * @tc.name: AttachmentObjectRefViaResourceIdWithColons
 * @tc.desc: objectRef with a resource-id path where the resource name contains :: (escaped as \:\:).
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefViaResourceIdWithColons, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_att_objref_resource_colon.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    // Find the TestAttachment on the ext node
    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            attObj = a;
        }
    }
    ASSERT_TRUE(attObj);

    auto objRefProp = META_NS::Metadata(attObj).GetProperty<META_NS::IObject::WeakPtr>("ObjectRef");
    ASSERT_TRUE(objRefProp);

    auto locked = objRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
}

/**
 * @tc.name: AttachmentObjectRefViaResourceIdWithEscapedGroupColons
 * @tc.desc: objectRef with a resource-id path where the group name contains :: (escaped as \::).
 * @tc.type: FUNC
 */
UNIT_TEST_F(
    API_ObjectRefImportTest, AttachmentObjectRefViaResourceIdWithEscapedGroupColons, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_att_objref_resource_esc_group.json");
    ASSERT_TRUE(scene);

    auto ext = scene->FindNode("//ext").GetResult();
    ASSERT_TRUE(ext);

    // Find the TestAttachment on the ext node
    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::Metadata(a).GetProperty<float>("Float")) {
            attObj = a;
        }
    }
    ASSERT_TRUE(attObj);

    auto objRefProp = META_NS::Metadata(attObj).GetProperty<META_NS::IObject::WeakPtr>("ObjectRef");
    ASSERT_TRUE(objRefProp);

    auto locked = objRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
}

/**
 * @tc.name: NodeTemplateLocalRootAnchor
 * @tc.desc: An objectRef path starting with `/` inside a node template anchors at
 *           the instantiated subtree's local root (the host node), not at the global
 *           scene root. The template defines a sibling `inner` node and an
 *           `ext_with_ref` node whose attachment references `/inner`. After
 *           instantiation under `hostA`, the ref must resolve to the host's own
 *           inner node.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, NodeTemplateLocalRootAnchor, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_template_local_root.json");
    ASSERT_TRUE(scene);

    auto inner = scene->FindNode("//tmplRoot/inner").GetResult();
    ASSERT_TRUE(inner);

    auto ext = scene->FindNode("//tmplRoot/ext_with_ref").GetResult();
    ASSERT_TRUE(ext);

    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::GetValue(META_NS::Metadata(a).GetProperty<BASE_NS::string>("Name")) == "RefAtt") {
            attObj = a;
            break;
        }
    }
    ASSERT_TRUE(attObj);

    auto nodeRefProp = META_NS::Metadata(attObj).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);
    auto locked = nodeRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked, inner);
}

/**
 * @tc.name: NodeTemplateLocalRootAnchorBackwardRef
 * @tc.desc: Inside a node template, an objectRef path `/target` must resolve even
 *           when the target node is declared AFTER the referencing node in the
 *           template's children array. The template instantiation provides its
 *           own deferred-resolution scope that flushes once all template content
 *           is imported.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, NodeTemplateLocalRootAnchorBackwardRef, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_template_local_root_backward.json");
    ASSERT_TRUE(scene);

    auto inner = scene->FindNode("//tmplRoot/inner").GetResult();
    ASSERT_TRUE(inner);

    auto ext = scene->FindNode("//tmplRoot/ext_with_ref").GetResult();
    ASSERT_TRUE(ext);

    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(ext)->GetAttachments()) {
        if (META_NS::GetValue(META_NS::Metadata(a).GetProperty<BASE_NS::string>("Name")) == "RefAtt") {
            attObj = a;
            break;
        }
    }
    ASSERT_TRUE(attObj);

    auto nodeRefProp = META_NS::Metadata(attObj).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);
    auto locked = nodeRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked, inner);
}

/**
 * @tc.name: NodeTemplateNestedLocalRootAnchor
 * @tc.desc: Outer node template instantiates an inner node template. Each template
 *           uses a /-anchored objectRef to reference one of its own siblings.
 *           Each must resolve to its own local root, not the outer template's
 *           root and not the scene root.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, NodeTemplateNestedLocalRootAnchor, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_template_nested_local_root.json");
    ASSERT_TRUE(scene);

    auto outerInner = scene->FindNode("//outerRoot/outerInner").GetResult();
    ASSERT_TRUE(outerInner);
    auto outerExt = scene->FindNode("//outerRoot/outerExt").GetResult();
    ASSERT_TRUE(outerExt);

    auto innerInner = scene->FindNode("//outerRoot/innerRoot/innerInner").GetResult();
    ASSERT_TRUE(innerInner);
    auto innerExt = scene->FindNode("//outerRoot/innerRoot/innerExt").GetResult();
    ASSERT_TRUE(innerExt);

    auto findRef = [](const INode::Ptr& node, BASE_NS::string_view name) {
        META_NS::IObject::Ptr attObj;
        for (auto&& a : interface_cast<META_NS::IAttach>(node)->GetAttachments()) {
            if (META_NS::GetValue(META_NS::Metadata(a).GetProperty<BASE_NS::string>("Name")) == name) {
                attObj = a;
                break;
            }
        }
        return attObj;
    };

    auto outerAtt = findRef(outerExt, "OuterRefAtt");
    ASSERT_TRUE(outerAtt);
    auto outerProp = META_NS::Metadata(outerAtt).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(outerProp);
    auto outerLocked = outerProp->GetValue().lock();
    ASSERT_TRUE(outerLocked);
    EXPECT_EQ(outerLocked, outerInner);

    auto innerAtt = findRef(innerExt, "InnerRefAtt");
    ASSERT_TRUE(innerAtt);
    auto innerProp = META_NS::Metadata(innerAtt).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(innerProp);
    auto innerLocked = innerProp->GetValue().lock();
    ASSERT_TRUE(innerLocked);
    EXPECT_EQ(innerLocked, innerInner);
}

/**
 * @tc.name: AttachmentObjectRefDeepRoot
 * @tc.desc: A `/target` objectRef in a deeply-nested node's attachment must anchor
 *           on the scene root, not on the immediate parent. Guards against
 *           regression where `/` was anchored on `params.object` (which is the
 *           current parent during normal scene-tree iteration). Outside of node
 *           templates there is no instantiation root, so `/` falls back to the
 *           scene root.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefDeepRoot, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_deep_root_ref.json");
    ASSERT_TRUE(scene);

    auto target = scene->FindNode("//target").GetResult();
    ASSERT_TRUE(target);

    auto deep = scene->FindNode("//A/B/C").GetResult();
    ASSERT_TRUE(deep);

    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(deep)->GetAttachments()) {
        if (META_NS::GetValue(META_NS::Metadata(a).GetProperty<BASE_NS::string>("Name")) == "RefAtt") {
            attObj = a;
            break;
        }
    }
    ASSERT_TRUE(attObj);

    auto nodeRefProp = META_NS::Metadata(attObj).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);
    auto locked = nodeRefProp->GetValue().lock();
    ASSERT_TRUE(locked);
    EXPECT_EQ(locked, target);
}

/**
 * @tc.name: AttachmentObjectRefInvalidPath
 * @tc.desc: An objectRef whose path does not resolve to any node fails the import with a
 *           property-import error. Verifies the importer rejects unresolved objectRefs
 *           rather than silently leaving the target property null.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefInvalidPath, testing::ext::TestSize.Level1)
{
    auto res = imp->Import("test://import/node_template/scene_att_objref_invalid_path.json", {});
    EXPECT_FALSE(res);
    // The path "/doesNotExist" cannot be resolved against the scene root.
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "No such child for object");
    EXPECT_DIAGNOSTIC_CONTAINS(res.error, "doesNotExist");
}

/**
 * @tc.name: AttachmentObjectRefWrongTypeTarget
 * @tc.desc: An objectRef pointing at a resource of the wrong type (here: a material when the
 *           target property is INode::WeakPtr) imports — the resource id resolves successfully
 *           but the value cannot be cast to INode, so the property remains null. This documents
 *           current best-effort behavior; a future stricter check would surface an error here.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectRefImportTest, AttachmentObjectRefWrongTypeTarget, testing::ext::TestSize.Level1)
{
    auto scene = LoadTestScene("test://import/node_template/scene_att_objref_wrong_type.json");
    ASSERT_TRUE(scene);
    auto host = scene->FindNode("//host").GetResult();
    ASSERT_TRUE(host);

    META_NS::IObject::Ptr attObj;
    for (auto&& a : interface_cast<META_NS::IAttach>(host)->GetAttachments()) {
        if (META_NS::GetValue(META_NS::Metadata(a).GetProperty<BASE_NS::string>("Name")) == "RefAtt") {
            attObj = a;
            break;
        }
    }
    ASSERT_TRUE(attObj);

    // NodeRef expected an INode but resolved to a material; weak-cast yields null.
    auto nodeRefProp = META_NS::Metadata(attObj).GetProperty<INode::WeakPtr>("NodeRef");
    ASSERT_TRUE(nodeRefProp);
    EXPECT_FALSE(nodeRefProp->GetValue().lock());
}

}  // namespace UTest
SCENE_END_NAMESPACE()
