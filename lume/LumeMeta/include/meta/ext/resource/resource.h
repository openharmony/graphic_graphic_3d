
#ifndef META_EXT_RESOURCE_RESOURCE_H
#define META_EXT_RESOURCE_RESOURCE_H

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/serialization/serializer.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/resource/intf_resource_consumer.h>
#include <meta/interface/serialization/intf_serializable.h>

META_BEGIN_NAMESPACE()

class Resource : public IntroduceInterfaces<CORE_NS::IResource, ISerializable, CORE_NS::ISetResourceId> {
public:
    bool SerialiseAsResourceId(IExportContext& c) const
    {
        auto ri = interface_cast<IResourceConsumer>(c.Context());
        return ri && ri->GetResourceManager();
    }
    ReturnError ExportResourceId(IExportContext& c) const
    {
        ReturnError res = GenericError::FAIL;
        if (resourceId_.IsValid()) {
            if (auto ph = GetObjectRegistry().Create<CORE_NS::ISetResourceId>(ClassId::ResourcePlaceholder)) {
                ph->SetResourceId(resourceId_);
                ISerNode::Ptr node;
                res = c.ExportValueToNode(interface_pointer_cast<IObject>(ph), node);
                if (res) {
                    return c.SubstituteThis(node);
                }
            }
        } else {
            CORE_LOG_E("Empty resource id when serialising resource");
        }
        return res;
    }
    ReturnError Export(IExportContext& c) const override
    {
        if (SerialiseAsResourceId(c)) {
            return ExportResourceId(c);
        }
        return Serializer(c) & AutoSerialize();
    }
    ReturnError Import(IImportContext& c) override
    {
        return Serializer(c) & AutoSerialize();
    }
    CORE_NS::ResourceId GetResourceId() const override
    {
        return resourceId_;
    }

    void SetResourceId(CORE_NS::ResourceId id) override
    {
        resourceId_ = BASE_NS::move(id);
    }

private:
    CORE_NS::ResourceId resourceId_;
};

META_END_NAMESPACE()

#endif
