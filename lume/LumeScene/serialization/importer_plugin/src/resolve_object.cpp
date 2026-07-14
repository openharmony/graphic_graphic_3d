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

#include "resolve_object.h"

#include <charconv>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_scene.h>

#include <meta/interface/intf_attach.h>

#include "perf/cpu_perf_scope.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {
constexpr const BASE_NS::string_view SEPARATOR_CHARS = "./![]";
constexpr const char ESCAPE_CHAR = '\\';

bool IsSeparatorChar(char ch)
{
    return SEPARATOR_CHARS.find(ch) != BASE_NS::string_view::npos;
}

BASE_NS::string UnescapeName(BASE_NS::string_view str)
{
    BASE_NS::string res{str};
    for (size_t i = 0; i < res.size(); ++i) {
        if (res[i] == ESCAPE_CHAR) {
            res.erase(i, 1);
        }
    }
    return res;
}

BASE_NS::string_view::size_type FindUnescapedDoubleColon(BASE_NS::string_view str)
{
    for (size_t i = 0; i + 1 < str.size(); ++i) {
        if (str[i] == ESCAPE_CHAR) {
            ++i;
        } else if (str[i] == ':' && str[i + 1] == ':') {
            return i;
        }
    }
    return BASE_NS::string_view::npos;
}

struct ObjectResolver {
    explicit ObjectResolver(ImportContext& context) : context_(context)
    {}

    BASE_NS::string_view::size_type FindFirstSeparator(BASE_NS::string_view path)
    {
        BASE_NS::string_view::size_type res{};

        while (res != BASE_NS::string_view::npos) {
            res = path.find_first_of(".!/", res);
            if (res != BASE_NS::string_view::npos) {
                if (res == 0 || path[res - 1] != ESCAPE_CHAR) {
                    return res;
                }
                ++res;
            }
        }
        return res;
    }

    BASE_NS::string GetNameAndSub(BASE_NS::string_view element, size_t& index)
    {
        if (element.size() > 3 && element.back() == ']' && element[element.size() - 2] != ESCAPE_CHAR) {
            auto pos = element.find_last_of('[');
            if (pos != BASE_NS::string_view::npos) {
                std::from_chars_result res =
                    std::from_chars(element.data() + pos + 1, element.data() + element.size() - 1, index);
                if (res.ec != std::errc{} || res.ptr != element.data() + element.size() - 1) {
                    CORE_LOG_W("Invalid index");
                }
                element = element.substr(0, pos);
            }
        }
        return UnescapeName(element);
    }

    ImportResult GetIObjectFromProperty(const META_NS::IProperty::Ptr& p, size_t index)
    {
        META_NS::IObject::Ptr res;
        if (index != -1) {
            if (META_NS::ArrayPropertyLock arr{p}) {
                if (index < arr->GetSize()) {
                    res = interface_pointer_cast<META_NS::IObject>(META_NS::GetPointer(arr->GetAnyAt(index)));
                } else {
                    CORE_LOG_W("Index out of bounds: %s [%zu]", p->GetName().c_str(), index);
                    return ImportResult{context_.CreateDiagnostics(
                        "Index out of bounds: " + p->GetName() + "[" + BASE_NS::to_string(index) + "]")};
                }
            } else {
                CORE_LOG_W("Trying to index non-array property: %s", p->GetName().c_str());
                return ImportResult{context_.CreateDiagnostics("Trying to index non-array property: " + p->GetName())};
            }
        } else {
            if (auto obj = META_NS::GetPointer(p)) {
                res = interface_pointer_cast<META_NS::IObject>(obj);
            }
        }
        return ImportResult{res};
    }

    ImportResult ResolveAttachment(
        const META_NS::IObject::Ptr& obj, const BASE_NS::string& name, BASE_NS::string_view path)
    {
        auto att = interface_cast<META_NS::IAttach>(obj);
        if (!att) {
            CORE_LOG_E("Object does not support attachments (resolving attachment '%s')", name.c_str());
            return ImportResult{context_.CreateDiagnostics(
                "Object does not support attachments (resolving attachment '" + name + "')")};
        }

        auto attcont = att->GetAttachmentContainer(true);
        auto child = interface_pointer_cast<META_NS::IObject>(
            attcont->FindAny({name, META_NS::TraversalType::NO_HIERARCHY, {}, false}));
        if (!child) {
            CORE_LOG_W("No such attachment for object [%s]", name.c_str());
            return ImportResult{context_.CreateDiagnostics("No such attachment for object [" + name + "]")};
        }
        return ResolveRecursive(child, path);
    }

    ImportResult ResolveProperty(
        const META_NS::IObject::Ptr& obj, const BASE_NS::string& name, size_t index, BASE_NS::string_view path)
    {
        auto prop = interface_cast<META_NS::IMetadata>(obj);
        if (!prop) {
            CORE_LOG_E("Object does not implement IMetadata (resolving property '%s')", name.c_str());
            return ImportResult{
                context_.CreateDiagnostics("Object does not implement IMetadata (resolving property '" + name + "')")};
        }
        auto p = prop->GetProperty(name);
        auto child = interface_pointer_cast<META_NS::IObject>(p);
        if (!child) {
            CORE_LOG_W("No such property for object [%s]", name.c_str());
            return ImportResult{context_.CreateDiagnostics("No such property for object [" + name + "]")};
        }
        if (path.empty()) {
            // Terminal property: surface the wrapper this property was looked
            // up on so callers can do correct readonly checks on forwarded
            // properties.
            if (parent_) {
                *parent_ = obj;
            }
        } else {
            // if the path is not empty we try to take the object in the property to continue
            auto res = GetIObjectFromProperty(p, index);
            if (!res) {
                return res;
            }
            child = res.object;
        }
        if (!child) {
            CORE_LOG_W("Property value cannot be followed to resolve path [%s]", name.c_str());
            return ImportResult{
                context_.CreateDiagnostics("Property value cannot be followed to resolve path [" + name + "]")};
        }
        return ResolveRecursive(child, path);
    }

    ImportResult ResolveChild(const META_NS::IObject::Ptr& obj, const BASE_NS::string& name, BASE_NS::string_view path)
    {
        auto cont = interface_cast<META_NS::IContainer>(obj);
        if (!cont) {
            CORE_LOG_E("Object is not a container (resolving child '%s')", name.c_str());
            return ImportResult{
                context_.CreateDiagnostics("Object is not a container (resolving child '" + name + "')")};
        }
        auto child = cont->FindByName(name);
        if (!child) {
            CORE_LOG_W("No such child for object [%s]", name.c_str());
            return ImportResult{context_.CreateDiagnostics("No such child for object [" + name + "]")};
        }
        return ResolveRecursive(child, path);
    }

    ImportResult ResolveRecursive(const META_NS::IObject::Ptr& obj, BASE_NS::string_view path)
    {
        if (path.empty()) {
            return ImportResult{obj};
        }
        char pre = '/';
        if (path[0] == '!' || path[0] == '.' || path[0] == '/') {
            pre = path[0];
            path.remove_prefix(1);
        }
        auto pos = FindFirstSeparator(path);
        auto element = path.substr(0, pos);
        std::size_t index = -1;
        BASE_NS::string name = GetNameAndSub(element, index);
        path.remove_prefix(element.size());
        META_NS::IObject::Ptr child;
        if (pre == '!') {
            return ResolveAttachment(obj, name, path);
        } else if (pre == '.') {
            // dot without name is interpreted as current object
            if (name.empty()) {
                return ResolveRecursive(obj, path);
            }
            return ResolveProperty(obj, name, index, path);
        } else if (pre == '/') {
            return ResolveChild(obj, name, path);
        }
        CORE_LOG_W("Invalid path string: %s", BASE_NS::string(path).c_str());
        return ImportResult{context_.CreateDiagnostics("Invalid path string: " + path)};
    }

    CORE_NS::ResourceId ParseResourceId(BASE_NS::string_view& path) const
    {
        if (path.empty()) {
            return {};
        }
        size_t i = 0;
        for (; i < path.size() && !IsSeparatorChar(path[i]); ++i) {
            if (path[i] == ESCAPE_CHAR) {
                ++i;
            }
        }
        auto raw = path.substr(0, i);
        auto pos = FindUnescapedDoubleColon(raw);
        CORE_NS::ResourceId resource;
        if (pos != BASE_NS::string_view::npos) {
            resource = CORE_NS::ResourceId{UnescapeName(raw.substr(pos + 2)), UnescapeName(raw.substr(0, pos))};
        } else {
            resource = CORE_NS::ResourceId{UnescapeName(raw)};
        }
        path.remove_prefix(i);
        return resource;
    }

    CORE_NS::IResourceManager::Ptr GetResourceManager() const
    {
        auto resources = interface_pointer_cast<CORE_NS::IResourceManager>(context_.GetImportParameters().object);
        if (!resources) {
            resources = SCENE_NS::GetResourceManager(context_.GetImportParameters().scene);
        }
        if (!resources) {
            resources = context_.GetRenderContext()->GetResources();
        }
        return resources;
    }

    ImportResult ResolveResource(BASE_NS::string_view& path)
    {
        path.remove_prefix(1);
        auto rid = ParseResourceId(path);
        if (!rid.IsValid()) {
            CORE_LOG_E("Invalid resource id in object path: '%s'", BASE_NS::string(path).c_str());
            return ImportResult{context_.CreateDiagnostics("Invalid resource id in object path: '" + path + "'")};
        }
        if (auto overr = context_.FindResourceGroupOverride(rid.group)) {
            rid.group = *overr;
        }
        auto resources = GetResourceManager();
        if (!resources) {
            CORE_LOG_E(
                "Invalid state: resource manager not available (resolving resource '%s')", rid.ToString().c_str());
            return ImportResult{context_.CreateDiagnostics(
                "Invalid state: resource manager not available (resolving resource '" + rid.ToString() + "')")};
        }
        CORE_NS::ResourceIdContext pres{rid, context_.GetImportParameters().scene};
        auto resource = resources->GetResource(pres);
        // we first try with context and if not found, try without
        if (!resource) {
            pres.context = nullptr;
            resource = resources->GetResource(pres);
        }
        if (!resource) {
            CORE_LOG_E("No such resource: %s", rid.ToString().c_str());
            return ImportResult{context_.CreateDiagnostics("No such resource: " + rid.ToString())};
        }
        return ImportResult{interface_pointer_cast<META_NS::IObject>(resource)};
    }

    ImportResult Resolve(META_NS::IObject::Ptr obj, BASE_NS::string_view path, bool onlyChildren)
    {
        if (path.empty()) {
            return ImportResult{obj};
        }
        if (onlyChildren && (path[0] == '/' || path[0] == '@')) {
            CORE_LOG_E("Invalid object path, only children allowed in this context: %s", BASE_NS::string(path).c_str());
            return ImportResult{
                context_.CreateDiagnostics("Invalid object path, only children allowed in this context: " + path)};
        }
        if (path[0] == '/') {
            // Anchor on the importRoot when set (template instantiation), so
            // paths inside an instantiated subtree resolve relative to that
            // subtree's root rather than the global scene root. Otherwise fall
            // back to the scene root, preserving scene-absolute `/` semantics
            // for ordinary scene authoring.
            const auto& p = context_.GetImportParameters();
            if (p.importRoot) {
                obj = p.importRoot;
            } else if (p.scene) {
                obj = interface_pointer_cast<META_NS::IObject>(p.scene->GetRootNode().GetResult());
            }
            path.remove_prefix(1);
        } else if (path[0] == '@') {
            // check starts with @ and resource id
            auto res = ResolveResource(path);
            if (!res) {
                return res;
            }
            obj = res.object;
        }
        return ResolveRecursive(obj, path);
    }

    void SetParentOut(META_NS::IObject::Ptr* parent)
    {
        parent_ = parent;
    }

private:
    ImportContext& context_;
    META_NS::IObject::Ptr* parent_{};
};
}  // namespace

ImportResult ResolveObject(ImportContext& context, const META_NS::IObject::Ptr& base, BASE_NS::string_view path,
    bool onlyChildren, META_NS::IObject::Ptr* parent)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "ResolveObject");
    ObjectResolver resolver(context);
    resolver.SetParentOut(parent);
    auto res = resolver.Resolve(base, path, onlyChildren);
    if (res.error) {
        CORE_LOG_W("Resolving object failed (%s)", BASE_NS::string(path).c_str());
    }
    return res;
}

SCENE_IMP_END_NAMESPACE()