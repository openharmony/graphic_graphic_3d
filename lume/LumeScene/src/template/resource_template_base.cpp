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

#include <scene/ext/scene_utils.h>
#include <scene/interface/resource/intf_resource_context.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/property/intf_stack_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "template_copy_helpers.h"

SCENE_BEGIN_NAMESPACE()

// Mirrors the bit used by the importer plugin to mark resources whose values came from a template entry.
static constexpr uint64_t IMPORTED_FROM_TEMPLATE_BIT = 128;

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

// Exposed via template_copy_helpers.h
void CopyToDefaultMaybeDeep(META_NS::IMetadata& dst, const META_NS::IProperty::ConstPtr& srcProp)
{
    auto dstProp = dst.GetProperty(srcProp->GetName());
    if (dstProp && NeedsDeepCopy(dst, srcProp)) {
        CopyReadonlyProperty(srcProp, dstProp, CopyMode::WITH_DEFAULTS);
    } else {
        META_NS::CopyToDefaultAndReset(srcProp, dst);
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

void CopyMetadataExcept(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode,
    std::initializer_list<BASE_NS::string_view> skipNames)
{
    for (auto&& srcProp : src.GetProperties()) {
        const bool excluded = IsExcludedName(skipNames, srcProp->GetName());
        const bool isSubObject = !excluded && (IsArrayOfSubObjects(srcProp) || IsOwnedSubObject(srcProp));
        const bool shouldCopy = !excluded && (mode == CopyMode::WITH_DEFAULTS || srcProp->IsValueSet() || isSubObject);
        if (shouldCopy) {
            auto dstProp = dst.GetProperty(srcProp->GetName());
            if (dstProp) {
                CopyMetadataProperty(dst, srcProp, mode);
            } else if (mode == CopyMode::OVERRIDES_ONLY && srcProp->IsValueSet()) {
                META_NS::Clone(srcProp, dst);
            }
        }
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
    auto dst = interface_cast<META_NS::IMetadata>(&target);
    if (!dst) {
        return false;
    }
    CopyMetadata(*this, *dst, CopyMode::OVERRIDES_ONLY);
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
    if (auto baseTmpl = interface_pointer_cast<IResourceTemplate>(base)) {
        baseTmpl->ApplyTo(*resObj);
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
    auto applied = ApplyTo(*resObj);
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
