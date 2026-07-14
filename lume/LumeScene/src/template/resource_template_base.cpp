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

#include "resource_template_base.h"

#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/scene_utils.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/intf_resource_context.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/property/intf_stack_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "template_copy_helpers.h"

SCENE_BEGIN_NAMESPACE()

// ---------------------------------------------------------------------------
// Promote helpers
// ---------------------------------------------------------------------------

static void PromoteProperty(const META_NS::IProperty::Ptr& prop)
{
    META_NS::PropertyLock lock{prop};
    if (lock) {
        if (auto sc = interface_cast<META_NS::IStackProperty>(prop.get())) {
            sc->SetDefaultValue(prop->GetValue());
        }
        prop->ResetValue();
    }
}

static void PromoteMetadata(META_NS::IMetadata& meta);
static bool IsArrayOfSubObjects(const META_NS::IProperty::ConstPtr& prop);

// Exposed via template_copy_helpers.h
META_NS::IMetadata* GetArrayElementMetadata(const META_NS::IArrayAny& arr, size_t index)
{
    auto itemAny = arr.Clone({META_NS::CloneValueType::DEFAULT_VALUE, META_NS::TypeIdRole::ITEM});
    if (!itemAny) {
        return nullptr;
    }
    if (!arr.GetAnyAt(index, *itemAny)) {
        return nullptr;
    }
    auto ptr = META_NS::GetPointer(*itemAny);
    return interface_cast<META_NS::IMetadata>(ptr.get());
}

static void PromoteArrayElements(const META_NS::IProperty::Ptr& prop)
{
    auto arr = interface_cast<META_NS::IArrayAny>(&prop->GetValue());
    if (!arr) {
        return;
    }
    for (size_t i = 0; i < arr->GetSize(); ++i) {
        if (auto m = GetArrayElementMetadata(*arr, i)) {
            PromoteMetadata(*m);
        }
    }
}

static void PromoteReadonlyProperty(const META_NS::IProperty::Ptr& prop)
{
    if (auto sub = META_NS::GetPointer<META_NS::IMetadata>(prop)) {
        PromoteMetadata(*sub);
    } else {
        PromoteArrayElements(prop);
    }
}

// Returns true if the property holds an owned sub-object (has IMetadata but is
// NOT a resource reference like IImage::Ptr or IShader::Ptr).
static bool IsOwnedSubObject(const META_NS::IProperty::ConstPtr& prop)
{
    auto sub = META_NS::GetPointer<META_NS::IMetadata>(prop);
    if (!sub) {
        return false;
    }
    return interface_cast<CORE_NS::IResource>(sub) == nullptr;
}

static void PromoteMetadata(META_NS::IMetadata& meta)
{
    for (auto&& p : meta.GetProperties()) {
        const bool isSubObject = IsArrayOfSubObjects(p) || IsOwnedSubObject(p);
        if (meta.GetMetadata(META_NS::MetadataType::PROPERTY, p->GetName()).readOnly || isSubObject) {
            PromoteReadonlyProperty(p);
        } else if (p->IsValueSet()) {
            PromoteProperty(p);
        }
    }
}

// ---------------------------------------------------------------------------
// Copy helpers
// ---------------------------------------------------------------------------

static void CopyDefaultToDefault(const META_NS::IProperty::ConstPtr& src, META_NS::IProperty& dst)
{
    META_NS::PropertyLock plock{src};
    META_NS::PropertyLock lock{&dst};
    if (!plock || !lock) {
        return;
    }
    auto srcStack = interface_cast<META_NS::IStackProperty>(src.get());
    auto dstStack = interface_cast<META_NS::IStackProperty>(&dst);
    if (srcStack && dstStack) {
        dstStack->SetDefaultValue(srcStack->GetDefaultValue());
    }
    dst.ResetValue();
}

static bool IsArrayOfSubObjects(const META_NS::IProperty::ConstPtr& prop)
{
    auto arr = interface_cast<META_NS::IArrayAny>(&prop->GetValue());
    if (!arr || arr->GetSize() == 0) {
        return false;
    }
    return GetArrayElementMetadata(*arr, 0) != nullptr;
}

static bool NeedsDeepCopy(const META_NS::IMetadata& meta, const META_NS::IProperty::ConstPtr& prop)
{
    if (meta.GetMetadata(META_NS::MetadataType::PROPERTY, prop->GetName()).readOnly) {
        return true;
    }
    if (IsArrayOfSubObjects(prop)) {
        return true;
    }
    return IsOwnedSubObject(prop);
}

static void CopyMetadataProperty(META_NS::IMetadata& dst, const META_NS::IProperty::ConstPtr& srcProp, CopyMode mode)
{
    auto dstProp = dst.GetProperty(srcProp->GetName());
    if (!dstProp) {
        return;
    }
    if (NeedsDeepCopy(dst, srcProp)) {
        CopyReadonlyProperty(srcProp, dstProp, mode);
    } else {
        CopyProperty(srcProp, *dstProp, mode);
    }
}

// ---------------------------------------------------------------------------
// Resource-id reference helpers (see template_copy_helpers.h)
// ---------------------------------------------------------------------------

static CORE_NS::IResourceManager::Ptr GetResourceManagerFromContext(const CORE_NS::ResourceContextPtr& context);

bool IsResourceIdProperty(const META_NS::IProperty::ConstPtr& prop)
{
    return prop && prop->IsCompatible(META_NS::UidFromType<CORE_NS::ResourceId>());
}

template<typename TPtr>
static void AssignTypedAsValueOrDefault(const META_NS::IProperty::Ptr& dstProp, const TPtr& typed, bool asDefault)
{
    if (!asDefault) {
        META_NS::SetValue<TPtr>(dstProp, typed);
        return;
    }
    META_NS::PropertyLock l{dstProp.get()};
    auto any = l->GetValueAny().Clone();
    if (!any || !META_NS::SetPointer(*any, interface_pointer_cast<CORE_NS::IInterface>(typed))) {
        return;
    }
    l->SetDefaultValueAny(*any);
    if (!META_NS::IsValueSet(*dstProp)) {
        dstProp->ResetValue();
    }
}

void AssignTypedResource(
    const META_NS::IProperty::Ptr& dstProp, const CORE_NS::IResource::Ptr& resource, bool asDefault)
{
    if (auto image = interface_pointer_cast<SCENE_NS::IImage>(resource)) {
        AssignTypedAsValueOrDefault(dstProp, image, asDefault);
    } else if (auto shader = interface_pointer_cast<SCENE_NS::IShader>(resource)) {
        AssignTypedAsValueOrDefault(dstProp, shader, asDefault);
    } else if (auto env = interface_pointer_cast<SCENE_NS::IEnvironment>(resource)) {
        AssignTypedAsValueOrDefault(dstProp, env, asDefault);
    } else if (auto pp = interface_pointer_cast<SCENE_NS::IPostProcess>(resource)) {
        AssignTypedAsValueOrDefault(dstProp, pp, asDefault);
    }
}

CORE_NS::IResource::Ptr ResolveResourceId(
    const CORE_NS::ResourceId& id, BASE_NS::string_view propName, const CORE_NS::ResourceContextPtr& context)
{
    if (!id.IsValid()) {
        return nullptr;
    }
    auto rm = GetResourceManagerFromContext(context);
    if (!rm) {
        CORE_LOG_W("Resource ref '%.*s' (%s): no resource manager in apply context",
            static_cast<int>(propName.size()),
            propName.data(),
            id.ToString().c_str());
        return nullptr;
    }
    auto resource = rm->GetResource({id, context});
    if (!resource) {
        resource = rm->GetResource(CORE_NS::ResourceIdContext{id});
    }
    if (!resource) {
        CORE_LOG_W("Resource ref '%.*s' (%s) could not be resolved at apply time",
            static_cast<int>(propName.size()),
            propName.data(),
            id.ToString().c_str());
    }
    return resource;
}

// Recurse into an owned sub-object (e.g. PostProcess's Bloom). Resources themselves are
// excluded — they are leaf refs, not owners.
static void TryRecurseIntoSubObject(const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp,
    const CORE_NS::ResourceContextPtr& context, bool asDefault)
{
    auto srcSub = META_NS::GetPointer<META_NS::IMetadata>(srcProp);
    if (!srcSub || interface_cast<CORE_NS::IResource>(srcSub.get())) {
        return;
    }
    auto dstSub = META_NS::GetPointer<META_NS::IMetadata>(dstProp);
    if (dstSub) {
        ResolveResourceIdRefs(*srcSub, *dstSub, context, asDefault);
    }
}

static void ResolveOneIdRef(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dst,
    const CORE_NS::ResourceContextPtr& context, bool asDefault)
{
    const auto name = srcProp->GetName();
    auto dstProp = dst.GetProperty(name);
    if (!dstProp) {
        return;
    }
    auto typedSrc = META_NS::Property<const CORE_NS::ResourceId>(srcProp);
    if (!typedSrc) {
        return;
    }
    auto resource = ResolveResourceId(META_NS::GetValue(typedSrc), name, context);
    if (resource) {
        AssignTypedResource(dstProp, resource, asDefault);
    }
}

void ResolveResourceIdRefs(
    const META_NS::IMetadata& src, META_NS::IMetadata& dst, const CORE_NS::ResourceContextPtr& context, bool asDefault)
{
    if (!context) {
        return;
    }
    for (auto&& srcProp : src.GetProperties()) {
        if (IsResourceIdProperty(srcProp)) {
            ResolveOneIdRef(srcProp, dst, context, asDefault);
        } else {
            TryRecurseIntoSubObject(srcProp, dst.GetProperty(srcProp->GetName()), context, asDefault);
        }
    }
}

// Exposed via template_copy_helpers.h
bool CaptureResourceRefAsId(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dst)
{
    if (!srcProp) {
        return false;
    }
    auto resource = META_NS::GetPointer<CORE_NS::IResource>(srcProp);
    if (!resource) {
        return false;
    }
    auto id = resource->GetResourceId();
    if (!id.IsValid()) {
        return false;
    }
    if (auto existing = dst.GetProperty<CORE_NS::ResourceId>(srcProp->GetName(), META_NS::MetadataQuery::EXISTING)) {
        META_NS::SetValue(existing, id);
        return true;
    }
    auto idProp = META_NS::ConstructProperty<CORE_NS::ResourceId>(srcProp->GetName());
    META_NS::SetValue<CORE_NS::ResourceId>(idProp.GetProperty(), id);
    dst.AddProperty(idProp.GetProperty());
    return true;
}

// True when src's value is a typed pointer to a CORE_NS::IResource (e.g. IImage::Ptr,
// IShader::Ptr). Used to decide whether a generic copy should be redirected through
// CaptureResourceRefAsId for live→template captures.
static bool IsResourcePointerProperty(const META_NS::IProperty::ConstPtr& srcProp)
{
    return srcProp && META_NS::GetPointer<CORE_NS::IResource>(srcProp) != nullptr;
}

// Deep-copy `srcSub` into a fresh META_NS::ClassId::Object instance attached to `dst` under
// `name`. The recursive CopyMetadata picks up CopyOnePropertyExcept's capture rules so any
// resource-pointer sub-properties become ResourceId on the captured sub-object, and any
// nested owned sub-objects get their own fresh copies. The fresh instance gives the captured
// template an independent copy of the source's sub-object state rather than a shared
// pointer back into the live source. `srcWasSet` mirrors the source property's IsValueSet
// state onto the new property (matches the META_NS::Clone behaviour this branch replaces).
static void CaptureSubObjectInto(
    BASE_NS::string_view name, const META_NS::IMetadata& srcSub, META_NS::IMetadata& dst, CopyMode mode, bool srcWasSet)
{
    auto newSub = META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
    auto newSubMeta = interface_cast<META_NS::IMetadata>(newSub);
    if (!newSubMeta) {
        return;
    }
    CopyMetadata(srcSub, *newSubMeta, mode);
    auto prop = META_NS::ConstructProperty<META_NS::IObject::Ptr>(name);
    dst.AddProperty(prop.GetProperty());
    if (srcWasSet) {
        META_NS::SetValue<META_NS::IObject::Ptr>(prop.GetProperty(), newSub);
    }
}

// Exposed via template_copy_helpers.h
void CopyToDefaultMaybeDeep(META_NS::IMetadata& dst, const META_NS::IProperty::ConstPtr& srcProp)
{
    // ResourceId-typed source props are bound by ResolveResourceIdRefs at apply time; their
    // dst counterpart is a typed pointer, so a default-slot copy here would be a type
    // mismatch.
    if (IsResourceIdProperty(srcProp)) {
        return;
    }
    auto dstProp = dst.GetProperty(srcProp->GetName());
    if (dstProp && NeedsDeepCopy(dst, srcProp)) {
        CopyReadonlyProperty(srcProp, dstProp, CopyMode::WITH_DEFAULTS);
    } else {
        // Update the default slot but keep any user override (reset only when not explicitly set),
        // matching the resource-ref bind path. For the base-resource apply (fresh destination, no
        // override yet) this is equivalent to a reset.
        META_NS::CopyToDefault(srcProp, dst);
    }
}

// Exposed via template_copy_helpers.h

void CopyArrayElements(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode)
{
    auto srcArr = interface_cast<META_NS::IArrayAny>(&srcProp->GetValue());
    auto dstArr = interface_cast<META_NS::IArrayAny>(&dstProp->GetValue());
    if (!srcArr || !dstArr) {
        return;
    }
    const auto count = srcArr->GetSize();
    for (size_t i = 0; i < count && i < dstArr->GetSize(); ++i) {
        auto srcMeta = GetArrayElementMetadata(*srcArr, i);
        auto dstMeta = GetArrayElementMetadata(*dstArr, i);
        if (srcMeta && dstMeta) {
            CopyMetadata(*srcMeta, *dstMeta, mode);
        }
    }
}

void CopyReadonlyProperty(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode)
{
    auto srcMeta = META_NS::GetPointer<META_NS::IMetadata>(srcProp);
    auto dstMeta = META_NS::GetPointer<META_NS::IMetadata>(dstProp);
    if (srcMeta && dstMeta) {
        CopyMetadata(*srcMeta, *dstMeta, mode);
    } else {
        CopyArrayElements(srcProp, dstProp, mode);
    }
}

void CopyProperty(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IProperty& dstProp, CopyMode mode)
{
    if (mode == CopyMode::SET_AS_DEFAULTS) {
        if (srcProp->IsValueSet()) {
            // Land the value on dst's default slot, but only clear the override when the user has
            // not explicitly set one — re-applying a template must not stomp a user override.
            META_NS::CopyToDefault(srcProp, dstProp);
        }
        return;
    }
    if (mode == CopyMode::WITH_DEFAULTS) {
        CopyDefaultToDefault(srcProp, dstProp);
    }
    if (srcProp->IsValueSet()) {
        META_NS::CopyValue(srcProp, dstProp);
    }
}

void CopyMetadata(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode)
{
    CopyMetadataExcept(src, dst, mode, {});
}

static bool IsExcludedName(std::initializer_list<BASE_NS::string_view> skipNames, BASE_NS::string_view name)
{
    for (auto&& skip : skipNames) {
        if (!skip.empty() && name == skip) {
            return true;
        }
    }
    return false;
}

// Copy one property from src onto dst, honoring the skip list and the resource-id
// convention. Used by CopyMetadataExcept's per-property loop.
static void CopyOnePropertyExcept(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dst, CopyMode mode,
    std::initializer_list<BASE_NS::string_view> skipNames)
{
    // ResourceId-typed source properties are bound onto the live instance's typed pointer
    // property of the same name by ResolveResourceIdRefs at apply time. A generic copy here
    // would be a type mismatch (ResourceId → IShader::Ptr etc.).
    if (IsResourceIdProperty(srcProp)) {
        return;
    }
    if (IsExcludedName(skipNames, srcProp->GetName())) {
        return;
    }
    // Live-to-template capture: when src is a resource-pointer property and dst either
    // lacks the property or already has a ResourceId-typed slot of that name (i.e. dst is
    // a template), capture the bound resource's id rather than attempting a typed copy.
    // When dst has a same-typed pointer property (live-to-live copy), fall through.
    if (IsResourcePointerProperty(srcProp)) {
        auto dstProp = dst.GetProperty(srcProp->GetName());
        if (!dstProp || IsResourceIdProperty(dstProp)) {
            CaptureResourceRefAsId(srcProp, dst);
            return;
        }
    }
    const bool isSubObject = IsArrayOfSubObjects(srcProp) || IsOwnedSubObject(srcProp);
    const bool shouldCopy = mode == CopyMode::WITH_DEFAULTS || srcProp->IsValueSet() || isSubObject;
    if (!shouldCopy) {
        return;
    }
    auto dstProp = dst.GetProperty(srcProp->GetName());
    if (dstProp) {
        CopyMetadataProperty(dst, srcProp, mode);
    } else if (IsOwnedSubObject(srcProp)) {
        // Capture-into-template path for owned sub-objects (e.g. IPostProcess's Bloom):
        // create a fresh, independent sub-object on dst and deep-copy contents. The
        // recursive copy hits this same function and so nested resource-pointer fields
        // inside the sub-object (e.g. IBloom::DirtMaskImage) get captured as ResourceIds.
        if (auto srcSub = META_NS::GetPointer<META_NS::IMetadata>(srcProp)) {
            CaptureSubObjectInto(srcProp->GetName(), *srcSub, dst, mode, srcProp->IsValueSet());
        }
    } else if (mode == CopyMode::OVERRIDES_ONLY && srcProp->IsValueSet()) {
        META_NS::Clone(srcProp, dst);
    }
}

void CopyMetadataExcept(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode,
    std::initializer_list<BASE_NS::string_view> skipNames)
{
    for (auto&& srcProp : src.GetProperties()) {
        CopyOnePropertyExcept(srcProp, dst, mode, skipNames);
    }
}

void CopyPropertyIfSet(const META_NS::IProperty::ConstPtr& src, const META_NS::IProperty::Ptr& dst)
{
    if (src && dst && src->IsValueSet()) {
        META_NS::CopyValue(src, *dst);
    }
}

void ClonePropertyWithDefaults(const META_NS::IProperty::ConstPtr& src, META_NS::IMetadata& dst)
{
    META_NS::PropertyLock lock{src};
    if (!lock) {
        return;
    }
    auto copy = META_NS::DuplicatePropertyType(META_NS::GetObjectRegistry(), src);
    if (!copy) {
        return;
    }
    auto srcStack = interface_cast<META_NS::IStackProperty>(src.get());
    auto dstStack = interface_cast<META_NS::IStackProperty>(copy.get());
    if (srcStack && dstStack) {
        dstStack->SetDefaultValue(srcStack->GetDefaultValue());
    }
    if (src->IsValueSet()) {
        copy->SetValue(src->GetValue());
    }
    dst.AddProperty(copy);
}

// ---------------------------------------------------------------------------
// Other helpers
// ---------------------------------------------------------------------------

static void ResetSetProperties(META_NS::IMetadata& meta)
{
    for (auto&& md : meta.GetAllMetadatas(META_NS::MetadataType::PROPERTY)) {
        auto p = meta.GetProperty(md.name);
        if (p && p->IsValueSet()) {
            META_NS::PropertyLock lock{p};
            if (lock) {
                p->ResetValue();
            }
        }
    }
}

static void CopyIntoReadonlySubObjects(const META_NS::IMetadata& src, META_NS::IMetadata& dst)
{
    for (auto&& srcProp : src.GetProperties()) {
        auto dstProp = dst.GetProperty(srcProp->GetName());
        if (dstProp && NeedsDeepCopy(dst, srcProp)) {
            auto srcMeta = META_NS::GetPointer<META_NS::IMetadata>(srcProp);
            auto dstMeta = META_NS::GetPointer<META_NS::IMetadata>(dstProp);
            if (srcMeta && dstMeta) {
                META_NS::CopyValue(*srcMeta, *dstMeta);
            }
        }
    }
}

static CORE_NS::IResourceManager::Ptr GetResourceManagerFromContext(const CORE_NS::ResourceContextPtr& context)
{
    if (auto rcontext = interface_cast<IResourceContext>(context)) {
        return GetResourceManager(rcontext->GetScene());
    }
    if (auto scene = interface_pointer_cast<IScene>(context)) {
        return GetResourceManager(scene);
    }
    return nullptr;
}

CORE_NS::ResourceContextPtr GetApplyContextFromObject(const META_NS::IObject& target)
{
    if (auto access = interface_cast<IEcsObjectAccess>(&target)) {
        if (auto ecsObject = access->GetEcsObject()) {
            if (auto internalScene = ecsObject->GetScene()) {
                return interface_pointer_cast<CORE_NS::IInterface>(internalScene->GetScene());
            }
        }
    }
    // Fallback: ECS object not yet initialised (no associated scene) — use the context the
    // resource itself was created with, from its ResourceId context.
    if (auto resource = interface_cast<CORE_NS::IResource>(&target)) {
        return resource->GetContext().lock();
    }
    return nullptr;
}

// Default impl of the base-defaults property walk. Enumerates via
// GetAllMetadatas / GetProperty so forwarded/lazy properties on the base
// resource (e.g. Material's META_FORWARDED_PROPERTY entries) are instantiated
// and visible. Plain GetProperties() would miss them and the chain wouldn't
// propagate values for resources that haven't yet materialized their
// properties.
static void DefaultApplyBaseDefaults(META_NS::IObject& target, const META_NS::IMetadata& base)
{
    auto dst = interface_cast<META_NS::IMetadata>(&target);
    if (!dst) {
        return;
    }
    for (auto&& md : base.GetAllMetadatas(META_NS::MetadataType::PROPERTY)) {
        if (auto p = base.GetProperty(md.name)) {
            CopyToDefaultMaybeDeep(*dst, p);
        }
    }
}

// ---------------------------------------------------------------------------
// ResourceTemplateBase implementation
// ---------------------------------------------------------------------------

BASE_NS::string ResourceTemplateBase::GetName() const
{
    const META_NS::IMetadata& meta = *this;
    auto prop = meta.GetProperty<const BASE_NS::string>("Name", META_NS::MetadataQuery::EXISTING);
    if (prop) {
        return META_NS::GetValue(prop);
    }
    return {};
}

void ResourceTemplateBase::PromoteToDefaults()
{
    if (promoted_) {
        return;
    }
    promoted_ = true;
    PromoteMetadata(*this);
}

void ResourceTemplateBase::SetTemplateContext(bool isTemplate)
{
    if (isTemplate && !promoted_) {
        PromoteToDefaults();
    }
    templateContext_ = isTemplate;
}

bool ResourceTemplateBase::ValidateResourceType(const CORE_NS::IResource& res) const
{
    return true;
}

bool ResourceTemplateBase::CopyFromTyped(const META_NS::IObject& source, bool onlySetValues)
{
    return false;
}

void ResourceTemplateBase::CloneSubObjects(const META_NS::IMetadata& srcMeta, META_NS::IMetadata& cloneMeta) const
{}

bool ResourceTemplateBase::ApplyTo(META_NS::IObject& target) const
{
    // Standalone apply: template-supplied values manifest as defaults on the live instance.
    return ApplyToTarget(target, /*asDefault=*/true);
}

bool ResourceTemplateBase::ApplyToTarget(META_NS::IObject& target, bool asDefault) const
{
    auto dst = interface_cast<META_NS::IMetadata>(&target);
    if (!dst) {
        return false;
    }
    CopyMetadata(*this, *dst, asDefault ? CopyMode::SET_AS_DEFAULTS : CopyMode::OVERRIDES_ONLY);
    return true;
}

CORE_NS::IResource::Ptr ResourceTemplateBase::ResolveBaseResource(const CORE_NS::ResourceContextPtr& context) const
{
    if (!baseResource_.IsValid()) {
        return nullptr;
    }
    auto rm = GetResourceManagerFromContext(context);
    if (!rm) {
        return nullptr;
    }
    auto base = rm->GetResource({baseResource_, context});
    if (!base) {
        base = rm->GetResource(CORE_NS::ResourceIdContext{baseResource_});
    }
    return base;
}

void ResourceTemplateBase::ApplyBaseResource(
    CORE_NS::IResource& res, const CORE_NS::IResource::Ptr& base, const ApplyBaseDefaultsFn& applyBaseDefaults) const
{
    if (!base) {
        CORE_LOG_W("Could not load base resource for %s", res.GetResourceId().ToString().c_str());
        return;
    }
    auto baseMeta = interface_cast<META_NS::IMetadata>(base);
    auto resObj = interface_cast<META_NS::IObject>(&res);
    auto resMeta = interface_cast<META_NS::IMetadata>(&res);
    if (!baseMeta || !resObj || !resMeta) {
        return;
    }
    // Copy base properties to the derived resource's default slot, regardless of whether the
    // base is an IResourceTemplate (e.g. MaterialTemplate) or a resolved resource (e.g.
    // Material). Going through metadata directly is what makes derivedFrom transitive across
    // multi-level chains: each layer's resolved values become the next layer's defaults, even
    // when an intermediate layer was registered as "material" (resource type) rather than
    // "materialTemplate" (template type).
    if (applyBaseDefaults) {
        applyBaseDefaults(*resObj, *baseMeta);
    } else {
        DefaultApplyBaseDefaults(*resObj, *baseMeta);
    }
    // If the base is a template, also run its structural ApplyTo on res so data that
    // lives outside the meta-property surface (container children delivered via
    // AddAnimation, etc.) is installed before the derived template's ApplyTo layers
    // its overrides on top.
    // Run the base template's structural apply with asDefault=false to preserve the historical
    // override semantics inside the ApplyOptions flow (the standalone defaults path is reached only
    // via the public IResourceTemplate::ApplyTo wrapper).
    if (auto baseApply = interface_cast<ITemplateApplyTarget>(base.get())) {
        baseApply->ApplyToTarget(*resObj, /*asDefault=*/false);
    }
    if (auto derived = interface_cast<META_NS::IDerivedFromTemplate>(&res)) {
        derived->SetTemplateId(baseResource_);
    }
    CopyIntoReadonlySubObjects(*baseMeta, *resMeta);
}

void ResourceTemplateBase::StampTemplateId(CORE_NS::IResource& res) const
{
    if (auto id = interface_cast<META_NS::IDerivedFromTemplate>(&res)) {
        id->SetTemplateId(GetResourceId());
    }
}

void ResourceTemplateBase::MarkImportedFromTemplate(META_NS::IMetadata& resMeta) const
{
    META_NS::SetObjectFlags(&resMeta, IMPORTED_FROM_TEMPLATE_BIT, true);
}

void ResourceTemplateBase::ResolveRefsFromTarget(META_NS::IObject& target, bool asDefault) const
{
    if (auto dst = interface_cast<META_NS::IMetadata>(&target)) {
        ResolveResourceIdRefs(*this, *dst, GetApplyContextFromObject(target), asDefault);
    }
}

bool ResourceTemplateBase::CopyFrom(const META_NS::IObject& source, bool onlySetValues)
{
    if (CopyFromTyped(source, onlySetValues)) {
        return true;
    }
    auto srcMeta = interface_cast<META_NS::IMetadata>(&source);
    if (!srcMeta) {
        return false;
    }
    META_NS::IMetadata& dstMeta = *this;
    CopyMetadata(*srcMeta, dstMeta, onlySetValues ? CopyMode::OVERRIDES_ONLY : CopyMode::WITH_DEFAULTS);
    return true;
}

bool ResourceTemplateBase::ApplyOptions(CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) const
{
    if (!ValidateResourceType(res)) {
        return false;
    }
    auto resObj = interface_cast<META_NS::IObject>(&res);
    if (!resObj) {
        return false;
    }
    if (baseResource_.IsValid()) {
        ApplyBaseResource(res, ResolveBaseResource(context));
    }
    StampTemplateId(res);
    auto resMeta = interface_cast<META_NS::IMetadata>(&res);
    if (templateContext_ && !baseResource_.IsValid() && resMeta) {
        // Template context, no base: copy this template's defaults onto res so the
        // OVERRIDES_ONLY ApplyTo below isn't the only contribution. Don't reset
        // res's existing properties — those include Build-time setup like the
        // Controller registration on Animations that ApplyTo doesn't reinstall.
        CopyMetadata(*this, *resMeta, CopyMode::WITH_DEFAULTS);
    }
    auto applied = ApplyToTarget(*resObj, /*asDefault=*/false);
    if (applied && templateContext_ && resMeta) {
        if (baseResource_.IsValid()) {
            // The JSON parser stores derived's values as defaults rather than overrides,
            // and ApplyTo above uses OVERRIDES_ONLY which would skip them. Layer a
            // WITH_DEFAULTS pass without ResetSetProperties so base.ApplyTo's structural
            // contributions survive.
            CopyMetadata(*this, *resMeta, CopyMode::WITH_DEFAULTS);
        }
        MarkImportedFromTemplate(*resMeta);
    }
    // Bind any ResourceId refs (e.g. RadianceImage on Environment, DirtMaskImage on
    // PostProcess's Bloom) against the apply context's resource manager. Done after the
    // main copy so the refs override any defaults that ApplyTo just established. In
    // template context the refs land on the default slot, matching the rest of the
    // template-context invariant (template-supplied values are property defaults).
    if (resMeta) {
        ResolveResourceIdRefs(*this, *resMeta, context, templateContext_);
    }
    return applied;
}

bool ResourceTemplateBase::UpdateOptions(const CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context)
{
    auto obj = interface_cast<META_NS::IObject>(&res);
    if (!obj) {
        return false;
    }
    return CopyFrom(static_cast<const META_NS::IObject&>(*obj));
}

bool ResourceTemplateBase::Merge(const CORE_NS::IResourceOptions& options)
{
    auto otherMeta = interface_cast<const META_NS::IMetadata>(&options);
    if (!otherMeta) {
        return false;
    }
    PromoteToDefaults();

    META_NS::IMetadata& thisMeta = *this;
    CopyMetadata(*otherMeta, thisMeta, CopyMode::OVERRIDES_ONLY);

    if (auto derived = interface_cast<const META_NS::IDerivedResourceOptions>(&options)) {
        auto otherBase = derived->GetBaseResource();
        if (otherBase.IsValid()) {
            baseResource_ = BASE_NS::move(otherBase);
        }
    }
    return true;
}

CORE_NS::IResourceOptions::Ptr ResourceTemplateBase::Clone() const
{
    auto clone = META_NS::GetObjectRegistry().Create<CORE_NS::IResourceOptions>(GetTemplateClassId());
    if (!clone) {
        return nullptr;
    }
    auto cloneMeta = interface_cast<META_NS::IMetadata>(clone);
    if (!cloneMeta) {
        return nullptr;
    }
    const META_NS::IMetadata& srcMeta = *this;
    for (auto&& srcProp : srcMeta.GetProperties()) {
        auto name = srcProp->GetName();
        auto md = srcMeta.GetMetadata(META_NS::MetadataType::PROPERTY, name);
        if (md.readOnly) {
            // Readonly/sub-object arrays — handled by CloneSubObjects.
        } else if (auto dstProp = cloneMeta->GetProperty(name)) {
            // Static property (Name) — copy in place.
            CopyProperty(srcProp, *dstProp, CopyMode::WITH_DEFAULTS);
        } else {
            // Dynamic property — duplicate into clone.
            ClonePropertyWithDefaults(srcProp, *cloneMeta);
        }
    }
    CloneSubObjects(srcMeta, *cloneMeta);

    if (auto derived = interface_cast<META_NS::IDerivedResourceOptions>(clone)) {
        derived->SetBaseResource(baseResource_);
    }
    // Transfer promoted_ before SetTemplateContext to prevent re-promotion
    // of scene overlay values that were already layered via Merge.
    if (auto pt = interface_cast<IPromotableTemplate>(clone)) {
        pt->SetPromoted(promoted_);
    }
    if (auto tc = interface_cast<ITemplateOptions>(clone)) {
        tc->SetTemplateContext(templateContext_);
    }
    return clone;
}

SCENE_END_NAMESPACE()
