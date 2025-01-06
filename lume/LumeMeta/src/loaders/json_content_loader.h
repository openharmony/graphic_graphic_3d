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
