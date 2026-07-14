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

#include "node_template_internal.h"

#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/ext/minimal_object.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "../import_helpers.h"
#include "../resolve_object.h"
#include "index.h"
#include "node_template_updater.h"

SCENE_IMP_BEGIN_NAMESPACE()

META_REGISTER_CLASS(NodeImportResult, "66a48497-cddc-4d00-9281-d8374ac02eeb", META_NS::ObjectCategoryBits::NO_CATEGORY)

class NodeImportResult : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, SCENE_NS::INodeImportResult> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::NodeImportResult)
public:
    explicit NodeImportResult(SCENE_NS::INode::Ptr n, BASE_NS::vector<CORE_NS::ResourceIdContext> res = {},
        BASE_NS::vector<CORE_NS::ResourceIdContext> ass = {})
        : node_(n), res_(BASE_NS::move(res)), ass_(BASE_NS::move(ass))
    {}

    SCENE_NS::INode::Ptr GetNode() const override
    {
        return node_;
    }
    BASE_NS::vector<CORE_NS::ResourceIdContext> GetNewResourcesImported() const override
    {
        return res_;
    }
    BASE_NS::vector<CORE_NS::ResourceIdContext> GetAssociatedResources() const override
    {
        return ass_;
    }

private:
    SCENE_NS::INode::Ptr node_;
    BASE_NS::vector<CORE_NS::ResourceIdContext> res_;
    BASE_NS::vector<CORE_NS::ResourceIdContext> ass_;
};

class TInstantiator {
public:
    TInstantiator(
        IImporterInternal& importer, INodeTemplatePayloadInternal& pint, SCENE_NS::INodeTemplateContext& tcontext)
        : importer_(importer), pint_(pint), tcontext_(tcontext)
    {}

    void CopyResourceInfos(const BASE_NS::array_view<const CORE_NS::ResourceInfo>& infos,
        META_NS::IResourceManagerExtension& destExt, const CORE_NS::ResourceContextPtr& context,
        BASE_NS::vector<CORE_NS::ResourceId>& out)
    {
        for (auto&& i : infos) {
            META_NS::ResourceData d{i};
            if (i.options) {
                d.options = i.options->Clone();
            }
            auto destI = destExt.GetResources({i.id}, context);
            if (!destI.empty()) {
                out.push_back(destI.front().id);
                d.object = destI.front().object;
            }
            destExt.AddResources({d}, context);
        }
    }

    bool MergeOption(CORE_NS::ResourceInfo& r, const META_NS::ResourceData& destI,
        BASE_NS::vector<CORE_NS::IResource::Ptr>& merged, bool destIsTemplateReimport) const
    {
        bool res = destI.id == r.id && destI.path == r.path && destI.type == r.type;
        if (res) {
            if (r.options && destI.options) {
                r.options->Merge(*destI.options);
                // On a template→template re-import we will discard the existing live resource
                // (so v2 defaults land on a fresh instance with pristine type defaults), so
                // there is nothing useful to ReapplyOptions against.
                if (destI.object && !destIsTemplateReimport) {
                    merged.push_back(destI.object);
                }
            }
        }
        return res;
    }

    void RemapGroup(CORE_NS::ResourceId& id, BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup)
    {
        if (!newGroup.empty() && !oldGroup.empty() && id.group == oldGroup) {
            id.group = newGroup;
        }
    }

    void RemapDerivedFromGroup(
        CORE_NS::IResourceOptions::Ptr& options, BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup)
    {
        if (newGroup.empty() || oldGroup.empty()) {
            return;
        }
        auto dr = interface_cast<META_NS::IDerivedResourceOptions>(options);
        if (!dr) {
            return;
        }
        auto base = dr->GetBaseResource();
        if (base.IsValid() && base.group == oldGroup) {
            base.group = newGroup;
            dr->SetBaseResource(BASE_NS::move(base));
        }
    }

    META_NS::ResourceData PrepareResourceData(
        CORE_NS::ResourceInfo& r, BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup)
    {
        META_NS::ResourceData d{r};
        d.version = "JsonSchemaImport";
        if (r.options) {
            d.options = r.options->Clone();
            RemapDerivedFromGroup(d.options, oldGroup, newGroup);
        }
        if (auto tc = interface_cast<SCENE_NS::ITemplateOptions>(d.options.get())) {
            tc->SetTemplateContext(tcontext_.ApplyAsTemplate());
        }
        return d;
    }

    bool ImportResources(BASE_NS::vector<CORE_NS::ResourceInfo> res, BASE_NS::string_view oldGroup,
        BASE_NS::string_view newGroup, const SCENE_NS::IScene::Ptr& scene)
    {
        auto resManager = interface_cast<META_NS::IResourceManagerExtension>(SCENE_NS::GetResourceManager(scene));
        if (!resManager) {
            CORE_LOG_E("Failed to import node template, invalid target scene");
            return false;
        }
        BASE_NS::vector<CORE_NS::IResource::Ptr> merged;
        for (auto&& r : res) {
            RemapGroup(r.id, oldGroup, newGroup);
            auto destR = resManager->GetResources({r.id}, scene);
            // A previously stored options object that was added by a prior node-template
            // instantiation reliably has its template-context flag set (PrepareResourceData
            // sets it on the clone). When that is the case, we are seeing a v1→v2 re-import
            // and must rebuild the live resource from scratch so its property defaults come
            // from v2's MaterialTemplate, not from leftover defaults v1 wrote.
            bool destIsTemplateReimport = false;
            if (!destR.empty()) {
                if (auto destTc = interface_cast<SCENE_NS::ITemplateOptions>(destR.front().options.get())) {
                    destIsTemplateReimport = destTc->IsTemplateContext();
                }
            }
            if (destR.empty() || !MergeOption(r, destR.front(), merged, destIsTemplateReimport)) {
                importedResources_.push_back({r.id, scene});
            }
            associatedResources_.push_back({r.id, scene});
            if (r.type != SCENE_NS::ClassId::AnimationResource.Id().ToUid()) {
                auto d = PrepareResourceData(r, oldGroup, newGroup);
                if (!destR.empty() && !destIsTemplateReimport) {
                    d.object = destR.front().object;
                }
                resManager->AddResource(BASE_NS::move(d), scene);
            }
        }
        for (auto&& m : merged) {
            resManager->ReapplyOptions(m, scene);
            resManager->UpdateOptionsData({m->GetResourceId()}, scene);
        }
        return true;
    }

    ImportResult ImportNodes(const SCENE_NS::IScene::Ptr& scene, const SCENE_NS::INode::Ptr& node,
        BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup)
    {
        ImporterConfig config = importer_.GetConfig();
        if (!config.context) {
            CORE_LOG_E("Failed to import node template, invalid context");
            return ImportResult{};
        }
        ImportContextParameters params;
        params.scene = scene;
        params.object = interface_pointer_cast<META_NS::IObject>(node);
        // Leave `importRoot` null. ImportNodeCommon auto-pins it to the first
        // node imported in this fresh context, which is the template's content
        // root (e.g. tmplRoot), so `/` paths inside the template anchor there.
        params.filename = pint_.GetSourceFilename();
        params.fileDepth = pint_.GetFileDepth();
        ImportContext context(config, params, pint_.GetJson());
        context.SetTemplateContext(tcontext_.ApplyAsTemplate());
        if (!oldGroup.empty() && !newGroup.empty()) {
            context.AddResourceGroupOverride(BASE_NS::string(oldGroup), BASE_NS::string(newGroup));
        }

        // Self-contained Hierarchy-tier scope: deferred resolutions registered
        // while importing the template (e.g. objectRef paths to siblings within
        // the template) flush before this function returns, so backward refs
        // inside the template work without depending on any outer scope.
        DeferredHierarchyScope deferred(context);
        auto result = context.ImportSubType("abstract-node");
        ErrorHandler h(context);
        if (auto err = deferred.Execute(h)) {
            return ImportResult{err};
        }
        return result;
    }

    // Resolve a single PropertyPath stored in `meta` against `contentRoot`. Used both
    // for top-level animation options and for inline child animation meta walked
    // recursively below. Skips when no PropertyPath, when Property is already set, or
    // when the path can't be resolved.
    void ResolvePropertyPathInMeta(
        META_NS::IMetadata& meta, const SCENE_NS::IScene::Ptr& scene, const META_NS::IObject::Ptr& contentRoot)
    {
        auto path = META_NS::GetValue<BASE_NS::string>(&meta, "PropertyPath");
        if (path.empty()) {
            return;
        }
        if (auto existing = meta.GetProperty<META_NS::IProperty::WeakPtr>("Property", META_NS::MetadataQuery::EXISTING);
            existing && META_NS::GetValue(existing).lock()) {
            return;
        }
        ImportContextParameters params;
        params.scene = scene;
        params.object = contentRoot;
        params.importRoot = contentRoot;
        ImportContext ctx(importer_.GetConfig(), params, CORE_NS::json::value{});
        auto result = ResolveObject(ctx, contentRoot, path);
        if (!result) {
            return;
        }
        if (auto resolved = interface_pointer_cast<META_NS::IProperty>(result.object)) {
            AddSetProperty(meta, "Property", META_NS::IProperty::WeakPtr(resolved));
        }
    }

    // Walk a parent animation's inline children recursively and resolve PropertyPath
    // fields at every level. Handles arbitrary container nesting (e.g. sequential
    // -> parallel -> track). Bounded by MAX_ANIMATION_NESTING_DEPTH so a pathological
    // JSON payload can't blow the stack.
    static constexpr int MAX_ANIMATION_NESTING_DEPTH = 32;
    void ResolveInlineAnimationChildren(META_NS::IMetadata& parentMeta, const SCENE_NS::IScene::Ptr& scene,
        const META_NS::IObject::Ptr& contentRoot, int depth = 0)
    {
        if (depth >= MAX_ANIMATION_NESTING_DEPTH) {
            CORE_LOG_W("ResolveInlineAnimationChildren: max nesting depth (%d) reached, stopping recursion",
                MAX_ANIMATION_NESTING_DEPTH);
            return;
        }
        auto childrenProp =
            parentMeta.GetArrayProperty<META_NS::IObject::Ptr>("Animations", META_NS::MetadataQuery::EXISTING);
        if (!childrenProp) {
            return;
        }
        for (auto&& child : childrenProp->GetValue()) {
            if (auto childMeta = interface_cast<META_NS::IMetadata>(child)) {
                ResolvePropertyPathInMeta(*childMeta, scene, contentRoot);
                ResolveInlineAnimationChildren(*childMeta, scene, contentRoot, depth + 1);
            }
        }
    }

    // Resolve a single animation resource's PropertyPath (top-level + inline children)
    // against the template's instantiated content root. Skips entries with no
    // scene-side options.
    void ResolveAnimationOption(const CORE_NS::ResourceInfo& src, BASE_NS::string_view oldGroup,
        BASE_NS::string_view newGroup, const SCENE_NS::IScene::Ptr& scene, const META_NS::IObject::Ptr& contentRoot)
    {
        auto resManager = interface_cast<META_NS::IResourceManagerExtension>(SCENE_NS::GetResourceManager(scene));
        if (!resManager) {
            return;
        }
        CORE_NS::ResourceId remapped = src.id;
        RemapGroup(remapped, oldGroup, newGroup);
        auto entries = resManager->GetResources({remapped}, scene);
        if (entries.empty() || !entries.front().options) {
            return;
        }
        auto meta = interface_cast<META_NS::IMetadata>(entries.front().options);
        if (!meta) {
            return;
        }
        ResolvePropertyPathInMeta(*meta, scene, contentRoot);
        ResolveInlineAnimationChildren(*meta, scene, contentRoot);
    }

    // After the template content is built we know the content root, so options
    // that carry path strings (today: AnimationTemplate's PropertyPath) can be
    // resolved against it. Each resource type with this pattern needs its own
    // case here; only animations have the pattern today.
    void ResolveTemplatePathOptions(const BASE_NS::vector<CORE_NS::ResourceInfo>& resources,
        BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup, const SCENE_NS::IScene::Ptr& scene,
        const META_NS::IObject::Ptr& contentRoot)
    {
        if (!scene || !contentRoot) {
            return;
        }
        const auto animType = META_NS::ClassId::AnimationResource.Id().ToUid();
        for (auto&& r : resources) {
            if (r.type == animType) {
                ResolveAnimationOption(r, oldGroup, newGroup, scene, contentRoot);
            }
        }
    }

    META_NS::IObject::Ptr Instantiate()
    {
        auto expNode = interface_cast<SCENE_NS::IExportedNode>(&pint_);
        if (!expNode) {
            return nullptr;
        }
        auto oldGroup = expNode->GetPrimaryGroup();
        auto resources = expNode->GetResources();
        auto group = tcontext_.GetGroup();
        auto node = tcontext_.GetTargetNode();
        if (!node) {
            CORE_LOG_E("Failed to import node template, target node missing");
            return nullptr;
        }
        if (!ImportResources(resources, oldGroup, group, node->GetScene())) {
            CORE_LOG_E("Failed to import node template resources");
            return nullptr;
        }

        auto res = ImportNodes(node->GetScene(), node, oldGroup, group);
        if (!res) {
            return nullptr;
        }
        ResolveTemplatePathOptions(resources, oldGroup, group, node->GetScene(), res.object);
        return META_NS::IObject::Ptr(new NodeImportResult{interface_pointer_cast<SCENE_NS::INode>(res.object),
            BASE_NS::move(importedResources_),
            BASE_NS::move(associatedResources_)});
    }

private:
    IImporterInternal& importer_;
    INodeTemplatePayloadInternal& pint_;
    SCENE_NS::INodeTemplateContext& tcontext_;

    BASE_NS::vector<CORE_NS::ResourceIdContext> importedResources_;
    BASE_NS::vector<CORE_NS::ResourceIdContext> associatedResources_;
};

META_NS::IObject::Ptr NodeTemplateInstantiator::Instantiate(
    const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context)
{
    auto ntcontext = interface_cast<SCENE_NS::INodeTemplateContext>(context);
    if (!ntcontext) {
        CORE_LOG_W("No node template context");
        return nullptr;
    }
    auto obj = META_NS::GetValue(templ.Content());
    auto internal = interface_cast<INodeTemplatePayloadInternal>(obj);
    if (!internal) {
        CORE_LOG_W("Invalid content");
        return nullptr;
    }
    auto importer = internal->GetImporter();
    if (!importer) {
        CORE_LOG_W("Importer has died already");
        return nullptr;
    }
    return TInstantiator(*importer, *internal, *ntcontext).Instantiate();
}

META_NS::IObject::Ptr NodeTemplateInstantiator::Update(
    META_NS::IObject& inst, const META_NS::IObjectTemplate& templ, const META_NS::SharedPtrIInterface& context)
{
    auto obj = META_NS::GetValue(templ.Content());
    auto internal = interface_cast<INodeTemplatePayloadInternal>(obj);
    if (!internal) {
        CORE_LOG_W("Invalid content");
        return nullptr;
    }
    auto importer = internal->GetImporter();
    if (!importer) {
        CORE_LOG_W("Importer has died already");
        return nullptr;
    }
    if (auto node = interface_cast<SCENE_NS::INode>(&inst)) {
        return NodeTemplateUpdater(*importer, templ, *node).Update();
    }
    return nullptr;
}

SCENE_IMP_END_NAMESPACE()