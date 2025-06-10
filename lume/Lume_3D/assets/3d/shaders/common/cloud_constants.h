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
#ifndef CLOUDS_CONSTANTS_H
#define CLOUDS_CONSTANTS_H

const float PI = 3.1415926535897932384626433832795;
const float TWO_PI = 6.2831853071795864769252867665590;
const float HALF_PI = 1.5707963267948966192313216916397;

// atmosphere
// const float innerHeight      = 10000;
// const float outerHeight      = 45000;

const float innerHeight = 2000;
const float outerHeight = 18000;

const float renderingScale = 1;
const float extinctionCoeff = 1.0 / (outerHeight - innerHeight);

const float weatherMapScale = 8.0;

const float LIGHTING_SAMPLES_1TO5_STEP_SIZE = 60.0;
const float LIGHTING_SAMPLES_6_STEP_SIZE = 800.0;

// status = 0  2,000 meters
// cumulus = 200  2,000 meters
// Cumulonimbus = 50016,000 meters
// Alto = 2,0007,000 meters
// Cirrus = 4,000-20,000 meters

// first, lets define some constants to use (planet radius, position, and scattering coefficients)
const float DEFAULT_PLANET_RADIUS = 6371e3;
const float DEFAULT_ATMOS_RADIUS = 6471e3;
const vec3 DEFAULT_PLANET_POS = vec3(0.0, -DEFAULT_PLANET_RADIUS, 0.0);
const float DEFAULT_PLANET_BIAS = 5;
const vec3 DEFAULT_RAY_BETA = vec3(5.5e-6, 13.0e-6, 22.4e-6);
const vec3 DEFAULT_MIE_BETA = vec3(4e-6);
const vec3 DEFAULT_AMBIENT_BETA = vec3(.5e-10, .51e-10, .5e-10);
const vec3 DEFAULT_ABSORPTION_BETA = vec3(2.04e-5, 4.97e-5, 1.95e-6);
const float DEFAULT_G = 0.7;
const float DEFAULT_HEIGHT_RAY = 8e3;
const float DEFAULT_HEIGHT_MIE = 1.2e3;
const float DEFAULT_HEIGHT_ABSORPTION = 30e3;
const float DEFAULT_ABSORPTION_FALLOFF = 4e3;
const int DEFAULT_PRIMARY_STEPS = 12;
const int DEFAULT_LIGHT_STEPS = 4;
#endif // CLOUDS_CONSTANTS_H