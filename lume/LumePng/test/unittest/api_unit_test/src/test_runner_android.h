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

#ifndef TEST_RUNNER_ANDROID
#define TEST_RUNNER_ANDROID

#include <android/log.h>
#include <android/native_activity.h>
#include <android/native_window_jni.h>
#include <android/window.h>
#include <android_native_app_glue.h>
#include <fcntl.h>
#include <fstream>
#include <jni.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include <base/containers/string.h>
#include <base/containers/vector.h>

#include "test_runner_common.h"

namespace Test {

class AndroidApp : public TestApp {
public:
    AndroidApp(android_app* state) : m_state(state)
    {}
    AndroidApp(JNIEnv* env, jobject thiz, jobject context, jobject surface, jstring outputFile, jobjectArray arguments)
        : m_env(env), m_context(context), m_surface(surface), m_outputFile(outputFile), m_arguments(arguments)
    {}

    void Init() override;

    void Run() override;

    void Terminate() override;

    ANativeWindow* GetWindowHandle();

    int GetGTestResult() const;

    JNIEnv* GetEnv();

    jobject* GetContext();

    AAssetManager* GetRsManager();

    BASE_NS::string GetAppDir() const;

private:
    void SetPaths();

    void SetGtestEnv();

    void CleanGtestEnv();

    static void AndroidAppExeCmdCB(android_app* state, int32_t cmd);

    BASE_NS::string GetIntentExtra(JNIEnv* env, jobject context, const BASE_NS::string& parameter);

    struct TestData {
        BASE_NS::vector<char*> gtestArgs;  // Input data for Gtest
        BASE_NS::string reportTestFile;    // Report log file
        BASE_NS::string gtestTestFile;     // Gtest results file
        int result = -1;                   // Gtest results exit code
        bool hasRunTests = false;
        ANativeWindow* window{nullptr};  // Window
    };

    JNIEnv* m_env{nullptr};
    jobject m_context{nullptr};
    android_app* m_state{nullptr};
    AAssetManager* m_rsManager{nullptr};
    BASE_NS::string m_internalDir, m_externalDir, m_cacheDir, m_filesDir;
    jobject m_surface{nullptr};
    jstring m_outputFile{nullptr};
    jobjectArray m_arguments{nullptr};
    TestData m_testData;
};

inline AndroidApp* g_androidApp;

}  // namespace Test

#endif  // TEST_RUNNER_ANDROID
