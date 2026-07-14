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

#include "material_template.h"

#include <scene/interface/intf_shader.h>
#include <scene/interface/intf_texture.h>

#include <core/log.h>
#include <core/resources/intf_resource.h>

#include <meta/api/metadata_util.h>
#include <meta/api/util.h>
#include <meta/interface/intf_attach.h>
#include <meta/interface/intf_attachment_container.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/property/construct_array_property.h>
#include <meta/interface/property/construct_property.h>

#include "../component/material_component.h"
#include "template_copy_helpers.h"

SCENE_BEGIN_NAMESPACE()

// ---------------------------------------------------------------------------
// MaterialTemplate implementation
// ---------------------------------------------------------------------------

static META_NS::IObject::Ptr MakeEmptyEntry()
{
    return META_NS::GetObjectRegistry().Create<META_NS::IObject>(META_NS::ClassId::Object);
}

void MaterialTemplate::InitTextures()
{
    META_NS::IMetadata& meta = *this;
    if (meta.GetProperty("Textures")) {
        return;
    }
    BASE_NS::vector<META_NS::IObject::Ptr> empty;
    auto prop = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Textures", empty);
    meta.AddProperty(prop.GetProperty());
}

bool MaterialTemplate::ValidateResourceType(const CORE_NS::IResource& res) const
{
    return interface_cast<IMaterial>(&res) != nullptr;
}

bool MaterialTemplate::CopyFromTyped(const META_NS::IObject& source, bool onlySetValues)
{
    if (auto material = interface_cast<IMaterial>(&source)) {
        return CopyFromMaterial(*material, onlySetValues);
    }
    return false;
}

static void CopyOrClone(const META_NS::IProperty::ConstPtr& src, META_NS::IMetadata& dst, CopyMode mode)
{
    if (!src) {
        return;
    }
    if (mode == CopyMode::OVERRIDES_ONLY && !src->IsValueSet()) {
        return;
    }
    if (auto dstProp = dst.GetProperty(src->GetName())) {
        CopyProperty(src, *dstProp, mode);
    } else {
        ClonePropertyWithDefaults(src, dst);
    }
}

// Capture a typed resource-pointer source property (e.g. IShader::Ptr on a live IMaterial)
// as a ResourceId on the template's metadata. Templates carry resource refs as ids — the
// apply path resolves them against the live scene's rman.
static void CaptureRefAsId(const META_NS::IProperty::ConstPtr& src, META_NS::IMetadata& dst)
{
    if (!src) {
        return;
    }
    auto resource = META_NS::GetPointer<CORE_NS::IResource>(src);
    if (!resource) {
        return;
    }
    auto id = resource->GetResourceId();
    if (!id.IsValid()) {
        return;
    }
    if (auto existing = dst.GetProperty<CORE_NS::ResourceId>(src->GetName(), META_NS::MetadataQuery::EXISTING)) {
        META_NS::SetValue(existing, id);
        return;
    }
    auto idProp = META_NS::ConstructProperty<CORE_NS::ResourceId>(src->GetName());
    META_NS::SetValue<CORE_NS::ResourceId>(idProp.GetProperty(), id);
    dst.AddProperty(idProp.GetProperty());
}

// CaptureLiveTextureEntry's per-property fallthrough: when the source property holds a
// resource pointer (e.g. ITexture::Image as IImage::Ptr), record its id on the template
// entry instead of cloning the typed property; otherwise plain CopyOrClone.
static void CopyOrCaptureEntryField(const META_NS::IProperty::ConstPtr& src, META_NS::IMetadata& dst, CopyMode mode)
{
    if (src && META_NS::GetPointer<CORE_NS::IResource>(src)) {
        CaptureRefAsId(src, dst);
    } else {
        CopyOrClone(src, dst, mode);
    }
}

// Build a sparse template entry mirroring a live texture: only properties that
// are actually set on the live side are propagated. Returns an empty Ptr if
// the live entry has no name (an inactive slot we shouldn't capture).
static META_NS::IObject::Ptr CaptureLiveTextureEntry(const META_NS::IMetadata& srcMeta, CopyMode mode)
{
    auto nameProp = srcMeta.GetProperty<BASE_NS::string>("Name");
    if (!nameProp) {
        return {};
    }
    BASE_NS::string name = META_NS::GetValue(nameProp);
    if (name.empty()) {
        return {};
    }
    auto entry = MakeEmptyEntry();
    auto entryMeta = interface_cast<META_NS::IMetadata>(entry);
    if (!entryMeta) {
        return {};
    }
    auto entryName = META_NS::ConstructProperty<BASE_NS::string>("Name", name);
    entryMeta->AddProperty(entryName.GetProperty());
    for (auto&& prop : srcMeta.GetProperties()) {
        if (prop && prop->GetName() != "Name") {
            CopyOrCaptureEntryField(prop, *entryMeta, mode);
        }
    }
    return entry;
}

static void AppendCapturedTextures(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode)
{
    META_NS::ArrayProperty<const ITexture::Ptr> src{srcProp};
    META_NS::ArrayProperty<META_NS::IObject::Ptr> dst{dstProp};
    if (!src || !dst) {
        return;
    }
    for (size_t i = 0; i < src->GetSize(); ++i) {
        auto srcEntryMeta = interface_cast<META_NS::IMetadata>(src->GetValueAt(i));
        if (!srcEntryMeta) {
            // skip non-meta entry
        } else if (auto entry = CaptureLiveTextureEntry(*srcEntryMeta, mode)) {
            dst->AddValue(entry);
        }
    }
}

bool MaterialTemplate::CopyFromMaterial(const IMaterial& material, bool onlySetValues)
{
    auto srcMeta = interface_cast<META_NS::IMetadata>(&material);
    if (!srcMeta) {
        return false;
    }
    META_NS::IMetadata& dstMeta = *this;
    const CopyMode mode = onlySetValues ? CopyMode::OVERRIDES_ONLY : CopyMode::WITH_DEFAULTS;

    CopyOrClone(material.Type(), dstMeta, mode);
    CopyOrClone(material.AlphaCutoff(), dstMeta, mode);
    CopyOrClone(material.LightingFlags(), dstMeta, mode);
    CaptureRefAsId(material.MaterialShader(), dstMeta);
    CaptureRefAsId(material.DepthShader(), dstMeta);
    CopyOrClone(material.RenderSort(), dstMeta, mode);

    InitTextures();
    AppendCapturedTextures(srcMeta->GetProperty("Textures", META_NS::MetadataQuery::EXISTING),
        dstMeta.GetProperty("Textures", META_NS::MetadataQuery::EXISTING),
        mode);
    return true;
}

// Deep-copy a sub-object's metadata (including all its present properties).
static META_NS::IObject::Ptr CloneEntryDeep(const META_NS::IObject::Ptr& src)
{
    if (!src) {
        return {};
    }
    auto srcMeta = interface_cast<META_NS::IMetadata>(src);
    if (!srcMeta) {
        return src;
    }
    auto clone = MakeEmptyEntry();
    auto cloneMeta = interface_cast<META_NS::IMetadata>(clone);
    if (cloneMeta) {
        CopyMetadata(*srcMeta, *cloneMeta, CopyMode::WITH_DEFAULTS);
    }
    return clone;
}

static void CloneTexturesSubObject(const META_NS::IMetadata& srcMeta, META_NS::IMetadata& cloneMeta)
{
    auto srcArr = srcMeta.GetArrayProperty<META_NS::IObject::Ptr>("Textures", META_NS::MetadataQuery::EXISTING);
    if (!srcArr) {
        return;
    }
    BASE_NS::vector<META_NS::IObject::Ptr> entries;
    entries.reserve(srcArr->GetSize());
    for (size_t i = 0; i < srcArr->GetSize(); ++i) {
        if (auto entry = CloneEntryDeep(srcArr->GetValueAt(i))) {
            entries.push_back(entry);
        }
    }
    auto prop = META_NS::ConstructArrayProperty<META_NS::IObject::Ptr>("Textures", entries);
    cloneMeta.AddProperty(prop.GetProperty());
}

static void CloneOptionsSubObject(const META_NS::IMetadata& srcMeta, META_NS::IMetadata& cloneMeta)
{
    auto srcOpt = srcMeta.GetProperty("Options", META_NS::MetadataQuery::EXISTING);
    if (!srcOpt) {
        return;
    }
    if (auto clone = CloneEntryDeep(META_NS::GetPointer<META_NS::IObject>(srcOpt))) {
        cloneMeta.AddProperty(META_NS::ConstructProperty<META_NS::IObject::Ptr>("Options", clone).GetProperty());
    }
}

void MaterialTemplate::CloneSubObjects(const META_NS::IMetadata& srcMeta, META_NS::IMetadata& cloneMeta) const
{
    CloneTexturesSubObject(srcMeta, cloneMeta);
    CloneOptionsSubObject(srcMeta, cloneMeta);
}

// ---------------------------------------------------------------------------
// Apply path: name-resolved Textures
// ---------------------------------------------------------------------------

// Walks attachments to locate the IInternalMaterial component bound to a live
// IMaterial. Mirrors how Material::SetEcsObject discovers the component.
static IInternalMaterial::Ptr FindInternalMaterial(CORE_NS::IResource& res)
{
    auto attach = interface_cast<META_NS::IAttach>(&res);
    if (!attach) {
        return {};
    }
    auto container = attach->GetAttachmentContainer(false);
    if (!container) {
        return {};
    }
    return container->FindByName<IInternalMaterial>("MaterialComponent");
}

// Find the slot index in tsi whose declared name matches `name`, or -1.
static int FindSlotByName(const IInternalMaterial::ActiveTextureSlotInfo& tsi, BASE_NS::string_view name)
{
    for (size_t i = 0; i < tsi.slots.size(); ++i) {
        if (tsi.slots[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// Bundles the recurring copy-semantics parameters threaded through the texture/metadata
// apply helpers, replacing the former (mode, asBaseDefaults, asDefault, context) tuple.
struct CopyParams {
public:
    CopyMode mode{CopyMode::OVERRIDES_ONLY};
    bool asBaseDefaults{};
    bool asDefault{};
    CORE_NS::ResourceContextPtr context;
};

// Copy only properties that the source actually carries onto the destination
// slot's metadata. Never touches destination properties the source didn't
// provide.

// Two semantics, selected by `asBaseDefaults`:
//  - false (own-template apply): src's set-state is preserved on dst per
//    `mode` — CopyProperty's standard set-vs-default rules.
//  - true (base-resource apply): src's *effective* values land on dst's
//    *default* slot (resetting any prior override on dst) — mirrors
//    CopyToDefaultMaybeDeep, the primitive used by the non-texture base path.

static void CopyEntryFieldsByName(const META_NS::IMetadata& srcEntry, META_NS::IMetadata& dstSlot, const CopyParams& p);

// Case (a): resolve a ResourceId-typed srcProp against the apply context's resource
// manager and assign the resolved pointer to the live slot's typed property.
static void CopyResourceIdField(
    const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dstSlot, const CopyParams& p)
{
    if (!p.context) {
        return;
    }
    const auto name = srcProp->GetName();
    auto dstProp = dstSlot.GetProperty(name);
    if (!dstProp) {
        return;
    }
    auto typedSrc = META_NS::Property<const CORE_NS::ResourceId>(srcProp);
    if (!typedSrc) {
        return;
    }
    if (auto resource = ResolveResourceId(META_NS::GetValue(typedSrc), name, p.context)) {
        AssignTypedResource(dstProp, resource, p.asDefault);
    }
}

// Cases (b)-(d): resource-typed values (swap wholesale), owned sub-objects (recurse),
// and plain scalars (CopyToDefaultMaybeDeep or CopyProperty per mode).
static void CopyPlainOrSubField(
    const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dstSlot, const CopyParams& p)
{
    const auto name = srcProp->GetName();
    const bool isName = name == "Name";
    auto dstProp = isName ? nullptr : dstSlot.GetProperty(name);
    auto srcSub = META_NS::GetPointer<META_NS::IMetadata>(srcProp);
    auto dstSub = META_NS::GetPointer<META_NS::IMetadata>(dstProp);
    const bool srcIsResource = interface_pointer_cast<CORE_NS::IResource>(srcSub) != nullptr;
    if (srcIsResource && dstProp) {
        META_NS::CopyValue(srcProp, *dstProp);
    } else if (srcSub && dstSub) {
        CopyEntryFieldsByName(*srcSub, *dstSub, p);
    } else if (dstProp) {
        if (p.asBaseDefaults) {
            CopyToDefaultMaybeDeep(dstSlot, srcProp);
        } else {
            CopyProperty(srcProp, *dstProp, p.mode);
        }
    }
}

// Copy one srcProp from a texture entry onto its dstSlot counterpart. Handles, in order:
// (a) ResourceId-typed values (e.g. template's `Image: ResourceId`) — resolve against the
//     apply context's resource manager and assign to the live slot's typed pointer. When
//     `asDefault` is true the resolved pointer lands on the destination's default slot so
//     the template-context invariant (template-supplied values are defaults on the live
//     instance) extends to resolved refs.
// (b) Resource-typed values (IImage::Ptr etc., e.g. when src is a live ITexture being
//     captured) — swap the destination's pointer wholesale. Cannot use CopyProperty here
//     because the source's IsValueSet flag is unreliable for values installed
//     post-construction.
// (c) Owned sub-objects (e.g. Sampler) — recurse with the same semantics.
// (d) Plain scalars — CopyToDefaultMaybeDeep when `asBaseDefaults` or `asDefault` is set
//     (template-supplied values manifest on the slot's default), otherwise CopyProperty
//     with the requested copy mode.
static void CopyOneEntryField(
    const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dstSlot, const CopyParams& p)
{
    if (IsResourceIdProperty(srcProp)) {
        CopyResourceIdField(srcProp, dstSlot, p);
        return;
    }
    CopyPlainOrSubField(srcProp, dstSlot, p);
}

static void CopyEntryFieldsByName(const META_NS::IMetadata& srcEntry, META_NS::IMetadata& dstSlot, const CopyParams& p)
{
    for (auto&& srcProp : srcEntry.GetProperties()) {
        CopyOneEntryField(srcProp, dstSlot, p);
    }
}

// Resolve the destination slot index for a source texture entry.
// Two source shapes are supported.

//  - A *live* IMaterial source: its Textures array is fixed-size, indexed by
//    MaterialComponent::TextureIndex. Each entry's Name follows the
//    UpdateTextureNames convention (e.g. uppercase enum names for default-PBR
//    slots) which does NOT match the shader-declared slot names verbatim, so
//    name lookup is unreliable. We use the array index directly.

//  - A sparse MaterialTemplate source: each entry's Name is the JSON key,
//    matching the bound shader's declared slot names. We look up via tsi.
static int ResolveSlotIndex(size_t srcIndex, bool srcIsPositional, const META_NS::IMetadata& srcEntry,
    const IInternalMaterial::ActiveTextureSlotInfo& tsi, BASE_NS::string& outName)
{
    if (srcIsPositional) {
        return static_cast<int>(srcIndex);
    }
    auto nameProp = srcEntry.GetProperty<BASE_NS::string>("Name");
    outName = nameProp ? META_NS::GetValue(nameProp) : BASE_NS::string{};
    return outName.empty() ? -1 : FindSlotByName(tsi, outName);
}

// Apply one source entry to the destination slot at `slotIx` (no-op if the
// index is out of range or the destination doesn't carry metadata).
static void ApplyEntryToSlot(const META_NS::IMetadata& srcEntry,
    const META_NS::ArrayProperty<const ITexture::Ptr>& dstArr, int slotIx, const CopyParams& p)
{
    if (slotIx < 0 || slotIx >= static_cast<int>(dstArr->GetSize())) {
        return;
    }
    auto dstSlotMeta = interface_cast<META_NS::IMetadata>(dstArr->GetValueAt(slotIx));
    if (dstSlotMeta) {
        CopyEntryFieldsByName(srcEntry, *dstSlotMeta, p);
    }
}

// Resolve the destination slot index for source entry `i`, warning when the entry's
// name maps to no shader-declared slot (returns -1, which ApplyEntryToSlot no-ops on).
static int ResolveEntrySlot(size_t i, bool srcIsPositional, const META_NS::IMetadata& srcEntry,
    const IInternalMaterial::ActiveTextureSlotInfo& tsi)
{
    BASE_NS::string lookupName;
    const int slotIx = ResolveSlotIndex(i, srcIsPositional, srcEntry, tsi, lookupName);
    if (slotIx < 0) {
        CORE_LOG_W(
            "Material texture slot '%s' not declared by shader", lookupName.empty() ? "<unnamed>" : lookupName.c_str());
    }
    return slotIx;
}

// Apply a source Textures array onto a live IMaterial's fixed Textures slots.
// Routing depends on the source kind (see ResolveSlotIndex). Slots not selected
// by any source entry are left untouched. `asBaseDefaults` selects the per-
// property copy semantics — see CopyEntryFieldsByName.

// Iteration is type-agnostic via IArrayAny so the source can be either a
// MaterialTemplate (`IObject::Ptr` array) or a live IMaterial (`ITexture::Ptr`
// array) without separate code paths.
static void ApplyTextureData(const META_NS::IMetadata& src, IMaterial& material,
    const IInternalMaterial::ActiveTextureSlotInfo& tsi, const CopyParams& p)
{
    auto srcProp = src.GetProperty("Textures", META_NS::MetadataQuery::EXISTING);
    if (!srcProp) {
        return;
    }
    auto srcArr = interface_cast<META_NS::IArrayAny>(&srcProp->GetValue());
    if (!srcArr) {
        return;
    }
    auto dstArr = material.Textures();
    if (!dstArr) {
        return;
    }
    const bool srcIsPositional = interface_cast<const IMaterial>(&src) != nullptr;
    const size_t count = srcArr->GetSize();
    for (size_t i = 0; i < count; ++i) {
        auto srcEntryMeta = GetArrayElementMetadata(*srcArr, i);
        if (srcEntryMeta) {
            const int slotIx = ResolveEntrySlot(i, srcIsPositional, *srcEntryMeta, tsi);
            ApplyEntryToSlot(*srcEntryMeta, dstArr, slotIx, p);
        }
    }
}

// Walk all metadatas (including forwarded) on `src` and copy to `dst`'s default
// slots, skipping the "Textures" property which the caller handles separately
// via name-routed apply.
static void CopyBaseDefaultsExceptTextures(const META_NS::IMetadata& src, META_NS::IMetadata& dst)
{
    for (auto&& md : src.GetAllMetadatas(META_NS::MetadataType::PROPERTY)) {
        if (md.name == "Textures" || md.name == "Options") {
            // skip — applied separately (Textures by name, Options onto the shader)
        } else if (auto p = src.GetProperty(md.name)) {
            CopyToDefaultMaybeDeep(dst, p);
        }
    }
}

// Push the sparse "Options" graphics-state sub-object (if any) onto the
// material's bound shader. Only fields present in the sub-object are applied.
// The shader is shared, so this is interim behaviour until per-material
// graphics state lands.
static void ApplyMaterialOptions(const META_NS::IMetadata& src, IMaterial& material)
{
    auto optMeta =
        META_NS::GetPointer<META_NS::IMetadata>(src.GetProperty("Options", META_NS::MetadataQuery::EXISTING));
    if (!optMeta) {
        return;
    }
    auto shader = META_NS::GetValue(material.MaterialShader());
    if (!shader) {
        return;
    }
    // Blend first: its handler rebuilds the graphics state from scratch, so
    // cull/polygon must be applied on top of the final blend state to survive.
    if (auto p = optMeta->GetProperty<bool>("Blend")) {
        META_NS::SetValue(shader->Blend(), META_NS::GetValue(p));
    }
    if (auto p = optMeta->GetProperty<PolygonMode>("PolygonMode")) {
        META_NS::SetValue(shader->PolygonMode(), META_NS::GetValue(p));
    }
    if (auto p = optMeta->GetProperty<CullModeFlags>("CullMode")) {
        META_NS::SetValue(shader->CullMode(), META_NS::GetValue(p));
    }
}

// Copy the generic (non-material-specific) metadata from src onto dst, then bind any
// ResourceId-typed refs (MaterialShader, DepthShader, ...) onto dst's typed pointers.
// The copy passes skip Textures/Options (applied separately); ResolveResourceIdRefs
// resolves refs against the live scene's resource manager. `p.asDefault` routes the bind
// to the destination's default slot so template-context apply preserves the
// "template values are defaults" invariant for resource refs too.
static void CopyGenericMetadata(const META_NS::IMetadata& src, META_NS::IMetadata& dstMeta, const CopyParams& p)
{
    if (p.asBaseDefaults) {
        CopyBaseDefaultsExceptTextures(src, dstMeta);
    } else {
        CopyMetadataExcept(src, dstMeta, p.mode, {"Textures", "Options"});
    }
    ResolveResourceIdRefs(src, dstMeta, p.context, p.asDefault);
}

// Apply the material-only portion of a source onto a live target: graphics-state
// Options onto the bound shader, then name-routed textures against the shader's slots.
static void ApplyMaterialSpecific(const META_NS::IMetadata& src, META_NS::IObject& target, const CopyParams& p)
{
    auto material = interface_cast<IMaterial>(&target);
    if (!material) {
        return;
    }
    ApplyMaterialOptions(src, *material);
    auto resource = interface_cast<CORE_NS::IResource>(&target);
    if (!resource) {
        return;
    }
    auto internal = FindInternalMaterial(*resource);
    if (!internal) {
        return;
    }
    auto tsi = internal->GetActiveTextureSlotInfo();
    ApplyTextureData(src, *material, tsi, p);
}

// Apply this template's content (or any other source's content) to a live
// IMaterial: generic per-property copy with `Textures` peeled out, plus a
// name-resolved texture apply against the bound shader's slot info.

// Used by both the OVERRIDES pass (ApplyTo) and the WITH_DEFAULTS passes
// (ApplyOptions, with the base resource's metadata or this template's own).
static void ApplyMaterialMetadata(const META_NS::IMetadata& src, META_NS::IObject& target, const CopyParams& p)
{
    auto dstMeta = interface_cast<META_NS::IMetadata>(&target);
    if (!dstMeta) {
        return;
    }
    CopyGenericMetadata(src, *dstMeta, p);
    ApplyMaterialSpecific(src, target, p);
}

bool MaterialTemplate::ApplyToTarget(META_NS::IObject& target, bool asDefault) const
{
    // `asDefault` selects the apply semantics end-to-end. Standalone (true): top-level scalars
    // (SET_AS_DEFAULTS), texture-slot scalars and resource refs all land as defaults on the live
    // material. ApplyOptions / ApplyBaseResource flow (false): OVERRIDES_ONLY scalars and override
    // ref/texture binds — the historical behaviour (an earlier standalone-only fix accidentally
    // forced asDefault=true here, which leaked defaults into the base-resource apply path).
    const CopyParams params{asDefault ? CopyMode::SET_AS_DEFAULTS : CopyMode::OVERRIDES_ONLY,
        /*asBaseDefaults=*/false,
        /*asDefault=*/asDefault,
        GetApplyContextFromObject(target)};
    ApplyMaterialMetadata(*this, target, params);
    return true;
}

bool MaterialTemplate::ApplyOptions(CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) const
{
    if (!ValidateResourceType(res)) {
        return false;
    }
    auto resObj = interface_cast<META_NS::IObject>(&res);
    if (!resObj) {
        return false;
    }
    if (baseResource_.IsValid()) {
        ApplyBaseResource(
            res, ResolveBaseResource(context), [&context](META_NS::IObject& target, const META_NS::IMetadata& base) {
                ApplyMaterialMetadata(
                    base, target, {CopyMode::WITH_DEFAULTS, /*asBaseDefaults=*/true, /*asDefault=*/true, context});
            });
    }
    StampTemplateId(res);
    auto resMeta = interface_cast<META_NS::IMetadata>(&res);
    if (templateContext_ && !baseResource_.IsValid() && resMeta) {
        ApplyMaterialMetadata(
            *this, *resObj, {CopyMode::WITH_DEFAULTS, /*asBaseDefaults=*/false, /*asDefault=*/true, context});
    }
    // ApplyTo is called via the IObjectTemplate interface with no context, so we invoke the
    // internal apply directly to thread the apply-time resource manager through.
    ApplyMaterialMetadata(
        *this, *resObj, {CopyMode::OVERRIDES_ONLY, /*asBaseDefaults=*/false, /*asDefault=*/templateContext_, context});
    if (templateContext_ && resMeta) {
        if (baseResource_.IsValid()) {
            ApplyMaterialMetadata(
                *this, *resObj, {CopyMode::WITH_DEFAULTS, /*asBaseDefaults=*/false, /*asDefault=*/true, context});
        }
        MarkImportedFromTemplate(*resMeta);
    }
    return true;
}

SCENE_END_NAMESPACE()
