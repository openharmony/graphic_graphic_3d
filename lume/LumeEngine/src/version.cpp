/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#include <core/namespace.h>
#include <base/containers/string_view.h>

CORE_BEGIN_NAMESPACE()
CORE_PUBLIC BASE_NS::string_view GetVersion() { return ""; }
CORE_PUBLIC BASE_NS::string_view GetVersionRev() { return ""; }
CORE_PUBLIC BASE_NS::string_view GetVersionBranch() { return ""; }
#ifdef NDEBUG
CORE_PUBLIC bool IsDebugBuild() { return true; }
#else
CORE_PUBLIC bool IsDebugBuild() { return false; }
#endif
CORE_END_NAMESPACE()
