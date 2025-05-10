/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: "plugin uid"
 */

#ifndef META_BASE_PLUGIN_HEADER
#define META_BASE_PLUGIN_HEADER

#include <base/util/uid.h>

#include <meta/base/namespace.h>
#include <meta/base/version.h>

META_BEGIN_NAMESPACE()

/**
 * @brief UID for the meta object library plugin, used to identify the dynamic library.
 */
constexpr BASE_NS::Uid META_OBJECT_PLUGIN_UID { "532f7256-8849-472a-933a-d73efc232b01" };
constexpr const char* META_VERSION_STRING = "2.0";
constexpr Version META_VERSION(META_VERSION_STRING);

META_END_NAMESPACE()

#endif
