/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_INTERFACE_TEMPLATE_ITEMPLATE_OPTIONS_H
#define SCENE_INTERFACE_TEMPLATE_ITEMPLATE_OPTIONS_H

#include <cstdint>

#include <scene/base/namespace.h>

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Object-flag bit marking objects, properties, and attachments whose state originated from a
 *        template application (rather than scene-local edits). Query with
 *        META_NS::IsFlagSet(object, IMPORTED_FROM_TEMPLATE_BIT).
 */
constexpr uint64_t IMPORTED_FROM_TEMPLATE_BIT = 128;

/**
 * @brief Internal protocol used by the importer to mark a typed template options object
 *        (e.g. MaterialTemplate) as having been built from a "*Template" index entry rather
 *        than from inline options. When set, ApplyOptions copies values into the live
 *        resource as defaults (and flags it with IMPORTED_FROM_TEMPLATE_BIT) instead of
 *        marking them as set. This is an apply-time concern only — direct ApplyTo paths
 *        are unaffected.
 */
class ITemplateOptions : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITemplateOptions, "f3a6c9d2-1b48-4e07-9c5a-2d8e7b9f0a14")
public:
    virtual void SetTemplateContext(bool isTemplate) = 0;
    virtual bool IsTemplateContext() const = 0;
};

SCENE_END_NAMESPACE()

#endif
