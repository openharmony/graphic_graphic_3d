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

#include <base/math/matrix.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#include "util/frustum_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using BASE_NS::Math::Mat4X4;
using BASE_NS::Math::Vec3;
using BASE_NS::Math::Vec4;

/**
 * @tc.name: FrustumUtil_CreateFrustum_IdentityMatrix
 * @tc.desc: Tests frustum creation from identity matrix [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_CreateFrustum_IdentityMatrix, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;
    Frustum frustum = frustumUtil.CreateFrustum(Base::Math::IDENTITY_4X4);

    // Check that all planes are normalized (magnitude should be 1.0)
    for (uint32_t i = 0; i < Frustum::PLANE_COUNT; ++i) {
        const Vec4& plane = frustum.planes[i];
        float magnitude = Math::Magnitude(Vec3(plane.x, plane.y, plane.z));
        EXPECT_FLOAT_EQ(1.0f, magnitude);
    }
}

/**
 * @tc.name: FrustumUtil_CreateFrustum_PerspectiveMatrix
 * @tc.desc: Tests frustum creation from perspective projection matrix [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_CreateFrustum_PerspectiveMatrix, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    // Create a simple perspective projection matrix
    const float fov = 1.57f; // ~90 degrees
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);

    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Check that all planes exist and are normalized
    for (uint32_t i = 0; i < Frustum::PLANE_COUNT; ++i) {
        const Vec4& plane = frustum.planes[i];
        float magnitude = Math::Magnitude(Vec3(plane.x, plane.y, plane.z));
        EXPECT_FLOAT_EQ(1.0f, magnitude);
    }
}

/**
 * @tc.name: FrustumUtil_CreateFrustum_OrthographicMatrix
 * @tc.desc: Tests frustum creation from orthographic projection matrix [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_CreateFrustum_OrthographicMatrix, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    // Create an orthographic projection matrix
    const float left = -10.0f;
    const float right = 10.0f;
    const float bottom = -10.0f;
    const float top = 10.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::OrthoRhNo(left, right, bottom, top, nearPlane, farPlane);

    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Check that all planes exist and are normalized
    for (uint32_t i = 0; i < Frustum::PLANE_COUNT; ++i) {
        const Vec4& plane = frustum.planes[i];
        float magnitude = Math::Magnitude(Vec3(plane.x, plane.y, plane.z));
        EXPECT_FLOAT_EQ(1.0f, magnitude);
    }
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_OriginInside
 * @tc.desc: Tests sphere at origin inside frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_OriginInside, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    // Create a simple perspective projection matrix
    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere at origin should be inside
    Vec3 position(0.0f, 0.0f, -10.0f);
    float radius = 1.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SphereBehind
 * @tc.desc: Tests sphere behind camera (should be outside) [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SphereBehind, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere behind near plane
    Vec3 position(0.0f, 0.0f, 0.05f); // Too close, behind near plane
    float radius = 0.01f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SphereBeyondFar
 * @tc.desc: Tests sphere beyond far plane (should be outside) [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SphereBeyondFar, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere beyond far plane
    Vec3 position(0.0f, 0.0f, -200.0f); // Beyond far plane
    float radius = 1.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SphereLeftOfFrustum
 * @tc.desc: Tests sphere to the left of frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SphereLeftOfFrustum, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere far to the left
    Vec3 position(-100.0f, 0.0f, -10.0f);
    float radius = 1.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SphereRightOfFrustum
 * @tc.desc: Tests sphere to the right of frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SphereRightOfFrustum, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere far to the right
    Vec3 position(100.0f, 0.0f, -10.0f);
    float radius = 1.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SphereAboveFrustum
 * @tc.desc: Tests sphere above frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SphereAboveFrustum, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere far above
    Vec3 position(0.0f, 100.0f, -10.0f);
    float radius = 1.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SphereBelowFrustum
 * @tc.desc: Tests sphere below frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SphereBelowFrustum, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere far below
    Vec3 position(0.0f, -100.0f, -10.0f);
    float radius = 1.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_LargeSphereInside
 * @tc.desc: Tests large sphere that should be inside frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_LargeSphereInside, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Large sphere at origin should still be inside
    Vec3 position(0.0f, 0.0f, -10.0f);
    float radius = 5.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_ZeroRadius
 * @tc.desc: Tests sphere with zero radius (point) [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_ZeroRadius, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Point at center
    Vec3 position(0.0f, 0.0f, -10.0f);
    float radius = 0.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_SmallRadiusOutside
 * @tc.desc: Tests small sphere outside frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_SmallRadiusOutside, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Point just outside left plane
    Vec3 position(0.0f, 0.0f, 0.0f);
    float radius = 0.001f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumCollision_TouchingPlane
 * @tc.desc: Tests sphere exactly touching frustum plane boundary [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_TouchingPlane, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Sphere slightly inside near plane should be considered inside
    Vec3 position(0.0f, 0.0f, -nearPlane - 0.01f);
    float radius = 0.0f;

    bool result = frustumUtil.SphereFrustumCollision(frustum, position, radius);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: FrustumUtil_SphereFrustumMultiplePositions
 * @tc.desc: Tests multiple sphere positions around frustum [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumCollision_MultiplePositions, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Test multiple positions
    struct TestCase {
        Vec3 position;
        float radius;
        bool expectedInside;
    };

    BASE_NS::vector<TestCase> testCases;
    testCases.push_back({ Vec3(0.0f, 0.0f, -10.0f), 1.0f, true });   // Center
    testCases.push_back({ Vec3(-5.0f, 0.0f, -10.0f), 1.0f, true });  // Slightly left
    testCases.push_back({ Vec3(5.0f, 0.0f, -10.0f), 1.0f, true });   // Slightly right
    testCases.push_back({ Vec3(0.0f, 3.0f, -10.0f), 1.0f, true });   // Slightly up
    testCases.push_back({ Vec3(0.0f, -3.0f, -10.0f), 1.0f, true });  // Slightly down
    testCases.push_back({ Vec3(0.0f, 0.0f, -1.0f), 1.0f, true });   // Close to near
    testCases.push_back({ Vec3(0.0f, 0.0f, -50.0f), 1.0f, true });  // Close to far
    testCases.push_back({ Vec3(-50.0f, 0.0f, -10.0f), 1.0f, false }); // Far left
    testCases.push_back({ Vec3(50.0f, 0.0f, -10.0f), 1.0f, false });  // Far right
    testCases.push_back({ Vec3(0.0f, 50.0f, -10.0f), 1.0f, false });  // Far up
    testCases.push_back({ Vec3(0.0f, -50.0f, -10.0f), 1.0f, false }); // Far down

    for (const auto& testCase : testCases) {
        bool result = frustumUtil.SphereFrustumCollision(frustum, testCase.position, testCase.radius);
        EXPECT_EQ(testCase.expectedInside, result);
    }
}

/**
 * @tc.name: FrustumUtil_CreateFrustum_ViewProjectionMatrix
 * @tc.desc: Tests frustum creation from view-projection matrix [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_CreateFrustum_ViewProjectionMatrix, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    // Create view matrix
    Vec3 eye(0.0f, 0.0f, 5.0f);
    Vec3 target(0.0f, 0.0f, 0.0f);
    Vec3 up(0.0f, 1.0f, 0.0f);
    Mat4X4 view = Math::LookAtRh(eye, target, up);

    // Create projection matrix
    const float fov = 1.57f;
    const float aspect = 16.0f / 9.0f;
    const float nearPlane = 0.1f;
    const float farPlane = 100.0f;
    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);

    // Combine to view-projection
    Mat4X4 viewProjection = projection * view;

    Frustum frustum = frustumUtil.CreateFrustum(viewProjection);

    // Verify planes are normalized
    for (uint32_t i = 0; i < Frustum::PLANE_COUNT; ++i) {
        const Vec4& plane = frustum.planes[i];
        float magnitude = Math::Magnitude(Vec3(plane.x, plane.y, plane.z));
        EXPECT_FLOAT_EQ(1.0f, magnitude);
    }

    // Sphere at target (origin) should be visible from eye position
    Vec3 spherePosition(0.0f, 0.0f, 0.0f);
    float radius = 1.0f;
    bool result = frustumUtil.SphereFrustumCollision(frustum, spherePosition, radius);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: FrustumUtil_PlaneConstants
 * @tc.desc: Tests frustum plane enumeration constants [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_PlaneConstants, testing::ext::TestSize.Level1)
{
    // Verify plane constants
    EXPECT_EQ(0, static_cast<int>(Frustum::PLANE_LEFT));
    EXPECT_EQ(1, static_cast<int>(Frustum::PLANE_RIGHT));
    EXPECT_EQ(2, static_cast<int>(Frustum::PLANE_BOTTOM));
    EXPECT_EQ(3, static_cast<int>(Frustum::PLANE_TOP));
    EXPECT_EQ(4, static_cast<int>(Frustum::PLANE_NEAR));
    EXPECT_EQ(5, static_cast<int>(Frustum::PLANE_FAR));
    EXPECT_EQ(6, static_cast<int>(Frustum::PLANE_COUNT));
}

/**
 * @tc.name: FrustumUtil_SphereFrustumAllPlaneTests
 * @tc.desc: Tests sphere against each individual frustum plane [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_FrustumUtilTest, FrustumUtil_SphereFrustumAllPlaneTests, testing::ext::TestSize.Level1)
{
    FrustumUtil frustumUtil;

    const float fov = 1.57f;
    const float aspect = 1.0f; // Square aspect for simpler calculations
    const float nearPlane = 1.0f;
    const float farPlane = 100.0f;

    Mat4X4 projection = Math::PerspectiveRhNo(fov, aspect, nearPlane, farPlane);
    Frustum frustum = frustumUtil.CreateFrustum(projection);

    // Calculate the approximate frustum dimensions at z = -10
    // For fov = 90 degrees (1.57 rad) and aspect = 1.0, tan(fov/2) = 1.0
    // At z = -10, half height = 10 * tan(45°) = 10
    const float zPos = -10.0f;
    const float halfHeight = 10.0f;
    const float halfWidth = 10.0f;

    // Test positions just outside each plane
    struct OutsideTest {
        Vec3 position;
        const char* planeName;
    };

    BASE_NS::vector<OutsideTest> outsideTests;
    outsideTests.push_back({ Vec3(-(halfWidth + 2.0f), 0.0f, zPos), "left" });
    outsideTests.push_back({ Vec3(halfWidth + 2.0f, 0.0f, zPos), "right" });
    outsideTests.push_back({ Vec3(0.0f, -(halfHeight + 2.0f), zPos), "bottom" });
    outsideTests.push_back({ Vec3(0.0f, halfHeight + 2.0f, zPos), "top" });
    outsideTests.push_back({ Vec3(0.0f, 0.0f, -0.5f), "near" });
    outsideTests.push_back({ Vec3(0.0f, 0.0f, -150.0f), "far" });

    for (const auto& test : outsideTests) {
        bool result = frustumUtil.SphereFrustumCollision(frustum, test.position, 0.1f);
        EXPECT_FALSE(result) << "Sphere should be outside " << test.planeName << " plane";
    }

    // Test positions just inside each plane boundary
    struct InsideTest {
        Vec3 position;
        const char* planeName;
    };

    BASE_NS::vector<InsideTest> insideTests;
    insideTests.push_back({ Vec3(-(halfWidth - 1.0f), 0.0f, zPos), "left" });
    insideTests.push_back({ Vec3(halfWidth - 1.0f, 0.0f, zPos), "right" });
    insideTests.push_back({ Vec3(0.0f, -(halfHeight - 1.0f), zPos), "bottom" });
    insideTests.push_back({ Vec3(0.0f, halfHeight - 1.0f, zPos), "top" });
    insideTests.push_back({ Vec3(0.0f, 0.0f, -1.5f), "near" });
    insideTests.push_back({ Vec3(0.0f, 0.0f, -90.0f), "far" });

    for (const auto& test : insideTests) {
        bool result = frustumUtil.SphereFrustumCollision(frustum, test.position, 0.1f);
        EXPECT_TRUE(result) << "Sphere should be inside " << test.planeName << " plane";
    }
}
