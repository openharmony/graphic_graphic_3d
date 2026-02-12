/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <memory>

#include "data_type/vec3.h"
#include "data_type/quaternion.h"
#include "data_type/position.h"
#include "data_type/pointer_event.h"
#include "data_type/light.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class DataTypeUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Vec3 Tests

/**
 * @tc.name: Vec3_DefaultConstructor_InitializesToZero
 * @tc.desc: test Vec3 default constructor initializes to zero
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_DefaultConstructor_InitializesToZero, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_DefaultConstructor_InitializesToZero");
    Vec3 vec3;

    EXPECT_FLOAT_EQ(vec3.GetX(), 0.0f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 0.0f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 0.0f);
}

/**
 * @tc.name: Vec3_ParameterizedConstructor_InitializesCorrectly
 * @tc.desc: test Vec3 parameterized constructor initializes correctly
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_ParameterizedConstructor_InitializesCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_ParameterizedConstructor_InitializesCorrectly");
    Vec3 vec3(1.5f, 2.5f, 3.5f);

    EXPECT_FLOAT_EQ(vec3.GetX(), 1.5f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 2.5f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 3.5f);
}

/**
 * @tc.name: Vec3_SetX_UpdatesXValue
 * @tc.desc: test Vec3 SetX updates X value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_SetX_UpdatesXValue, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_SetX_UpdatesXValue");
    Vec3 vec3;
    vec3.SetX(5.0f);

    EXPECT_FLOAT_EQ(vec3.GetX(), 5.0f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 0.0f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 0.0f);
}

/**
 * @tc.name: Vec3_SetY_UpdatesYValue
 * @tc.desc: test Vec3 SetY updates Y value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_SetY_UpdatesYValue, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_SetY_UpdatesYValue");
    Vec3 vec3;
    vec3.SetY(10.0f);

    EXPECT_FLOAT_EQ(vec3.GetX(), 0.0f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 10.0f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 0.0f);
}

/**
 * @tc.name: Vec3_SetZ_UpdatesZValue
 * @tc.desc: test Vec3 SetZ updates Z value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_SetZ_UpdatesZValue, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_SetZ_UpdatesZValue");
    Vec3 vec3;
    vec3.SetZ(15.0f);

    EXPECT_FLOAT_EQ(vec3.GetX(), 0.0f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 0.0f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 15.0f);
}

/**
 * @tc.name: Vec3_SetAllValues_UpdatesAllValues
 * @tc.desc: test Vec3 SetX, SetY, SetZ updates all values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_SetAllValues_UpdatesAllValues, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_SetAllValues_UpdatesAllValues");
    Vec3 vec3;
    vec3.SetX(1.0f);
    vec3.SetY(2.0f);
    vec3.SetZ(3.0f);

    EXPECT_FLOAT_EQ(vec3.GetX(), 1.0f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 2.0f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 3.0f);
}

/**
 * @tc.name: Vec3_NegativeValues_HandledCorrectly
 * @tc.desc: test Vec3 with negative values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_NegativeValues_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_NegativeValues_HandledCorrectly");
    Vec3 vec3(-1.5f, -2.5f, -3.5f);

    EXPECT_FLOAT_EQ(vec3.GetX(), -1.5f);
    EXPECT_FLOAT_EQ(vec3.GetY(), -2.5f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), -3.5f);
}

/**
 * @tc.name: Vec3_ModifyMultipleTimes_MaintainsCorrectValues
 * @tc.desc: test Vec3 modification multiple times
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Vec3_ModifyMultipleTimes_MaintainsCorrectValues, TestSize.Level1)
{
    WIDGET_LOGD("Vec3_ModifyMultipleTimes_MaintainsCorrectValues");
    Vec3 vec3(1.0f, 2.0f, 3.0f);
    vec3.SetX(10.0f);
    vec3.SetY(20.0f);
    vec3.SetX(100.0f);

    EXPECT_FLOAT_EQ(vec3.GetX(), 100.0f);
    EXPECT_FLOAT_EQ(vec3.GetY(), 20.0f);
    EXPECT_FLOAT_EQ(vec3.GetZ(), 3.0f);
}

// Quaternion Tests

/**
 * @tc.name: Quaternion_DefaultConstructor_InitializesToZero
 * @tc.desc: test Quaternion default constructor initializes to zero
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_DefaultConstructor_InitializesToZero, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_DefaultConstructor_InitializesToZero");
    Quaternion quat;

    EXPECT_FLOAT_EQ(quat.GetX(), 0.0f);
    EXPECT_FLOAT_EQ(quat.GetY(), 0.0f);
    EXPECT_FLOAT_EQ(quat.GetZ(), 0.0f);
    EXPECT_FLOAT_EQ(quat.GetW(), 0.0f);
}

/**
 * @tc.name: Quaternion_ParameterizedConstructor_InitializesCorrectly
 * @tc.desc: test Quaternion parameterized constructor initializes correctly
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_ParameterizedConstructor_InitializesCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_ParameterizedConstructor_InitializesCorrectly");
    Quaternion quat(1.0f, 2.0f, 3.0f, 4.0f);

    EXPECT_FLOAT_EQ(quat.GetX(), 1.0f);
    EXPECT_FLOAT_EQ(quat.GetY(), 2.0f);
    EXPECT_FLOAT_EQ(quat.GetZ(), 3.0f);
    EXPECT_FLOAT_EQ(quat.GetW(), 4.0f);
}

/**
 * @tc.name: Quaternion_SetX_UpdatesXValue
 * @tc.desc: test Quaternion SetX updates X value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_SetX_UpdatesXValue, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_SetX_UpdatesXValue");
    Quaternion quat;
    quat.SetX(5.0f);

    EXPECT_FLOAT_EQ(quat.GetX(), 5.0f);
}

/**
 * @tc.name: Quaternion_SetY_UpdatesYValue
 * @tc.desc: test Quaternion SetY updates Y value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_SetY_UpdatesYValue, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_SetY_UpdatesYValue");
    Quaternion quat;
    quat.SetY(10.0f);

    EXPECT_FLOAT_EQ(quat.GetY(), 10.0f);
}

/**
 * @tc.name: Quaternion_SetZ_UpdatesZValue
 * @tc.desc: test Quaternion SetZ updates Z value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_SetZ_UpdatesZValue, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_SetZ_UpdatesZValue");
    Quaternion quat;
    quat.SetZ(15.0f);

    EXPECT_FLOAT_EQ(quat.GetZ(), 15.0f);
}

/**
 * @tc.name: Quaternion_SetW_UpdatesWValue
 * @tc.desc: test Quaternion SetW updates W value
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_SetW_UpdatesWValue, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_SetW_UpdatesWValue");
    Quaternion quat;
    quat.SetW(20.0f);

    EXPECT_FLOAT_EQ(quat.GetW(), 20.0f);
}

/**
 * @tc.name: Quaternion_EqualityOperator_SameValues_ReturnsTrue
 * @tc.desc: test Quaternion equality operator with same values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_EqualityOperator_SameValues_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_EqualityOperator_SameValues_ReturnsTrue");
    Quaternion quat1(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion quat2(1.0f, 2.0f, 3.0f, 4.0f);

    EXPECT_TRUE(quat1 == quat2);
}

/**
 * @tc.name: Quaternion_EqualityOperator_DifferentValues_ReturnsFalse
 * @tc.desc: test Quaternion equality operator with different values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_EqualityOperator_DifferentValues_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_EqualityOperator_DifferentValues_ReturnsFalse");
    Quaternion quat1(1.0f, 2.0f, 3.0f, 4.0f);
    Quaternion quat2(1.0f, 2.0f, 3.0f, 5.0f);

    EXPECT_FALSE(quat1 == quat2);
}

/**
 * @tc.name: Quaternion_NegativeValues_HandledCorrectly
 * @tc.desc: test Quaternion with negative values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Quaternion_NegativeValues_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Quaternion_NegativeValues_HandledCorrectly");
    Quaternion quat(-1.0f, -2.0f, -3.0f, -4.0f);

    EXPECT_FLOAT_EQ(quat.GetX(), -1.0f);
    EXPECT_FLOAT_EQ(quat.GetY(), -2.0f);
    EXPECT_FLOAT_EQ(quat.GetZ(), -3.0f);
    EXPECT_FLOAT_EQ(quat.GetW(), -4.0f);
}

// Position Tests

/**
 * @tc.name: Position_DefaultConstructor_InitializesWithDefaults
 * @tc.desc: test Position default constructor initializes with default values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Position_DefaultConstructor_InitializesWithDefaults, TestSize.Level1)
{
    WIDGET_LOGD("Position_DefaultConstructor_InitializesWithDefaults");
    Position pos;

    EXPECT_FLOAT_EQ(pos.GetX(), 0.0f);
    EXPECT_FLOAT_EQ(pos.GetY(), 0.0f);
    EXPECT_FLOAT_EQ(pos.GetZ(), 4.0f);
    EXPECT_FLOAT_EQ(pos.GetDistance(), 0.0f);
    EXPECT_FALSE(pos.GetIsAngular());
}

/**
 * @tc.name: Position_SetPosition_UpdatesPosition
 * @tc.desc: test Position SetPosition updates position
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Position_SetPosition_UpdatesPosition, TestSize.Level1)
{
    WIDGET_LOGD("Position_SetPosition_UpdatesPosition");
    Position pos;
    Vec3 vec(1.0f, 2.0f, 3.0f);
    pos.SetPosition(vec);

    EXPECT_FLOAT_EQ(pos.GetX(), 1.0f);
    EXPECT_FLOAT_EQ(pos.GetY(), 2.0f);
    EXPECT_FLOAT_EQ(pos.GetZ(), 3.0f);
}

/**
 * @tc.name: Position_SetDistance_UpdatesDistance
 * @tc.desc: test Position SetDistance updates distance
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Position_SetDistance_UpdatesDistance, TestSize.Level1)
{
    WIDGET_LOGD("Position_SetDistance_UpdatesDistance");
    Position pos;
    pos.SetDistance(10.5f);

    EXPECT_FLOAT_EQ(pos.GetDistance(), 10.5f);
}

/**
 * @tc.name: Position_SetIsAngular_UpdatesIsAngular
 * @tc.desc: test Position SetIsAngular updates is angular flag
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Position_SetIsAngular_UpdatesIsAngular, TestSize.Level1)
{
    WIDGET_LOGD("Position_SetIsAngular_UpdatesIsAngular");
    Position pos;
    pos.SetIsAngular(true);

    EXPECT_TRUE(pos.GetIsAngular());
}

/**
 * @tc.name: Position_GetPosition_ReturnsCorrectVec3
 * @tc.desc: test Position GetPosition returns correct Vec3
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Position_GetPosition_ReturnsCorrectVec3, TestSize.Level1)
{
    WIDGET_LOGD("Position_GetPosition_ReturnsCorrectVec3");
    Position pos;
    Vec3 vec(5.0f, 6.0f, 7.0f);
    pos.SetPosition(vec);

    const Vec3& result = pos.GetPosition();
    EXPECT_FLOAT_EQ(result.GetX(), 5.0f);
    EXPECT_FLOAT_EQ(result.GetY(), 6.0f);
    EXPECT_FLOAT_EQ(result.GetZ(), 7.0f);
}

/**
 * @tc.name: Position_MultipleSetOperations_MaintainsLatestValues
 * @tc.desc: test Position multiple set operations maintain latest values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Position_MultipleSetOperations_MaintainsLatestValues, TestSize.Level1)
{
    WIDGET_LOGD("Position_MultipleSetOperations_MaintainsLatestValues");
    Position pos;
    Vec3 vec1(1.0f, 2.0f, 3.0f);
    Vec3 vec2(10.0f, 20.0f, 30.0f);

    pos.SetPosition(vec1);
    pos.SetDistance(5.0f);
    pos.SetIsAngular(true);
    pos.SetPosition(vec2);
    pos.SetDistance(15.0f);

    EXPECT_FLOAT_EQ(pos.GetX(), 10.0f);
    EXPECT_FLOAT_EQ(pos.GetY(), 20.0f);
    EXPECT_FLOAT_EQ(pos.GetZ(), 30.0f);
    EXPECT_FLOAT_EQ(pos.GetDistance(), 15.0f);
    EXPECT_TRUE(pos.GetIsAngular());
}

// PointerEvent Tests

/**
 * @tc.name: PointerEvent_DefaultConstructor_InitializesToReleased
 * @tc.desc: test PointerEvent default constructor initializes to RELEASED
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_DefaultConstructor_InitializesToReleased, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_DefaultConstructor_InitializesToReleased");
    PointerEvent event;

    EXPECT_EQ(event.eventType_, PointerEventType::RELEASED);
    EXPECT_EQ(event.pointerId_, 0);
    EXPECT_EQ(event.buttonIndex_, 0);
    EXPECT_FLOAT_EQ(event.x_, 0.0f);
    EXPECT_FLOAT_EQ(event.y_, 0.0f);
    EXPECT_FLOAT_EQ(event.deltaX_, 0.0f);
    EXPECT_FLOAT_EQ(event.deltaY_, 0.0f);
}

/**
 * @tc.name: PointerEvent_SetPressedEventType_EventTypeIsPressed
 * @tc.desc: test PointerEvent with PRESSED event type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetPressedEventType_EventTypeIsPressed, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetPressedEventType_EventTypeIsPressed");
    PointerEvent event;
    event.eventType_ = PointerEventType::PRESSED;

    EXPECT_EQ(event.eventType_, PointerEventType::PRESSED);
}

/**
 * @tc.name: PointerEvent_SetMovedEventType_EventTypeIsMoved
 * @tc.desc: test PointerEvent with MOVED event type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetMovedEventType_EventTypeIsMoved, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetMovedEventType_EventTypeIsMoved");
    PointerEvent event;
    event.eventType_ = PointerEventType::MOVED;

    EXPECT_EQ(event.eventType_, PointerEventType::MOVED);
}

/**
 * @tc.name: PointerEvent_SetScrollEventType_EventTypeIsScroll
 * @tc.desc: test PointerEvent with SCROLL event type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetScrollEventType_EventTypeIsScroll, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetScrollEventType_EventTypeIsScroll");
    PointerEvent event;
    event.eventType_ = PointerEventType::SCROLL;

    EXPECT_EQ(event.eventType_, PointerEventType::SCROLL);
}

/**
 * @tc.name: PointerEvent_SetCancelledEventType_EventTypeIsCancelled
 * @tc.desc: test PointerEvent with CANCELLED event type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetCancelledEventType_EventTypeIsCancelled, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetCancelledEventType_EventTypeIsCancelled");
    PointerEvent event;
    event.eventType_ = PointerEventType::CANCELLED;

    EXPECT_EQ(event.eventType_, PointerEventType::CANCELLED);
}

/**
 * @tc.name: PointerEvent_SetPointerId_UpdatesPointerId
 * @tc.desc: test PointerEvent set pointer id
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetPointerId_UpdatesPointerId, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetPointerId_UpdatesPointerId");
    PointerEvent event;
    event.pointerId_ = 5;

    EXPECT_EQ(event.pointerId_, 5);
}

/**
 * @tc.name: PointerEvent_SetButtonIndex_UpdatesButtonIndex
 * @tc.desc: test PointerEvent set button index
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetButtonIndex_UpdatesButtonIndex, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetButtonIndex_UpdatesButtonIndex");
    PointerEvent event;
    event.buttonIndex_ = 2;

    EXPECT_EQ(event.buttonIndex_, 2);
}

/**
 * @tc.name: PointerEvent_SetCoordinates_UpdatesCoordinates
 * @tc.desc: test PointerEvent set x and y coordinates
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetCoordinates_UpdatesCoordinates, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetCoordinates_UpdatesCoordinates");
    PointerEvent event;
    event.x_ = 100.5f;
    event.y_ = 200.5f;

    EXPECT_FLOAT_EQ(event.x_, 100.5f);
    EXPECT_FLOAT_EQ(event.y_, 200.5f);
}

/**
 * @tc.name: PointerEvent_SetDelta_UpdatesDelta
 * @tc.desc: test PointerEvent set delta values
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_SetDelta_UpdatesDelta, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_SetDelta_UpdatesDelta");
    PointerEvent event;
    event.deltaX_ = 10.5f;
    event.deltaY_ = 20.5f;

    EXPECT_FLOAT_EQ(event.deltaX_, 10.5f);
    EXPECT_FLOAT_EQ(event.deltaY_, 20.5f);
}

/**
 * @tc.name: PointerEvent_NegativeCoordinates_HandledCorrectly
 * @tc.desc: test PointerEvent with negative coordinates
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_NegativeCoordinates_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_NegativeCoordinates_HandledCorrectly");
    PointerEvent event;
    event.x_ = -50.0f;
    event.y_ = -100.0f;

    EXPECT_FLOAT_EQ(event.x_, -50.0f);
    EXPECT_FLOAT_EQ(event.y_, -100.0f);
}

/**
 * @tc.name: PointerEvent_FullEventInitialization_AllFieldsSet
 * @tc.desc: test PointerEvent with all fields set
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, PointerEvent_FullEventInitialization_AllFieldsSet, TestSize.Level1)
{
    WIDGET_LOGD("PointerEvent_FullEventInitialization_AllFieldsSet");
    PointerEvent event;
    event.eventType_ = PointerEventType::MOVED;
    event.pointerId_ = 1;
    event.buttonIndex_ = 0;
    event.x_ = 150.0f;
    event.y_ = 250.0f;
    event.deltaX_ = 5.0f;
    event.deltaY_ = 10.0f;

    EXPECT_EQ(event.eventType_, PointerEventType::MOVED);
    EXPECT_EQ(event.pointerId_, 1);
    EXPECT_EQ(event.buttonIndex_, 0);
    EXPECT_FLOAT_EQ(event.x_, 150.0f);
    EXPECT_FLOAT_EQ(event.y_, 250.0f);
    EXPECT_FLOAT_EQ(event.deltaX_, 5.0f);
    EXPECT_FLOAT_EQ(event.deltaY_, 10.0f);
}

// Light Tests

/**
 * @tc.name: Light_Constructor_InitializesAllFields
 * @tc.desc: test Light constructor initializes all fields
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_Constructor_InitializesAllFields, TestSize.Level1)
{
    WIDGET_LOGD("Light_Constructor_InitializesAllFields");
    Vec3 color(1.0f, 0.8f, 0.6f);
    Position pos;
    Quaternion rot(0.0f, 0.0f, 0.0f, 1.0f);

    Light light(LightType::DIRECTIONAL, color, 5.0f, true, pos, rot);

    EXPECT_EQ(light.GetLightType(), LightType::DIRECTIONAL);
    EXPECT_FLOAT_EQ(light.GetLightColor().GetX(), 1.0f);
    EXPECT_FLOAT_EQ(light.GetLightColor().GetY(), 0.8f);
    EXPECT_FLOAT_EQ(light.GetLightColor().GetZ(), 0.6f);
    EXPECT_FLOAT_EQ(light.GetLightIntensity(), 5.0f);
    EXPECT_TRUE(light.GetLightShadow());
}

/**
 * @tc.name: Light_SetLightType_UpdatesType
 * @tc.desc: test Light SetLightType updates type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SetLightType_UpdatesType, TestSize.Level1)
{
    WIDGET_LOGD("Light_SetLightType_UpdatesType");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Position pos;
    Quaternion rot;

    Light light(LightType::DIRECTIONAL, color, 1.0f, false, pos, rot);
    light.SetLightType(LightType::POINT);

    EXPECT_EQ(light.GetLightType(), LightType::POINT);
}

/**
 * @tc.name: Light_SetColor_UpdatesColor
 * @tc.desc: test Light SetColor updates color
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SetColor_UpdatesColor, TestSize.Level1)
{
    WIDGET_LOGD("Light_SetColor_UpdatesColor");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Vec3 newColor(0.5f, 0.3f, 0.7f);
    Position pos;
    Quaternion rot;

    Light light(LightType::DIRECTIONAL, color, 1.0f, false, pos, rot);
    light.SetColor(newColor);

    EXPECT_FLOAT_EQ(light.GetLightColor().GetX(), 0.5f);
    EXPECT_FLOAT_EQ(light.GetLightColor().GetY(), 0.3f);
    EXPECT_FLOAT_EQ(light.GetLightColor().GetZ(), 0.7f);
}

/**
 * @tc.name: Light_SetIntensity_UpdatesIntensity
 * @tc.desc: test Light SetIntensity updates intensity
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SetIntensity_UpdatesIntensity, TestSize.Level1)
{
    WIDGET_LOGD("Light_SetIntensity_UpdatesIntensity");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Position pos;
    Quaternion rot;

    Light light(LightType::DIRECTIONAL, color, 1.0f, false, pos, rot);
    light.SetIntensity(3.5f);

    EXPECT_FLOAT_EQ(light.GetLightIntensity(), 3.5f);
}

/**
 * @tc.name: Light_SetLightShadow_UpdatesShadow
 * @tc.desc: test Light SetLightShadow updates shadow flag
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SetLightShadow_UpdatesShadow, TestSize.Level1)
{
    WIDGET_LOGD("Light_SetLightShadow_UpdatesShadow");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Position pos;
    Quaternion rot;

    Light light(LightType::DIRECTIONAL, color, 1.0f, false, pos, rot);
    light.SetLightShadow(true);

    EXPECT_TRUE(light.GetLightShadow());
}

/**
 * @tc.name: Light_SetPosition_UpdatesPosition
 * @tc.desc: test Light SetPosition updates position
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SetPosition_UpdatesPosition, TestSize.Level1)
{
    WIDGET_LOGD("Light_SetPosition_UpdatesPosition");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Position pos;
    Vec3 lightPos(10.0f, 20.0f, 30.0f);
    Position newLightPos;
    newLightPos.SetPosition(lightPos);
    Quaternion rot;

    Light light(LightType::POINT, color, 1.0f, false, pos, rot);
    light.SetPosition(newLightPos);

    EXPECT_FLOAT_EQ(light.GetPosition().GetX(), 10.0f);
    EXPECT_FLOAT_EQ(light.GetPosition().GetY(), 20.0f);
    EXPECT_FLOAT_EQ(light.GetPosition().GetZ(), 30.0f);
}

/**
 * @tc.name: Light_SetRotation_UpdatesRotation
 * @tc.desc: test Light SetRotation updates rotation
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SetRotation_UpdatesRotation, TestSize.Level1)
{
    WIDGET_LOGD("Light_SetRotation_UpdatesRotation");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Position pos;
    Quaternion rot(0.0f, 0.0f, 0.0f, 1.0f);
    Quaternion newRot(0.5f, 0.5f, 0.5f, 0.5f);

    Light light(LightType::SPOT, color, 1.0f, false, pos, rot);
    light.SetRotation(newRot);

    EXPECT_FLOAT_EQ(light.GetRotation().GetX(), 0.5f);
    EXPECT_FLOAT_EQ(light.GetRotation().GetY(), 0.5f);
    EXPECT_FLOAT_EQ(light.GetRotation().GetZ(), 0.5f);
    EXPECT_FLOAT_EQ(light.GetRotation().GetW(), 0.5f);
}

/**
 * @tc.name: Light_DirectionalLightType_HandledCorrectly
 * @tc.desc: test Light with DIRECTIONAL type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_DirectionalLightType_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Light_DirectionalLightType_HandledCorrectly");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Position pos;
    Quaternion rot;

    Light light(LightType::DIRECTIONAL, color, 2.0f, false, pos, rot);

    EXPECT_EQ(light.GetLightType(), LightType::DIRECTIONAL);
    EXPECT_FLOAT_EQ(light.GetLightIntensity(), 2.0f);
}

/**
 * @tc.name: Light_PointLightType_HandledCorrectly
 * @tc.desc: test Light with POINT type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_PointLightType_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Light_PointLightType_HandledCorrectly");
    Vec3 color(1.0f, 1.0f, 1.0f);
    Vec3 lightPos(0.0f, 5.0f, 0.0f);
    Position pos;
    pos.SetPosition(lightPos);
    Quaternion rot;

    Light light(LightType::POINT, color, 1.5f, true, pos, rot);

    EXPECT_EQ(light.GetLightType(), LightType::POINT);
    EXPECT_FLOAT_EQ(light.GetPosition().GetY(), 5.0f);
    EXPECT_TRUE(light.GetLightShadow());
}

/**
 * @tc.name: Light_SpotLightType_HandledCorrectly
 * @tc.desc: test Light with SPOT type
 * @tc.type: FUNC
 */
HWTEST_F(DataTypeUT, Light_SpotLightType_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Light_SpotLightType_HandledCorrectly");
    Vec3 color(1.0f, 0.9f, 0.8f);
    Position pos;
    Quaternion rot;

    Light light(LightType::SPOT, color, 3.0f, false, pos, rot);

    EXPECT_EQ(light.GetLightType(), LightType::SPOT);
    EXPECT_FLOAT_EQ(light.GetLightIntensity(), 3.0f);
    EXPECT_FALSE(light.GetLightShadow());
}

} // namespace OHOS::Render3D
