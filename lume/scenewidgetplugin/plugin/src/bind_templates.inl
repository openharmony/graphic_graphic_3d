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

#include <algorithm>
#include <scene_plugin/api/material_uid.h>
#include <scene_plugin/interface/compatibility.h>
#include <scene_plugin/interface/intf_bitmap.h>

#include <scene_plugin/interface/intf_scene.h>
#include <meta/api/engine/util.h>
#include <meta/api/property/property_event_handler.h>

#include "intf_postprocess_private.h"
#include "PropertyHandlerArrayHolder.h"
#include "scene_holder.h"


BASE_BEGIN_NAMESPACE()

// our vector does not have op==
template<typename T>
bool operator==(const BASE_NS::vector<T>& l, const BASE_NS::vector<T>& r)
{
    if (l.size() != r.size()) {
        return false;
    }
    auto it1 = l.begin();
    auto it2 = r.begin();
    for (; it1 != l.end(); ++it1, ++it2) {
        if constexpr (META_NS::HasEqualOperator_v<T>) {
            if (!(*it1 == *it2)) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

template<typename T>
bool operator!=(const BASE_NS::vector<T>& l, const BASE_NS::vector<T>& r)
{
    return !(l == r);
}

BASE_END_NAMESPACE()

bool HasChangedProperties(META_NS::IMetadata& meta);

template<class EcsType, class UiType>
struct StaticConverter {
    static inline EcsType ToEcs(SceneHolder& sceneHolder, const UiType& v)
    {
        return static_cast<EcsType>(v);
    }
    static inline UiType ToUi(SceneHolder& sceneHolder, const EcsType& v)
    {
        return static_cast<UiType>(v);
    }
};

struct ImageConverter {
    static inline CORE_NS::EntityReference ToEcs(SceneHolder& sceneHolder, const SCENE_NS::IBitmap::Ptr& v)
    {
        // Get ecs image handle from bitmap.
        if (v) {
	    if (auto rh = v->GetRenderHandle()) {
                auto uri = META_NS::GetValue(v->Uri());
		return sceneHolder.LoadImage(uri, rh);
	    } else {
		auto uri = META_NS::GetValue(v->Uri());
		if (!uri.empty()) {
                    return sceneHolder.LoadImage(uri);
		}
            }
        }
        return {};
    }

    static inline SCENE_NS::IBitmap::Ptr ToUi(SceneHolder& sceneHolder, const CORE_NS::EntityReference& v)
    {
        CORE_NS::Entity image;
        BASE_NS::string uri;
        if (sceneHolder.GetEntityUri(v, uri)) {
	    RENDER_NS::GpuImageDesc desc;
	    RENDER_NS::RenderHandleReference rh;
            auto uriBitmap = META_NS::GetObjectRegistry().Create<SCENE_NS::IBitmap>(SCENE_NS::ClassId::Bitmap);
                if (sceneHolder.GetImageHandle(v, rh, desc)) {
                uriBitmap->SetRenderHandle(rh, { desc.width, desc.height });	        
	    }
            META_NS::SetValue(uriBitmap->Uri(), uri);
            return uriBitmap;
        }

        return {};
    }
};

template<class ObjectType>
struct EntityConverter {
    static inline CORE_NS::Entity ToEcs(SceneHolder& sceneHolder, const typename ObjectType::Ptr& v)
    {
        // Get ecs object to entity.
        if (auto ecsObject = interface_cast<SCENE_NS::IEcsObject>(v)) {
            return ecsObject->GetEntity();
        }
        if (auto ippp = interface_cast<IPostProcessPrivate>(v)) {
            return ippp->GetEntity();
        }
        return {};
    }

    static inline typename ObjectType::Ptr ToUi(SceneHolder& sceneHolder, const CORE_NS::Entity& v)
    {
        return {};
    }
};

template<class originalType>
inline auto& GetValueRef(originalType& from, size_t slot)
{
    return from[slot];
}

template<>
inline auto& GetValueRef<SCENE_NS::Color>(SCENE_NS::Color& from, size_t slot)
{
    if (slot == 0) {
        return from.x;
    } else if (slot == 1) {
        return from.y;
    } else if (slot == 2) { // 2 slot index
        return from.z;
    }
    return from.w;
}

template<class originalType, class valueType>
inline bool SetValueFromComponent(META_NS::Property<valueType> to,
    const META_NS::Property<originalType>& from, size_t slot, bool setAsDefault = false)
{
    originalType fromValue = from->GetValue();

    if (to->GetValue() != fromValue[slot]) {
        if (setAsDefault) {
            to->SetDefaultValue(fromValue[slot], true);
        } else {
            to->SetValue(fromValue[slot]);
        }
        return true;
    }
    return false;
}

template<>
inline bool SetValueFromComponent<SCENE_NS::Color, float>(META_NS::Property<float> to,
    const META_NS::Property<SCENE_NS::Color>& from, size_t slot, bool setAsDefault)
{
    SCENE_NS::Color fromValue = from->GetValue();

    if (slot == 0) {
        if (to->GetValue() != fromValue.x) {
            if (setAsDefault) {
                to->SetDefaultValue(fromValue.x, true);
            } else {
                to->SetValue(fromValue.x);
            }
        }
    } else if (slot == 1) {
        if (to->GetValue() != fromValue.y) {
            if (setAsDefault) {
                to->SetDefaultValue(fromValue.y, true);
            } else {
                to->SetValue(fromValue.y);
            }
        }
    } else if (slot == 2) {
        if (to->GetValue() != fromValue.z) {
            if (setAsDefault) {
                to->SetDefaultValue(fromValue.z, true);
            } else {
                to->SetValue(fromValue.z);
            }
        }
    } else if (slot == 3) {
        if (to->GetValue() != fromValue.w) {
            if (setAsDefault) {
                to->SetDefaultValue(fromValue.w, true);
            } else {
                to->SetValue(fromValue.w);
            }
        }
    } else {
        return false;
    }
    return true;
}

// Bending the implementation a bit, implicit conversion from TimeSpan to Seconds in float
template<>
inline bool SetValueFromComponent<float, META_NS::TimeSpan>(META_NS::Property<META_NS::TimeSpan> to,
    const META_NS::Property<float>& from, size_t /*slot*/, bool setAsDefault)
{
    META_NS::TimeSpan value;
    value.SetSeconds(from->GetValue());
    if (setAsDefault) {
        to->SetDefaultValue(value, true);
    } else {
        to->SetValue(value);
    }
    return true;
}

template<class originalType, class valueType>
inline bool SetValueToComponent(META_NS::Property<originalType> to,
    const META_NS::Property<valueType>& from, size_t slot)
{
    valueType fromValue = from->GetValue();
    originalType toValue = to->GetValue();

    if (toValue[slot] != fromValue) {
        toValue[slot] = fromValue;
        to->SetValue(toValue);
        return true;
    }
    return false;
}

// Bending the implementation a bit, implicit conversion from TimeSpan to Seconds in float
template<>
inline bool SetValueToComponent<float, META_NS::TimeSpan>(META_NS::Property<float> to,
    const META_NS::Property<META_NS::TimeSpan>& from, size_t /*slot*/)
{
    to->SetValue(from->GetValue().ToSecondsFloat());
    return true;
}

template<>
inline bool SetValueToComponent<SCENE_NS::Color, float>(META_NS::Property<SCENE_NS::Color> to,
    const META_NS::Property<float>& from, size_t slot)
{
    float fromValue = from->GetValue();
    SCENE_NS::Color toValue = to->GetValue();

    if (slot == 0) {
        if (toValue.x != fromValue) {
            toValue.x = fromValue;
        }
    } else if (slot == 1) {
        if (toValue.y != fromValue) {
            toValue.y = fromValue;
        }
    } else if (slot == 2) { // 2 slot index
        if (toValue.z != fromValue) {
            toValue.z = fromValue;
        }
    } else if (slot == 3) { // 3 slot index
        if (toValue.w != fromValue) {
            toValue.w = fromValue;
        }
    } else {
        return false;
    }

    to->SetValue(toValue);
    return true;
}

enum PropertyBindingType { ONE_WAY_TO_ECS, ONE_WAY_TO_UI, TWO_WAY };

template<class valueType, class sourceType, class Conv = StaticConverter<sourceType, valueType>>
inline bool BindPropChanges(PropertyHandlerArrayHolder& handler,
    META_NS::Property<valueType> propertyInstance,
    META_NS::Property<sourceType> prop, PropertyBindingType type = TWO_WAY)
{
    if (!prop || !propertyInstance) {
        return false;
    }

    auto sceneHolder = handler.GetSceneHolder();
    if (!sceneHolder) {
        return false;
    }

    bool useEcsPropertyValue = handler.GetUseEcsDefaults();
    if (propertyInstance->IsValueSet()) {
        useEcsPropertyValue = false;
    }

    // If we don't have local value, initialize value from ECS>
    if (useEcsPropertyValue) {
        auto ecsValue = Conv::ToUi(*sceneHolder, prop->GetValue());
        // try to keep our value intact if the engine default value is the same as ours, otherwise we should
        // update our default
        if (ecsValue != propertyInstance->GetValue()) {
            // Set to default value.
            propertyInstance->SetDefaultValue(ecsValue, true);
        }
    } else { // otherwise reflect our value to ecs directly
        prop->SetValue(Conv::ToEcs(*sceneHolder, propertyInstance->GetValue()));
    }
    if (type == ONE_WAY_TO_ECS || type == TWO_WAY) {
        // and reflect the changes from us to ecs from now on
        handler.NewHandler(prop, propertyInstance)
            .Subscribe(propertyInstance, META_NS::MakeCallback<META_NS::IOnChanged>(
                                             [propertyInstance, sh = sceneHolder](const auto& prop) {
                                                if (prop) {
                                                    prop->SetValue(Conv::ToEcs(*sh, propertyInstance->GetValue()));
                                                    //hack
                                                    /*if (auto v = META_NS::GetEngineValueFromProperty(prop.GetProperty())) {
                                                        v->Sync(META_NS::EngineSyncDirection::TO_ENGINE);
                                                    }*/
                                                }
                                             },
                                             prop));
    }

    if (type == ONE_WAY_TO_UI || type == TWO_WAY) {
        //  EcsObject reports changes back only if they take place on ecs side, so this should not cause race
        //  They anyway don't support bindings properly, so having the feedback channel this way
        handler.NewHandler(nullptr, nullptr)
            .Subscribe(prop, META_NS::MakeCallback<META_NS::IOnChanged>(
                                 [propertyInstance, sh = sceneHolder](const auto& prop) {
                                    if (prop) {
                                        if (!propertyInstance->GetBind()) {
                                            propertyInstance->SetValue(Conv::ToUi(*sh, prop->GetValue()));
                                        }
                                    }
                                },
                                prop));
    }

    return true;
}

template<class valueType, class sourceType, class Conv = StaticConverter<BASE_NS::vector<sourceType>, BASE_NS::vector<valueType>>>
inline bool BindPropChanges(PropertyHandlerArrayHolder& handler, META_NS::ArrayProperty<valueType> propertyInstance,
    META_NS::ArrayProperty<sourceType> prop, PropertyBindingType type = TWO_WAY)
{
    if (!prop || !propertyInstance) {
        return false;
    }

    auto sceneHolder = handler.GetSceneHolder();
    if (!sceneHolder) {
        return false;
    }

    bool useEcsPropertyValue = handler.GetUseEcsDefaults();
    if (propertyInstance->IsValueSet()) {
        useEcsPropertyValue = false;
    }

    // If we don't have local value, initialize value from ECS>
    if (useEcsPropertyValue) {
        auto ecsValue = Conv::ToUi(*sceneHolder, prop->GetValue());
        // try to keep our value intact if the engine default value is the same as ours, otherwise we should
        // update our default
        if (ecsValue != propertyInstance->GetValue()) {
            // Set to default value.
            propertyInstance->SetDefaultValue(ecsValue, true);
        }
    } else { // otherwise reflect our value to ecs directly
        prop->SetValue(Conv::ToEcs(*sceneHolder, propertyInstance->GetValue()));
    }
    if (type == ONE_WAY_TO_ECS || type == TWO_WAY) {
        // and reflect the changes from us to ecs from now on
        handler.NewHandler(prop, propertyInstance)
            .Subscribe(
                propertyInstance, META_NS::MakeCallback<META_NS::IOnChanged>(
                                      [propertyInstance, sh = sceneHolder](const auto& prop) {
                                          if (prop) {
                                              prop->SetValue(Conv::ToEcs(*sh, propertyInstance->GetValue()));
                                              // hack
                                          }
                                      },
                                      prop));
    }

    if (type == ONE_WAY_TO_UI || type == TWO_WAY) {
        //  EcsObject reports changes back only if they take place on ecs side, so this should not cause race
        //  They anyway don't support bindings properly, so having the feedback channel this way
        handler.NewHandler(nullptr, nullptr)
            .Subscribe(prop, META_NS::MakeCallback<META_NS::IOnChanged>(
                                 [propertyInstance, sh = sceneHolder](const auto& prop) {
                                     if (prop) {
                                         if (!propertyInstance->GetBind()) {
                                             propertyInstance->SetValue(Conv::ToUi(*sh, prop->GetValue()));
                                         }
                                     }
                                 },
                                 prop));
    }

    return true;
}

template<class valueType>
inline bool BindChanges(PropertyHandlerArrayHolder& handler,
    META_NS::Property<valueType> propertyInstance, META_NS::IMetadata::Ptr meta,
    const BASE_NS::string_view name)
{
    if (!meta || !propertyInstance) {
        return false;
    }

    if (auto prop = meta->GetPropertyByName<valueType>(name)) {
        // check if the target is already a proxy
        if (auto target = handler.GetTarget(prop)) {
            // this could go wrong if the target and proxy are different, mapped types
            if (META_NS::Property<valueType> typed { target }) {
                prop = typed;
            } else {
                CORE_LOG_W("%s: could not match the types for %s", __func__, BASE_NS::string(name).c_str());
            }
        }
        return BindPropChanges<valueType>(handler, propertyInstance, prop);
    }

    SCENE_PLUGIN_VERBOSE_LOG("%s: could not find '%s'", __func__, name.data());
    return false;
}

template<class valueType>
inline bool BindChanges(PropertyHandlerArrayHolder& handler, META_NS::ArrayProperty<valueType> propertyInstance,
    META_NS::IMetadata::Ptr meta, const BASE_NS::string_view name)
{
    if (!meta || !propertyInstance) {
        return false;
    }

    if (auto prop = meta->GetArrayPropertyByName<valueType>(name)) {
        // check if the target is already a proxy
        if (auto target = handler.GetTarget(prop)) {
            // this could go wrong if the target and proxy are different, mapped types
            if (META_NS::ArrayProperty<valueType> typed { target }) {
                prop = typed;
            } else {
                CORE_LOG_W("%s: could not match the types for %s", __func__, BASE_NS::string(name).c_str());
            }
        }
        return BindPropChanges<valueType>(handler, propertyInstance, prop);
    }

    SCENE_PLUGIN_VERBOSE_LOG("%s: could not find '%s'", __func__, name.data());
    return false;
}

template<class originalType, class valueType>
inline bool setComponentValues(META_NS::Property<originalType> to,
    const META_NS::Property<valueType>& from, BASE_NS::vector<size_t> slots,
    bool setAsDefault = false)
{
    bool changed { false };
    valueType fromValue = from->GetValue();
    originalType toValue = to->GetValue();

    for (size_t ix = 1; ix < slots.size();) {
        auto slot1 = slots[ix - 1];
        auto slot2 = slots[ix];
        auto& value = GetValueRef<originalType>(toValue, slot1);
        auto& value2 = GetValueRef<valueType>(fromValue, slot2);
        if (value != value2) {
            if (value != value && value2 != value2) {
                CORE_LOG_D("omitting changes on NaN");
            } else {
                changed = true;
                value = value2;
            }
        }
        ix += 2; // 2 index shift
    }
    if (changed) {
        if (setAsDefault) {
            to->SetDefaultValue(toValue, true);
        } else {
            to->SetValue(toValue);
        }
    }
    return changed;
}

template<class originalType, class valueType>
inline bool BindSlottedChanges(PropertyHandlerArrayHolder& handler,
    META_NS::Property<valueType> propertyInstance, META_NS::IMetadata::Ptr meta,
    const BASE_NS::string_view name, BASE_NS::vector<size_t> slots)
{
    if (!meta || !propertyInstance) {
        return false;
    }

    if (auto prop = meta->GetPropertyByName<originalType>(name)) {
        // If we don't have local value, initialize value from ECS>
        bool useEcsPropertyValue = handler.GetUseEcsDefaults();
        if (propertyInstance->IsValueSet()) {
            useEcsPropertyValue = false;
        }

        // If we don't have local value, initialize value from ECS
        if (useEcsPropertyValue) {
            // Set default value.
            setComponentValues(propertyInstance, prop, slots, true);
        } else { // otherwise reflect our value to ecs directly
            setComponentValues(prop, propertyInstance, slots);
        }
        // and reflect the changes from us to ecs from now on
        handler.NewHandler(prop, propertyInstance)
            .Subscribe(propertyInstance, META_NS::MakeCallback<META_NS::IOnChanged>(
                                             [propertyInstance, slots](const auto& prop) {
                                                 if (prop) {
                                                     setComponentValues(prop, propertyInstance, slots);
                                                 }
                                             },
                                             prop));
        handler.NewHandler(nullptr, nullptr)
            .Subscribe(prop, META_NS::MakeCallback<META_NS::IOnChanged>(
                                 [propertyInstance, slots](const auto& prop) {
                                     if (prop) {
                                         setComponentValues(propertyInstance, prop, slots);
                                     }
                                 },
                                 prop));

        return true;
    }
    CORE_LOG_W("%s: could not find '%s'", __func__, name.data());
    return false;
}

template<class valueType, class sourceType, class Conv = StaticConverter<sourceType, valueType>>
inline bool ConvertBindChanges(PropertyHandlerArrayHolder& handler,
    META_NS::Property<valueType> propertyInstance, META_NS::IMetadata::Ptr meta,
    const BASE_NS::string_view name, PropertyBindingType type = TWO_WAY)
{
    if (!meta || !propertyInstance) {
        return false;
    }

    if (auto prop = meta->GetPropertyByName<sourceType>(name)) {
        return BindPropChanges<valueType, sourceType, Conv>(handler, propertyInstance, prop, type);
    }

    SCENE_PLUGIN_VERBOSE_LOG("%s: could not find '%s'", __func__, name.data());
    return false;
}


template<class valueType>
inline bool BindIfCorrectType(
    PropertyHandlerArrayHolder& handler, META_NS::IProperty::Ptr& clone, META_NS::IProperty::Ptr& prop)
{
    if (clone->IsCompatible(META_NS::UidFromType<valueType>())) {
        auto typed = META_NS::Property<valueType>(clone);
        BindPropChanges<valueType>(handler, typed, META_NS::Property<valueType>(prop));
        return true;
    }

    return false;
}

template<class originalType, class valueType>
inline bool BindChanges(PropertyHandlerArrayHolder& handler,
    META_NS::Property<valueType> propertyInstance, META_NS::IMetadata::Ptr meta,
    const BASE_NS::string_view name, size_t slot)
{
    if (!meta || !propertyInstance) {
        return false;
    }

    if (auto prop = meta->GetPropertyByName<originalType>(name)) {
        // check if the target is already a proxy
        if (auto target = handler.GetTarget(prop)) {
            // this could go wrong if the target and proxy are different, mapped types
            if (auto typed = META_NS::Property<originalType>(target)) {
                prop = typed;
            } else {
                CORE_LOG_W("%s: could not match the types for %s", __func__, BASE_NS::string(name).c_str());
            }
        }

        bool useEcsPropertyValue = handler.GetUseEcsDefaults();
        if (propertyInstance->IsValueSet()) {
            useEcsPropertyValue = false;
        }

        // If we don't have local value, initialize value from ECS
        if (useEcsPropertyValue) {
            // Set to default value
            SetValueFromComponent(propertyInstance, prop, slot, true);
        } else { // otherwise reflect our value to ecs directly
            SetValueToComponent(prop, propertyInstance, slot);
        }
        // and reflect the changes from us to ecs from now on
        handler.NewHandler(prop, propertyInstance)
            .Subscribe(propertyInstance, META_NS::MakeCallback<META_NS::IOnChanged>(
                                             [propertyInstance, slot](const auto& prop) {
                                                 if (prop) {
                                                     SetValueToComponent(prop, propertyInstance, slot);
                                                 }
                                             },
                                             prop));
        handler.NewHandler(nullptr, nullptr)
            .Subscribe(prop, META_NS::MakeCallback<META_NS::IOnChanged>(
                                 [propertyInstance, slot](const auto& prop) {
                                     if (prop) {
                                         SetValueFromComponent(propertyInstance, prop, slot);
                                     }
                                 },
                                 prop));

        return true;
    }
    CORE_LOG_W("%s: could not find '%s'", __func__, name.data());
    return false;
}
