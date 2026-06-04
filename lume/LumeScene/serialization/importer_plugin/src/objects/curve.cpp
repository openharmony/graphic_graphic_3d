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

#include "curve.h"

#include <meta/api/util.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/curves/intf_bezier.h>

#include "../import_any.h"
#include "../import_helpers.h"

SCENE_IMP_BEGIN_NAMESPACE()

namespace {

constexpr BASE_NS::string_view CUBIC_BEZIER_NAME{"cubicBezier"};

// Map JSON built-in curve names to their META_NS ObjectId (ClassInfo implicitly
// converts to ObjectId). Names are lowerCamelCase, stripped of "EasingCurve".
const NamedValue<META_NS::ObjectId> BUILTIN_CURVES[] = {
    {"linear", META_NS::ClassId::LinearEasingCurve},
    {"inQuad", META_NS::ClassId::InQuadEasingCurve},
    {"outQuad", META_NS::ClassId::OutQuadEasingCurve},
    {"inOutQuad", META_NS::ClassId::InOutQuadEasingCurve},
    {"inCubic", META_NS::ClassId::InCubicEasingCurve},
    {"outCubic", META_NS::ClassId::OutCubicEasingCurve},
    {"inOutCubic", META_NS::ClassId::InOutCubicEasingCurve},
    {"inSine", META_NS::ClassId::InSineEasingCurve},
    {"outSine", META_NS::ClassId::OutSineEasingCurve},
    {"inOutSine", META_NS::ClassId::InOutSineEasingCurve},
    {"inQuart", META_NS::ClassId::InQuartEasingCurve},
    {"outQuart", META_NS::ClassId::OutQuartEasingCurve},
    {"inOutQuart", META_NS::ClassId::InOutQuartEasingCurve},
    {"inQuint", META_NS::ClassId::InQuintEasingCurve},
    {"outQuint", META_NS::ClassId::OutQuintEasingCurve},
    {"inOutQuint", META_NS::ClassId::InOutQuintEasingCurve},
    {"inExpo", META_NS::ClassId::InExpoEasingCurve},
    {"outExpo", META_NS::ClassId::OutExpoEasingCurve},
    {"inOutExpo", META_NS::ClassId::InOutExpoEasingCurve},
    {"inCirc", META_NS::ClassId::InCircEasingCurve},
    {"outCirc", META_NS::ClassId::OutCircEasingCurve},
    {"inOutCirc", META_NS::ClassId::InOutCircEasingCurve},
    {"inBack", META_NS::ClassId::InBackEasingCurve},
    {"outBack", META_NS::ClassId::OutBackEasingCurve},
    {"inOutBack", META_NS::ClassId::InOutBackEasingCurve},
    {"inElastic", META_NS::ClassId::InElasticEasingCurve},
    {"outElastic", META_NS::ClassId::OutElasticEasingCurve},
    {"inOutElastic", META_NS::ClassId::InOutElasticEasingCurve},
    {"inBounce", META_NS::ClassId::InBounceEasingCurve},
    {"outBounce", META_NS::ClassId::OutBounceEasingCurve},
    {"inOutBounce", META_NS::ClassId::InOutBounceEasingCurve},
    {"stepStart", META_NS::ClassId::StepStartEasingCurve},
    {"stepEnd", META_NS::ClassId::StepEndEasingCurve},
    {CUBIC_BEZIER_NAME, META_NS::ClassId::CubicBezierEasingCurve},
};

IDiagnostics::Ptr ApplyBezierShorthand(ImportContext& context, const META_NS::IObject::Ptr& obj)
{
    auto bezier = interface_cast<META_NS::ICubicBezier>(obj);
    if (!bezier) {
        return context.CreateDiagnostics("cubicBezier curve does not implement ICubicBezier");
    }
    ErrorHandler h(context);
    if (auto v = GetOptVec2(context, "controlPoint1"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        META_NS::SetValue(bezier->ControlPoint1(), v.GetValue());
    }
    if (auto v = GetOptVec2(context, "controlPoint2"); h.HandleOptValue(v)) {
        if (v.error) {
            return v.error;
        }
        META_NS::SetValue(bezier->ControlPoint2(), v.GetValue());
    }
    return h;
}

IDiagnostics::Ptr RejectBezierShorthand(ImportContext& context, BASE_NS::string_view type)
{
    if (context.GetOptValue("controlPoint1") || context.GetOptValue("controlPoint2")) {
        return context.CreateDiagnostics(
            BASE_NS::string("controlPoint1/controlPoint2 only allowed for cubicBezier, got '") + type + "'");
    }
    return nullptr;
}

ImportResult ImportBuiltinCurve(ImportContext& context, BASE_NS::string_view type)
{
    auto found = LookupValue(BUILTIN_CURVES, type);
    if (!found) {
        return ImportResult{context.CreateDiagnostics(BASE_NS::string("Unknown built-in curve type: '") + type + "'")};
    }
    auto obj = META_NS::GetObjectRegistry().Create(*found);
    if (!obj) {
        return ImportResult{
            context.CreateDiagnostics(BASE_NS::string("Failed to create curve for type '") + type + "'")};
    }
    if (type == CUBIC_BEZIER_NAME) {
        if (auto err = ApplyBezierShorthand(context, obj)) {
            return ImportResult{err};
        }
    } else if (auto err = RejectBezierShorthand(context, type)) {
        return ImportResult{err};
    }
    return ImportResult{BASE_NS::move(obj)};
}

}  // namespace

ImportResult ImportCurve(ImportContext& context)
{
    auto type = context.GetOptString("type");
    auto id = GetOptObjectId(context, "objectUid");
    if (id.error) {
        return ImportResult{id.error};
    }
    if (!type.empty()) {
        return ImportBuiltinCurve(context, type);
    }
    if (id.value) {
        return ImportObject(context);
    }
    return ImportResult{context.CreateDiagnostics("Curve requires 'type' or 'objectUid'")};
}

SCENE_IMP_END_NAMESPACE()
