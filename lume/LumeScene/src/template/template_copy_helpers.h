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

#ifndef SCENE_SRC_TEMPLATE_TEMPLATE_COPY_HELPERS_H
#define SCENE_SRC_TEMPLATE_TEMPLATE_COPY_HELPERS_H

#include <initializer_list>
#include <scene/base/namespace.h>

#include <core/log.h>
#include <core/resources/intf_resource.h>
#include <meta/api/util.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>

SCENE_BEGIN_NAMESPACE()

enum class CopyMode {
    OVERRIDES_ONLY,  // Propagate only properties with IsValueSet (the overrides). If dst lacks the
                     // property, clone it into dst so the override lands.
    WITH_DEFAULTS,   // Copy default slot to dst default; also copy set values if present.
    SET_AS_DEFAULTS  // Propagate only properties with IsValueSet, but land each set value on dst's
                     // *default* slot (resetting any override) instead of as an override. Recurses
                     // into sub-objects with the same semantics. Used by the standalone ApplyTo
                     // path so template-supplied values manifest as defaults on the live instance.
};

void CopyMetadata(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode);
void CopyMetadataExcept(const META_NS::IMetadata& src, META_NS::IMetadata& dst, CopyMode mode,
    std::initializer_list<BASE_NS::string_view> skipNames);
// Copy a single property from src onto dst's *default* slot, preserving any user override
// (the override is reset only when dst is not explicitly set). Sub-object / array-of-sub-object
// properties are deep-copied with CopyMode::WITH_DEFAULTS. Used by base-resource loading and the
// template apply-as-default paths.
void CopyToDefaultMaybeDeep(META_NS::IMetadata& dst, const META_NS::IProperty::ConstPtr& srcProp);

// Type-agnostic accessor for an element's IMetadata aspect from an IArrayAny.
// Works whether the array's value type is META_NS::IObject::Ptr (templates) or
// a more specific interface (e.g. ITexture::Ptr on a live IMaterial).
META_NS::IMetadata* GetArrayElementMetadata(const META_NS::IArrayAny& arr, size_t index);
void CopyArrayElements(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode);
void CopyReadonlyProperty(
    const META_NS::IProperty::ConstPtr& srcProp, const META_NS::IProperty::Ptr& dstProp, CopyMode mode);
void CopyProperty(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IProperty& dstProp, CopyMode mode);
void CopyPropertyIfSet(const META_NS::IProperty::ConstPtr& src, const META_NS::IProperty::Ptr& dst);
void ClonePropertyWithDefaults(const META_NS::IProperty::ConstPtr& src, META_NS::IMetadata& dst);

// Resource-id property convention.

// Resource-pointer properties (Image, Shader, Environment, PostProcess, ...) live as typed
// pointer properties (IImage::Ptr etc.) on the live interface (IMaterial, IEnvironment,
// IBloom, ...). On templates the same names carry a CORE_NS::ResourceId — the importer
// stores the unresolved id without ever creating a typed-pointer placeholder. At apply
// time, the apply path walks template properties and binds the live instance's typed
// pointer to the resource resolved from the apply-time IResourceManager.

// This means the copy primitives MUST skip ResourceId-typed source properties — copying a
// ResourceId value onto a typed pointer property of the same name would be a type
// mismatch. CopyOnePropertyExcept enforces the skip; ResolveResourceIdRefs (below) handles
// the binding.

// True if `prop`'s value type is CORE_NS::ResourceId. Used by the copy primitives to filter
// out ids from generic metadata copies and by the apply path to identify ref properties.
bool IsResourceIdProperty(const META_NS::IProperty::ConstPtr& prop);

// Walk every ResourceId property on `src`; for each, find the matching named property on
// `dst`, resolve the id against `context`'s resource manager, and assign the typed pointer
// using whichever known scene resource interface (IImage / IShader / IEnvironment /
// IPostProcess) the resolved resource implements. Recurses into owned sub-objects (e.g.
// PostProcess's Bloom). Silently no-ops when `context` is empty. When `asDefault` is true
// the resolved pointer lands on the destination's default slot (with the override reset),
// matching the template-context invariant established by the importer's property path.
void ResolveResourceIdRefs(
    const META_NS::IMetadata& src, META_NS::IMetadata& dst, const CORE_NS::ResourceContextPtr& context, bool asDefault);

// Resolve a single ResourceId against `context`'s resource manager. Returns null if no
// rman is reachable, the id is invalid, or the resource is not registered. Logs a warning
// when configured but unresolvable. Exposed because per-property apply paths (e.g.
// MaterialTemplate's texture slot copy) need to resolve a single id without walking.
CORE_NS::IResource::Ptr ResolveResourceId(
    const CORE_NS::ResourceId& id, BASE_NS::string_view propName, const CORE_NS::ResourceContextPtr& context);

// Set `resource` onto `dstProp` by trying each known scene resource interface in turn;
// the cast that succeeds picks the typed assignment. No-op if `resource` matches none of
// IImage / IShader / IEnvironment / IPostProcess. When `asDefault` is true the pointer is
// stored on dstProp's default slot and any override is reset, so `IsValueSet()` stays
// false — used by template-context apply paths where template-supplied values must
// manifest as defaults on the live instance.
void AssignTypedResource(
    const META_NS::IProperty::Ptr& dstProp, const CORE_NS::IResource::Ptr& resource, bool asDefault);

// Used to capture a resource-pointer property (e.g. IShader::Ptr on a live IMaterial) into
// a template: reads the bound resource's CORE_NS::IResource::GetResourceId() and writes it
// onto `dst` as a CORE_NS::ResourceId-typed property with the same name. Updates an existing
// ResourceId property of that name if present, otherwise constructs a new one. Returns true
// when a capture happened (srcProp is a resource-pointer with a valid id); false otherwise.
bool CaptureResourceRefAsId(const META_NS::IProperty::ConstPtr& srcProp, META_NS::IMetadata& dst);

// Derive a resource-apply context (the owning IScene) from a live scene object, so resource
// references can be resolved when only the target object is available (e.g.
// IResourceTemplate::ApplyTo, which carries no context parameter). Prefers the object's ECS
// scene; if the object is not yet wired to a scene, falls back to the context the resource
// itself was created with (CORE_NS::IResource::GetContext). Returns null if neither is
// available. The result is suitable for ResolveResourceIdRefs / ResolveResourceId.
CORE_NS::ResourceContextPtr GetApplyContextFromObject(const META_NS::IObject& target);

SCENE_END_NAMESPACE()

#endif
