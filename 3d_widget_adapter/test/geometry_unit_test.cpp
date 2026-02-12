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

#include "data_type/geometry/geometry.h"
#include "data_type/geometry/cube.h"
#include "data_type/geometry/sphere.h"
#include "data_type/geometry/cone.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class GeometryUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Base Geometry Tests

/**
 * @tc.name: Geometry_GetName_ReturnsCorrectName
 * @tc.desc: test Geometry GetName returns correct name
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_GetName_ReturnsCorrectName, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_GetName_ReturnsCorrectName");
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Cube cube("test_cube", 1.0f, 2.0f, 3.0f, pos);

    EXPECT_EQ(cube.GetName(), "test_cube");
}

/**
 * @tc.name: Geometry_GetPosition_ReturnsCorrectPosition
 * @tc.desc: test Geometry GetPosition returns correct position
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_GetPosition_ReturnsCorrectPosition, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_GetPosition_ReturnsCorrectPosition");
    Vec3 pos(5.0f, 10.0f, 15.0f);
    Cube cube("cube", 1.0f, 1.0f, 1.0f, pos);

    const Vec3& result = cube.GetPosition();
    EXPECT_FLOAT_EQ(result.GetX(), 5.0f);
    EXPECT_FLOAT_EQ(result.GetY(), 10.0f);
    EXPECT_FLOAT_EQ(result.GetZ(), 15.0f);
}

/**
 * @tc.name: Geometry_CastShadows_DefaultIsFalse
 * @tc.desc: test Geometry CastShadows default is false
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_CastShadows_DefaultIsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_CastShadows_DefaultIsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("cube", 1.0f, 1.0f, 1.0f, pos);

    EXPECT_FALSE(cube.CastShadows());
}

/**
 * @tc.name: Geometry_ReceiveShadows_DefaultIsFalse
 * @tc.desc: test Geometry ReceiveShadows default is false
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_ReceiveShadows_DefaultIsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_ReceiveShadows_DefaultIsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("cube", 1.0f, 1.0f, 1.0f, pos);

    EXPECT_FALSE(cube.ReceiveShadows());
}

/**
 * @tc.name: Geometry_PositionEquals_SamePosition_ReturnsTrue
 * @tc.desc: test Geometry PositionEquals with same position
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_PositionEquals_SamePosition_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_PositionEquals_SamePosition_ReturnsTrue");
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Cube cube1("cube1", 1.0f, 1.0f, 1.0f, pos);
    Cube cube2("cube2", 2.0f, 2.0f, 2.0f, pos);

    EXPECT_TRUE(cube1.PositionEquals(cube2));
}

/**
 * @tc.name: Geometry_PositionEquals_DifferentPosition_ReturnsFalse
 * @tc.desc: test Geometry PositionEquals with different position
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_PositionEquals_DifferentPosition_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_PositionEquals_DifferentPosition_ReturnsFalse");
    Vec3 pos1(1.0f, 2.0f, 3.0f);
    Vec3 pos2(4.0f, 5.0f, 6.0f);
    Cube cube1("cube1", 1.0f, 1.0f, 1.0f, pos1);
    Cube cube2("cube2", 1.0f, 1.0f, 1.0f, pos2);

    EXPECT_FALSE(cube1.PositionEquals(cube2));
}

/**
 * @tc.name: Geometry_Equals_SameGeometry_ReturnsTrue
 * @tc.desc: test Geometry Equals with same geometry
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_Equals_SameGeometry_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_Equals_SameGeometry_ReturnsTrue");
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Cube cube1("cube", 1.0f, 2.0f, 3.0f, pos);
    Cube cube2("cube", 1.0f, 2.0f, 3.0f, pos);

    EXPECT_TRUE(cube1.Equals(cube2));
}

/**
 * @tc.name: Geometry_Equals_DifferentName_ReturnsFalse
 * @tc.desc: test Geometry Equals with different name
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_Equals_DifferentName_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_Equals_DifferentName_ReturnsFalse");
    Vec3 pos(1.0f, 2.0f, 3.0f);
    Cube cube1("cube1", 1.0f, 2.0f, 3.0f, pos);
    Cube cube2("cube2", 1.0f, 2.0f, 3.0f, pos);

    EXPECT_FALSE(cube1.Equals(cube2));
}

// Cube Tests

/**
 * @tc.name: Cube_Constructor_InitializesDimensions
 * @tc.desc: test Cube constructor initializes dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_Constructor_InitializesDimensions, TestSize.Level1)
{
    WIDGET_LOGD("Cube_Constructor_InitializesDimensions");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("cube", 5.0f, 10.0f, 15.0f, pos);

    EXPECT_FLOAT_EQ(cube.GetWidth(), 5.0f);
    EXPECT_FLOAT_EQ(cube.GetHeight(), 10.0f);
    EXPECT_FLOAT_EQ(cube.GetDepth(), 15.0f);
}

/**
 * @tc.name: Cube_GetType_ReturnsCubeType
 * @tc.desc: test Cube GetType returns CUBE type
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_GetType_ReturnsCubeType, TestSize.Level1)
{
    WIDGET_LOGD("Cube_GetType_ReturnsCubeType");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("cube", 1.0f, 1.0f, 1.0f, pos);

    EXPECT_EQ(cube.GetType(), GeometryType::CUBE);
}

/**
 * @tc.name: Cube_Equals_SameDimensions_ReturnsTrue
 * @tc.desc: test Cube Equals with same dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_Equals_SameDimensions_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("Cube_Equals_SameDimensions_ReturnsTrue");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube1("cube", 1.0f, 2.0f, 3.0f, pos);
    Cube cube2("cube", 1.0f, 2.0f, 3.0f, pos);

    EXPECT_TRUE(cube1.Equals(cube2));
}

/**
 * @tc.name: Cube_Equals_DifferentWidth_ReturnsFalse
 * @tc.desc: test Cube Equals with different width
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_Equals_DifferentWidth_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Cube_Equals_DifferentWidth_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube1("cube", 1.0f, 2.0f, 3.0f, pos);
    Cube cube2("cube", 5.0f, 2.0f, 3.0f, pos);

    EXPECT_FALSE(cube1.Equals(cube2));
}

/**
 * @tc.name: Cube_Equals_DifferentHeight_ReturnsFalse
 * @tc.desc: test Cube Equals with different height
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_Equals_DifferentHeight_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Cube_Equals_DifferentHeight_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube1("cube", 1.0f, 2.0f, 3.0f, pos);
    Cube cube2("cube", 1.0f, 5.0f, 3.0f, pos);

    EXPECT_FALSE(cube1.Equals(cube2));
}

/**
 * @tc.name: Cube_Equals_DifferentDepth_ReturnsFalse
 * @tc.desc: test Cube Equals with different depth
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_Equals_DifferentDepth_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Cube_Equals_DifferentDepth_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube1("cube", 1.0f, 2.0f, 3.0f, pos);
    Cube cube2("cube", 1.0f, 2.0f, 5.0f, pos);

    EXPECT_FALSE(cube1.Equals(cube2));
}

/**
 * @tc.name: Cube_NegativeDimensions_HandledCorrectly
 * @tc.desc: test Cube with negative dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cube_NegativeDimensions_HandledCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("Cube_NegativeDimensions_HandledCorrectly");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("cube", -1.0f, -2.0f, -3.0f, pos);

    EXPECT_FLOAT_EQ(cube.GetWidth(), -1.0f);
    EXPECT_FLOAT_EQ(cube.GetHeight(), -2.0f);
    EXPECT_FLOAT_EQ(cube.GetDepth(), -3.0f);
}

// Sphere Tests

/**
 * @tc.name: Sphere_Constructor_InitializesDimensions
 * @tc.desc: test Sphere constructor initializes dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Sphere_Constructor_InitializesDimensions, TestSize.Level1)
{
    WIDGET_LOGD("Sphere_Constructor_InitializesDimensions");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Sphere sphere("sphere", 5.0f, 10.0f, 20.0f, pos);

    EXPECT_FLOAT_EQ(sphere.GetRadius(), 5.0f);
    EXPECT_FLOAT_EQ(sphere.GetRings(), 10.0f);
    EXPECT_FLOAT_EQ(sphere.GetSectors(), 20.0f);
}

/**
 * @tc.name: Sphere_GetType_ReturnsSphereType
 * @tc.desc: test Sphere GetType returns SPHARE type
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Sphere_GetType_ReturnsSphereType, TestSize.Level1)
{
    WIDGET_LOGD("Sphere_GetType_ReturnsSphereType");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Sphere sphere("sphere", 1.0f, 10.0f, 20.0f, pos);

    EXPECT_EQ(sphere.GetType(), GeometryType::SPHARE);
}

/**
 * @tc.name: Sphere_Equals_SameDimensions_ReturnsTrue
 * @tc.desc: test Sphere Equals with same dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Sphere_Equals_SameDimensions_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("Sphere_Equals_SameDimensions_ReturnsTrue");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Sphere sphere1("sphere", 5.0f, 10.0f, 20.0f, pos);
    Sphere sphere2("sphere", 5.0f, 10.0f, 20.0f, pos);

    EXPECT_TRUE(sphere1.Equals(sphere2));
}

/**
 * @tc.name: Sphere_Equals_DifferentRadius_ReturnsFalse
 * @tc.desc: test Sphere Equals with different radius
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Sphere_Equals_DifferentRadius_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Sphere_Equals_DifferentRadius_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Sphere sphere1("sphere", 5.0f, 10.0f, 20.0f, pos);
    Sphere sphere2("sphere", 10.0f, 10.0f, 20.0f, pos);

    EXPECT_FALSE(sphere1.Equals(sphere2));
}

/**
 * @tc.name: Sphere_Equals_DifferentRings_ReturnsFalse
 * @tc.desc: test Sphere Equals with different rings
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Sphere_Equals_DifferentRings_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Sphere_Equals_DifferentRings_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Sphere sphere1("sphere", 5.0f, 10.0f, 20.0f, pos);
    Sphere sphere2("sphere", 5.0f, 20.0f, 20.0f, pos);

    EXPECT_FALSE(sphere1.Equals(sphere2));
}

/**
 * @tc.name: Sphere_Equals_DifferentSectors_ReturnsFalse
 * @tc.desc: test Sphere Equals with different sectors
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Sphere_Equals_DifferentSectors_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Sphere_Equals_DifferentSectors_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Sphere sphere1("sphere", 5.0f, 10.0f, 20.0f, pos);
    Sphere sphere2("sphere", 5.0f, 10.0f, 30.0f, pos);

    EXPECT_FALSE(sphere1.Equals(sphere2));
}

// Cone Tests

/**
 * @tc.name: Cone_Constructor_InitializesDimensions
 * @tc.desc: test Cone constructor initializes dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cone_Constructor_InitializesDimensions, TestSize.Level1)
{
    WIDGET_LOGD("Cone_Constructor_InitializesDimensions");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cone cone("cone", 5.0f, 10.0f, 20.0f, pos);

    EXPECT_FLOAT_EQ(cone.GetRadius(), 5.0f);
    EXPECT_FLOAT_EQ(cone.GetLength(), 10.0f);
    EXPECT_FLOAT_EQ(cone.GetSectors(), 20.0f);
}

/**
 * @tc.name: Cone_GetType_ReturnsConeType
 * @tc.desc: test Cone GetType returns CONE type
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cone_GetType_ReturnsConeType, TestSize.Level1)
{
    WIDGET_LOGD("Cone_GetType_ReturnsConeType");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cone cone("cone", 5.0f, 10.0f, 20.0f, pos);

    EXPECT_EQ(cone.GetType(), GeometryType::CONE);
}

/**
 * @tc.name: Cone_Equals_SameDimensions_ReturnsTrue
 * @tc.desc: test Cone Equals with same dimensions
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cone_Equals_SameDimensions_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("Cone_Equals_SameDimensions_ReturnsTrue");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cone cone1("cone", 5.0f, 10.0f, 20.0f, pos);
    Cone cone2("cone", 5.0f, 10.0f, 20.0f, pos);

    EXPECT_TRUE(cone1.Equals(cone2));
}

/**
 * @tc.name: Cone_Equals_DifferentRadius_ReturnsFalse
 * @tc.desc: test Cone Equals with different radius
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cone_Equals_DifferentRadius_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Cone_Equals_DifferentRadius_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cone cone1("cone", 5.0f, 10.0f, 20.0f, pos);
    Cone cone2("cone", 10.0f, 10.0f, 20.0f, pos);

    EXPECT_FALSE(cone1.Equals(cone2));
}

/**
 * @tc.name: Cone_Equals_DifferentLength_ReturnsFalse
 * @tc.desc: test Cone Equals with different length
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cone_Equals_DifferentLength_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Cone_Equals_DifferentLength_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cone cone1("cone", 5.0f, 10.0f, 20.0f, pos);
    Cone cone2("cone", 5.0f, 20.0f, 20.0f, pos);

    EXPECT_FALSE(cone1.Equals(cone2));
}

/**
 * @tc.name: Cone_Equals_DifferentSectors_ReturnsFalse
 * @tc.desc: test Cone Equals with different sectors
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Cone_Equals_DifferentSectors_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Cone_Equals_DifferentSectors_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cone cone1("cone", 5.0f, 10.0f, 20.0f, pos);
    Cone cone2("cone", 5.0f, 10.0f, 30.0f, pos);

    EXPECT_FALSE(cone1.Equals(cone2));
}

// Cross Geometry Type Tests

/**
 * @tc.name: Geometry_Equals_DifferentTypes_ReturnsFalse
 * @tc.desc: test Geometry Equals with different types
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_Equals_DifferentTypes_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_Equals_DifferentTypes_ReturnsFalse");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("geometry", 1.0f, 1.0f, 1.0f, pos);
    Sphere sphere("geometry", 1.0f, 10.0f, 20.0f, pos);

    EXPECT_FALSE(cube.Equals(sphere));
}

/**
 * @tc.name: Geometry_GetType_DifferentForEachType
 * @tc.desc: test Geometry GetType returns different values for each type
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_GetType_DifferentForEachType, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_GetType_DifferentForEachType");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    Cube cube("cube", 1.0f, 1.0f, 1.0f, pos);
    Sphere sphere("sphere", 1.0f, 10.0f, 20.0f, pos);
    Cone cone("cone", 1.0f, 1.0f, 10.0f, pos);

    EXPECT_EQ(cube.GetType(), GeometryType::CUBE);
    EXPECT_EQ(sphere.GetType(), GeometryType::SPHARE);
    EXPECT_EQ(cone.GetType(), GeometryType::CONE);

    EXPECT_NE(cube.GetType(), sphere.GetType());
    EXPECT_NE(sphere.GetType(), cone.GetType());
    EXPECT_NE(cone.GetType(), cube.GetType());
}

/**
 * @tc.name: Geometry_VectorOfGeometries_AllTypes
 * @tc.desc: test storing different geometry types in vector
 * @tc.type: FUNC
 */
HWTEST_F(GeometryUT, Geometry_VectorOfGeometries_AllTypes, TestSize.Level1)
{
    WIDGET_LOGD("Geometry_VectorOfGeometries_AllTypes");
    Vec3 pos(0.0f, 0.0f, 0.0f);
    std::vector<std::shared_ptr<Geometry>> geometries;

    geometries.push_back(std::make_shared<Cube>("cube", 1.0f, 1.0f, 1.0f, pos));
    geometries.push_back(std::make_shared<Sphere>("sphere", 1.0f, 10.0f, 20.0f, pos));
    geometries.push_back(std::make_shared<Cone>("cone", 1.0f, 1.0f, 10.0f, pos));

    EXPECT_EQ(geometries.size(), 3u);
    EXPECT_EQ(geometries[0]->GetType(), GeometryType::CUBE);
    EXPECT_EQ(geometries[1]->GetType(), GeometryType::SPHARE);
    EXPECT_EQ(geometries[2]->GetType(), GeometryType::CONE);
}

} // namespace OHOS::Render3D
