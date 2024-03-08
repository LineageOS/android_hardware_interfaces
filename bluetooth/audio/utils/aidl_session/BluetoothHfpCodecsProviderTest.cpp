/*
 * Copyright (C) 2024 The Android Open Source Project
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

#include <gtest/gtest.h>

#include <optional>
#include <tuple>

#include "BluetoothHfpCodecsProvider.h"
#include "gtest/gtest.h"

using aidl::android::hardware::bluetooth::audio::BluetoothHfpCodecsProvider;
using aidl::android::hardware::bluetooth::audio::CodecInfo;
using aidl::android::hardware::bluetooth::audio::hfp::setting::CodecType;
using aidl::android::hardware::bluetooth::audio::hfp::setting::Configuration;
using aidl::android::hardware::bluetooth::audio::hfp::setting::
    HfpOffloadSetting;
using aidl::android::hardware::bluetooth::audio::hfp::setting::
    PathConfiguration;
using aidl::android::hardware::bluetooth::audio::hfp::setting::
    TransportConfiguration;

typedef std::tuple<std::vector<PathConfiguration>,
                   std::vector<TransportConfiguration>,
                   std::vector<Configuration>>
    HfpOffloadSettingTuple;

// Define valid components for each list
// PathConfiguration
static const PathConfiguration kValidPathConfigurationCVSD("CVSD_IO", 16000,
                                                           CodecType::CVSD, 16,
                                                           2, 0, 1, 0);
static const PathConfiguration kInvalidPathConfigurationNULL(std::nullopt,
                                                             16000,
                                                             CodecType::CVSD,
                                                             16, 2, 0, 1, 0);
static const PathConfiguration kInvalidPathConfigurationNoPath(
    "CVSD_NULL", 16000, CodecType::CVSD, 16, 2, 0, std::nullopt, 0);

// Configuration
static const Configuration kValidConfigurationCVSD("CVSD", CodecType::CVSD,
                                                   65535, 7, 0, true, "CVSD_IO",
                                                   "CVSD_IO", std::nullopt,
                                                   std::nullopt);
static const Configuration kInvalidConfigurationCVSDNoPath(
    "CVSD", CodecType::CVSD, 65535, 7, 0, true, "CVSD_NULL", "CVSD_NULL",
    std::nullopt, std::nullopt);
static const Configuration kInvalidConfigurationCVSDNotFound(
    "CVSD", CodecType::CVSD, 65535, 7, 0, true, "CVSD_N", "CVSD_N",
    std::nullopt, std::nullopt);

class BluetoothHfpCodecsProviderTest : public ::testing::Test {
 public:
  static std::vector<HfpOffloadSettingTuple> CreateTestCases(
      const std::vector<std::vector<PathConfiguration>> path_configs_list,
      const std::vector<std::vector<TransportConfiguration>>
          transport_configs_list,
      const std::vector<std::vector<Configuration>> configs_list) {
    std::vector<HfpOffloadSettingTuple> test_cases;
    for (const auto& path_configs : path_configs_list) {
      for (const auto& transport_configs : transport_configs_list) {
        for (const auto& configs : configs_list)
          test_cases.push_back(
              CreateTestCase(path_configs, transport_configs, configs));
      }
    }
    return test_cases;
  }

 protected:
  std::vector<CodecInfo> RunTestCase(HfpOffloadSettingTuple test_case) {
    auto& [path_configuration_list, transport_configuration_list,
           configuration_list] = test_case;
    HfpOffloadSetting hfp_offload_setting(path_configuration_list,
                                          transport_configuration_list,
                                          configuration_list);
    auto capabilities =
        BluetoothHfpCodecsProvider::GetHfpAudioCodecInfo(hfp_offload_setting);
    return capabilities;
  }

 private:
  static inline HfpOffloadSettingTuple CreateTestCase(
      const std::vector<PathConfiguration> path_config_list,
      const std::vector<TransportConfiguration> transport_config_list,
      const std::vector<Configuration> config_list) {
    return std::make_tuple(path_config_list, transport_config_list,
                           config_list);
  }
};

class GetHfpCodecInfoTest : public BluetoothHfpCodecsProviderTest {
 public:
  static std::vector<std::vector<PathConfiguration>>
  GetInvalidPathConfigurationLists() {
    std::vector<std::vector<PathConfiguration>> result;
    result.push_back({kInvalidPathConfigurationNULL});
    result.push_back({kInvalidPathConfigurationNoPath});
    result.push_back({});
    return result;
  }

  static std::vector<std::vector<Configuration>>
  GetInvalidConfigurationLists() {
    std::vector<std::vector<Configuration>> result;
    result.push_back({kInvalidConfigurationCVSDNotFound});
    result.push_back({kInvalidConfigurationCVSDNoPath});
    result.push_back({});
    return result;
  }
};

TEST_F(GetHfpCodecInfoTest, InvalidPathConfiguration) {
  auto test_cases = BluetoothHfpCodecsProviderTest::CreateTestCases(
      GetHfpCodecInfoTest::GetInvalidPathConfigurationLists(), {{}},
      {{kValidConfigurationCVSD}});
  for (auto& test_case : test_cases) {
    auto hfp_codec_capabilities = RunTestCase(test_case);
    ASSERT_TRUE(hfp_codec_capabilities.empty());
  }
}

TEST_F(GetHfpCodecInfoTest, InvalidConfigurationName) {
  auto test_cases = BluetoothHfpCodecsProviderTest::CreateTestCases(
      GetHfpCodecInfoTest::GetInvalidPathConfigurationLists(), {{}},
      {GetHfpCodecInfoTest::GetInvalidConfigurationLists()});
  for (auto& test_case : test_cases) {
    auto hfp_codec_capabilities = RunTestCase(test_case);
    ASSERT_TRUE(hfp_codec_capabilities.empty());
  }
}

TEST_F(GetHfpCodecInfoTest, ValidConfiguration) {
  auto test_cases = BluetoothHfpCodecsProviderTest::CreateTestCases(
      {{kValidPathConfigurationCVSD}}, {{}}, {{kValidConfigurationCVSD}});
  for (auto& test_case : test_cases) {
    auto hfp_codec_capabilities = RunTestCase(test_case);
    ASSERT_FALSE(hfp_codec_capabilities.empty());
  }
}
