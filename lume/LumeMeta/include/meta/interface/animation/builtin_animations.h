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

#ifndef META_INTERFACE_BUILTIN_ANIMATIONS_H
#define META_INTERFACE_BUILTIN_ANIMATIONS_H

#include <meta/base/namespace.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

/** Animation controller */
META_REGISTER_CLASS(AnimationController, "32ccf293-e684-46e3-b733-85f9bbcc703c", ObjectCategoryBits::NO_CATEGORY)

/** Animation containers */
META_REGISTER_CLASS(SequentialAnimation, "4fa1577f-2096-49b9-8327-2b7f2d9a6363", ObjectCategoryBits::ANIMATION)
META_REGISTER_CLASS(ParallelAnimation, "09a6805a-cfac-4ea5-b39d-dbc07cb05d79", ObjectCategoryBits::ANIMATION)

/** Property animation types */
META_REGISTER_CLASS(PropertyAnimation, "f28ac10f-9410-46e6-bf1c-613557d2d391", ObjectCategoryBits::ANIMATION)
META_REGISTER_CLASS(KeyframeAnimation, "78d13777-10b4-43de-925d-c9ad7c28874a", ObjectCategoryBits::ANIMATION)
META_REGISTER_CLASS(TrackAnimation, "acdcc754-8d97-4a4d-8885-67fb8e35ca12", ObjectCategoryBits::ANIMATION)

/** Animation modifiers */
META_REGISTER_CLASS(
    LoopAnimationModifier, "b16ff5f2-8b37-476b-ab4e-84db1fc8e5f2", ObjectCategoryBits::ANIMATION_MODIFIER)
META_REGISTER_CLASS(
    SpeedAnimationModifier, "f265ac2b-edc7-4d80-bffb-1cfa290fd118", ObjectCategoryBits::ANIMATION_MODIFIER)
META_REGISTER_CLASS(
    ReverseAnimationModifier, "274784a4-bdf5-4cb2-9468-d17299364f17", ObjectCategoryBits::ANIMATION_MODIFIER)

/** Interpolator types */
META_REGISTER_SINGLETON_CLASS(FloatInterpolator, "a419ec55-de61-44ca-b38c-c3e90c20ae43", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(DoubleInterpolator, "7af8c81d-a7ed-4daa-ad6e-5cd50d9146bf", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UInt8Interpolator, "9d49b47e-4070-49e8-b6aa-53c7de5c5467", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UInt16Interpolator, "0c01dda0-effb-48c4-8913-7f207da453b9", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UInt32Interpolator, "3961c6de-96c1-49b1-8cc2-778816a2a560", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UInt64Interpolator, "25d90077-1c4b-4637-9409-774a63b569f0", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Int8Interpolator, "a9615d85-20d6-4685-a530-1459b896cc93", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Int16Interpolator, "b311b907-7082-41c6-96a5-0bc7d50ff994", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Int32Interpolator, "6d0ffdb2-5067-491d-8b74-8345e8633b93", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Int64Interpolator, "4872d903-a5e3-4695-b111-7799f888a7ed", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Vec2Interpolator, "94e0b14c-b278-4d3e-896e-dff0f64c539c", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Vec3Interpolator, "bccaa813-f363-481f-956f-f6c4189ec856", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(Vec4Interpolator, "bef3ed39-f685-4c21-bd86-77bde62a03b7", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UVec2Interpolator, "14f191e7-0e1c-4066-819f-c4579c7a93b3", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UVec3Interpolator, "534682fe-09c1-4f01-846c-f22d2b895dfc", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(UVec4Interpolator, "12e8ba49-5077-45a4-8753-4e7c451a00f1", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(IVec2Interpolator, "3cc6de25-323c-4671-aad4-23e2a782fdde", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(IVec3Interpolator, "4fd8ad06-bac7-4dda-bd91-e96148a7a29c", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(IVec4Interpolator, "437fe435-3d71-4ef2-ac37-b422ad00ced9", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(QuatInterpolator, "21b7a73f-3389-4107-8c04-902b0768d7cf", ObjectCategoryBits::INTERNAL)
META_REGISTER_SINGLETON_CLASS(DefaultInterpolator, "7011a72b-ce36-4f17-a017-daeef39cf57c", ObjectCategoryBits::INTERNAL)

/** Easing curves */
META_REGISTER_SINGLETON_CLASS(LinearEasingCurve, "849f98a4-03dd-4b12-ae59-72a5eb6914c2", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InQuadEasingCurve, "8cdf008d-5266-4877-8ff7-ceb19997e485", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutQuadEasingCurve, "8df10f87-b508-4727-a97a-66e6c348346d", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutQuadEasingCurve, "25412788-ee21-4665-adfb-ad5d4deed592", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InCubicEasingCurve, "efe63f17-ad14-49a9-8083-9a963eab2ec6", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutCubicEasingCurve, "2ff9d5d1-0de9-40ef-8222-bb841245a027", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutCubicEasingCurve, "cd1858f6-0bbd-420e-b72e-baaf5bec333c", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InSineEasingCurve, "784695ed-1e31-4829-b780-d837152d35fc", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutSineEasingCurve, "baeaa8b2-e69b-472a-a161-76d3c04bd6b6", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutSineEasingCurve, "e183b7af-949d-4a78-92ba-3fe3b4e5cfec", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InQuartEasingCurve, "edf9877d-1486-4342-b36c-59bdda6994b5", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutQuartEasingCurve, "ed0affff-cc84-4863-8f44-5d20fdc4ea86", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutQuartEasingCurve, "9f858bfc-e11a-4e69-a02f-ed8ccc6249f5", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InQuintEasingCurve, "34fc80e8-9e62-478e-8f7e-72a8df7a278c", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutQuintEasingCurve, "d28e6874-e7f2-414a-ae84-c38b016279af", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutQuintEasingCurve, "3f25d8fb-d0e6-44d6-a6dc-00eee4c7f642", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InExpoEasingCurve, "ddb621d4-de30-42c3-acb3-5d755237fc5a", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutExpoEasingCurve, "65038ea5-5121-4b33-bbc4-8961c556b7e4", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutExpoEasingCurve, "55c64ecc-4f13-48c6-bacc-be69c5fab32d", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InCircEasingCurve, "79dfe129-6930-40c8-8fb1-b5c4375b65c7", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutCircEasingCurve, "3a2f76ff-f966-4828-b4ef-d66a5f502801", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutCircEasingCurve, "6f03058d-389f-4fb6-8c77-5bb0e9a84ec0", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InBackEasingCurve, "0a4d2948-ccb4-48d4-8d1b-61d1bf65a2f6", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutBackEasingCurve, "3651feb8-46cc-41d0-bf6b-4f692f07206c", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutBackEasingCurve, "9995161e-bc6e-45d0-9b47-2ca624ffd323", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InElasticEasingCurve, "2f0069d9-0cd5-479a-89ef-bf1dba8fba33", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutElasticEasingCurve, "e67e747b-d1a0-4a88-b443-32866437e787", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(
    InOutElasticEasingCurve, "9be248cb-c778-4ae8-b4f4-9035ef3ba142", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InBounceEasingCurve, "7bbb09aa-85b6-4082-8d4a-5ccbe30e6cb7", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(OutBounceEasingCurve, "72375660-1277-434e-9bb7-e4621c3ec855", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(InOutBounceEasingCurve, "6fcad301-dace-48a8-8f53-cbfd7f0b2480", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(StepStartEasingCurve, "293aeff7-aa4f-403c-b284-aa3358a91469", ObjectCategoryBits::CURVE)
META_REGISTER_SINGLETON_CLASS(StepEndEasingCurve, "3b1f2d61-f9e5-4d8b-a041-fe216d265628", ObjectCategoryBits::CURVE)

META_REGISTER_CLASS(CubicBezierEasingCurve, "72e786e9-3c8f-404a-89f7-ee49144d3a03", ObjectCategoryBits::CURVE)

META_END_NAMESPACE()

#endif
