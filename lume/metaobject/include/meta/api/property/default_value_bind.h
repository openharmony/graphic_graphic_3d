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

#ifndef META_API_PROPERTY_DEFAULT_VALUE_BIND_H
#define META_API_PROPERTY_DEFAULT_VALUE_BIND_H

#include <meta/api/make_callback.h>
#include <meta/base/interface_macros.h>
#include <meta/interface/property/property.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

/**
 * @brief A helper class for binding the default value of a property to the value of a source property
 */
class DefaultValueBind final {
public:
    DefaultValueBind() = default;
    explicit DefaultValueBind(const IProperty::Ptr& property) : property_(property) {}
    ~DefaultValueBind()
    {
        UnBind();
        property_.reset();
    }
    DefaultValueBind(const DefaultValueBind& other) : property_(other.property_)
    {
        Bind(other.source_.lock());
    }
    DefaultValueBind(DefaultValueBind&& other) : property_(other.property_)
    {
        Bind(other.source_.lock());
        other.UnBind();
        other.property_ = nullptr;
    }
    DefaultValueBind& operator=(const DefaultValueBind& other)
    {
        UnBind();
        property_ = other.property_;
        Bind(other.source_.lock());
        return *this;
    }
    DefaultValueBind& operator=(DefaultValueBind&& other)
    {
        UnBind();
        property_ = other.property_;
        Bind(other.source_.lock());
        other.UnBind();
        other.property_ = nullptr;
        return *this;
    }

    bool Bind(const IProperty::ConstPtr& source)
    {
        if (source == source_.lock()) {
            return true; // No change
        }
        UnBind();
        const auto property = property_.lock();
        if (property && source) {
            source_ = source;
            token_ = source->OnChanged()->AddHandler(MakeCallback<IOnChanged>([this]() {
                const auto property = property_.lock();
                const auto source = source_.lock();
                if (property && source) {
                    if (auto s = interface_cast<IStackProperty>(property)) {
                        s->SetDefaultValue(source->GetValue());
                    }
                }
            }));
            if (auto s = interface_cast<IStackProperty>(property)) {
                s->SetDefaultValue(source->GetValue());
            }
        }
        return token_ != 0;
    }
    void UnBind()
    {
        if (token_) {
            if (const auto source = source_.lock()) {
                source->OnChanged()->RemoveHandler(token_);
            }
            token_ = {};
            source_.reset();
        }
    }
    void SetProperty(const IProperty::Ptr& property)
    {
        UnBind();
        property_ = property;
    }
    IProperty::Ptr GetProperty() const
    {
        return property_.lock();
    }
    bool IsDefaultValue() const
    {
        if (token_) {
            if (auto p = GetProperty()) {
                PropertyLock s { p };
                return s.IsDefaultValue();
            }
        }
        return false;
    }

private:
    IProperty::WeakPtr property_;
    IProperty::ConstWeakPtr source_;
    IEvent::Token token_ {};
};

META_END_NAMESPACE()

#endif
