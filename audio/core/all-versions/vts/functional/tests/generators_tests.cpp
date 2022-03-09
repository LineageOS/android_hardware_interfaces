/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <memory>
#include <string>
#include <vector>

#include <android-base/macros.h>
#include <gtest/gtest.h>
#define LOG_TAG "Generators_Test"
#include <log/log.h>

#if MAJOR_VERSION == 6
#include <system/audio.h>
#include "6.0/Generators.h"
#include "PolicyConfig.h"
#elif MAJOR_VERSION == 7
#include "7.0/Generators.h"
#include "7.0/PolicyConfig.h"
#endif

using namespace android;
using namespace ::android::hardware::audio::common::CPP_VERSION;
#if MAJOR_VERSION == 7
namespace xsd {
using namespace ::android::audio::policy::configuration::CPP_VERSION;
}
#endif

// Stringify the argument.
#define QUOTE(x) #x
#define STRINGIFY(x) QUOTE(x)

struct PolicyConfigManager {
    static PolicyConfigManager& getInstance() {
        static PolicyConfigManager instance;
        return instance;
    }
    bool init(const std::string& filePath, const std::string& fileName) {
        mConfig = std::make_unique<PolicyConfig>(filePath, fileName);
        mDeviceParameters.clear();
        if (mConfig->getStatus() == OK) {
            const auto devices = mConfig->getModulesWithDevicesNames();
            mDeviceParameters.reserve(devices.size());
            for (const auto& deviceName : devices) {
                mDeviceParameters.emplace_back(
                        "android.hardware.audio.IDevicesFactory@" STRINGIFY(FILE_VERSION),
                        deviceName);
            }
            return true;
        } else {
            ALOGE("%s", mConfig->getError().c_str());
            return false;
        }
    }
    const PolicyConfig& getConfig() { return *mConfig; }
    const std::vector<DeviceParameter>& getDeviceParameters() { return mDeviceParameters; }

  private:
    std::unique_ptr<PolicyConfig> mConfig;
    std::vector<DeviceParameter> mDeviceParameters;
};

// Test implementations
const PolicyConfig& getCachedPolicyConfig() {
    return PolicyConfigManager::getInstance().getConfig();
}

const std::vector<DeviceParameter>& getDeviceParameters() {
    return PolicyConfigManager::getInstance().getDeviceParameters();
}

static const std::string kDataDir = "/data/local/tmp";

class GeneratorsTest : public ::testing::TestWithParam<std::string> {
  public:
    static void validateConfig(const AudioConfig& config) {
#if MAJOR_VERSION == 6
        ASSERT_TRUE(audio_is_valid_format(static_cast<audio_format_t>(config.format)))
                << "Audio format is invalid " << ::testing::PrintToString(config.format);
        ASSERT_TRUE(
                audio_channel_mask_is_valid(static_cast<audio_channel_mask_t>(config.channelMask)))
                << "Audio channel mask is invalid " << ::testing::PrintToString(config.channelMask);
#elif MAJOR_VERSION == 7
        ASSERT_FALSE(xsd::isUnknownAudioFormat(config.base.format))
                << "Audio format is invalid " << ::testing::PrintToString(config.base.format);
        ASSERT_FALSE(xsd::isUnknownAudioChannelMask(config.base.channelMask))
                << "Audio channel mask is invalid "
                << ::testing::PrintToString(config.base.channelMask);
#endif
    }
    static void validateDeviceConfigs(const std::vector<DeviceConfigParameter>& params) {
        for (const auto& param : params) {
            ASSERT_NO_FATAL_FAILURE(validateConfig(std::get<PARAM_CONFIG>(param)));
        }
    }
};

TEST_P(GeneratorsTest, ValidateConfigs) {
    ASSERT_TRUE(PolicyConfigManager::getInstance().init(kDataDir, GetParam()));
    EXPECT_NE(nullptr, getCachedPolicyConfig().getPrimaryModule());
    EXPECT_FALSE(getCachedPolicyConfig().getModulesWithDevicesNames().empty());
    const auto allOutConfigs = generateOutputDeviceConfigParameters(false /*oneProfilePerDevice*/);
    EXPECT_FALSE(allOutConfigs.empty());
    EXPECT_NO_FATAL_FAILURE(validateDeviceConfigs(allOutConfigs));
    const auto singleOutConfig = generateOutputDeviceConfigParameters(true /*oneProfilePerDevice*/);
    EXPECT_FALSE(singleOutConfig.empty());
    EXPECT_NO_FATAL_FAILURE(validateDeviceConfigs(singleOutConfig));
    const auto allInConfigs = generateInputDeviceConfigParameters(false /*oneProfilePerDevice*/);
    EXPECT_FALSE(allInConfigs.empty());
    EXPECT_NO_FATAL_FAILURE(validateDeviceConfigs(allInConfigs));
    const auto singleInConfig = generateInputDeviceConfigParameters(true /*oneProfilePerDevice*/);
    EXPECT_FALSE(singleInConfig.empty());
    EXPECT_NO_FATAL_FAILURE(validateDeviceConfigs(singleInConfig));
}

// Target file names are the same for all versions, see 'HalAudioVx_0GeneratorTest.xml' test configs
// clang-format off
INSTANTIATE_TEST_SUITE_P(Generators, GeneratorsTest,
                         ::testing::Values("apm_config_no_vx.xml", "apm_config_with_vx.xml"
#if MAJOR_VERSION == 6
                                         , "apm_config_b_205808571_6_0.xml"
#elif MAJOR_VERSION == 7
                                         , "apm_config_b_204314749_7_0.xml"
                                         , "apm_config_b_205808571_7_0.xml"
#endif
                                           ));
// clang-format on

TEST(GeneratorsDeviceTest, AttachedDevicesOnly) {
    static const std::string kTestFile =
            "apm_config_b_205808571_" STRINGIFY(MAJOR_VERSION) "_0.xml";
    ASSERT_TRUE(PolicyConfigManager::getInstance().init(kDataDir, kTestFile));
    EXPECT_NE(nullptr, getCachedPolicyConfig().getPrimaryModule());
    const auto allInConfigs = generateInputDeviceConfigParameters(false /*oneProfilePerDevice*/);
    EXPECT_FALSE(allInConfigs.empty());
    for (const auto& configParam : allInConfigs) {
        const AudioConfig& config = std::get<PARAM_CONFIG>(configParam);
        // The config contains multichannel masks for mixPort connected to
        // input devicePorts that are not attached. These multichannel masks must
        // not appear among generated masks.
        const uint32_t channelCount =
#if MAJOR_VERSION == 6
                audio_channel_count_from_in_mask(
                        static_cast<audio_channel_mask_t>(config.channelMask));
#elif MAJOR_VERSION == 7
                xsd::getChannelCount(config.base.channelMask);
#endif
        EXPECT_TRUE(channelCount <= 4) << "Unexpected channel count: " << channelCount << " " <<
#if MAJOR_VERSION == 6
                ::testing::PrintToString(config.format) << ", "
                                       << ::testing::PrintToString(config.sampleRateHz) << ", "
                                       << ::testing::PrintToString(config.channelMask);
#elif MAJOR_VERSION == 7
                ::testing::PrintToString(config.base.format) << ", "
                                       << ::testing::PrintToString(config.base.sampleRateHz) << ", "
                                       << ::testing::PrintToString(config.base.channelMask);
#endif
    }
}
