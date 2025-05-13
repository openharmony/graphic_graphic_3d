/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "3d/shaders/common/3d_dm_structures_common.h"

#ifndef SHADERS_COMMON_3D_LIGHT_CLUSTERING_COMMON_H
#define SHADERS_COMMON_3D_LIGHT_CLUSTERING_COMMON_H

vec2 GetNearFar(const mat4 projectionMatrix)
{
    const float a = projectionMatrix[2][2];
    const float b = projectionMatrix[3][2];

    const float near = b / (a - 1.0);
    const float far = b / (1.0 + a);

    return vec2(near, far);
}

uint CoordToIdx(const uvec3 coord)
{
    const uint x = coord.x;
    const uint y = coord.y;
    const uint z = coord.z;
    const uint area = LIGHT_CLUSTERS_X * LIGHT_CLUSTERS_Y;

    return (z * area) + (y * LIGHT_CLUSTERS_X) + x;
}

uvec3 IdxToCoord(const uint idx)
{
    const uint area = LIGHT_CLUSTERS_X * LIGHT_CLUSTERS_Y;
    const uint z = idx / area;
    const uint xy = idx % area;
    const uint y = xy / LIGHT_CLUSTERS_X;
    const uint x = xy % LIGHT_CLUSTERS_X;
    return uvec3(x, y, z);
}

vec4 ScreenPointToView(const vec2 screenPoint, const vec2 resolution, const mat4 invProj)
{
    const vec4 ndc =
        vec4((screenPoint.x / resolution.x) * 2.0 - 1.0, 1.0 - (screenPoint.y / resolution.y) * 2.0, 0.0, 1.0);

    const vec4 view = invProj * ndc;
    return view / view.w;
}

vec2 WorldPointToScreen(const vec3 worldPoint, const mat4 view, const mat4 proj, const vec2 resolution)
{
    const vec4 viewPoint = view * vec4(worldPoint, 1.0);
    const vec4 clipPoint = proj * viewPoint;
    const vec3 ndcPoint = clipPoint.xyz / clipPoint.w;

    vec2 screenPoint;
    screenPoint.x = (ndcPoint.x * 0.5 + 0.5) * resolution.x; // 0.5: parm
    screenPoint.y = (1.0 - (ndcPoint.y * 0.5 + 0.5)) * resolution.y;
    return screenPoint;
}

uint PointToClusterIdx(const vec3 worldPoint, const mat4 view, const mat4 proj, const vec2 resolution)
{
    const vec4 viewPoint = view * vec4(worldPoint, 1.0);
    const vec2 screenPoint = WorldPointToScreen(worldPoint, view, proj, resolution);

    const float tileSizePxX = resolution.x / float(LIGHT_CLUSTERS_X);
    const float tileSizePxY = resolution.y / float(LIGHT_CLUSTERS_Y);

    const uint clusterX = min(uint(screenPoint.x / tileSizePxX), LIGHT_CLUSTERS_X - 1);
    const uint clusterY = min(uint(screenPoint.y / tileSizePxY), LIGHT_CLUSTERS_Y - 1);

    const vec2 nearfar = GetNearFar(proj);
    const float zNear = nearfar.x;
    const float zFar = nearfar.y;

    const uint clusterZ = uint((log(abs(viewPoint.z) / zNear) * float(LIGHT_CLUSTERS_Z)) / log(zFar / zNear));

    return CoordToIdx(uvec3(clusterX, clusterY, min(clusterZ, LIGHT_CLUSTERS_Z - 1)));
}

vec3 IntersectRayZPlane(const vec3 aa, const vec3 bb, const float zDist)
{
    const vec3 normal = vec3(0.0, 0.0, -1.0);
    const vec3 dir = bb - aa;

    const float tt = (zDist - dot(normal, aa)) / dot(normal, dir);
    const vec3 result = aa + tt * dir;

    return result;
}

void ClusterIdxToClusterCorners(
    const uint clusterIdx, const mat4 proj, const vec2 resolution, out vec3 minCornerView, out vec3 maxCornerView)
{
    const uvec3 coord = IdxToCoord(clusterIdx);
    const vec2 nearfar = GetNearFar(proj);
    const float zNear = nearfar.x;
    const float zFar = nearfar.y;
    const float tileSizePxX = resolution.x / float(LIGHT_CLUSTERS_X);
    const float tileSizePxY = resolution.y / float(LIGHT_CLUSTERS_Y);

    const vec2 minPointSS = coord.xy * vec2(tileSizePxX, tileSizePxY);
    const vec2 maxPointSS = (coord.xy + 1.0) * vec2(tileSizePxX, tileSizePxY);

    const mat4 invProj = inverse(proj);
    const vec3 maxPointVS = ScreenPointToView(maxPointSS, resolution, invProj).xyz;
    const vec3 minPointVS = ScreenPointToView(minPointSS, resolution, invProj).xyz;

    const float tileNear = zNear * pow(zFar / zNear, float(coord.z) / float(LIGHT_CLUSTERS_Z));
    const float tileFar = zNear * pow(zFar / zNear, float(coord.z + 1) / float(LIGHT_CLUSTERS_Z));

    const vec3 minPointNear = IntersectRayZPlane(vec3(0), minPointVS, tileNear);
    const vec3 minPointFar = IntersectRayZPlane(vec3(0), minPointVS, tileFar);
    const vec3 maxPointNear = IntersectRayZPlane(vec3(0), maxPointVS, tileNear);
    const vec3 maxPointFar = IntersectRayZPlane(vec3(0), maxPointVS, tileFar);

    const vec3 minPointView = min(minPointNear, minPointFar);
    const vec3 maxPointView = max(maxPointNear, maxPointFar);

    minCornerView = minPointView;
    maxCornerView = maxPointView;
}

bool SphereClusterIntersect(
    const vec3 sphereCenter, const float sphereRadius, const vec3 minCorner, const vec3 maxCorner)
{
    const vec3 closestPoint = clamp(sphereCenter, minCorner, maxCorner);
    const float distSq = dot(closestPoint - sphereCenter, closestPoint - sphereCenter);

    return distSq <= sphereRadius * sphereRadius;
}

bool CameraClusterSpotLightIntersect(const vec3 lightPosWorld, const vec3 lightDir, const float range,
    const vec3 clusterMinWorld, const vec3 clusterMaxWorld, const mat4 proj, const mat4 view, const vec3 cameraPosition,
    const vec3 cameraForward)
{
    /**
     *  If the light origin is outside the camera's view frustum, ConeClusterIntersect(..)
     *  can sometimes fail. This is a special case check which results in some lights being
     *  assigned to clusters they don't necessarily belong to, but this guarantees that all
     *  lights that belong to a cluster will always be registered.
     */

    const vec3 clusterCenter = (clusterMaxWorld + clusterMinWorld) / 2.0;
    const vec3 lightToCluster = normalize(clusterCenter - lightPosWorld);

    const vec4 pointClip = proj * view * vec4(lightPosWorld, 1.0);
    const vec3 pointNDC = pointClip.xyz / pointClip.w;
    const bool lightOutsideFrustum = any(lessThan(pointNDC, vec3(-1.0))) || any(greaterThan(pointNDC, vec3(1.0)));

    const bool lightCanCastOnCluster = dot(lightToCluster, lightDir) > 0.0;

    const vec3 clusterSidesHalf = abs(clusterMaxWorld - clusterMinWorld) / 2.0;
    const float clusterSphereRadius =
        sqrt(clusterSidesHalf.x * clusterSidesHalf.x + clusterSidesHalf.y * clusterSidesHalf.y +
             clusterSidesHalf.z * clusterSidesHalf.z);
    const bool clusterWithinRange = length(lightPosWorld - clusterCenter) < range + clusterSphereRadius;

    return lightOutsideFrustum && clusterWithinRange && lightCanCastOnCluster;
}

bool ConeClusterIntersect(const vec3 coneOrigin, const vec3 coneDirection, const float coneRange, const float coneAngle,
    const vec3 minCorner, const vec3 maxCorner)
{
    /**
     *  This function essentially just fits a sphere around the cluster AABB bounds
     *  and then checks that bounds sphere against the spot light cone.
     *  Original implementation similar to Bart Wronski.
     */

    const vec3 clusterCenter = (minCorner + maxCorner) / 2.0;
    const vec3 clusterSidesHalf = abs(maxCorner - minCorner) / 2.0;
    const float boundSphereRadius =
        sqrt(clusterSidesHalf.x * clusterSidesHalf.x + clusterSidesHalf.y * clusterSidesHalf.y +
             clusterSidesHalf.z * clusterSidesHalf.z);

    const vec3 vv = clusterCenter - coneOrigin;
    const float lenSq = dot(vv, vv);
    const float v1Len = dot(vv, normalize(coneDirection));
    const float closestPointDistance = cos(coneAngle) * sqrt(lenSq - v1Len * v1Len) - v1Len * sin(coneAngle);

    const bool cullAngle = closestPointDistance > boundSphereRadius;
    const bool cullFront = v1Len > boundSphereRadius + coneRange;
    const bool cullBack = v1Len < -boundSphereRadius;

    return !(cullAngle || cullFront || cullBack);
}

#endif // SHADERS_COMMON_LIGHT_CLUSTERING_COMMON_H
