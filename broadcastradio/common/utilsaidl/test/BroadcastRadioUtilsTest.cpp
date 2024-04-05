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
constexpr int64_t kRdsValue = 0xBEEF;

const Properties kAmFmTunerProp = {
        .maker = "makerTest",
        .product = "productTest",
        .supportedIdentifierTypes = {IdentifierType::AMFM_FREQUENCY_KHZ, IdentifierType::RDS_PI,
                                     IdentifierType::HD_STATION_ID_EXT}};

struct GetBandTestCase {
    std::string name;
    int64_t frequency;
    utils::FrequencyBand bandResult;
};

std::vector<GetBandTestCase> getBandTestCases() {
    return std::vector<GetBandTestCase>(
            {GetBandTestCase{.name = "unknown_low_band",
                             .frequency = 0,
                             .bandResult = utils::FrequencyBand::UNKNOWN},
             GetBandTestCase{.name = "am_lw_band",
                             .frequency = 30,
                             .bandResult = utils::FrequencyBand::AM_LW},
             GetBandTestCase{.name = "am_mw_band",
                             .frequency = 700,
                             .bandResult = utils::FrequencyBand::AM_MW},
             GetBandTestCase{.name = "am_sw_band",
                             .frequency = 2000,
                             .bandResult = utils::FrequencyBand::AM_SW},
             GetBandTestCase{
                     .name = "fm_band", .frequency = 97900, .bandResult = utils::FrequencyBand::FM},
             GetBandTestCase{.name = "unknown_high_band",
                             .frequency = 110000,
                             .bandResult = utils::FrequencyBand::UNKNOWN}});
}
}  // namespace

class GetBandTest : public testing::TestWithParam<GetBandTestCase> {};

INSTANTIATE_TEST_SUITE_P(GetBandTests, GetBandTest, testing::ValuesIn(getBandTestCases()),
                         [](const testing::TestParamInfo<GetBandTest::ParamType>& info) {
                             return info.param.name;
                         });

TEST_P(GetBandTest, GetBand) {
    GetBandTestCase testcase = GetParam();

    ASSERT_EQ(utils::getBand(testcase.frequency), testcase.bandResult);
}

TEST(BroadcastRadioUtilsTest, IdentifierIteratorBegin) {
    ProgramSelector sel = {
            .primaryId = utils::makeIdentifier(IdentifierType::RDS_PI, kRdsValue),
            .secondaryIds = {
                    utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz),
                    utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ,
                                          kFmFrequencyKHz + 200)}};

    utils::IdentifierIterator it = begin(sel);

    utils::IdentifierIterator selEnd = end(sel);
    ASSERT_NE(selEnd, it);
    EXPECT_EQ(sel.primaryId, *it);
}

TEST(BroadcastRadioUtilsTest, IdentifierIteratorIncrement) {
    ProgramSelector sel = {
            .primaryId = utils::makeIdentifier(IdentifierType::RDS_PI, kRdsValue),
            .secondaryIds = {
                    utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz),
                    utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ,
                                          kFmFrequencyKHz + 200)}};
    utils::IdentifierIterator it = begin(sel);
    utils::IdentifierIterator selEnd = end(sel);

    ASSERT_NE(selEnd, ++it);
    EXPECT_EQ(sel.secondaryIds[0], *it);
    ASSERT_NE(selEnd, ++it);
    EXPECT_EQ(sel.secondaryIds[1], *it);
    ASSERT_EQ(selEnd, ++it);
}

TEST(BroadcastRadioUtilsTest, IdentifierIteratorIncrementWithValue) {
    ProgramSelector sel = {
            .primaryId = utils::makeIdentifier(IdentifierType::RDS_PI, kRdsValue),
            .secondaryIds = {
                    utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz),
                    utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ,
                                          kFmFrequencyKHz + 200)}};
    utils::IdentifierIterator it1 = begin(sel);
    utils::IdentifierIterator it2 = it1;
    it2++;
    it2++;

    ASSERT_EQ(it1 + 2, it2);
}

TEST(BroadcastRadioUtilsTest, IdentifierIteratorBeginEndWithoutSecondaryIds) {
    ProgramSelector sel = {.primaryId = utils::makeIdentifier(IdentifierType::RDS_PI, kRdsValue)};

    utils::IdentifierIterator it = begin(sel);
    utils::IdentifierIterator selEnd = end(sel);

    ASSERT_EQ(selEnd, ++it);
}

TEST(BroadcastRadioUtilsTest, IdentifierIteratorBeginEndWithDifferentObjects) {
    ProgramSelector sel1 = utils::makeSelectorAmfm(kFmFrequencyKHz);
    ProgramSelector sel2 = utils::makeSelectorAmfm(kFmFrequencyKHz);

    utils::IdentifierIterator it1 = begin(sel1);
    utils::IdentifierIterator it2 = begin(sel2);
    utils::IdentifierIterator end1 = end(sel1);
    utils::IdentifierIterator end2 = end(sel2);

    ASSERT_NE(it1, it2);
    ASSERT_NE(end1, end2);
}

TEST(BroadcastRadioUtilsTest, IdentifierIteratorBeginEndWithTheSameObject) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    utils::IdentifierIterator it1 = begin(sel);
    utils::IdentifierIterator it2 = begin(sel);
    utils::IdentifierIterator end1 = end(sel);
    utils::IdentifierIterator end2 = end(sel);

    ASSERT_EQ(it1, it2);
    ASSERT_EQ(end1, end2);
}

TEST(BroadcastRadioUtilsTest, IsSupportedWithSupportedSelector) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::isSupported(kAmFmTunerProp, sel));
}

TEST(BroadcastRadioUtilsTest, IsSupportedWithUnsupportedSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_FALSE(utils::isSupported(kAmFmTunerProp, sel));
}

TEST(BroadcastRadioUtilsTest, GetBandWithFmFrequency) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::hasId(sel, IdentifierType::AMFM_FREQUENCY_KHZ));
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

TEST(BroadcastRadioUtilsTest, GetAllIdsWithAvailableIds) {
    int64_t secondaryFrequencyKHz = kFmFrequencyKHz + 200;
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);
    sel.secondaryIds.push_back(
            utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, secondaryFrequencyKHz));

    std::vector<int64_t> allIds = utils::getAllIds(sel, IdentifierType::AMFM_FREQUENCY_KHZ);

    ASSERT_EQ(allIds.size(), 2u);
    EXPECT_NE(std::find(allIds.begin(), allIds.end(), kFmFrequencyKHz), allIds.end());
    EXPECT_NE(std::find(allIds.begin(), allIds.end(), secondaryFrequencyKHz), allIds.end());
}

TEST(BroadcastRadioUtilsTest, GetAllIdsWithIdLongerThan32Bit) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    std::vector<int64_t> allIds = utils::getAllIds(sel, IdentifierType::DAB_SID_EXT);

    ASSERT_EQ(allIds.size(), 1u);
    EXPECT_NE(std::find(allIds.begin(), allIds.end(), kDabSidExt), allIds.end());
}

TEST(BroadcastRadioUtilsTest, GetAllIdsWithIdNotFound) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_TRUE(utils::getAllIds(sel, IdentifierType::AMFM_FREQUENCY_KHZ).empty());
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

TEST(BroadcastRadioUtilsTest, TunesToWithTheSameHdSelector) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);
    ProgramSelector selTarget = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    ASSERT_TRUE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToAmFmSelectorWithDifferentSubChannels) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);
    ProgramSelector selTarget = utils::makeSelectorAmfm(kHdFrequency);

    ASSERT_FALSE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToMainHdChannelWithDifferentSubChannels) {
    ProgramSelector sel = utils::makeSelectorAmfm(kHdFrequency);
    ProgramSelector selTarget =
            utils::makeSelectorHd(kHdStationId, /* subChannel= */ 0, kHdFrequency);

    ASSERT_TRUE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToWithTheSameAmFmSelector) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);
    ProgramSelector selTarget = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToWithDifferentFrequencies) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);
    ProgramSelector selTarget = utils::makeSelectorAmfm(kFmFrequencyKHz + 200);

    ASSERT_FALSE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToWithTheSameDabSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector selTarget = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_TRUE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToWithDabSelectorOfDifferentPrimaryIds) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt + 1, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector selTarget = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_FALSE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToWithDabSelectorOfDifferentSecondayIds) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble + 100, kDabFrequencyKhz);
    ProgramSelector selTarget = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_FALSE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, TunesToWithDabSelectorWithoutSecondaryIds) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt);
    ProgramSelector selTarget = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_TRUE(utils::tunesTo(sel, selTarget));
}

TEST(BroadcastRadioUtilsTest, SatisfiesWithSatisfiedIdTypesFilter) {
    ProgramFilter filter = ProgramFilter{.identifierTypes = {IdentifierType::DAB_FREQUENCY_KHZ}};
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_TRUE(utils::satisfies(filter, sel));
}

TEST(BroadcastRadioUtilsTest, SatisfiesWithUnsatisfiedIdTypesFilter) {
    ProgramFilter filter = ProgramFilter{.identifierTypes = {IdentifierType::DAB_FREQUENCY_KHZ}};
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_FALSE(utils::satisfies(filter, sel));
}

TEST(BroadcastRadioUtilsTest, SatisfiesWithSatisfiedIdsFilter) {
    ProgramFilter filter =
            ProgramFilter{.identifiers = {utils::makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ,
                                                                kDabFrequencyKhz)}};
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_TRUE(utils::satisfies(filter, sel));
}

TEST(BroadcastRadioUtilsTest, SatisfiesWithUnsatisfiedIdsFilter) {
    ProgramFilter filter =
            ProgramFilter{.identifiers = {utils::makeIdentifier(IdentifierType::DAB_FREQUENCY_KHZ,
                                                                kDabFrequencyKhz)}};
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz + 100);

    ASSERT_FALSE(utils::satisfies(filter, sel));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentAmFmFrequencies) {
    ProgramSelector sel1 = utils::makeSelectorAmfm(kHdFrequency - 200);
    ProgramSelector sel2 = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentAmFmSubChannels) {
    ProgramSelector sel1 = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);
    ProgramSelector sel2 = utils::makeSelectorHd(kHdStationId, kHdSubChannel + 1, kHdFrequency);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentDabFrequencies) {
    ProgramSelector sel1 = utils::makeSelectorDab(kDabSidExt + 100, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector sel2 = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz + 100);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentDabEccCode) {
    ProgramSelector sel1 =
            utils::makeSelectorDab(/* stationId= */ 0x0E10000C221u, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector sel2 =
            utils::makeSelectorDab(/* stationId= */ 0x0E20000C221u, kDabEnsemble, kDabFrequencyKhz);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentDabEnsembles) {
    ProgramSelector sel1 = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector sel2 = utils::makeSelectorDab(kDabSidExt, kDabEnsemble + 1, kDabFrequencyKhz);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentDabSid) {
    ProgramSelector sel1 =
            utils::makeSelectorDab(/* stationId= */ 0x0E10000C221u, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector sel2 =
            utils::makeSelectorDab(/* stationId= */ 0x0E10000C222u, kDabEnsemble, kDabFrequencyKhz);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramSelectorComparatorWithDifferentDabSCIdS) {
    ProgramSelector sel1 =
            utils::makeSelectorDab(/* stationId= */ 0x0E10000C221u, kDabEnsemble, kDabFrequencyKhz);
    ProgramSelector sel2 =
            utils::makeSelectorDab(/* stationId= */ 0x1E10000C221u, kDabEnsemble, kDabFrequencyKhz);

    EXPECT_TRUE(utils::ProgramSelectorComparator()(sel1, sel2));
    EXPECT_FALSE(utils::ProgramSelectorComparator()(sel2, sel1));
}

TEST(BroadcastRadioUtilsTest, ProgramInfoComparator) {
    ProgramSelector sel1 = utils::makeSelectorAmfm(kFmFrequencyKHz);
    ProgramSelector sel2 = utils::makeSelectorAmfm(kFmFrequencyKHz + 200);
    ProgramInfo info1 = {.selector = sel1,
                         .logicallyTunedTo = sel1.primaryId,
                         .physicallyTunedTo = sel1.primaryId};
    ProgramInfo info2 = {.selector = sel2,
                         .logicallyTunedTo = sel2.primaryId,
                         .physicallyTunedTo = sel2.primaryId};

    EXPECT_TRUE(utils::ProgramInfoComparator()(info1, info2));
    EXPECT_FALSE(utils::ProgramInfoComparator()(info2, info1));
}

}  // namespace aidl::android::hardware::broadcastradio
