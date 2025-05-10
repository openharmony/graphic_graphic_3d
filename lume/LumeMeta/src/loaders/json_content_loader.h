/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2024. All rights reserved.
 * Description: JsonContentLoader class implementation
 * Author: Lauri Jaaskela
 * Create: 2022-05-30
 */

#ifndef META_SRC_LOADERS_JSON_CONTENT_LOADER_H
#define META_SRC_LOADERS_JSON_CONTENT_LOADER_H

#include <meta/api/property/property_event_handler.h>
#include <meta/base/namespace.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_file_content_loader.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

class JsonContentLoader final : public IntroduceInterfaces<ObjectFwd, IFileContentLoader> {
    META_OBJECT(JsonContentLoader, ClassId::JsonContentLoader, IntroduceInterfaces)
public:
    bool Build(const IMetadata::Ptr& data) override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IFileContentLoader, bool, Cached)
    META_STATIC_EVENT_DATA(IDynamicContentLoader, IOnChanged, ContentChanged)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(bool, Cached)
    META_IMPLEMENT_EVENT(IOnChanged, ContentChanged)
public: // IDynamicContentLoader
    void Reload() override;

public: // IFileContentLoader
    bool SetFile(CORE_NS::IFile::Ptr file) override;
    IObject::Ptr Create(const IObject::Ptr& params) override;

private:
    IObject::Ptr LoadCached();
    bool UpdateTree();

private:
    CORE_NS::IFile::Ptr file_;
    PropertyChangedEventHandler cachedChangedHandler_;
    ISerNode::Ptr tree_;
};

META_END_NAMESPACE()

#endif
