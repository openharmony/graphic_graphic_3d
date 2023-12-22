/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
void GenericLerp(in vec2 a, in vec2 b, in float t, out vec2 c)
{
    c = mix(a, b, t);
}

void GenericLerp(in vec3 a, in vec3 b, in float t, out vec3 c)
{
    c = mix(a, b, t);
}

void GenericLerp(in vec4 a, in vec4 b, in float t, out vec4 c)
{
    c = mix(a, b, t);
}
