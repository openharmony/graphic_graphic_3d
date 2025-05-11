/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: ClassContentLoader class implementation
 * Author: Lauri Jaaskela
 * Create: 2022-06-21
 */

#ifndef META_SRC_LOADERS_CLASS_CONTENT_LOADER_H
#define META_SRC_LOADERS_CLASS_CONTENT_LOADER_H

#include <meta/api/property/property_event_handler.h>
#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/implementation_macros.h>
#include <meta/ext/object_fwd.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_class_content_loader.h>

META_BEGIN_NAMESPACE()

class ClassContentLoader final : public IntroduceInterfaces<ObjectFwd, IClassContentLoader> {
    META_OBJECT(ClassContentLoader, ClassId::ClassContentLoader, IntroduceInterfaces)
public:
    bool Build(const IMetadata::Ptr& data) override;

    META_BEGIN_STATIC_DATA()
    META_STATIC_PROPERTY_DATA(IClassContentLoader, ObjectId, ClassId)
    META_STATIC_EVENT_DATA(IDynamicContentLoader, IOnChanged, ContentChanged)
    META_END_STATIC_DATA()
    META_IMPLEMENT_PROPERTY(ObjectId, ClassId)
    META_IMPLEMENT_EVENT(IOnChanged, ContentChanged)

public: // IDynamicContentLoader
    void Reload() override;

public: // IContentLoader
    IObject::Ptr Create(const IObject::Ptr& params) override;

private:
    PropertyChangedEventHandler classIdChangedHandler_;
};

META_END_NAMESPACE()

#endif
