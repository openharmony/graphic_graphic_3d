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

#include <gtest/gtest.h>
#include <fstream>

#include "util.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Rosen {
namespace {
    constexpr int32_t TEST_DURATION = 60;
    constexpr int32_t TEST_SIDE_LEN = 720;
    constexpr int32_t TEST_VP = 12;
    constexpr float TEST_RATIO = 360.0f;
    constexpr unsigned long TEST_FILE_SIZE = 1000;
    constexpr int32_t TEST_ROTATE_DEGREE = 270;
    constexpr int32_t TEST_BOOT_SOUND_VOLUME = 5;
    constexpr int32_t TEST_ERROR_CODE = -1;
}

class UtilTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void UtilTest::SetUpTestCase() {}
void UtilTest::TearDownTestCase() {}
void UtilTest::SetUp() {}
void UtilTest::TearDown() {}

HWTEST_F(UtilTest, UtilTest_001, TestSize.Level1)
{
    std::vector<BootAnimationConfig> configs;

    std::string jsonStr1 = "{}";
    cJSON* jsonData1 = cJSON_Parse(jsonStr1.c_str());
    OHOS::ParseOldConfigFile(jsonData1, configs);
}

HWTEST_F(UtilTest, UtilTest_002, TestSize.Level1)
{
    bool isMultiDisplay = false;
    std::vector<BootAnimationConfig> configs;

    std::string jsonStr1 = "{}";
    cJSON* jsonData1 = cJSON_Parse(jsonStr1.c_str());
    OHOS::ParseNewConfigFile(jsonData1, isMultiDisplay, configs);

    std::string jsonStr2 = "{\"cust.bootanimation.multi_display\":1}";
    cJSON* jsonData2 = cJSON_Parse(jsonStr2.c_str());
    OHOS::ParseNewConfigFile(jsonData2, isMultiDisplay, configs);

    std::string jsonStr3 = "{\"cust.bootanimation.multi_display\":false}";
    cJSON* jsonData3 = cJSON_Parse(jsonStr3.c_str());
    OHOS::ParseNewConfigFile(jsonData3, isMultiDisplay, configs);
    EXPECT_EQ(isMultiDisplay, false);

    std::string jsonStr4 = "{\"cust.bootanimation.multi_display\":true}";
    cJSON* jsonData4 = cJSON_Parse(jsonStr4.c_str());
    OHOS::ParseNewConfigFile(jsonData4, isMultiDisplay, configs);

    std::string jsonStr5 = "{\"screen_config\":[]}";
    cJSON* jsonData5 = cJSON_Parse(jsonStr5.c_str());
    OHOS::ParseNewConfigFile(jsonData5, isMultiDisplay, configs);

    std::string jsonStr6 = "{\"screen_config\":[{}]}";
    cJSON* jsonData6 = cJSON_Parse(jsonStr6.c_str());
    OHOS::ParseNewConfigFile(jsonData6, isMultiDisplay, configs);
}

HWTEST_F(UtilTest, UtilTest_003, TestSize.Level1)
{
    BootAnimationConfig config;
    std::string jsonStr1 = "{}";
    cJSON* jsonData1 = cJSON_Parse(jsonStr1.c_str());
    OHOS::ParseVideoExtraPath(jsonData1, config);

    std::string jsonStr2 = "[]";
    cJSON* jsonData2 = cJSON_Parse(jsonStr2.c_str());
    OHOS::ParseVideoExtraPath(jsonData2, config);

    std::string jsonStr3 = "{\"1\":\"abc\"}";
    cJSON* jsonData3 = cJSON_Parse(jsonStr3.c_str());
    OHOS::ParseVideoExtraPath(jsonData3, config);
    EXPECT_EQ(config.videoExtPath.size(), 1);
}

HWTEST_F(UtilTest, UtilTest_004, TestSize.Level1)
{
    BootAnimationConfig config;
    int32_t duration;
    std::string jsonStr1 = "{}";
    cJSON* jsonData1 = cJSON_Parse(jsonStr1.c_str());
    OHOS::ParseBootDuration(jsonData1, duration);

    std::string jsonStr2 = "{\"cust.bootanimation.duration\":10}";
    cJSON* jsonData2 = cJSON_Parse(jsonStr2.c_str());
    OHOS::ParseBootDuration(jsonData2, duration);

    std::string jsonStr3 = "{\"cust.bootanimation.duration\":\"10\"}";
    cJSON* jsonData3 = cJSON_Parse(jsonStr3.c_str());
    OHOS::ParseBootDuration(jsonData3, duration);
    EXPECT_EQ(duration, 10);
}

HWTEST_F(UtilTest, UtilTest_005, TestSize.Level1)
{
    std::string filePath = "";
    bool isFileExist = OHOS::IsFileExisted(filePath);
    EXPECT_EQ(false, isFileExist);

    filePath = "/sys_prod/etc/bootanimation/bootanimation_custom_config.json1";
    isFileExist = OHOS::IsFileExisted(filePath);
    EXPECT_EQ(false, isFileExist);
}

HWTEST_F(UtilTest, UtilTest_006, TestSize.Level1)
{
    std::string filePath = "";
    std::vector<BootAnimationConfig> animationConfigs;
    bool isMultiDisplay = false;
    bool isCompatible = false;
    int32_t duration = TEST_DURATION;
    bool parseResult = OHOS::ParseBootConfig(filePath, duration, isCompatible, isMultiDisplay, animationConfigs);
    EXPECT_EQ(false, parseResult);

    filePath = "/sys_prod/etc/bootanimation/bootanimation_custom_config.json1";
    parseResult = OHOS::ParseBootConfig(filePath, duration, isCompatible, isMultiDisplay, animationConfigs);
    EXPECT_EQ(false, parseResult);

    filePath = "/sys_prod/etc/bootanimation/bootanimation_custom_config.json";
    parseResult = OHOS::ParseBootConfig(filePath, duration, isCompatible, isMultiDisplay, animationConfigs);
    bool isFileExist = OHOS::IsFileExisted(filePath);
    EXPECT_EQ(isFileExist ? true : false, parseResult);
}

HWTEST_F(UtilTest, UtilTest_007, TestSize.Level1)
{
    std::string filePath = "";
    std::string content = OHOS::ReadFile(filePath);
    EXPECT_EQ(0, content.length());

    filePath = "/sys_prod/etc/bootanimation/bootanimation_custom_config.json1";
    content = OHOS::ReadFile(filePath);
    EXPECT_EQ(0, content.length());

    filePath = "/sys_prod/etc/bootanimation/bootanimation_custom_config.json";
    content = OHOS::ReadFile(filePath);
    bool isFileExist = OHOS::IsFileExisted(filePath);
    EXPECT_EQ(content.empty(), isFileExist ? false : true);
}

HWTEST_F(UtilTest, UtilTest_008, TestSize.Level1)
{
    std::string hingStatusInfoPath = "/sys/class/sensors/hinge_sensor/hinge_status_info";
    std::string content = OHOS::GetHingeStatus();
    if (OHOS::IsFileExisted(hingStatusInfoPath)) {
        EXPECT_NE(0, content.length());
    } else {
        EXPECT_EQ(0, content.length());
    }
}

HWTEST_F(UtilTest, UtilTest_009, TestSize.Level1)
{
    const char* fileBuffer;
    int totalsize = 0;
    FrameRateConfig frameConfig;
    bool result = OHOS::ParseImageConfig(fileBuffer, totalsize, frameConfig);
    EXPECT_FALSE(result);
}

HWTEST_F(UtilTest, UtilTest_010, TestSize.Level1)
{
    std::string filename = "abc";
    std::shared_ptr<ImageStruct> imageStruct = std::make_shared<ImageStruct>();
    int32_t bufferLen = 0;
    ImageStructVec imgVec;
    bool result = OHOS::CheckImageData(filename, imageStruct, bufferLen, imgVec);
    EXPECT_FALSE(result);
    unsigned long fileSize = TEST_FILE_SIZE;
    imageStruct->memPtr.SetBufferSize(fileSize);
    result = OHOS::CheckImageData(filename, imageStruct, bufferLen, imgVec);
    EXPECT_TRUE(result);
}

HWTEST_F(UtilTest, UtilTest_011, TestSize.Level2)
{
    std::string filePath = "/sys_prod/etc/bootanimation/bootanimation_custom_config.json1";
    std::map<int32_t, BootAnimationProgressConfig> configs;
    OHOS::ParseProgressConfig(filePath, configs);
    EXPECT_EQ(configs.empty(), true);
}

HWTEST_F(UtilTest, UtilTest_012, TestSize.Level1)
{
    std::map<int32_t, BootAnimationProgressConfig> configs;
    std::string jsonStr = "{\"1\":\"abc\"}";
    cJSON* jsonData = cJSON_Parse(jsonStr.c_str());
    OHOS::ParseProgressData(jsonData, configs);
    EXPECT_EQ(configs.empty(), true);

    jsonStr = "{\"progress_config\":[]}";
    jsonData = cJSON_Parse(jsonStr.c_str());
    OHOS::ParseProgressData(jsonData, configs);
    EXPECT_EQ(configs.empty(), true);

    jsonStr = "{\"progress_config\":[{\"cust.bootanimation.screen_id\":\"0\"}]}";
    jsonData = cJSON_Parse(jsonStr.c_str());
    OHOS::ParseProgressData(jsonData, configs);
    EXPECT_EQ(configs.empty(), false);

    jsonStr = "{\"progress_config\":[{\"cust.bootanimation.progress_screen_id\":\"1\", "
    "\"cust.bootanimation.progress_font_size\":\"3\", \"cust.bootanimation.progress_radius_size\":\"10\", "
    "\"cust.bootanimation.progress_x_offset\":\"-1088\", \"cust.bootanimation.progress_degree90\", "
    "\"cust.bootanimation.progress_height\":\"2232\", \"cust.bootanimation.progress_frame_height\":\"112\"}]}";
    jsonData = cJSON_Parse(jsonStr.c_str());
    OHOS::ParseProgressData(jsonData, configs);
    EXPECT_EQ(configs.empty(), false);
}

HWTEST_F(UtilTest, UtilTest_013, TestSize.Level2)
{
    std::string str = "";
    EXPECT_EQ(OHOS::StringToInt32(str), 0);
    str = "1234567890";
    EXPECT_EQ(OHOS::StringToInt32(str), 0);
    str = "-123";
    EXPECT_EQ(OHOS::StringToInt32(str), -123);
    str = "123ab";
    EXPECT_EQ(OHOS::StringToInt32(str), 0);
    str = "123";
    EXPECT_EQ(OHOS::StringToInt32(str), 123);
}

HWTEST_F(UtilTest, UtilTest_014, TestSize.Level1)
{
    int32_t sideLen = TEST_SIDE_LEN;
    int32_t vp = TEST_VP;
    int32_t result = OHOS::TranslateVp2Pixel(sideLen, vp);
    EXPECT_GT(result, 0);
}

HWTEST_F(UtilTest, UtilTest_015, TestSize.Level1)
{
    int32_t sideLen = TEST_SIDE_LEN;
    int32_t vp = TEST_VP;
    float ratio = 0;
    int32_t result = OHOS::TranslateVp2Pixel(sideLen, vp, ratio);
    EXPECT_EQ(result, 0);
}

HWTEST_F(UtilTest, UtilTest_016, TestSize.Level1)
{
    int32_t sideLen = TEST_SIDE_LEN;
    int32_t vp = TEST_VP;
    float ratio = TEST_RATIO;
    int32_t result = OHOS::TranslateVp2Pixel(sideLen, vp, ratio);
    EXPECT_GT(result, 0);
}

HWTEST_F(UtilTest, UtilTest_017, TestSize.Level1)
{
    OHOS::PostTask([]() {}, 0);
    EXPECT_TRUE(true);
}

HWTEST_F(UtilTest, UtilTest_018, TestSize.Level1)
{
    int32_t result = OHOS::GetSystemCurrentTime();
    EXPECT_GT(result, 0);
}

HWTEST_F(UtilTest, UtilTest_019, TestSize.Level1)
{
    std::string deviceType = OHOS::GetDeviceType();
    EXPECT_NE(deviceType.empty(), true);
}

HWTEST_F(UtilTest, UtilTest_020, TestSize.Level1)
{
    ImageStructVec imgVec;
    OHOS::SortZipFile(imgVec);
    EXPECT_EQ(imgVec.size(), 0);
}

HWTEST_F(UtilTest, UtilTest_021, TestSize.Level1)
{
    std::string invalidPath = "/invalid/path/test.zip";
    ImageStructVec imgVec;
    FrameRateConfig frameConfig;
    bool result = OHOS::ReadZipFile(invalidPath, imgVec, frameConfig);
    EXPECT_EQ(result, false);
}

HWTEST_F(UtilTest, UtilTest_022, TestSize.Level1)
{
    unzFile zipFile = nullptr;
    bool result = OHOS::CloseZipFile(zipFile, false);
    EXPECT_EQ(result, false);
}

HWTEST_F(UtilTest, UtilTest_023, TestSize.Level1)
{
    unzFile zipFile = nullptr;
    std::string fileName = "test.png";
    ImageStructVec imgVec;
    FrameRateConfig frameConfig;
    bool result = OHOS::ReadImageFile(zipFile, fileName, imgVec, frameConfig, TEST_FILE_SIZE);
    EXPECT_EQ(result, false);
}

HWTEST_F(UtilTest, UtilTest_024, TestSize.Level1)
{
    std::string jsonStr = "{\"FrameRate\":60}";
    cJSON* jsonData = cJSON_Parse(jsonStr.c_str());
    FrameRateConfig frameConfig;
    char* buffer = jsonStr.data();
    bool result = OHOS::ParseImageConfig(buffer, jsonStr.length(), frameConfig);
    cJSON_Delete(jsonData);
    EXPECT_TRUE(result);
}

HWTEST_F(UtilTest, UtilTest_025, TestSize.Level1)
{
    cJSON* overallData = nullptr;
    int32_t duration = 0;
    OHOS::ParseBootDuration(overallData, duration);
    EXPECT_EQ(duration, 0);
}

HWTEST_F(UtilTest, UtilTest_026, TestSize.Level1)
{
    std::string path = "";
    cJSON* result = OHOS::ParseFileConfig(path);
    EXPECT_EQ(result, nullptr);
}

HWTEST_F(UtilTest, UtilTest_027, TestSize.Level1)
{
    BootAnimationConfig config;
    std::string jsonStr = "{\"screen1\":\"path1\",\"screen2\":\"path2\"}";
    cJSON* jsonData = cJSON_Parse(jsonStr.c_str());
    OHOS::ParseVideoExtraPath(jsonData, config);
    cJSON_Delete(jsonData);
    EXPECT_EQ(config.videoExtPath.size(), 2);
}

/**
 * @tc.name: StringToInt32_BoundaryMaxLength_ReturnCorrectValue
 * @tc.desc: Verify the StringToInt32 function with maximum length string.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, StringToInt32_BoundaryMaxLength_ReturnCorrectValue, TestSize.Level1)
{
    std::string str = "123456789";
    EXPECT_EQ(OHOS::StringToInt32(str), 123456789);
}

/**
 * @tc.name: StringToInt32_ExceedMaxLength_ReturnZero
 * @tc.desc: Verify the StringToInt32 function with string exceeding maximum length.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, StringToInt32_ExceedMaxLength_ReturnZero, TestSize.Level1)
{
    std::string str = "1234567890";
    EXPECT_EQ(OHOS::StringToInt32(str), 0);
}

/**
 * @tc.name: TranslateVp2Pixel_ZeroRatio_ReturnZero
 * @tc.desc: Verify the TranslateVp2Pixel function with zero ratio.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, TranslateVp2Pixel_ZeroRatio_ReturnZero, TestSize.Level1)
{
    int32_t sideLen = TEST_SIDE_LEN;
    int32_t vp = TEST_VP;
    float ratio = 0;
    int32_t result = OHOS::TranslateVp2Pixel(sideLen, vp, ratio);
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: IsFileExisted_EmptyPath_ReturnFalse
 * @tc.desc: Verify the IsFileExisted function with empty path.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, IsFileExisted_EmptyPath_ReturnFalse, TestSize.Level1)
{
    std::string filePath = "";
    bool isFileExist = OHOS::IsFileExisted(filePath);
    EXPECT_EQ(false, isFileExist);
}

/**
 * @tc.name: ParseBootConfig_EmptyPath_ReturnFalse
 * @tc.desc: Verify the ParseBootConfig function with empty path.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, ParseBootConfig_EmptyPath_ReturnFalse, TestSize.Level1)
{
    std::string filePath = "";
    std::vector<BootAnimationConfig> animationConfigs;
    bool isMultiDisplay = false;
    bool isCompatible = false;
    int32_t duration = TEST_DURATION;
    bool parseResult = OHOS::ParseBootConfig(filePath, duration, isCompatible, isMultiDisplay, animationConfigs);
    EXPECT_EQ(false, parseResult);
}

/**
 * @tc.name: CheckImageData_NullBuffer_ReturnFalse
 * @tc.desc: Verify the CheckImageData function with null buffer.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, CheckImageData_NullBuffer_ReturnFalse, TestSize.Level1)
{
    std::string filename = "abc";
    std::shared_ptr<ImageStruct> imageStruct = std::make_shared<ImageStruct>();
    int32_t bufferLen = 0;
    ImageStructVec imgVec;
    bool result = OHOS::CheckImageData(filename, imageStruct, bufferLen, imgVec);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ReadZipFile_NullZipFile_ReturnFalse
 * @tc.desc: Verify the ReadZipFile function with invalid zip path.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, ReadZipFile_NullZipFile_ReturnFalse, TestSize.Level1)
{
    std::string invalidPath = "/invalid/path/test.zip";
    ImageStructVec imgVec;
    FrameRateConfig frameConfig;
    bool result = OHOS::ReadZipFile(invalidPath, imgVec, frameConfig);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: ParseImageConfig_NullBuffer_ReturnFalse
 * @tc.desc: Verify the ParseImageConfig function with null buffer.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, ParseImageConfig_NullBuffer_ReturnFalse, TestSize.Level1)
{
    const char* fileBuffer;
    int totalsize = 0;
    FrameRateConfig frameConfig;
    bool result = OHOS::ParseImageConfig(fileBuffer, totalsize, frameConfig);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ReadFile_NullPath_ReturnEmpty
 * @tc.desc: Verify the ReadFile function with null path.
 * @tc.type: FUNC
 */
HWTEST_F(UtilTest, ReadFile_NullPath_ReturnEmpty, TestSize.Level1)
{
    std::string filePath = "";
    std::string content = OHOS::ReadFile(filePath);
    EXPECT_EQ(0, content.length());
}
}

class BootVideoPlayerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void BootVideoPlayerTest::SetUpTestCase() {}
void BootVideoPlayerTest::TearDownTestCase() {}
void BootVideoPlayerTest::SetUp() {}
void BootVideoPlayerTest::TearDown() {}

/**
 * @tc.name: OnError_CallbackInvoked_CallbackFlagSet
 * @tc.desc: Verify the OnError function invokes callback correctly.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, OnError_CallbackInvoked_CallbackFlagSet, TestSize.Level1)
{
    PlayerParams params;
    int flag = 0;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&flag](void*) { flag = 1; },
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    std::shared_ptr<VideoPlayerCallback> cb = std::make_shared<VideoPlayerCallback>(player);
    cb->OnError(TEST_ERROR_CODE, "test");
    EXPECT_EQ(flag, 1);
}

/**
 * @tc.name: StopVideo_CallbackInvoked_CallbackFlagSet
 * @tc.desc: Verify the StopVideo function invokes callback correctly.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, StopVideo_CallbackInvoked_CallbackFlagSet, TestSize.Level1)
{
    PlayerParams params;
    int flag = 0;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&flag](void*) { flag = 1; },
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    player->StopVideo();
    EXPECT_EQ(flag, 1);
}

/**
 * @tc.name: SetVideoSound_DifferentSoundSettings_VolumeSetCorrectly
 * @tc.desc: Verify the SetVideoSound function with different sound settings.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, SetVideoSound_DifferentSoundSettings_VolumeSetCorrectly, TestSize.Level1)
{
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };

    PlayerParams params1;
    params1.soundEnabled = true;
    params1.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player1 = std::make_shared<BootVideoPlayer>(params1);
    ASSERT_NE(player1, nullptr);
    player1->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    player1->SetVideoSound();
    EXPECT_TRUE(true);

    PlayerParams params2;
    params2.soundEnabled = false;
    params2.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player2 = std::make_shared<BootVideoPlayer>(params2);
    ASSERT_NE(player2, nullptr);
    player2->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    player2->SetVideoSound();
    EXPECT_TRUE(true);

    PlayerParams params3;
    params3.soundEnabled = true;
    params3.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player3 = std::make_shared<BootVideoPlayer>(params3);
    ASSERT_NE(player3, nullptr);
    player3->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    BootAnimationUtils::SetBootAnimationSoundEnabled(false);
    EXPECT_EQ(BootAnimationUtils::GetBootAnimationSoundEnabled(), false);
    player3->SetVideoSound();
    EXPECT_TRUE(true);

    PlayerParams params4;
    params4.soundEnabled = true;
    params4.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player4 = std::make_shared<BootVideoPlayer>(params4);
    ASSERT_NE(player4, nullptr);
    player4->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    BootAnimationUtils::SetBootAnimationSoundEnabled(true);
    player4->SetVideoSound();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnInfo_DifferentInfoTypes_HandledCorrectly
 * @tc.desc: Verify the OnInfo function handles different info types correctly.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, OnInfo_DifferentInfoTypes_HandledCorrectly, TestSize.Level1)
{
    PlayerParams params;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    player->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    std::shared_ptr<VideoPlayerCallback> cb = std::make_shared<VideoPlayerCallback>(player);
    Media::Format format;
    cb->OnInfo(Media::INFO_TYPE_SEEKDONE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_SPEEDDONE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_BITRATEDONE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_EOS, 0, format);
    cb->OnInfo(Media::INFO_TYPE_BUFFERING_UPDATE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_BITRATE_COLLECT, 0, format);
    cb->OnInfo(Media::INFO_TYPE_STATE_CHANGE, 0, format);

    if (player->GetMediaPlayer() != nullptr) {
        cb->OnInfo(Media::INFO_TYPE_STATE_CHANGE, Media::PlayerStates::PLAYER_PREPARED, format);
    }

    cb->OnInfo(Media::INFO_TYPE_POSITION_UPDATE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_MESSAGE, 0, format);
    system::SetParameter(BOOT_ANIMATION_STARTED, "false");
    cb->OnInfo(Media::INFO_TYPE_MESSAGE, 0, format);
    EXPECT_EQ(system::GetBoolParameter(BOOT_ANIMATION_STARTED, false), true);
    cb->OnInfo(Media::INFO_TYPE_RESOLUTION_CHANGE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_VOLUME_CHANGE, 0, format);
    cb->OnInfo(Media::INFO_TYPE_SUBTITLE_UPDATE, 0, format);
}

/**
 * @tc.name: Play_NullSurface_MediaPlayerCreated
 * @tc.desc: Verify the Play function creates mediaPlayer when surface is null.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, Play_NullSurface_MediaPlayerCreated, TestSize.Level1)
{
    PlayerParams params;
    params.surface = nullptr;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    player->Play();
    ASSERT_NE(nullptr, player->mediaPlayer_);
}

/**
 * @tc.name: IsNormalBoot_DifferentBootReasons_ReturnCorrectResult
 * @tc.desc: Verify the IsNormalBoot function returns correct result for different boot reasons.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, IsNormalBoot_DifferentBootReasons_ReturnCorrectResult, TestSize.Level1)
{
    PlayerParams params;
    params.surface = nullptr;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    bool result = player->IsNormalBoot();
    std::string bootReason = system::GetParameter("ohos.boot.reboot_reason", "");
    std::vector<std::string> reasonArr = {"AP_S_COLDBOOT", "bootloader", "recovery", "fastbootd",
        "resetfactory", "at2resetfactory", "atfactoryreset0", "resetuser", "sdupdate", "chargereboot",
        "resize", "erecovery", "usbupdate", "cust", "oem_rtc", "UNKNOWN", "mountfail", "hungdetect",
        "COLDBOOT", "updatedataimg", "AP_S_FASTBOOTFLASH", "gpscoldboot", "AP_S_COMBINATIONKEY",
        "CP_S_NORMALRESET", "IOM3_S_USER_EXCEPTION", "BR_UPDATE_USB", "BR_UPDATA_SD_FORCE",
        "BR_KEY_VOLUMN_UP", "BR_PRESS_1S", "BR_CHECK_RECOVERY", "BR_CHECK_ERECOVERY",
        "BR_CHECK_SDUPDATE", "BR_CHECK_USBUPDATE", "BR_CHECK_RESETFACTORY", "BR_CHECK_HOTAUPDATE",
        "BR_POWERONNOBAT", "BR_NOGUI", "BR_FACTORY_VERSION", "BR_RESET_HAPPEN", "BR_POWEROFF_ALARM",
        "BR_POWEROFF_CHARGE", "BR_POWERON_BY_SMPL", "BR_CHECK_UPDATEDATAIMG", "BR_POWERON_CHARGE",
        "AP_S_PRESS6S", "BR_PRESS_10S"};
    if (std::find(reasonArr.begin(), reasonArr.end(), bootReason) != reasonArr.end()) {
        EXPECT_EQ(result, true);
    } else {
        EXPECT_EQ(result, false);
    }
}

/**
 * @tc.name: Play_FrameRateEnabled_PlaySuccessfully
 * @tc.desc: Verify the Play function with frameRateEnabled flag.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, Play_FrameRateEnabled_PlaySuccessfully, TestSize.Level1)
{
    PlayerParams params;
    params.surface = nullptr;
    params.isFrameRateEnable = true;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    ASSERT_NE(player, nullptr);
    player->Play();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: SetVideoSound_HmosUpdate_SoundSetCorrectly
 * @tc.desc: Verify the SetVideoSound function during HMOS update.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, SetVideoSound_HmosUpdate_SoundSetCorrectly, TestSize.Level0)
{
    system::SetParameter("persist.update.hmos_to_next_flag", "1");
    PlayerParams params;
    params.surface = nullptr;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    params.soundEnabled = true;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    ASSERT_NE(player, nullptr);
    player->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    BootAnimationUtils::SetBootAnimationSoundEnabled(true);
    system::SetParameter(BOOT_SOUND, std::to_string(TEST_BOOT_SOUND_VOLUME));
    player->SetVideoSound();
    EXPECT_TRUE(true);
    system::SetParameter("persist.update.hmos_to_next_flag", "0");
}

/**
 * @tc.name: SetVideoSound_InvalidVolume_SoundSetCorrectly
 * @tc.desc: Verify the SetVideoSound function with invalid volume.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, SetVideoSound_InvalidVolume_SoundSetCorrectly, TestSize.Level0)
{
    PlayerParams params;
    params.surface = nullptr;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    params.soundEnabled = true;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    ASSERT_NE(player, nullptr);
    player->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    BootAnimationUtils::SetBootAnimationSoundEnabled(true);
    system::SetParameter(BOOT_SOUND, std::to_string(INVALID_VOLUME));
    player->SetVideoSound();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: ResPath_EmptyPath_PathSetCorrectly
 * @tc.desc: Verify the resPath_ member variable with empty path.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, ResPath_EmptyPath_PathSetCorrectly, TestSize.Level1)
{
    PlayerParams params;
    params.resPath = "";
    params.surface = nullptr;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    ASSERT_NE(player, nullptr);
    player->resPath_ = "";
    EXPECT_EQ(player->resPath_, "");
}

/**
 * @tc.name: OnInfo_EosInfo_HandledCorrectly
 * @tc.desc: Verify the OnInfo function handles EOS info type correctly.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, OnInfo_EosInfo_HandledCorrectly, TestSize.Level1)
{
    PlayerParams params;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    ASSERT_NE(player, nullptr);
    player->mediaPlayer_ = Media::PlayerFactory::CreatePlayer();
    std::shared_ptr<VideoPlayerCallback> cb = std::make_shared<VideoPlayerCallback>(player);
    ASSERT_NE(cb, nullptr);
    Media::Format format;
    cb->OnInfo(Media::INFO_TYPE_EOS, 0, format);
    EXPECT_TRUE(true);
}

/**
 * @tc.name: OnError_ErrorCallback_CalledCorrectly
 * @tc.desc: Verify the OnError function calls error callback correctly.
 * @tc.type: FUNC
 */
HWTEST_F(BootVideoPlayerTest, OnError_ErrorCallback_CalledCorrectly, TestSize.Level1)
{
    PlayerParams params;
    BootAnimationCallback callback_ = {
        .userData = this,
        .callback = [&](void*) {},
    };
    params.callback = &callback_;
    std::shared_ptr<BootVideoPlayer> player = std::make_shared<BootVideoPlayer>(params);
    ASSERT_NE(player, nullptr);
    std::shared_ptr<VideoPlayerCallback> cb = std::make_shared<VideoPlayerCallback>(player);
    ASSERT_NE(cb, nullptr);
    cb->OnError(TEST_ERROR_CODE, "test error");
    EXPECT_TRUE(true);
}

class BootSoundPlayerTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void BootSoundPlayerTest::SetUpTestCase() {}
void BootSoundPlayerTest::TearDownTestCase() {}
void BootSoundPlayerTest::SetUp() {}
void BootSoundPlayerTest::TearDown() {}

/**
 * @tc.name: Play_DifferentSoundSettings_ExecuteSuccessfully
 * @tc.desc: Verify the Play function with different sound settings.
 * @tc.type: FUNC
 */
HWTEST_F(BootSoundPlayerTest, Play_DifferentSoundSettings_ExecuteSuccessfully, TestSize.Level1)
{
    PlayerParams params1;
    params1.soundEnabled = false;
    std::shared_ptr<BootSoundPlayer> player1 = std::make_shared<BootSoundPlayer>(params1);
    ASSERT_NE(player1, nullptr);
    player1->Play();
    EXPECT_TRUE(true);

    PlayerParams params2;
    params2.soundEnabled = true;
    std::shared_ptr<BootSoundPlayer> player2 = std::make_shared<BootSoundPlayer>(params2);
    ASSERT_NE(player2, nullptr);
    player2->Play();
    EXPECT_TRUE(true);

    PlayerParams params3;
    params3.soundEnabled = true;
    std::shared_ptr<BootSoundPlayer> player3 = std::make_shared<BootSoundPlayer>(params3);
    ASSERT_NE(player3, nullptr);
    BootAnimationUtils::SetBootAnimationSoundEnabled(false);
    EXPECT_EQ(BootAnimationUtils::GetBootAnimationSoundEnabled(), false);
    player3->Play();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: Play_SoundEnabledMinVolume_ExecuteSuccessfully
 * @tc.desc: Verify the Play function with sound enabled and min volume.
 * @tc.type: FUNC
 */
HWTEST_F(BootSoundPlayerTest, Play_SoundEnabledMinVolume_ExecuteSuccessfully, TestSize.Level1)
{
    PlayerParams params;
    params.soundEnabled = true;
    std::shared_ptr<BootSoundPlayer> player = std::make_shared<BootSoundPlayer>(params);
    ASSERT_NE(player, nullptr);
    BootAnimationUtils::SetBootAnimationSoundEnabled(true);
    system::SetParameter(BOOT_SOUND, std::to_string(MIN_VOLUME));
    EXPECT_EQ(BootAnimationUtils::GetBootAnimationSoundEnabled(), true);
    player->Play();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: Play_SoundEnabledAboveMinVolume_ExecuteSuccessfully
 * @tc.desc: Verify the Play function with sound enabled and above min volume.
 * @tc.type: FUNC
 */
HWTEST_F(BootSoundPlayerTest, Play_SoundEnabledAboveMinVolume_ExecuteSuccessfully, TestSize.Level1)
{
    PlayerParams params;
    params.soundEnabled = true;
    std::shared_ptr<BootSoundPlayer> player = std::make_shared<BootSoundPlayer>(params);
    ASSERT_NE(player, nullptr);
    BootAnimationUtils::SetBootAnimationSoundEnabled(true);
    system::SetParameter(BOOT_SOUND, std::to_string(MIN_VOLUME + 1));
    EXPECT_EQ(BootAnimationUtils::GetBootAnimationSoundEnabled(), true);
    player->Play();
    EXPECT_TRUE(true);
}

/**
 * @tc.name: Play_DifferentStates_ExecuteSuccessfully
 * @tc.desc: Verify the Play function executes with different states.
 * @tc.type: FUNC
 */
HWTEST_F(BootPicturePlayerTest, Play_DifferentStates_ExecuteSuccessfully, TestSize.Level1)
{
    PlayerParams params;
    std::shared_ptr<BootPicturePlayer> player = std::make_shared<BootPicturePlayer>(params);
    ASSERT_NE(player, nullptr);
    std::shared_ptr<OHOS::AppExecFwk::EventRunner> runner = AppExecFwk::EventRunner::Create(false);
    std::shared_ptr<OHOS::AppExecFwk::EventHandler> handler = std::make_shared<AppExecFwk::EventHandler>(runner);
    int flag = 0;
    handler->PostTask([&] {
        player->Play();
        flag = 1;
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, 1);
    
    player->receiver_ = nullptr;
    auto& rsClient = OHOS::Rosen::RSInterfaces::GetInstance();
    player->receiver_ = rsClient.CreateVSyncReceiver("BootAnimation", handler);
    handler->PostTask([&] {
        player->Play();
        flag = 0;
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, 0);

    ImageStructVec imgVec;
    player->imageVector_ = imgVec;
    handler->PostTask([&] {
        player->Play();
        flag = 1;
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, 1);
}

/**
 * @tc.name: OnDraw_DifferentConditions_ExecuteSuccessfully
 * @tc.desc: Verify the OnDraw function executes with different conditions.
 * @tc.type: FUNC
 */
HWTEST_F(BootPicturePlayerTest, OnDraw_DifferentConditions_ExecuteSuccessfully, TestSize.Level1)
{
    std::shared_ptr<OHOS::AppExecFwk::EventRunner> runner = AppExecFwk::EventRunner::Create(false);
    std::shared_ptr<OHOS::AppExecFwk::EventHandler> handler = std::make_shared<AppExecFwk::EventHandler>(runner);
    PlayerParams params;
    bool flag = false;
    std::shared_ptr<BootPicturePlayer> player = std::make_shared<BootPicturePlayer>(params);
    ASSERT_NE(player, nullptr);
    handler->PostTask([&] {
        flag = player->OnDraw(nullptr, 0);
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, false);

    Rosen::Drawing::CoreCanvas canvas;
    handler->PostTask([&] {
        flag = player->OnDraw(&canvas, 0);
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, false);

    player->imgVecSize_ = 1;
    handler->PostTask([&] {
        flag = player->OnDraw(&canvas, -1);
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, false);

    player->imgVecSize_ = -1;
    handler->PostTask([&] {
        flag = player->OnDraw(&canvas, -1);
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, false);

    ImageStructVec imgVec;
    player->imageVector_ = imgVec;
    player->picCurNo_ = 0;
    player->imgVecSize_ = 2;
    player->OnVsync();
    flag = player->Stop();
    EXPECT_EQ(flag, false);
}

/**
 * @tc.name: BootPicturePlayerTest_002
 * @tc.desc: Verify the CheckFrameRateValid
 * @tc.type:FUNC
 */
HWTEST_F(BootPicturePlayerTest, BootPicturePlayerTest_002, TestSize.Level1)
{
    PlayerParams params;
    std::shared_ptr<BootPicturePlayer> player = std::make_shared<BootPicturePlayer>(params);
    int32_t frameRate = 0;
    EXPECT_EQ(player->CheckFrameRateValid(frameRate), false);
}

/**
 * @tc.name: BootPicturePlayerTest_003
 * @tc.desc: Verify the CheckFrameRateValid
 * @tc.type:FUNC
 */
HWTEST_F(BootPicturePlayerTest, BootPicturePlayerTest_003, TestSize.Level1)
{
    PlayerParams params;
    std::shared_ptr<BootPicturePlayer> player = std::make_shared<BootPicturePlayer>(params);
    int32_t frameRate = 30;
    EXPECT_EQ(player->CheckFrameRateValid(frameRate), true);
}

/**
 * @tc.name: BootPicturePlayerTest_004
 * @tc.desc: Verify the Play
 * @tc.type:FUNC
 */
HWTEST_F(BootPicturePlayerTest, BootPicturePlayerTest_004, TestSize.Level1)
{
    PlayerParams params;
    std::shared_ptr<BootPicturePlayer> player = std::make_shared<BootPicturePlayer>(params);
    std::shared_ptr<OHOS::AppExecFwk::EventRunner> runner = AppExecFwk::EventRunner::Create(false);
    std::shared_ptr<OHOS::AppExecFwk::EventHandler> handler = std::make_shared<AppExecFwk::EventHandler>(runner);
    int flag = 0;
    handler->PostTask([&] {
        player->Play();
        flag = 1;
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, 1);
    
    player->receiver_ = nullptr;
    auto& rsClient = OHOS::Rosen::RSInterfaces::GetInstance();
    player->receiver_ = rsClient.CreateVSyncReceiver("BootAnimation", handler);
    handler->PostTask([&] {
        player->Play();
        flag = 0;
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, 0);

    ImageStructVec imgVec;
    player->imageVector_ = imgVec;
    handler->PostTask([&] {
        player->Play();
        flag = 1;
        runner->Stop();
    });
    runner->Run();
    EXPECT_EQ(flag, 1);
}
