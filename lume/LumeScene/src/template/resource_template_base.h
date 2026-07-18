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

#ifndef SCENE_SRC_TEMPLATE_RESOURCE_TEMPLATE_BASE_H
#define SCENE_SRC_TEMPLATE_RESOURCE_TEMPLATE_BASE_H

#include <functional>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_resource_template.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/intf_owner.h>
#include <meta/interface/resource/intf_resource.h>

#include "template_copy_helpers.h"

SCENE_BEGIN_NAMESPACE()

// Clone-time accessor for the promoted_ flag.
class IPromotableTemplate : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPromotableTemplate, "c8e2d1a7-5b3f-4e9a-a1c6-7d8f0e2b4a3c")
public:
    virtual void SetPromoted(bool promoted) = 0;
};

class ResourceTemplateBase : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::INamed, ITemplateOptions,
                                 META_NS::IDerivedResourceOptions, META_NS::Resource, META_NS::IMetadataOwner,
                                 IPromotableTemplate, IResourceTemplate> {
public:
    BASE_NS::string GetName() const override;

    // IDerivedResourceOptions
    void SetBaseResource(CORE_NS::ResourceId id) override
    {
        baseResource_ = BASE_NS::move(id);
    }
    CORE_NS::ResourceId GetBaseResource() const override
    {
        return baseResource_;
    }

    // ITemplateOptions
    void SetTemplateContext(bool isTemplate) override;
    bool IsTemplateContext() const override
    {
        return templateContext_;
    }

    // IPromotableTemplate
    void SetPromoted(bool promoted) override
    {
        promoted_ = promoted;
    }

protected:
    // IMetadataOwner
    void OnMetadataConstructed(const META_NS::StaticMetadata&, CORE_NS::IInterface&) override
    {}

    // IResourceOptions
    bool ApplyOptions(CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) const override;
    bool UpdateOptions(const CORE_NS::IResource& res, const CORE_NS::ResourceContextPtr& context) override;
    bool Merge(const CORE_NS::IResourceOptions& options) override;
    CORE_NS::IResourceOptions::Ptr Clone() const override;

    // IResourceTemplate
    bool ApplyTo(META_NS::IObject& target) const override;
    bool CopyFrom(const META_NS::IObject& source, bool onlySetValues = true) override;

    // --- Pure virtuals for derived classes ---

    // The META_NS::ObjectId used by Clone() to create instances via the registry.
    virtual META_NS::ObjectId GetTemplateClassId() const = 0;

    // --- Virtual hooks for derived classes ---

    // Called by ApplyOptions to type-check the resource. Return false if the
    // resource is not the expected concrete type. Default returns true.
    virtual bool ValidateResourceType(const CORE_NS::IResource& res) const;

    // Called by CopyFrom before the generic metadata copy.
    // Return true if handled (type-specific fast path), false to fall through.
    virtual bool CopyFromTyped(const META_NS::IObject& source, bool onlySetValues);

    // Called by Clone after copying static+dynamic properties.
    // Override to deep-copy sub-objects (e.g. Textures in MaterialTemplate).
    virtual void CloneSubObjects(const META_NS::IMetadata& srcMeta, META_NS::IMetadata& cloneMeta) const;

    // Building blocks for ApplyOptions. ResourceTemplateBase's default
    // implementation orchestrates them; subclasses that need property-specific
    // semantics (e.g. MaterialTemplate's name-resolved Textures) can override
    // ApplyOptions / ApplyTo and call these helpers directly instead of
    // re-deriving the orchestration.

    // Resolve the configured baseResource_ via the context's resource manager,
    // returning nullptr when no base is configured or the resource cannot be
    // found.
    CORE_NS::IResource::Ptr ResolveBaseResource(const CORE_NS::ResourceContextPtr& context) const;

    // Apply a base resource's properties to `res` as defaults, then run
    // base->ApplyTo on the resource (so a base template's structural overrides
    // take effect), stamp the templateId, and sync read-only sub-objects.

    // `applyBaseDefaults` is called for the property-walk step; the default
    // (empty function) walks every base property and copies it to `res`'s
    // default slot. Subclasses pass a custom function to substitute their own
    // semantics for specific properties.
    using ApplyBaseDefaultsFn = std::function<void(META_NS::IObject& target, const META_NS::IMetadata& base)>;
    void ApplyBaseResource(CORE_NS::IResource& res, const CORE_NS::IResource::Ptr& base,
        const ApplyBaseDefaultsFn& applyBaseDefaults = {}) const;

    void StampTemplateId(CORE_NS::IResource& res) const;
    void MarkImportedFromTemplate(META_NS::IMetadata& resMeta) const;

    // Helper available to derived classes
    template <typename T>
    META_NS::Property<T> PropertyByName(BASE_NS::string_view name)
    {
        META_NS::IMetadata& meta = *this;
        return meta.GetProperty<T>(name, META_NS::MetadataQuery::EXISTING);
    }

    void PromoteToDefaults();

    CORE_NS::ResourceId baseResource_{};
    bool templateContext_{};
    bool promoted_{};
};

SCENE_END_NAMESPACE()

#endif
