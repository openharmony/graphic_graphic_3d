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

#ifndef SCENE_SRC_CONVERTING_VALUE_H
#define SCENE_SRC_CONVERTING_VALUE_H

#include <atomic>
#include <scene/ext/intf_converting_value.h>

#include <meta/api/make_callback.h>
#include <meta/interface/property/intf_stack_property.h>
#include <meta/interface/property/intf_stack_resetable.h>

SCENE_BEGIN_NAMESPACE()

template<typename PropertyType>
class ConvertingValueBase : public META_NS::IntroduceInterfaces<META_NS::INotifyOnChange, META_NS::IStackResetable,
                                META_NS::IValue, IConvertingValue> {
public:
    ConvertingValueBase(PropertyType p) : p_(p) {}

    META_NS::ResetResult ProcessOnReset(const META_NS::IAny&) override
    {
        return META_NS::RESET_CONTINUE;
    }

    BASE_NS::shared_ptr<META_NS::IEvent> EventOnChanged(META_NS::MetadataQuery) const override
    {
        return p_->OnChanged();
    }

    META_NS::IProperty::Ptr GetTargetProperty() override
    {
        return p_;
    }

protected:
    PropertyType p_;
};

template<typename Converter>
class ConvertingValue : public ConvertingValueBase<META_NS::Property<typename Converter::TargetType>> {
public:
    using Super = ConvertingValueBase<META_NS::Property<typename Converter::TargetType>>;

    using TargetType = typename Converter::TargetType;
    using SourceType = typename Converter::SourceType;
    using TargetProperty = META_NS::Property<TargetType>;

    explicit ConvertingValue(TargetProperty p, Converter convert = {})
        : Super(p), any_(META_NS::ConstructAny<SourceType>()), convert_(BASE_NS::move(convert))
    {}

    META_NS::AnyReturnValue SetValue(const META_NS::IAny& any) override
    {
        SourceType value {};
        auto res = any.GetValue<SourceType>(value);
        if (res) {
            res = this->p_->SetValue(convert_.ConvertToTarget(value));
            if (res) {
                any_->SetValue(value);
            }
        }
        return res;
    }
    const META_NS::IAny& GetValue() const override
    {
        any_->SetValue(convert_.ConvertToSource(*any_, this->p_->GetValue()));
        return *any_;
    }
    bool IsCompatible(const META_NS::TypeId& id) const override
    {
        return id == META_NS::UidFromType<SourceType>() || id == META_NS::UidFromType<TargetType>();
    }

private:
    META_NS::IAny::Ptr any_;
    Converter convert_;
};

template<typename Converter>
class ConvertingArrayValue : public ConvertingValueBase<META_NS::ArrayProperty<typename Converter::TargetType>> {
public:
    using Super = ConvertingValueBase<META_NS::ArrayProperty<typename Converter::TargetType>>;

    using TargetType = typename Converter::TargetType;
    using SourceType = typename Converter::SourceType;
    using TargetProperty = META_NS::ArrayProperty<TargetType>;

    explicit ConvertingArrayValue(TargetProperty p, Converter convert = {})
        : Super(p), any_(META_NS::ConstructArrayAny<SourceType>()), convert_(BASE_NS::move(convert))
    {}

    META_NS::AnyReturnValue SetValue(const META_NS::IAny& any) override
    {
        BASE_NS::vector<SourceType> value {};
        auto res = any.GetValue<BASE_NS::vector<SourceType>>(value);
        if (res) {
            BASE_NS::vector<TargetType> converted;
            for (auto&& v : value) {
                converted.push_back(convert_.ConvertToTarget(v));
            }
            res = this->p_->SetValue(converted);
            if (res) {
                any_->SetValue(converted);
            }
        }
        return res;
    }
    const META_NS::IAny& GetValue() const override
    {
        BASE_NS::vector<SourceType> result;
        for (auto&& v : this->p_->GetValue()) {
            result.push_back(convert_.ConvertToSource(*any_, v));
        }
        any_->SetValue(result);
        return *any_;
    }
    bool IsCompatible(const META_NS::TypeId& id) const override
    {
        return id == META_NS::ArrayUidFromType<SourceType>() || id == META_NS::ArrayUidFromType<TargetType>();
    }

private:
    META_NS::IAny::Ptr any_;
    Converter convert_;
};

SCENE_END_NAMESPACE()

#endif
