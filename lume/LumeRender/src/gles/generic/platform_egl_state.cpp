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

#if RENDER_HAS_GLES_BACKEND

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "gles/egl_state.h"
#undef EGL_FUNCTIONS_H
#include "gles/egl_functions.h"

RENDER_BEGIN_NAMESPACE()

namespace EGLHelpers {

void EGLHelpers::EGLState::PlatformInitialize() {}
} // namespace EGLHelpers

RENDER_END_NAMESPACE()

#endif
