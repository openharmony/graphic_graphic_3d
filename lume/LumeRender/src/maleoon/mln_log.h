/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MALEOON_MLN_LOG_H
#define MALEOON_MLN_LOG_H

#include <parameters.h>
#include "util/log.h"

// ============================================================
// Maleoon Backend Categorized Logging
//
// Categories controlled via hdc shell param set:
//   persist.agp.mln.log.init  = 0/1  [MLN-INIT]  Device/resource init
//   persist.agp.mln.log.frame = 0/1  [MLN-FRAME] Per-frame flow
//   persist.agp.mln.log.graph = 0/1  [MLN-GRAPH] Graphics OG/DG/RT/PSO/BS
//   persist.agp.mln.log.trans = 0/1  [MLN-TRANS] Transfer OG/DG
//   persist.agp.mln.log.comp  = 0/1  [MLN-COMP]  Compute OG/DG
//   persist.agp.mln.og.pending     = 0/1  OG pool pending-stage fix (default=1=ON)
//   persist.agp.mln.og.direct_build = 0/1  Secondary ctx direct OG build (default=1=ON)
//   [MLN-ERR] always printed (not gated)
//
// Flags read once at init via MlnLogRefreshFlags(). App restart to take effect.
// When a flag is off, both the log call AND any data-gathering code
// guarded by if(g_mlnLog.xxx) are skipped — zero overhead.
// ============================================================

struct MlnLogFlags {
    bool init  = false;
    bool frame = false;
    bool graph = false;
    bool trans = false;
    bool comp  = false;
    bool ogPendingStage = true;  // OG pool pending-stage fix (default ON)
    bool ogDirectBuild  = true;  // Secondary ctx builds OG directly (default ON)
};

namespace Render {
extern MlnLogFlags g_mlnLog;
void MlnLogRefreshFlags();
} // namespace Render
using Render::g_mlnLog;

#define MLN_LOG_INIT(...) \
    do { if (g_mlnLog.init) { PLUGIN_LOG_E("[MLN-INIT] " __VA_ARGS__); } } while (0)

#define MLN_LOG_FRAME(...) \
    do { if (g_mlnLog.frame) { PLUGIN_LOG_E("[MLN-FRAME] " __VA_ARGS__); } } while (0)

#define MLN_LOG_GRAPH(...) \
    do { if (g_mlnLog.graph) { PLUGIN_LOG_E("[MLN-GRAPH] " __VA_ARGS__); } } while (0)

#define MLN_LOG_TRANS(...) \
    do { if (g_mlnLog.trans) { PLUGIN_LOG_E("[MLN-TRANS] " __VA_ARGS__); } } while (0)

#define MLN_LOG_COMP(...) \
    do { if (g_mlnLog.comp) { PLUGIN_LOG_E("[MLN-COMP] " __VA_ARGS__); } } while (0)

#define MLN_LOG_ERR(...) \
    do { PLUGIN_LOG_E("[MLN-ERR] " __VA_ARGS__); } while (0)

#endif // MALEOON_MLN_LOG_H
