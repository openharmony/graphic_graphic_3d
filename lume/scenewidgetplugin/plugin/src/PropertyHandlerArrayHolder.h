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
#ifndef PROPERTYHANDLEARRAYHOLDER_H
#define PROPERTYHANDLEARRAYHOLDER_H
#include <algorithm>
#include <scene_plugin/interface/intf_proxy_object.h>

#include <meta/api/property/property_event_handler.h>
#include <meta/interface/intf_proxy_object.h>
#include <meta/interface/property/intf_property.h>

#include "scene_holder.h"

struct PropertyHandlerArrayHolder {
    struct Handler {
        META_NS::PropertyChangedEventHandler handler;
        META_NS::IProperty::WeakPtr target;
        META_NS::IProperty::WeakPtr source;
    };

    SceneHolder* sceneHolder_ { nullptr };

    BASE_NS::vector<Handler> handlers;
    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> relatedPairs_;

    bool useEcsDefaults_ { true };

    void EraseHandler(META_NS::IProperty::Ptr target, META_NS::IProperty::Ptr source)
    {
        handlers.erase(std::remove_if(handlers.begin(), handlers.end(),
            [&target, &source](const Handler& h) { return h.target.lock() == target && h.source.lock() == source; }));
    }

    META_NS::PropertyChangedEventHandler& NewHandler(META_NS::IProperty::Ptr target, META_NS::IProperty::Ptr source)
    {
        handlers.push_back({ META_NS::PropertyChangedEventHandler(), target, source });
        return handlers.back().handler;
    }

    void MarkRelated(META_NS::IProperty::ConstPtr first, META_NS::IProperty::ConstPtr second)
    {
        relatedPairs_.push_back({ first, second });
    }

    void Reset()
    {
        sceneHolder_ = nullptr;
        handlers.clear();
        relatedPairs_.clear();
    }

    void SetUseEcsDefaults(bool value)
    {
        useEcsDefaults_ = value;
    }

    bool GetUseEcsDefaults() const
    {
        return useEcsDefaults_;
    }

    void SetSceneHolder(SceneHolder::Ptr holder)
    {
        sceneHolder_ = holder.get();
    }

    SceneHolder* GetSceneHolder() const
    {
        return sceneHolder_;
    }

    BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> GetBoundProperties() const
    {
        BASE_NS::vector<SCENE_NS::IProxyObject::PropertyPair> ret;
        for (const auto& pair : relatedPairs_) {
            ret.push_back({ pair.first, pair.second });
        }
        for (auto& handler : handlers) {
            if (auto target = handler.target.lock()) {
                if (auto source = handler.source.lock()) {
                    ret.push_back({ source, target });
                }
            }
        }
        return ret;
    }

    META_NS::IProperty::Ptr GetTarget(META_NS::IProperty::ConstPtr proxy)
    {
        for (auto& handler : handlers) {
            if (proxy == handler.source.lock()) {
                return handler.target.lock();
            }
        }
        return META_NS::IProperty::Ptr {};
    }
};

#endif
