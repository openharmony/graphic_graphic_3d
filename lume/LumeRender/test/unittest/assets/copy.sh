#!/bin/bash
#Copyright (c) 2025 Huawei Device Co., Ltd.
#Licensed under the Apache License, Version 2.0 (the "License");
#you may not use this file except in compliance with the License.
#You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing, software
#distributed under the License is distributed on an "AS IS" BASIS,
#WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#See the License for the specific language governing permissions and
#limitations under the License.
set -e
WORKING_DIR=$(cd "$(dirname "$0")"; pwd)
PROJECT_ROOT=${WORKING_DIR%/foundation*}

OUT_PATH=$1
OUT_DIR=${OUT_PATH#//}
OUT_DIR=${OUT_PATH%/obj*}
OUT_DIR=$PROJECT_ROOT/$OUT_DIR/gen/foundation/graphic/graphic_3d/lume/LumeRender/test/unittest/assets/test_data

cp -r $OUT_DIR $WORKING_DIR/.

