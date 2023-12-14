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
constexpr uint64_t kDabSidExt = 0x0E10000C221u;
constexpr uint32_t kDabEnsemble = 0xCE15u;
constexpr uint64_t kDabFrequencyKhz = 225648u;
constexpr uint64_t kHdStationId = 0xA0000001u;
constexpr uint64_t kHdSubChannel = 1u;
constexpr uint64_t kHdFrequency = 97700u;
}  // namespace

TEST(BroadcastRadioUtilsTest, hasIdWithPrimaryIdType) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::hasId(sel, IdentifierType::AMFM_FREQUENCY_KHZ));
}

TEST(BroadcastRadioUtilsTest, makeIdentifier) {
    ProgramIdentifier id =
            utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, kFmFrequencyKHz);

    ASSERT_EQ(id.type, IdentifierType::AMFM_FREQUENCY_KHZ);
    ASSERT_EQ(id.value, kFmFrequencyKHz);
}

TEST(BroadcastRadioUtilsTest, makeSelectorAmfm) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_EQ(sel.primaryId.type, IdentifierType::AMFM_FREQUENCY_KHZ);
    ASSERT_EQ(sel.primaryId.value, kFmFrequencyKHz);
    ASSERT_TRUE(sel.secondaryIds.empty());
}

TEST(BroadcastRadioUtilsTest, makeSelectorHd) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    ASSERT_EQ(sel.primaryId.type, IdentifierType::HD_STATION_ID_EXT);
    ASSERT_TRUE(sel.secondaryIds.empty());
    ASSERT_EQ(utils::getHdSubchannel(sel), static_cast<int>(kHdSubChannel));
    ASSERT_EQ(utils::getHdFrequency(sel), static_cast<uint32_t>(kHdFrequency));
}

TEST(BroadcastRadioUtilsTest, makeHdRadioStationName) {
    std::string stationName = "aB1-FM";
    int64_t expectedIdValue = 0x4D46314241;

    ProgramIdentifier stationNameId = utils::makeHdRadioStationName(stationName);

    ASSERT_EQ(stationNameId.type, IdentifierType::HD_STATION_NAME);
    ASSERT_EQ(stationNameId.value, expectedIdValue);
}

TEST(BroadcastRadioUtilsTest, getHdFrequencyWithoutHdId) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getHdFrequency(sel), 0u);
}

TEST(BroadcastRadioUtilsTest, hasAmFmFrequencyWithAmFmSelector) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_TRUE(utils::hasAmFmFrequency(sel));
}

TEST(BroadcastRadioUtilsTest, hasAmFmFrequencyWithHdSelector) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    ASSERT_TRUE(utils::hasAmFmFrequency(sel));
}

TEST(BroadcastRadioUtilsTest, hasAmFmFrequencyWithNonAmFmHdSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_FALSE(utils::hasAmFmFrequency(sel));
}

TEST(BroadcastRadioUtilsTest, getAmFmFrequencyWithAmFmSelector) {
    ProgramSelector sel = utils::makeSelectorAmfm(kFmFrequencyKHz);

    ASSERT_EQ(utils::getAmFmFrequency(sel), static_cast<uint32_t>(kFmFrequencyKHz));
}

TEST(BroadcastRadioUtilsTest, getAmFmFrequencyWithHdSelector) {
    ProgramSelector sel = utils::makeSelectorHd(kHdStationId, kHdSubChannel, kHdFrequency);

    ASSERT_EQ(utils::getAmFmFrequency(sel), static_cast<uint32_t>(kHdFrequency));
}

TEST(BroadcastRadioUtilsTest, getAmFmFrequencyWithNonAmFmHdSelector) {
    ProgramSelector sel = utils::makeSelectorDab(kDabSidExt, kDabEnsemble, kDabFrequencyKhz);

    ASSERT_EQ(utils::getAmFmFrequency(sel), 0u);
}

}  // namespace aidl::android::hardware::broadcastradio
