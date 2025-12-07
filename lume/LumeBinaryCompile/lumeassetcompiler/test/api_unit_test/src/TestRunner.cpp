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

#include "TestRunner.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h>
#include <fcntl.h>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#endif

#include <gtest/gtest.h>

#if defined(__ANDROID__)
static void startOutputFileRedirect(const char* filename, bool append);
static void stopOutputFileRedirect();
static int runLoggingThread();
#endif

namespace test {
namespace {
class TestRunnerEnv : public ::testing::Environment {
public:
    virtual void SetUp() override {}

    virtual void TearDown() override {}
};
} // namespace
} // namespace test

#if !defined(__ANDROID__)

int main(int argc, char** argv)
{
    printf("Running main() from TestRunner.cpp\n");
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::AddGlobalTestEnvironment(new test::TestRunnerEnv());
    return RUN_ALL_TESTS();
}

#else // __ANDROID__

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "unit_test", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "unit_test", __VA_ARGS__))

namespace {
//
// Runs googletest with given parameters.
// NOTE: that this can be only called once in a process as googletest cannot be initialized
// multiple times with different arguments.
//
// To write an xml/json output file use:
//  ::testing::GTEST_FLAG(output) = "xml:hello.xml";
//
static int runGoogleTest(int* argc, char** argv)
{
    LOGI("Running tests.");

    ::testing::InitGoogleTest(argc, argv);
    ::testing::AddGlobalTestEnvironment(new BASE_NS::test::TestRunnerEnv());
    LOGI("RUN_ALL_TESTS: start");
    int result = RUN_ALL_TESTS();
    LOGI("RUN_ALL_TESTS: done");

    return result;
}

} // namespace

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_huawei_agpbase_test_NativeTestRunner_getNativeTestCases(
    JNIEnv* env, jclass /* clazz */)
{
    int totalTestCaseCount = ::testing::UnitTest::GetInstance()->total_test_case_count();
    LOGI("Total cases: %d", totalTestCaseCount);

    jobjectArray testCaseArray = (jobjectArray)env->NewObjectArray(
        totalTestCaseCount, env->FindClass("java/lang/String"), env->NewStringUTF(""));

    for (int i = 0; i < totalTestCaseCount; ++i) {
        const char* testCaseName = ::testing::UnitTest::GetInstance()->GetTestCase(i)->name();
        LOGI("  case%d: %s", i, testCaseName);
        env->SetObjectArrayElement(testCaseArray, i, env->NewStringUTF(testCaseName));
    }

    return testCaseArray;
}

//
// This is for running the native tests from Java code. (e.g. from a test runner).
// Java signature:
//    native int runNative(Context context, String outputPath, String[] arguments);
//
extern "C" JNIEXPORT jint JNICALL Java_com_huawei_agpbase_test_NativeTestRunner_runNativeTests(
    JNIEnv* env, jclass /* clazz */, jobject context, jstring outputFile, jobjectArray arguments)
{
    const char* outputFileCString = env->GetStringUTFChars(outputFile, nullptr);
    std::string outputFileString(outputFileCString);
    env->ReleaseStringUTFChars(outputFile, outputFileCString);

    startOutputFileRedirect(outputFileString.c_str(), false);

    std::vector<char*> argv;

    if (arguments) {
        int argc = env->GetArrayLength(arguments);
        argv.reserve((unsigned int)(argc + 1));

        // Add the app name as the first parameter.
        argv.push_back(strdup("BaseAPITestRunner"));

        for (int i = 0; i < argc; i++) {
            jobject element = env->GetObjectArrayElement(arguments, i);
            const char* elementCString = env->GetStringUTFChars((jstring)element, nullptr);
            argv.push_back(strdup(elementCString));
            env->ReleaseStringUTFChars((jstring)element, elementCString);
        }
    }

    LOGI("Results to: '%s'", outputFileString.c_str());

    // Passing a copy because googletest removes strings that it's using.
    int argcTmp = argv.size();
    std::vector<char*> argvTmp = argv;
    int result = ::runGoogleTest(&argcTmp, &argvTmp[0]);

    for (int i = 0; i < argv.size(); i++) {
        free(argv[i]);
        argv[i] = nullptr;
    }

    stopOutputFileRedirect();

    return result;
}

// #define REDIRECT_TO_FILE
#define REDIRECT_TO_LOG

namespace {

// Retrieve extra parameters from the intent that launched the activity.
std::string getIntentExtra(JNIEnv* env, jobject context, const std::string& parameter)
{
    jclass acl = env->GetObjectClass(context);
    jmethodID getIntentId = env->GetMethodID(acl, "getIntent", "()Landroid/content/Intent;");
    jobject intent = env->CallObjectMethod(context, getIntentId);

    jclass icl = env->GetObjectClass(intent);
    jmethodID getStringExtraId = env->GetMethodID(icl, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");

    jstring jsExtra =
        static_cast<jstring>(env->CallObjectMethod(intent, getStringExtraId, env->NewStringUTF(parameter.c_str())));
    if (jsExtra) {
        const char* extra = env->GetStringUTFChars(static_cast<jstring>(jsExtra), 0);
        std::string result(extra);
        env->ReleaseStringUTFChars(jsExtra, extra);
        return result;
    }
    return {};
}
} // namespace

//
// A simple native activity for running tests without the test orchestrator.
//
// To define options to googletest you can add something like this to "am start":
//   -e googletest --gtest_filter=TaskQueueTest.*:EnvTest.*
//   -e googletest "--option --someOtherOption"
//
// (In Android Studio found from: Run -> Edit Configurations... -> Launch Flags)
//
void android_main(struct android_app* state)
{
#if defined(REDIRECT_TO_FILE)
    // NOTE: the internal path cannot be accessed from adb etc. but is guaranteed to work.
    std::string outputPath = state->activity->internalDataPath;
    // std::string outputPath = state->activity->externalDataPath;

    std::string reportTxtFile = outputPath + "/report.txt";
    startOutputFileRedirect(reportTxtFile.c_str(), false);
#elif defined(REDIRECT_TO_LOG)
    // Start redirecting stdout to logcat.
    runLoggingThread();
#endif

    // Attach current threads to allow jni calls.
    JNIEnv* env = nullptr;
    state->activity->vm->AttachCurrentThread(&env, NULL);

    // Create argc from intent extra parameters.
    std::string googletestArguments = getIntentExtra(env, state->activity->clazz, "googletest");
    std::istringstream iss(googletestArguments);
    std::vector<char*> args;
    args.push_back(new char[20]{ "BaseAPITestRunner" });
    std::string token;
    while (iss >> token) {
        char* arg = new char[token.size() + 1];
        token.copy(arg, token.size());
        arg[token.size()] = '\0';
        args.push_back(arg);
    }

    int argc = args.size();
    int result = ::runGoogleTest(&argc, &args[0]);

    // Release the parameters.
    for (auto& arg : args) {
        delete[] arg;
    }

#if defined(REDIRECT_TO_FILE)
    stopOutputFileRedirect();
    std::ifstream ifs(reportTxtFile);
    std::stringstream ss;
    ss << ifs.rdbuf();
    LOGI("  Test output:\n%s", ss.str().c_str());
#endif

    // Stop the activity.
    ANativeActivity_finish(state->activity);

    while (true) {
        int events;
        struct android_poll_source* source = nullptr;

        while ((ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {
            if (source != NULL) {
                source->process(state, source);
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                LOGI("Exiting test runner.");
                state->activity->vm->DetachCurrentThread();
                // NOTE: could set the result as the result code for the activity.
                // NOTE: why doesn't return completely exit the program?
                // (Need to press stop in android studio)
                // return;
                exit(result);
            }
        }
    }
}

//
// Redirecting stdout and stderr to a file
//
static int fd;

static void startOutputFileRedirect(const char* filename, bool append)
{
    LOGI("Redirecting test output to: '%s'", filename);

    if (!append) {
        if (remove(filename) != 0) {
            LOGW("Could not delete old output file.");
        }
    }

    fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0660);
    assert(fd >= 0);
    dup2(fd, 1);
    dup2(fd, 2);
}

static void stopOutputFileRedirect()
{
    fflush(stdout);
    fflush(stderr);
    close(fd);
}

//
// Redirecting stdout and stderr with a thread.
// from: https://stackoverflow.com/a/42715692
//
static int pfd[2];
static pthread_t loggingThread;
static const char* LOG_TAG = "unit_test";

static void* loggingFunction(void*)
{
    ssize_t readSize;
    char buf[1024];

    while ((readSize = read(pfd[0], buf, sizeof buf - 1)) > 0) {
        if (buf[readSize - 1] == '\n') {
            --readSize;
        }
        buf[readSize] = 0;                                   // add null-terminator
        __android_log_write(ANDROID_LOG_INFO, LOG_TAG, buf); // Set any log level you want
    }

    return 0;
}

// run this function to redirect your output to android log
static int runLoggingThread()
{
    setvbuf(stdout, 0, _IOLBF, 0); // make stdout line-buffered
    setvbuf(stderr, 0, _IONBF, 0); // make stderr unbuffered

    // create the pipe and redirect stdout and stderr
    pipe(pfd);
    dup2(pfd[1], 1);
    dup2(pfd[1], 2);

    // spawn the logging thread
    if (pthread_create(&loggingThread, 0, loggingFunction, 0) == -1) {
        return -1;
    }
    pthread_detach(loggingThread);
    return 0;
}

#endif // __ANDROID__

// Just making sure that RTTI is disabled. Remove this if we decide to enable RTTI at some point.
#if defined(_CPPRTTI) || defined(__GXX_RTTI)
// #error RTTI not disabled
#endif

TEST(API_EnvTest, env) {}
