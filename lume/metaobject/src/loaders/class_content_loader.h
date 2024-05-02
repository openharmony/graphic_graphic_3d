/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef META_SRC_LOADERS_CLASS_CONTENT_LOADER_H
#define META_SRC_LOADERS_CLASS_CONTENT_LOADER_H

#include <meta/api/property/property_event_handler.h>
#include <meta/base/meta_types.h>
#include <meta/ext/event_impl.h>
#include <meta/ext/object.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/loaders/intf_class_content_loader.h>

META_BEGIN_NAMESPACE()

class ClassContentLoader final
    : public ObjectFwd<ClassContentLoader, ClassId::ClassContentLoader, ClassId::Object, IClassContentLoader> {
    META_IMPLEMENT_INTERFACE_PROPERTY(IClassContentLoader, ObjectId, ClassId)
public:
    bool Build(const IMetadata::Ptr& data) override;

public: // IDynamicContentLoader
    META_IMPLEMENT_INTERFACE_EVENT(IDynamicContentLoader, IOnChanged, ContentChanged)
    void Reload() override;

public: // IContentLoader
    IObject::Ptr Create(const IObject::Ptr& params) override;

private:
    PropertyChangedEventHandler classIdChangedHandler_;
};

META_END_NAMESPACE()

#endif
