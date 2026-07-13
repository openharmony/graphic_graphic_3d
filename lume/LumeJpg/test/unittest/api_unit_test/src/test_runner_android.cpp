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

#include "test_runner_android.h"

#include <base/containers/unique_ptr.h>

namespace Test {

void AndroidApp::Init()
{
    if (m_state != nullptr) {
        // Get env and context
        // Attach current thread to allow jni calls
        m_state->activity->vm->AttachCurrentThread(&m_env, nullptr);
        m_context = m_state->activity->clazz;
    } else {
        // Get window handle
        m_testData.window = ANativeWindow_fromSurface(m_env, m_surface);

        if (m_testData.window == nullptr) {
            LOGE("Pointer to Android window is null!");
        }

        const char* outputFileCString = m_env->GetStringUTFChars(m_outputFile, nullptr);
        m_testData.reportTestFile = BASE_NS::string(outputFileCString);
        m_env->ReleaseStringUTFChars(m_outputFile, outputFileCString);
    }

    SetPaths();
}

void AndroidApp::Run()
{
    // Set gtest environment
    SetGtestEnv();

    // Start redirecting report to a file
    Test::StartOutputFileRedirect(m_testData.reportTestFile.c_str(), false);

    if (m_state != nullptr) {
        // Set android app (glue) cmd, callbacks, user data,...
        m_state->userData = &m_testData;
        m_state->onAppCmd = AndroidAppExeCmdCB;

        // Main loop
        while (!m_state->destroyRequested) {
            // Our test logic is all driven by callbacks, so we don't need to use the non-blocking poll.
            struct android_poll_source* source = nullptr;
            auto result = ALooper_pollOnce(-1, nullptr, nullptr, reinterpret_cast<void**>(&source));
            if (result == ALOOPER_POLL_ERROR) {
                LOGW("ALooper_pollOnce returned an error");
            }

            if (source != nullptr) {
                source->process(m_state, source);
            }
        }
    } else {
        // Run unit tests
        m_testData.result = RunGoogleTest(m_testData.gtestArgs.size(), m_testData.gtestArgs.data());
        m_testData.hasRunTests = true;
    }

    // Stop redirecting report to a file
    Test::StopOutputFileRedirect();

    // Dump report to stdout
    Test::PrintLogFromFile(m_testData.reportTestFile.c_str());

    // Clean GtestEnv
    CleanGtestEnv();
}

void AndroidApp::Terminate()
{
    if (m_state != nullptr) {
        // Detach current thread used for jni calls
        m_state->activity->vm->DetachCurrentThread();

        // NOTE: return doesn't completely exit the program!?
        exit(m_testData.result);
    } else {
        // Release the native window
        ANativeWindow_release(m_testData.window);
    }
}

ANativeWindow* AndroidApp::GetWindowHandle()
{
    return m_testData.window;
}

int AndroidApp::GetGTestResult() const
{
    return m_testData.result;
}

JNIEnv* AndroidApp::GetEnv()
{
    return m_env;
}

jobject* AndroidApp::GetContext()
{
    return &m_context;
}

AAssetManager* AndroidApp::GetRsManager()
{
    return m_rsManager;
}

BASE_NS::string AndroidApp::GetAppDir() const
{
    return m_externalDir;
}

void AndroidApp::SetPaths()
{
    if (m_state != nullptr) {
        // NOTE: App internal storage is for app internal usage only, inaccessible by ADB for Release
        // build and not rooted devices
        m_internalDir = m_state->activity->internalDataPath;
        m_externalDir = m_state->activity->externalDataPath;
        m_rsManager = m_state->activity->assetManager;
    }
}

void AndroidApp::SetGtestEnv()
{
    if (m_state != nullptr) {
        m_testData.reportTestFile = m_externalDir + "/report-All.txt";
        m_testData.gtestTestFile = m_externalDir + "/gtestReport_Android.xml";

        // Create argc from intent extra parameters (input arguments)
        BASE_NS::string gtestFilterVal = GetIntentExtra(m_env, m_state->activity->clazz, "gtest_filter");
        BASE_NS::string gtestOutputVal = GetIntentExtra(m_env, m_state->activity->clazz, "gtest_output");

        m_testData.gtestArgs.emplace_back(new char[20]{"JpgAPITestRunner"});
        BASE_NS::string tmpStr = "--gtest_filter=*";
        if (!gtestFilterVal.empty()) {
            tmpStr = "--gtest_filter=" + gtestFilterVal;
        }
        {
            char* temp = new char[tmpStr.size() + 1];
            std::strcpy(temp, tmpStr.data());
            m_testData.gtestArgs.emplace_back(temp);
        }

        tmpStr = "--gtest_output=xml:" + m_testData.gtestTestFile;
        if (!gtestOutputVal.empty()) {
            tmpStr = "--gtest_output=xml:" + gtestOutputVal;
        }
        {
            char* temp = new char[tmpStr.size() + 1];
            std::strcpy(temp, tmpStr.data());
            m_testData.gtestArgs.emplace_back(temp);
        }
    } else {
        if (m_arguments) {
            int argc = m_env->GetArrayLength(m_arguments);
            m_testData.gtestArgs.reserve((unsigned int)(argc + 1));

            // Add the app name as the first parameter.
            m_testData.gtestArgs.emplace_back(strdup("JpgAPITestRunner"));

            for (int i = 0; i < argc; i++) {
                jobject element = m_env->GetObjectArrayElement(m_arguments, i);
                const char* elementCString = m_env->GetStringUTFChars((jstring)element, nullptr);
                m_testData.gtestArgs.emplace_back(strdup(elementCString));
                m_env->ReleaseStringUTFChars((jstring)element, elementCString);
            }
        }
    }
}

void AndroidApp::AndroidAppExeCmdCB(android_app* state, int32_t cmd)
{
    auto* userData = static_cast<TestData*>(state->userData);
    switch (cmd) {
        case APP_CMD_INIT_WINDOW: {
            // The window is being shown, get it ready
            if (state->window != nullptr && !userData->hasRunTests) {
                userData->window = state->window;

                userData->result = RunGoogleTest(userData->gtestArgs.size(), userData->gtestArgs.data());

                userData->hasRunTests = true;

                // Stop activity
                ANativeActivity_finish(state->activity);
            }
            break;
        }
        case APP_CMD_TERM_WINDOW: {
            break;
        }
        case APP_CMD_WINDOW_REDRAW_NEEDED: {
            break;
        }
        case APP_CMD_SAVE_STATE: {
            break;
        }
        case APP_CMD_GAINED_FOCUS: {
            break;
        }
        case APP_CMD_LOST_FOCUS: {
            break;
        }
        case APP_CMD_DESTROY: {
            break;
        }
        default: {
            break;
        }
    }
}

void AndroidApp::CleanGtestEnv()
{
    // Cleanup
    for (auto& arg : m_testData.gtestArgs) {
        delete[] arg;
    }
}

BASE_NS::string AndroidApp::GetIntentExtra(JNIEnv* env, jobject context, const Base::string& parameter)
{
    jclass acl = env->GetObjectClass(context);
    jmethodID getIntentId = env->GetMethodID(acl, "getIntent", "()Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(context, getIntentId);

    jclass icl = env->GetObjectClass(intent);
    jmethodID getStringExtraId = env->GetMethodID(icl, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");

    auto jsExtra =
        static_cast<jstring>(env->CallObjectMethod(intent, getStringExtraId, env->NewStringUTF(parameter.c_str())));
    if (jsExtra) {
        const char* extra = env->GetStringUTFChars(jsExtra, 0);
        BASE_NS::string result(extra);
        env->ReleaseStringUTFChars(jsExtra, extra);
        return result;
    }
    return {};
}

}  // namespace Test

//
// A simple native activity for running tests by running an android app (without the android test
// orchestrator).
//
// To define options to "googletest" we can add something like this to "am start":
//   -e googletest "--gtest_output=xml:/data/app/file/ --gtest_filter=TaskQueueTest.*:EnvTest.*"
//
// (In Android Studio found from: Run -> Edit Configurations... -> Launch Flags)
//
void android_main(struct android_app* state)
{
    BASE_NS::unique_ptr<Test::TestApp> app = BASE_NS::make_unique<Test::AndroidApp>(state);
    // WARN: No dynamic_cast since it requires RTTI
    Test::g_androidApp = static_cast<Test::AndroidApp*>(app.get());

    Test::g_androidApp->Init();
    Test::g_androidApp->Run();
    Test::g_androidApp->Terminate();
}

extern "C" JNIEXPORT jint JNICALL Java_com_huawei_jpg_1api_1test_1runner_test_NativeTestRunner_onSurfaceCreatedNative(
    JNIEnv* env, jobject thiz, jobject context, jobject surface, jstring outputFile, jobjectArray arguments)
{
    // Set up AndroidApp
    BASE_NS::unique_ptr<Test::TestApp> app =
        BASE_NS::make_unique<Test::AndroidApp>(env, thiz, context, surface, outputFile, arguments);
    // WARN: No dynamic_cast since it requires RTTI
    Test::g_androidApp = static_cast<Test::AndroidApp*>(app.get());

    Test::g_androidApp->Init();
    Test::g_androidApp->Run();
    int result = Test::g_androidApp->GetGTestResult();
    Test::g_androidApp->Terminate();

    return result;
}

extern "C" JNIEXPORT jint JNICALL Java_com_huawei_jpg_1src_1test_1runner_test_NativeTestRunner_onSurfaceCreatedNative(
    JNIEnv* env, jobject thiz, jobject context, jobject surface, jstring outputFile, jobjectArray arguments)
{
    // Set up AndroidApp
    BASE_NS::unique_ptr<Test::TestApp> app =
        BASE_NS::make_unique<Test::AndroidApp>(env, thiz, context, surface, outputFile, arguments);
    // WARN: No dynamic_cast since it requires RTTI
    Test::g_androidApp = static_cast<Test::AndroidApp*>(app.get());

    Test::g_androidApp->Init();
    Test::g_androidApp->Run();
    int result = Test::g_androidApp->GetGTestResult();
    Test::g_androidApp->Terminate();

    return result;
}