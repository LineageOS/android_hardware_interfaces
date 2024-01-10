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

#include <broadcastradio-utils-aidl/UtilsV2.h>
#include <gtest/gtest.h>

namespace aidl::android::hardware::broadcastradio {

namespace {
struct IsValidMetadataV2TestCase {
    std::string name;
    Metadata metadata;
    bool valid;
};

std::vector<IsValidMetadataV2TestCase> getIsValidMetadataV2TestCases() {
    return std::vector<IsValidMetadataV2TestCase>({
            IsValidMetadataV2TestCase{.name = "valid_rds_pty",
                                      .metadata = Metadata::make<Metadata::rdsPty>(1),
                                      .valid = true},
            IsValidMetadataV2TestCase{.name = "negative_rds_pty",
                                      .metadata = Metadata::make<Metadata::rdsPty>(-1),
                                      .valid = false},
            IsValidMetadataV2TestCase{
                    .name = "valid_hd_station_name_short",
                    .metadata = Metadata::make<Metadata::hdStationNameShort>("name_short"),
                    .valid = true},
            IsValidMetadataV2TestCase{
                    .name = "too_long_hd_station_name_short",
                    .metadata = Metadata::make<Metadata::hdStationNameShort>("name_too_long"),
                    .valid = false},
            IsValidMetadataV2TestCase{
                    .name = "valid_hd_subchannel_available",
                    .metadata = Metadata::make<Metadata::hdSubChannelsAvailable>(1),
                    .valid = true},
            IsValidMetadataV2TestCase{
                    .name = "negative_subchannel_available",
                    .metadata = Metadata::make<Metadata::hdSubChannelsAvailable>(-1),
                    .valid = false},
            IsValidMetadataV2TestCase{
                    .name = "large_subchannel_available",
                    .metadata = Metadata::make<Metadata::hdSubChannelsAvailable>(256),
                    .valid = false},
    });
}
}  // namespace

class IsValidMetadataV2Test : public testing::TestWithParam<IsValidMetadataV2TestCase> {};

INSTANTIATE_TEST_SUITE_P(IsValidMetadataV2Tests, IsValidMetadataV2Test,
                         testing::ValuesIn(getIsValidMetadataV2TestCases()),
                         [](const testing::TestParamInfo<IsValidMetadataV2Test::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(IsValidMetadataV2Test, IsValidMetadataV2) {
    IsValidMetadataV2TestCase testParam = GetParam();

    ASSERT_EQ(utils::isValidMetadataV2(testParam.metadata), testParam.valid);
}

}  // namespace aidl::android::hardware::broadcastradio
