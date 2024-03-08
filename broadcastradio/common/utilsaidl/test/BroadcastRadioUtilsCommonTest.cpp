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

#include <broadcastradio-utils-aidl/Utils.h>
#include <broadcastradio-utils-aidl/UtilsV2.h>
#include <gtest/gtest.h>

namespace aidl::android::hardware::broadcastradio {

namespace {
constexpr int64_t kFmFrequencyKHz = 97900;
constexpr uint32_t kDabSid = 0x0000C221u;
constexpr int kDabEccCode = 0xE1u;
constexpr int kDabSCIdS = 0x1u;
constexpr uint64_t kDabSidExt = static_cast<uint64_t>(kDabSid) |
                                (static_cast<uint64_t>(kDabEccCode) << 32) |
                                (static_cast<uint64_t>(kDabSCIdS) << 40);
constexpr uint32_t kDabEnsemble = 0xCE15u;
constexpr uint64_t kDabFrequencyKhz = 225648u;
constexpr uint64_t kHdStationId = 0xA0000001u;
constexpr uint64_t kHdSubChannel = 1u;
constexpr uint64_t kHdStationLocation = 0x44E647003665CF6u;
constexpr uint64_t kHdStationLocationInvalid = 0x4E647007665CF6u;
constexpr uint64_t kHdFrequency = 97700u;
constexpr int64_t kRdsValue = 0xBEEF;

struct IsValidIdentifierTestCase {
    std::string name;
    ProgramIdentifier id;
    bool valid;
};

std::vector<IsValidIdentifierTestCase> getIsValidIdentifierTestCases() {
    return std::vector<IsValidIdentifierTestCase>({
            IsValidIdentifierTestCase{.name = "invalid_id_type",
                                      .id = utils::makeIdentifier(IdentifierType::INVALID, 0),
                                      .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_frequency_high",
                    .id = utils::makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ, 10000000u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_frequency_low",
                    .id = utils::makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ, 100000u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_dab_frequency",
                    .id = utils::makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ, 1000000u),
                    .valid = true},
            IsValidIdentifierTestCase{
                    .name = "invalid_am_fm_frequency_high",
                    .id = utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, 10000000u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_am_fm_frequency_low",
                    .id = utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, 100u),
                    .valid = false},
            IsValidIdentifierTestCase{.name = "valid_am_fm_frequency",
                                      .id = utils::makeIdentifier(
                                              IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz),
                                      .valid = true},
            IsValidIdentifierTestCase{
                    .name = "drmo_frequency_high",
                    .id = utils::makeIdentifier(IdentifierType::DRMO_FREQUENCY_KHZ, 10000000u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "drmo_frequency_low",
                    .id = utils::makeIdentifier(IdentifierType::DRMO_FREQUENCY_KHZ, 100u),
                    .valid = false},
            IsValidIdentifierTestCase{.name = "valid_drmo_frequency",
                                      .id = utils::makeIdentifier(
                                              IdentifierType::DRMO_FREQUENCY_KHZ, kFmFrequencyKHz),
                                      .valid = true},
            IsValidIdentifierTestCase{.name = "invalid_rds_low",
                                      .id = utils::makeIdentifier(IdentifierType::RDS_PI, 0x0),
                                      .valid = false},
            IsValidIdentifierTestCase{.name = "invalid_rds_high",
                                      .id = utils::makeIdentifier(IdentifierType::RDS_PI, 0x10000),
                                      .valid = false},
            IsValidIdentifierTestCase{.name = "valid_rds",
                                      .id = utils::makeIdentifier(IdentifierType::RDS_PI, 0x1000),
                                      .valid = true},
            IsValidIdentifierTestCase{
                    .name = "invalid_hd_id_zero",
                    .id = utils::makeSelectorHd(/* stationId= */ 0u, kHdSubChannel, kHdFrequency)
                                  .primaryId,
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_hd_suchannel",
                    .id = utils::makeSelectorHd(kHdStationId, /* subChannel= */ 8u, kHdFrequency)
                                  .primaryId,
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_hd_frequency_low",
                    .id = utils::makeSelectorHd(kHdStationId, kHdSubChannel, /* frequency= */ 100u)
                                  .primaryId,
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_hd_id",
                    .id = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency)
                                  .primaryId,
                    .valid = true},
            IsValidIdentifierTestCase{
                    .name = "invalid_hd_station_name",
                    .id = utils::makeIdentifier(IdentifierType::HD_STATION_NAME, 0x41422D464D),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_hd_station_name",
                    .id = utils::makeIdentifier(IdentifierType::HD_STATION_NAME, 0x414231464D),
                    .valid = true},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_sid",
                    .id = utils::makeIdentifier(IdentifierType::DAB_SID_EXT, 0x0E100000000u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_ecc_low",
                    .id = utils::makeIdentifier(IdentifierType::DAB_SID_EXT, 0x0F700000221u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_ecc_high",
                    .id = utils::makeIdentifier(IdentifierType::DAB_SID_EXT, 0x09900000221u),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_dab_sid_ext",
                    .id = utils::makeIdentifier(IdentifierType::DAB_SID_EXT, kDabSidExt),
                    .valid = true},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_ensemble_zero",
                    .id = utils::makeIdentifier(IdentifierType::DAB_ENSEMBLE, 0x0),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_dab_ensemble_high",
                    .id = utils::makeIdentifier(IdentifierType::DAB_ENSEMBLE, 0x10000),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_dab_ensemble",
                    .id = utils::makeIdentifier(IdentifierType::DAB_ENSEMBLE, kDabEnsemble),
                    .valid = true},
            IsValidIdentifierTestCase{.name = "invalid_dab_scid_low",
                                      .id = utils::makeIdentifier(IdentifierType::DAB_SCID, 0xF),
                                      .valid = false},
            IsValidIdentifierTestCase{.name = "invalid_dab_scid_high",
                                      .id = utils::makeIdentifier(IdentifierType::DAB_SCID, 0x1000),
                                      .valid = false},
            IsValidIdentifierTestCase{.name = "valid_dab_scid",
                                      .id = utils::makeIdentifier(IdentifierType::DAB_SCID, 0x100),
                                      .valid = true},
            IsValidIdentifierTestCase{
                    .name = "invalid_drmo_id_zero",
                    .id = utils::makeIdentifier(IdentifierType::DRMO_SERVICE_ID, 0x0),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "invalid_drmo_id_high",
                    .id = utils::makeIdentifier(IdentifierType::DRMO_SERVICE_ID, 0x1000000),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_drmo_id",
                    .id = utils::makeIdentifier(IdentifierType::DRMO_SERVICE_ID, 0x100000),
                    .valid = true},
    });
}

std::vector<IsValidIdentifierTestCase> getIsValidIdentifierV2TestCases() {
    std::vector<IsValidIdentifierTestCase> testcases = getIsValidIdentifierTestCases();
    std::vector<IsValidIdentifierTestCase> testcasesNew = std::vector<IsValidIdentifierTestCase>({
            IsValidIdentifierTestCase{
                    .name = "invalid_hd_station_location_id",
                    .id = utils::makeIdentifier(IdentifierType::HD_STATION_LOCATION,
                                                kHdStationLocationInvalid),
                    .valid = false},
            IsValidIdentifierTestCase{
                    .name = "valid_hd_station_location_id",
                    .id = utils::makeIdentifier(IdentifierType::HD_STATION_LOCATION,
                                                kHdStationLocation),
                    .valid = true},
    });
    testcases.insert(testcases.end(), testcasesNew.begin(), testcasesNew.end());
    return testcases;
}

struct IsValidSelectorTestCase {
    std::string name;
    ProgramSelector sel;
    bool valid;
};

std::vector<IsValidSelectorTestCase> getIsValidSelectorTestCases() {
    return std::vector<IsValidSelectorTestCase>({
            IsValidSelectorTestCase{.name = "valid_am_fm_selector",
                                    .sel = utils::makeSelectorAmfm(kFmFrequencyKHz),
                                    .valid = true},
            IsValidSelectorTestCase{
                    .name = "valid_hd_selector",
                    .sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency),
                    .valid = true},
            IsValidSelectorTestCase{
                    .name = "valid_dab_selector",
                    .sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz),
                    .valid = true},
            IsValidSelectorTestCase{.name = "valid_rds_selector",
                                    .sel = ProgramSelector{.primaryId = utils::makeIdentifier(
                                                                   IdentifierType::RDS_PI, 0x1000)},
                                    .valid = true},
            IsValidSelectorTestCase{.name = "selector_with_invalid_id",
                                    .sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel,
                                                                 /* frequency= */ 100u),
                                    .valid = false},
            IsValidSelectorTestCase{
                    .name = "selector_with_invalid_primary_id_type",
                    .sel = ProgramSelector{.primaryId = utils::makeIdentifier(
                                                   IdentifierType::DAB_ENSEMBLE, kDabEnsemble)},
                    .valid = false},
            IsValidSelectorTestCase{
                    .name = "selector_with_invalid_secondary_id",
                    .sel = ProgramSelector{.primaryId = utils::makeIdentifier(
                                                   IdentifierType::DAB_SID_EXT, kDabSidExt),
                                           .secondaryIds = {utils::makeIdentifier(
                                                   IdentifierType::DAB_ENSEMBLE, 0x0)}},
                    .valid = false},
    });
}

std::vector<IsValidSelectorTestCase> getIsValidSelectorV2TestCases() {
    ProgramSelector validHdSel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);
    validHdSel.secondaryIds = {
            utils::makeIdentifier(IdentifierType::HD_STATION_LOCATION, kHdStationLocation)};
    ProgramSelector invalidHdSel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);
    invalidHdSel.secondaryIds = {
            utils::makeIdentifier(IdentifierType::HD_STATION_LOCATION, kHdStationLocationInvalid)};
    std::vector<IsValidSelectorTestCase> testcasesNew = std::vector<IsValidSelectorTestCase>(
            {IsValidSelectorTestCase{.name = "hd_selector_with_valid_station_location",
                                     .sel = validHdSel,
                                     .valid = true},
             IsValidSelectorTestCase{.name = "hd_selector_with_invalid_station_location",
                                     .sel = invalidHdSel,
                                     .valid = false}});
    std::vector<IsValidSelectorTestCase> testcases = getIsValidSelectorTestCases();
    testcases.insert(testcases.end(), testcasesNew.begin(), testcasesNew.end());
    return testcases;
}

struct IsValidMetadataTestCase {
    std::string name;
    Metadata metadata;
    bool valid;
};

std::vector<IsValidMetadataTestCase> getIsValidMetadataTestCases() {
    return std::vector<IsValidMetadataTestCase>({
            IsValidMetadataTestCase{.name = "valid_rds_pty",
                                    .metadata = Metadata::make<Metadata::rdsPty>(1),
                                    .valid = true},
            IsValidMetadataTestCase{.name = "negative_rds_pty",
                                    .metadata = Metadata::make<Metadata::rdsPty>(-1),
                                    .valid = false},
            IsValidMetadataTestCase{.name = "large_rds_pty",
                                    .metadata = Metadata::make<Metadata::rdsPty>(256),
                                    .valid = false},
            IsValidMetadataTestCase{.name = "valid_rbds_pty",
                                    .metadata = Metadata::make<Metadata::rbdsPty>(1),
                                    .valid = true},
            IsValidMetadataTestCase{.name = "negative_rbds_pty",
                                    .metadata = Metadata::make<Metadata::rbdsPty>(-1),
                                    .valid = false},
            IsValidMetadataTestCase{.name = "large_rbds_pty",
                                    .metadata = Metadata::make<Metadata::rbdsPty>(256),
                                    .valid = false},
            IsValidMetadataTestCase{
                    .name = "valid_dab_ensemble_name_short",
                    .metadata = Metadata::make<Metadata::dabEnsembleNameShort>("name"),
                    .valid = true},
            IsValidMetadataTestCase{
                    .name = "too_long_dab_ensemble_name_short",
                    .metadata = Metadata::make<Metadata::dabEnsembleNameShort>("name_long"),
                    .valid = false},
            IsValidMetadataTestCase{
                    .name = "valid_dab_service_name_short",
                    .metadata = Metadata::make<Metadata::dabServiceNameShort>("name"),
                    .valid = true},
            IsValidMetadataTestCase{
                    .name = "too_long_dab_service_name_short",
                    .metadata = Metadata::make<Metadata::dabServiceNameShort>("name_long"),
                    .valid = false},
            IsValidMetadataTestCase{
                    .name = "valid_dab_component_name_short",
                    .metadata = Metadata::make<Metadata::dabComponentNameShort>("name"),
                    .valid = true},
            IsValidMetadataTestCase{
                    .name = "too_long_dab_component_name_short",
                    .metadata = Metadata::make<Metadata::dabComponentNameShort>("name_long"),
                    .valid = false},
    });
}

std::vector<IsValidMetadataTestCase> getIsValidMetadataV2TestCases() {
    std::vector<IsValidMetadataTestCase> testcases = getIsValidMetadataTestCases();
    std::vector<IsValidMetadataTestCase> testcasesNew = std::vector<IsValidMetadataTestCase>({
            IsValidMetadataTestCase{
                    .name = "valid_hd_station_name_short",
                    .metadata = Metadata::make<Metadata::hdStationNameShort>("name_short"),
                    .valid = true},
            IsValidMetadataTestCase{
                    .name = "too_long_hd_station_name_short",
                    .metadata = Metadata::make<Metadata::hdStationNameShort>("name_too_long"),
                    .valid = false},
            IsValidMetadataTestCase{.name = "valid_hd_subchannel_available",
                                    .metadata = Metadata::make<Metadata::hdSubChannelsAvailable>(1),
                                    .valid = true},
            IsValidMetadataTestCase{
                    .name = "negative_subchannel_available",
                    .metadata = Metadata::make<Metadata::hdSubChannelsAvailable>(-1),
                    .valid = false},
            IsValidMetadataTestCase{
                    .name = "large_subchannel_available",
                    .metadata = Metadata::make<Metadata::hdSubChannelsAvailable>(256),
                    .valid = false},
    });
    testcases.insert(testcases.end(), testcasesNew.begin(), testcasesNew.end());
    return testcases;
}
}  // namespace

class IsValidIdentifierTest : public testing::TestWithParam<IsValidIdentifierTestCase> {};

INSTANTIATE_TEST_SUITE_P(IsValidIdentifierTests, IsValidIdentifierTest,
                         testing::ValuesIn(getIsValidIdentifierTestCases()),
                         [](const testing::TestParamInfo<IsValidIdentifierTest::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(IsValidIdentifierTest, IsValid) {
    IsValidIdentifierTestCase testcase = GetParam();

    ASSERT_EQ(utils::isValid(testcase.id), testcase.valid);
}

class IsValidIdentifierV2Test : public testing::TestWithParam<IsValidIdentifierTestCase> {};

INSTANTIATE_TEST_SUITE_P(
        IsValidIdentifierV2Tests, IsValidIdentifierV2Test,
        testing::ValuesIn(getIsValidIdentifierV2TestCases()),
        [](const testing::TestParamInfo<IsValidIdentifierV2Test::ParamType>& info) {
            return info.param.name;
        });

TEST_P(IsValidIdentifierV2Test, IsValidV2) {
    IsValidIdentifierTestCase testcase = GetParam();

    ASSERT_EQ(utils::isValidV2(testcase.id), testcase.valid);
}

class IsValidSelectorTest : public testing::TestWithParam<IsValidSelectorTestCase> {};

INSTANTIATE_TEST_SUITE_P(IsValidSelectorTests, IsValidSelectorTest,
                         testing::ValuesIn(getIsValidSelectorTestCases()),
                         [](const testing::TestParamInfo<IsValidSelectorTest::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(IsValidSelectorTest, IsValid) {
    IsValidSelectorTestCase testcase = GetParam();

    ASSERT_EQ(utils::isValid(testcase.sel), testcase.valid);
}

class IsValidSelectorV2Test : public testing::TestWithParam<IsValidSelectorTestCase> {};

INSTANTIATE_TEST_SUITE_P(IsValidSelectorV2Tests, IsValidSelectorV2Test,
                         testing::ValuesIn(getIsValidSelectorV2TestCases()),
                         [](const testing::TestParamInfo<IsValidSelectorV2Test::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(IsValidSelectorV2Test, IsValidV2) {
    IsValidSelectorTestCase testcase = GetParam();

    ASSERT_EQ(utils::isValidV2(testcase.sel), testcase.valid);
}

class IsValidMetadataTest : public testing::TestWithParam<IsValidMetadataTestCase> {};

INSTANTIATE_TEST_SUITE_P(IsValidMetadataTests, IsValidMetadataTest,
                         testing::ValuesIn(getIsValidMetadataTestCases()),
                         [](const testing::TestParamInfo<IsValidMetadataTest::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(IsValidMetadataTest, IsValidMetadata) {
    IsValidMetadataTestCase testParam = GetParam();

    ASSERT_EQ(utils::isValidMetadata(testParam.metadata), testParam.valid);
}

class IsValidMetadataV2Test : public testing::TestWithParam<IsValidMetadataTestCase> {};

INSTANTIATE_TEST_SUITE_P(IsValidMetadataV2Tests, IsValidMetadataV2Test,
                         testing::ValuesIn(getIsValidMetadataV2TestCases()),
                         [](const testing::TestParamInfo<IsValidMetadataV2Test::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(IsValidMetadataV2Test, IsValidMetadataV2) {
    IsValidMetadataTestCase testParam = GetParam();

    ASSERT_EQ(utils::isValidMetadataV2(testParam.metadata), testParam.valid);
}

}  // namespace aidl::android::hardware::broadcastradio
