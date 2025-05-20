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

#include <meta/api/event_handler.h>
#include <meta/api/make_callback.h>
#include <meta/ext/minimal_object.h>
#include <meta/interface/property/intf_stack_property.h>
#include <meta/interface/property/intf_stack_resetable.h>
#include <meta/interface/resource/intf_dynamic_resource.h>
#include <meta/interface/serialization/intf_serializable.h>

SCENE_BEGIN_NAMESPACE()

META_REGISTER_CLASS(
    ConvertingValueBase, "41474a42-d7d1-4d4a-a6cd-5a53973e8f9e", META_NS::ObjectCategoryBits::NO_CATEGORY)

template<typename PropertyType>
class ConvertingValueBase : public META_NS::IntroduceInterfaces<META_NS::MinimalObject, META_NS::INotifyOnChange,
                                META_NS::IStackResetable, META_NS::IValue, IConvertingValue, META_NS::ISerializable> {
    META_IMPLEMENT_OBJECT_TYPE_INTERFACE(ClassId::ConvertingValueBase)
public:
    ConvertingValueBase(META_NS::IAny::Ptr any, PropertyType p) : any_(BASE_NS::move(any)), p_(p) {}

    META_NS::ResetResult ProcessOnReset(const META_NS::IAny& value) override
    {
        SetValue(value);
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

    META_NS::ReturnError Export(META_NS::IExportContext& c) const override
    {
        if (auto node = c.ExportValueToNode(any_)) {
            return c.SubstituteThis(node);
        }
        return META_NS::GenericError::FAIL;
    }
    META_NS::ReturnError Import(META_NS::IImportContext&) override
    {
        return META_NS::GenericError::FAIL;
    }

protected:
    META_NS::IAny::Ptr any_;
    PropertyType p_;
};

template<typename Converter>
class ConvertingValue : public ConvertingValueBase<META_NS::Property<typename Converter::TargetType>> {
public:
    using Super = ConvertingValueBase<META_NS::Property<typename Converter::TargetType>>;

    using TargetType = typename Converter::TargetType;
    using SourceType = typename Converter::SourceType;
    using TargetProperty = META_NS::Property<TargetType>;

    ConvertingValue(TargetProperty p, Converter convert = {})
        : Super(META_NS::ConstructAny<SourceType>(), p), convert_(BASE_NS::move(convert))
    {}

    META_NS::AnyReturnValue SetValue(const META_NS::IAny& any) override
    {
        SourceType value {};
        auto res = any.GetValue<SourceType>(value);
        if (res) {
            res = this->p_->SetValue(convert_.ConvertToTarget(value));
            if (res) {
                InternalSetValue(value);
            }
        }
        return res;
    }
    const META_NS::IAny& GetValue() const override
    {
        auto pvalue = this->p_->GetValue();
        auto value = convert_.ConvertToSource(*this->any_, pvalue);
        InternalSetValue(value);
        return *this->any_;
    }
    bool IsCompatible(const META_NS::TypeId& id) const override
    {
        return id == META_NS::UidFromType<SourceType>() || id == META_NS::UidFromType<TargetType>();
    }

private:
    void OnResourceChanged(bool forceNotification = false) const
    {
        SourceType value {};
        auto res = this->any_->template GetValue<SourceType>(value);
        if (res) {
            auto locked = this->p_.GetLockedAccess();
            res = locked->SetValue(convert_.ConvertToTarget(value));
            // force notification
            if (forceNotification && res == META_NS::AnyReturn::NOTHING_TO_DO) {
                locked->NotifyChange();
            }
        }
    }
    void InternalSetValue(const SourceType& value) const
    {
        if (this->any_->SetValue(value) == META_NS::AnyReturn::SUCCESS) {
            if constexpr (META_NS::IsInterfacePtr_v<SourceType>) {
                if (auto i = interface_cast<META_NS::IDynamicResource>(value)) {
                    changed_.Subscribe<META_NS::IOnChanged>(i->OnResourceChanged(), [this] {
                        // if the object itself says it changed, for the notification
                        OnResourceChanged(true);
                    });
                    OnResourceChanged();
                } else {
                    changed_.Unsubscribe();
                }
            }
        }
    }

private:
    Converter convert_;
    mutable META_NS::EventHandler changed_;
};

template<typename Converter>
class ConvertingArrayValue : public ConvertingValueBase<META_NS::ArrayProperty<typename Converter::TargetType>> {
public:
    using Super = ConvertingValueBase<META_NS::ArrayProperty<typename Converter::TargetType>>;

    using TargetType = typename Converter::TargetType;
    using SourceType = typename Converter::SourceType;
    using TargetProperty = META_NS::ArrayProperty<TargetType>;

    ConvertingArrayValue(TargetProperty p, Converter convert = {})
        : Super(META_NS::ConstructArrayAny<SourceType>(), p), convert_(BASE_NS::move(convert))
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
                this->any_->SetValue(converted);
            }
        }
        return res;
    }
    const META_NS::IAny& GetValue() const override
    {
        BASE_NS::vector<SourceType> result;
        for (auto&& v : this->p_->GetValue()) {
            result.push_back(convert_.ConvertToSource(*this->any_, v));
        }
        this->any_->SetValue(result);
        return *this->any_;
    }
    bool IsCompatible(const META_NS::TypeId& id) const override
    {
        return id == META_NS::ArrayUidFromType<SourceType>() || id == META_NS::ArrayUidFromType<TargetType>();
    }

private:
    Converter convert_;
};

SCENE_END_NAMESPACE()

#endif
