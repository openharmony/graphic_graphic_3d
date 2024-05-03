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
#ifndef EGL_FUNCTIONS_H
#define EGL_FUNCTIONS_H
#if RENDER_HAS_GLES_BACKEND
#ifndef declare
// clang-format off
#include <EGL/egl.h>
#include <EGL/eglext.h>
// clang-format on
#define declare(a, b) extern a b;
#endif

#ifdef EGL_ANDROID_get_native_client_buffer
declare(PFNEGLGETNATIVECLIENTBUFFERANDROIDPROC, eglGetNativeClientBufferANDROID);
#endif

#ifdef EGL_KHR_image_base
declare(PFNEGLCREATEIMAGEKHRPROC, eglCreateImageKHR);
declare(PFNEGLDESTROYIMAGEKHRPROC, eglDestroyImageKHR);
#endif

#endif // RENDER_HAS_GLES_BACKEND
#undef declare
#endif  // EGL_FUNCTIONS_H