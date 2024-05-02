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
#include "easing_curve.h"

#include <base/math/mathf.h>

META_BEGIN_NAMESPACE()

namespace Curves {
namespace Easing {

bool AreEqual(float v1, float v2)
{
    return BASE_NS::Math::abs(v1 - v2) < BASE_NS::Math::EPSILON;
}
float Pow2(float v)
{
    return v * v;
}
float Pow3(float v)
{
    return v * v * v;
}
float Pow4(float v)
{
    return v * v * v * v;
}
float Pow5(float v)
{
    return v * v * v * v * v;
}
float EaseLinear(float t)
{
    return t;
}
float EaseInSine(float t)
{
    return 1.f - BASE_NS::Math::cos((t * BASE_NS::Math::PI) / 2.f);
}
float EaseOutSine(float t)
{
    return BASE_NS::Math::sin((t * BASE_NS::Math::PI) / 2.f);
}
float EaseInOutSine(float t)
{
    return -(BASE_NS::Math::cos(BASE_NS::Math::PI * t) - 1.f) / 2.f;
}
float EaseInQuad(float t)
{
    return Pow2(t);
}
float EaseOutQuad(float t)
{
    return 1.f - Pow2(1.f - t);
}
float EaseInOutQuad(float t)
{
    if (t < 0.5f) {
        return 2 * t * t;
    }
    return 1.f - (Pow2(-2.f * t + 2.f) / 2.f);
}
float EaseInCubic(float t)
{
    return Pow3(t);
}
float EaseOutCubic(float t)
{
    return 1.f - Pow3(1.f - t);
}
float EaseInOutCubic(float t)
{
    if (t < 0.5f) {
        return 4 * t * t * t;
    }
    return 1.f - (Pow3(-2.f * t + 2.f) / 2.f);
}
float EaseInQuart(float t)
{
    return Pow4(t);
}
float EaseOutQuart(float t)
{
    return 1.f - Pow4(1.f - t);
}
float EaseInOutQuart(float t)
{
    if (t < 0.5f) {
        return 8 * Pow4(t);
    }
    return 1.f - (Pow4(-2.f * t + 2.f) / 2.f);
}
float EaseInQuint(float t)
{
    return Pow5(t);
}
float EaseOutQuint(float t)
{
    return 1.f - Pow5(1.f - t);
}
float EaseInOutQuint(float t)
{
    if (t < 0.5f) {
        return 16 * Pow5(t);
    }
    return 1.f - (Pow5(-2.f * t + 2.f) / 2.f);
}
float EaseInExpo(float t)
{
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    return BASE_NS::Math::pow(2.f, 10 * t - 10);
}
float EaseOutExpo(float t)
{
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    return 1.f - BASE_NS::Math::pow(2.f, -10 * t);
}
float EaseInOutExpo(float t)
{
    if (AreEqual(t, 0.f)) {
        return 0.f;
    }
    if (AreEqual(t, 1.f)) {
        return 1.f;
    }
    if (t < .5f) {
        return BASE_NS::Math::pow(2.f, 20 * t - 10) / 2.f;
    }
    return (2.f - BASE_NS::Math::pow(2.f, -20 * t + 10)) / 2.f;
}
float EaseInCirc(float t)
{
    return 1.f - BASE_NS::Math::sqrt(1.f - Pow2(t));
}
float EaseOutCirc(float t)
{
    return BASE_NS::Math::sqrt(1.f - Pow2(t - 1.f));
}
float EaseInOutCirc(float t)
{
    if (t < 0.5f) {
        return (1.f - BASE_NS::Math::sqrt(1.f - Pow2(2.f * t))) / 2.f;
    }
    return (BASE_NS::Math::sqrt(1.f - Pow2(-2.f * t + 2.f)) + 1.f) / 2.f;
}
float EaseInBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.f;
    return c3 * Pow3(t) - c1 * Pow2(t);
}
float EaseOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.f;
    return 1.f + c3 * Pow3(t - 1.f) + c1 * Pow2(t - 1.f);
}
float EaseInOutBack(float t)
{
    constexpr float c1 = 1.70158f;
    constexpr float c2 = c1 * 1.525f;
    if (t < 0.5f) {
        return (Pow2(2.f * t) * ((c2 + 1.f) * 2.f * t - c2)) / 2.f;
    }
    return (Pow2(2.f * t - 2.f) * ((c2 + 1.f) * (t * 2.f - 2.f) + c2) + 2.f) / 2.f;
}
float EaseInElastic(float t)
{
    constexpr float c4 = (2.f * BASE_NS::Math::PI) / 3.f;
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    return -BASE_NS::Math::pow(2.f, 10 * t - 10) * BASE_NS::Math::sin((t * 10 - 10.75f) * c4);
}
float EaseOutElastic(float t)
{
    constexpr float c4 = (2.f * BASE_NS::Math::PI) / 3.f;
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    return BASE_NS::Math::pow(2.f, -10 * t) * BASE_NS::Math::sin((t * 10 - 0.75f) * c4) + 1.f;
}
float EaseInOutElastic(float t)
{
    constexpr float c5 = (2.f * BASE_NS::Math::PI) / 4.5f;
    if (AreEqual(t, 0.f)) {
        return 0;
    }
    if (AreEqual(t, 1.f)) {
        return 1;
    }
    if (t < 0.5f) {
        return -(BASE_NS::Math::pow(2.f, 20 * t - 10) * BASE_NS::Math::sin((20 * t - 11.125f) * c5)) / 2.f;
    }
    return (BASE_NS::Math::pow(2.f, -20 * t + 10) * BASE_NS::Math::sin((20 * t - 11.125f) * c5)) / 2.f + 1.f;
}
float EaseOutBounce(float t)
{
    constexpr float n1 = 7.5625f;
    constexpr float d1 = 2.75f;
    if (t < 1.f / d1) {
        return n1 * Pow2(t);
    }
    if (t < (2.f / d1)) {
        return n1 * Pow2(t - (1.5f / d1)) + 0.75f;
    }
    if (t < (2.5f / d1)) {
        return n1 * Pow2(t - (2.25f / d1)) + 0.9375f;
    }
    return n1 * Pow2(t - (2.625f / d1)) + 0.984375f;
}
float EaseInBounce(float t)
{
    return 1.f - EaseOutBounce(1.f - t);
}
float EaseInOutBounce(float t)
{
    if (t < 0.5f) {
        return (1.f - EaseOutBounce(1.f - 2.f * t)) / 2.f;
    }
    return (1.f + EaseOutBounce(2.f * t - 1.f)) / 2.f;
}
float EaseStepStart(float)
{
    return 1.f;
}
float EaseStepEnd(float t)
{
    return t < 1.f ? 0.f : 1.f;
}

// Declares the implementation
#define IMPLEMENT_EASING_CURVE(name)                  \
    float name##EasingCurve::Transform(float t) const \
    {                                                 \
        return Ease##name(t);                         \
    }

IMPLEMENT_EASING_CURVE(Linear)
IMPLEMENT_EASING_CURVE(InQuad)
IMPLEMENT_EASING_CURVE(OutQuad)
IMPLEMENT_EASING_CURVE(InOutQuad)
IMPLEMENT_EASING_CURVE(InCubic)
IMPLEMENT_EASING_CURVE(OutCubic)
IMPLEMENT_EASING_CURVE(InOutCubic)
IMPLEMENT_EASING_CURVE(InSine)
IMPLEMENT_EASING_CURVE(OutSine)
IMPLEMENT_EASING_CURVE(InOutSine)
IMPLEMENT_EASING_CURVE(InQuart)
IMPLEMENT_EASING_CURVE(OutQuart)
IMPLEMENT_EASING_CURVE(InOutQuart)
IMPLEMENT_EASING_CURVE(InQuint)
IMPLEMENT_EASING_CURVE(OutQuint)
IMPLEMENT_EASING_CURVE(InOutQuint)
IMPLEMENT_EASING_CURVE(InExpo)
IMPLEMENT_EASING_CURVE(OutExpo)
IMPLEMENT_EASING_CURVE(InOutExpo)
IMPLEMENT_EASING_CURVE(InCirc)
IMPLEMENT_EASING_CURVE(OutCirc)
IMPLEMENT_EASING_CURVE(InOutCirc)
IMPLEMENT_EASING_CURVE(InBack)
IMPLEMENT_EASING_CURVE(OutBack)
IMPLEMENT_EASING_CURVE(InOutBack)
IMPLEMENT_EASING_CURVE(InElastic)
IMPLEMENT_EASING_CURVE(OutElastic)
IMPLEMENT_EASING_CURVE(InOutElastic)
IMPLEMENT_EASING_CURVE(InBounce)
IMPLEMENT_EASING_CURVE(OutBounce)
IMPLEMENT_EASING_CURVE(InOutBounce)
IMPLEMENT_EASING_CURVE(StepStart)
IMPLEMENT_EASING_CURVE(StepEnd)

} // namespace Easing
} // namespace Curves

META_END_NAMESPACE()
