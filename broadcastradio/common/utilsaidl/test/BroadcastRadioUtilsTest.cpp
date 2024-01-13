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
constexpr uint64_t kHdFrequency = 97700u;

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
}  // namespace

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

TEST(BroadcastRadioUtilsTest, HasIdWithPrimaryIdType) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::hasId(sel, IdentifierType::AMFM_FREQUENCY_KHZ));
}

TEST(BroadcastRadioUtilsTest, HasIdWithSecondaryIdType) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_TRUE(utils::hasId(sel, IdentifierType::DAB_FREQUENCY_KHZ));
}

TEST(BroadcastRadioUtilsTest, HasIdWithIdNotInSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_FALSE(utils::hasId(sel, IdentifierType::AMFM_FREQUENCY_KHZ));
}

TEST(BroadcastRadioUtilsTest, GetIdWithPrimaryIdType) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getId(sel, IdentifierType::DAB_SID_EXT), static_cast<int64_t>(kDabSidExt));
}

TEST(BroadcastRadioUtilsTest, GetIdWithSecondaryIdType) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getId(sel, IdentifierType::DAB_ENSEMBLE), static_cast<int64_t>(kDabEnsemble));
}

TEST(BroadcastRadioUtilsTest, GetIdWithIdNotFound) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getId(sel, IdentifierType::AMFM_FREQUENCY_KHZ), 0);
}

TEST(BroadcastRadioUtilsTest, GetIdWithIdFoundAndDefaultValue) {
    int64_t defaultValue = 0x0E10000C222u;
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getId(sel, IdentifierType::DAB_SID_EXT, defaultValue),
              static_cast<int64_t>(kDabSidExt));
}

TEST(BroadcastRadioUtilsTest, GetIdWithIdNotFoundAndDefaultValue) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getId(sel, IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz),
              static_cast<int64_t>(kFmFrequencyKHz));
}

TEST(BroadcastRadioUtilsTest, MakeIdentifier) {
    ProgramIdentifier id =
            utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz);

    EXPECT_EQ(id.type, IdentifierType::AMFM_FREQUENCY_KHZ);
    EXPECT_EQ(id.value, kFmFrequencyKHz);
}

TEST(BroadcastRadioUtilsTest, MakeSelectorAmfm) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    EXPECT_EQ(sel.primaryId.type, IdentifierType::AMFM_FREQUENCY_KHZ);
    EXPECT_EQ(sel.primaryId.value, kFmFrequencyKHz);
    EXPECT_TRUE(sel.secondaryIds.empty());
}

TEST(BroadcastRadioUtilsTest, MakeSelectorHd) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    EXPECT_EQ(sel.primaryId.type, IdentifierType::HD_STATION_ID_EXT);
    EXPECT_TRUE(sel.secondaryIds.empty());
    EXPECT_EQ(utils::getHdSubchannel(sel), static_cast<int>(kHdSubChannel));
    EXPECT_EQ(utils::getHdFrequency(sel), static_cast<uint32_t>(kHdFrequency));
}

TEST(BroadcastRadioUtilsTest, MakeHdRadioStationName) {
    std::string stationName = "aB1-FM";
    int64_t expectedIdValue = 0x4D46314241;

    ProgramIdentifier stationNameId = utils::makeHdRadioStationName(stationName);

    EXPECT_EQ(stationNameId.type, IdentifierType::HD_STATION_NAME);
    EXPECT_EQ(stationNameId.value, expectedIdValue);
}

TEST(BroadcastRadioUtilsTest, GetHdFrequencyWithoutHdId) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getHdFrequency(sel), 0u);
}

TEST(BroadcastRadioUtilsTest, HasAmFmFrequencyWithAmFmSelector) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::hasAmFmFrequency(sel));
}

TEST(BroadcastRadioUtilsTest, HasAmFmFrequencyWithHdSelector) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    ASSERT_TRUE(utils::hasAmFmFrequency(sel));
}

TEST(BroadcastRadioUtilsTest, HasAmFmFrequencyWithNonAmFmHdSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_FALSE(utils::hasAmFmFrequency(sel));
}

TEST(BroadcastRadioUtilsTest, GetAmFmFrequencyWithAmFmSelector) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_EQ(utils::getAmFmFrequency(sel), static_cast<uint32_t>(kFmFrequencyKHz));
}

TEST(BroadcastRadioUtilsTest, GetAmFmFrequencyWithHdSelector) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    ASSERT_EQ(utils::getAmFmFrequency(sel), static_cast<uint32_t>(kHdFrequency));
}

TEST(BroadcastRadioUtilsTest, GetAmFmFrequencyWithNonAmFmHdSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getAmFmFrequency(sel), 0u);
}

TEST(BroadcastRadioUtilsTest, MakeSelectorDabWithOnlySidExt) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt);

    EXPECT_EQ(sel.primaryId.type, IdentifierType::DAB_SID_EXT);
    EXPECT_EQ(sel.primaryId.value, static_cast<int64_t>(kDabSidExt));
    EXPECT_TRUE(sel.secondaryIds.empty());
}

TEST(BroadcastRadioUtilsTest, MakeSelectorDab) {
    ProgramIdentifier ensembleIdExpected =
            utils::makeIdentifier(IdentifierType::DAB_ENSEMBLE, kDabEnsemble);
    ProgramIdentifier frequencyIdExpected =
            utils::makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ, kDabFrequencyKhz);

    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    EXPECT_EQ(sel.primaryId.type, IdentifierType::DAB_SID_EXT);
    EXPECT_EQ(sel.primaryId.value, static_cast<int64_t>(kDabSidExt));
    EXPECT_EQ(sel.secondaryIds.size(), 2u);
    EXPECT_NE(std::find(sel.secondaryIds.begin(), sel.secondaryIds.end(), ensembleIdExpected),
              sel.secondaryIds.end());
    EXPECT_NE(std::find(sel.secondaryIds.begin(), sel.secondaryIds.end(), frequencyIdExpected),
              sel.secondaryIds.end());
}

TEST(BroadcastRadioUtilsTest, GetDabSId) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getDabSId(sel), kDabSid);
}

TEST(BroadcastRadioUtilsTest, GetDabEccCode) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getDabEccCode(sel), kDabEccCode);
}

TEST(BroadcastRadioUtilsTest, GetDabSCIdS) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getDabSCIdS(sel), kDabSCIdS);
}

}  // namespace aidl::android::hardware::broadcastradio
