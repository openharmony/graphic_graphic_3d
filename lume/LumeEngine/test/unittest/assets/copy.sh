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
ROFS_DIR=${2#//}
if [ -n "$ROFS_DIR" ]; then
    ROFS_DIR=$PROJECT_ROOT/$ROFS_DIR
fi
rm -rf "$WORKING_DIR/test_data/invalid_plugins"
rm -rf "$WORKING_DIR/invalid_plugins"

OUT_PATH=$1
OUT_DIR=${OUT_PATH#//}
OUT_DIR=${OUT_DIR%/obj*}
OUT_DIR=$PROJECT_ROOT/$OUT_DIR/graphic/graphic_3d

cp "$OUT_DIR/libCoreTestSharedPlugin.z.so" "$WORKING_DIR/libCoreTestSharedPlugin.z.so"
cp "$OUT_DIR/libCoreTestSharedPlugin2.z.so" "$WORKING_DIR/libCoreTestSharedPlugin2.z.so"
cp "$OUT_DIR/libCoreTestSharedPlugin3.z.so" "$WORKING_DIR/libCoreTestSharedPlugin3.z.so"
if [ -n "$ROFS_DIR" ]; then
    rm -rf "$ROFS_DIR"
    mkdir -p "$ROFS_DIR"
    cp -r "$WORKING_DIR/." "$ROFS_DIR/"
    mkdir -p "$ROFS_DIR/invalid_plugins"
    cp "$OUT_DIR/libCoreTestSharedPluginCircularA.z.so" "$ROFS_DIR/invalid_plugins/"
    cp "$OUT_DIR/libCoreTestSharedPluginCircularB.z.so" "$ROFS_DIR/invalid_plugins/"
fi

