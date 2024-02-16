/*
 * Copyright (C) 2023 The Android Open Source Project
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

#include "MockBroadcastRadioCallback.h"

#include <BroadcastRadio.h>
#include <VirtualRadio.h>
#include <broadcastradio-utils-aidl/Utils.h>

#include <android-base/logging.h>
#include <gtest/gtest.h>

namespace aidl::android::hardware::broadcastradio {

namespace {
using ::std::vector;

constexpr uint32_t kAmFreq1 = 560u;
constexpr uint32_t kAmFreq2 = 680u;
constexpr uint32_t kAmHdFreq = 1170u;
constexpr uint64_t kAmHdSid = 0xB0000001u;
constexpr uint32_t kFmFreq1 = 94900u;
constexpr uint64_t kFmHdSid1 = 0xA0000001u;
constexpr uint64_t kFmHdSid2 = 0xA0000002u;
constexpr uint32_t kFmHdFreq1 = 98500u;
constexpr uint32_t kFmHdSubChannel0 = 0u;
constexpr uint32_t kFmHdSubChannel1 = 1u;
constexpr uint32_t kFmFreq2 = 99100u;
constexpr uint32_t kFmHdFreq2 = 101100u;

const ProgramSelector kAmSel1 = utils::makeSelectorAmfm(kAmFreq1);
const ProgramSelector kAmSel2 = utils::makeSelectorAmfm(kAmFreq2);
const ProgramSelector kAmHdSel = utils::makeSelectorHd(kAmHdSid, kFmHdSubChannel0, kAmHdFreq);
const ProgramSelector kFmSel1 = utils::makeSelectorAmfm(kFmFreq1);
const ProgramSelector kFmSel2 = utils::makeSelectorAmfm(kFmFreq2);
const ProgramSelector kFmHdFreq1Sel1 =
        utils::makeSelectorHd(kFmHdSid1, kFmHdSubChannel0, kFmHdFreq1);
const ProgramSelector kFmHdFreq1Sel2 =
        utils::makeSelectorHd(kFmHdSid1, kFmHdSubChannel1, kFmHdFreq1);
const ProgramSelector kFmHdFreq2Sel1 =
        utils::makeSelectorHd(kFmHdSid2, kFmHdSubChannel0, kFmHdFreq2);
const ProgramSelector kFmHdFreq2Sel2 =
        utils::makeSelectorHd(kFmHdSid2, kFmHdSubChannel1, kFmHdFreq2);

const VirtualRadio& getAmFmMockTestRadio() {
    static VirtualRadio amFmRadioMockTestRadio(
            "AM/FM radio mock for test",
            {
                    {kAmSel1, "ProgramAm1", "ArtistAm1", "TitleAm1"},
                    {kAmSel2, "ProgramAm2", "ArtistAm2", "TitleAm2"},
                    {kFmSel1, "ProgramFm1", "ArtistFm1", "TitleFm1"},
                    {kFmSel2, "ProgramFm2", "ArtistFm2", "TitleFm2"},
                    {kAmHdSel, "ProgramAmHd1", "ArtistAmHd1", "TitleAmHd1"},
                    {kFmHdFreq1Sel1, "ProgramFmHd1", "ArtistFmHd1", "TitleFmHd1"},
                    {kFmHdFreq1Sel2, "ProgramFmHd2", "ArtistFmHd2", "TitleFmHd2"},
                    {kFmHdFreq2Sel1, "ProgramFmHd3", "ArtistFmHd3", "TitleFmHd3"},
                    {kFmHdFreq2Sel2, "ProgramFmHd4", "ArtistFmHd4", "TitleFmHd4"},
            });
    return amFmRadioMockTestRadio;
}

int getSignalAcquisitionFlags(const ProgramInfo& info) {
    return (info.infoFlags &
            (ProgramInfo::FLAG_SIGNAL_ACQUISITION | ProgramInfo::FLAG_HD_SIS_ACQUISITION |
             ProgramInfo::FLAG_HD_AUDIO_ACQUISITION)) >>
           6;
}

}  // namespace

class DefaultBroadcastRadioHalTest : public testing::Test {
  public:
    void SetUp() override {
        ::android::base::SetDefaultTag("BcRadioAidlDef.test");
        const VirtualRadio& amFmRadioMockTest = getAmFmMockTestRadio();
        mBroadcastRadioHal = ::ndk::SharedRefBase::make<BroadcastRadio>(amFmRadioMockTest);
        mTunerCallback = ndk::SharedRefBase::make<MockBroadcastRadioCallback>();
    }

    void TearDown() override {
        mBroadcastRadioHal->unsetTunerCallback();
        EXPECT_FALSE(mTunerCallback->isTunerFailed());
    }

    void verifyUpdatedProgramInfo(const ProgramSelector& sel) {
        ASSERT_TRUE(mTunerCallback->waitOnCurrentProgramInfoChangedCallback());
        ProgramInfo infoCb1 = mTunerCallback->getCurrentProgramInfo();
        mTunerCallback->reset();
        if (sel.primaryId.type == IdentifierType::HD_STATION_ID_EXT) {
            EXPECT_TRUE(mTunerCallback->waitOnCurrentProgramInfoChangedCallback());
            ProgramInfo infoCb2 = mTunerCallback->getCurrentProgramInfo();
            mTunerCallback->reset();
            EXPECT_TRUE(mTunerCallback->waitOnCurrentProgramInfoChangedCallback());
            ProgramInfo infoCb3 = mTunerCallback->getCurrentProgramInfo();
            mTunerCallback->reset();
            EXPECT_EQ(infoCb1.selector, sel);
            EXPECT_EQ(getSignalAcquisitionFlags(infoCb1), 0b001);
            EXPECT_EQ(infoCb2.selector, sel);
            EXPECT_EQ(getSignalAcquisitionFlags(infoCb2), 0b011);
            EXPECT_EQ(infoCb3.selector, sel);
            EXPECT_EQ(getSignalAcquisitionFlags(infoCb3), 0b111);
        } else {
            EXPECT_EQ(infoCb1.selector, sel);
        }
    }

    bool getAmFmBandRange(utils::FrequencyBand band, AmFmBandRange* res) {
        AmFmRegionConfig config;
        auto halResult = mBroadcastRadioHal->getAmFmRegionConfig(/* full= */ false, &config);
        if (!halResult.isOk()) {
            return false;
        }
        for (const auto& range : config.ranges) {
            if (utils::getBand(range.lowerBound) == band) {
                *res = range;
                return true;
            }
        }
        return false;
    }

    std::optional<utils::ProgramInfoSet> getProgramList() {
        ProgramFilter emptyFilter = {};
        return getProgramList(emptyFilter);
    }

    std::optional<utils::ProgramInfoSet> getProgramList(const ProgramFilter& filter) {
        mTunerCallback->reset();

        auto startResult = mBroadcastRadioHal->startProgramListUpdates(filter);

        EXPECT_TRUE(startResult.isOk());

        if (!startResult.isOk()) {
            return std::nullopt;
        }
        EXPECT_TRUE(mTunerCallback->waitProgramReady());

        auto stopResult = mBroadcastRadioHal->stopProgramListUpdates();

        EXPECT_TRUE(stopResult.isOk());

        return mTunerCallback->getProgramList();
    }

    void switchToFmBand() {
        ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
        mTunerCallback->reset();
        ASSERT_TRUE(mBroadcastRadioHal->tune(kFmSel1).isOk());
        verifyUpdatedProgramInfo(kFmSel1);
    }

    std::shared_ptr<BroadcastRadio> mBroadcastRadioHal;
    std::shared_ptr<MockBroadcastRadioCallback> mTunerCallback;
};

TEST_F(DefaultBroadcastRadioHalTest, GetAmFmRegionConfig) {
    AmFmRegionConfig config;

    auto halResult = mBroadcastRadioHal->getAmFmRegionConfig(/* full= */ false, &config);

    ASSERT_TRUE(halResult.isOk());
    EXPECT_EQ(config.fmDeemphasis, AmFmRegionConfig::DEEMPHASIS_D50);
    EXPECT_EQ(config.fmRds, AmFmRegionConfig::RDS);
}

TEST_F(DefaultBroadcastRadioHalTest, GetAmFmRegionConfigWithFullBand) {
    AmFmRegionConfig config;

    auto halResult = mBroadcastRadioHal->getAmFmRegionConfig(/* full= */ true, &config);

    ASSERT_TRUE(halResult.isOk());
    EXPECT_EQ(config.fmDeemphasis,
              AmFmRegionConfig::DEEMPHASIS_D50 | AmFmRegionConfig::DEEMPHASIS_D75);
    EXPECT_EQ(config.fmRds, AmFmRegionConfig::RDS | AmFmRegionConfig::RBDS);
}

TEST_F(DefaultBroadcastRadioHalTest, GetDabRegionConfig) {
    vector<DabTableEntry> config;

    auto halResult = mBroadcastRadioHal->getDabRegionConfig(&config);

    ASSERT_TRUE(halResult.isOk());
    ASSERT_FALSE(config.empty());
}

TEST_F(DefaultBroadcastRadioHalTest, GetImage) {
    vector<uint8_t> img;

    auto halResult = mBroadcastRadioHal->getImage(BroadcastRadio::INVALID_IMAGE, &img);

    ASSERT_TRUE(halResult.isOk());
    ASSERT_TRUE(img.empty());
}

TEST_F(DefaultBroadcastRadioHalTest, GetProperties) {
    vector<VirtualProgram> mockPrograms = getAmFmMockTestRadio().getProgramList();
    Properties prop;

    auto halResult = mBroadcastRadioHal->getProperties(&prop);

    ASSERT_TRUE(halResult.isOk());
    ASSERT_FALSE(prop.supportedIdentifierTypes.empty());
    std::unordered_set<IdentifierType> supportedTypeSet;
    for (const auto& supportedType : prop.supportedIdentifierTypes) {
        supportedTypeSet.insert(supportedType);
    }
    for (const auto& program : mockPrograms) {
        EXPECT_NE(supportedTypeSet.find(program.selector.primaryId.type), supportedTypeSet.end());
    }
}

TEST_F(DefaultBroadcastRadioHalTest, SetTunerCallback) {
    auto halResult = mBroadcastRadioHal->setTunerCallback(mTunerCallback);

    ASSERT_TRUE(halResult.isOk());
}

TEST_F(DefaultBroadcastRadioHalTest, SetTunerCallbackWithNull) {
    auto halResult = mBroadcastRadioHal->setTunerCallback(nullptr);

    ASSERT_EQ(halResult.getServiceSpecificError(), utils::resultToInt(Result::INVALID_ARGUMENTS));
}

TEST_F(DefaultBroadcastRadioHalTest, UnsetTunerCallbackWithNull) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());

    auto halResult = mBroadcastRadioHal->unsetTunerCallback();

    ASSERT_TRUE(halResult.isOk());
}

TEST_F(DefaultBroadcastRadioHalTest, TuneWithAmFmSelectorInProgramList) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();

    auto halResult = mBroadcastRadioHal->tune(kFmSel1);

    ASSERT_TRUE(halResult.isOk());
    ASSERT_TRUE(mTunerCallback->waitOnCurrentProgramInfoChangedCallback());
    ProgramInfo infoCb = mTunerCallback->getCurrentProgramInfo();
    EXPECT_EQ(infoCb.selector, kFmSel1);
}

TEST_F(DefaultBroadcastRadioHalTest, TuneWithHdSelectorInProgramList) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();

    auto halResult = mBroadcastRadioHal->tune(kFmHdFreq1Sel2);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel2);
}

TEST_F(DefaultBroadcastRadioHalTest, TuneWitFrequencyOfHdProgramInProgramList) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();

    auto halResult = mBroadcastRadioHal->tune(
            utils::makeSelectorAmfm(utils::getHdFrequency(kFmHdFreq1Sel1)));

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel1);
}

TEST_F(DefaultBroadcastRadioHalTest, TuneWithInvalidSelector) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    ProgramSelector invalidSelector = {utils::makeIdentifier(IdentifierType::AMFM_FREQUENCY_KHZ, 0),
                                       {}};

    auto halResult = mBroadcastRadioHal->tune(invalidSelector);

    ASSERT_EQ(halResult.getServiceSpecificError(), utils::resultToInt(Result::INVALID_ARGUMENTS));
}

TEST_F(DefaultBroadcastRadioHalTest, TuneWithoutTunerCallback) {
    auto halResult = mBroadcastRadioHal->tune(kFmSel1);

    ASSERT_EQ(halResult.getServiceSpecificError(), utils::resultToInt(Result::INVALID_STATE));
}

TEST_F(DefaultBroadcastRadioHalTest, StepUp) {
    AmFmBandRange fmRange;
    ASSERT_TRUE(getAmFmBandRange(utils::FrequencyBand::FM, &fmRange));
    ProgramSelector nextChannelSel =
            utils::makeSelectorAmfm(kFmSel1.primaryId.value + fmRange.spacing);
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmSel1).isOk());
    verifyUpdatedProgramInfo(kFmSel1);

    auto halResult = mBroadcastRadioHal->step(/* in_directionUp= */ true);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(nextChannelSel);
}

TEST_F(DefaultBroadcastRadioHalTest, StepUpFromUpperBound) {
    AmFmBandRange fmRange;
    ASSERT_TRUE(getAmFmBandRange(utils::FrequencyBand::FM, &fmRange));
    ProgramSelector upperBoundSel = utils::makeSelectorAmfm(fmRange.upperBound);
    ProgramSelector lowerBoundSel = utils::makeSelectorAmfm(fmRange.lowerBound);
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(upperBoundSel).isOk());
    verifyUpdatedProgramInfo(upperBoundSel);

    auto halResult = mBroadcastRadioHal->step(/* in_directionUp= */ true);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(lowerBoundSel);
}

TEST_F(DefaultBroadcastRadioHalTest, StepDown) {
    AmFmBandRange fmRange;
    ASSERT_TRUE(getAmFmBandRange(utils::FrequencyBand::FM, &fmRange));
    ProgramSelector nextChannelSel =
            utils::makeSelectorAmfm(kFmSel1.primaryId.value - fmRange.spacing);
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmSel1).isOk());
    verifyUpdatedProgramInfo(kFmSel1);

    auto halResult = mBroadcastRadioHal->step(/* directionUp= */ false);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(nextChannelSel);
}

TEST_F(DefaultBroadcastRadioHalTest, StepDownFromLowerBound) {
    AmFmBandRange fmRange;
    ASSERT_TRUE(getAmFmBandRange(utils::FrequencyBand::FM, &fmRange));
    ProgramSelector upperBoundSel = utils::makeSelectorAmfm(fmRange.upperBound);
    ProgramSelector lowerBoundSel = utils::makeSelectorAmfm(fmRange.lowerBound);
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(lowerBoundSel).isOk());
    verifyUpdatedProgramInfo(lowerBoundSel);

    auto halResult = mBroadcastRadioHal->step(/* directionUp= */ false);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(upperBoundSel);
}

TEST_F(DefaultBroadcastRadioHalTest, StepWithoutTunerCallback) {
    switchToFmBand();
    mBroadcastRadioHal->unsetTunerCallback();

    auto halResult = mBroadcastRadioHal->step(/* directionUp= */ false);

    ASSERT_EQ(halResult.getServiceSpecificError(), utils::resultToInt(Result::INVALID_STATE));
}

TEST_F(DefaultBroadcastRadioHalTest, SeekUpWithoutSkipSubchannel) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmHdFreq1Sel1).isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel1);

    auto halResult = mBroadcastRadioHal->seek(/* directionUp= */ true, /* skipSubChannel= */ false);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel2);
}

TEST_F(DefaultBroadcastRadioHalTest, SeekUpWithSkipSubchannel) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmHdFreq1Sel1).isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel1);

    auto halResult = mBroadcastRadioHal->seek(/* directionUp= */ true, /* skipSubChannel= */ true);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmSel2);
}

TEST_F(DefaultBroadcastRadioHalTest, SeekUpFromLastProgramInProgramList) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmHdFreq2Sel1).isOk());
    verifyUpdatedProgramInfo(kFmHdFreq2Sel1);

    auto halResult = mBroadcastRadioHal->seek(/* directionUp= */ true, /* skipSubChannel= */ true);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmSel1);
}

TEST_F(DefaultBroadcastRadioHalTest, SeekDownWithoutSkipSubchannel) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmHdFreq1Sel2).isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel2);

    auto halResult =
            mBroadcastRadioHal->seek(/* directionUp= */ false, /* skipSubChannel= */ false);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel1);
}

TEST_F(DefaultBroadcastRadioHalTest, SeekDownWithSkipSubchannel) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmHdFreq1Sel2).isOk());
    verifyUpdatedProgramInfo(kFmHdFreq1Sel2);

    auto halResult = mBroadcastRadioHal->seek(/* directionUp= */ false, /* skipSubChannel= */ true);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmSel1);
}

TEST_F(DefaultBroadcastRadioHalTest, SeekDownWithFirstProgramInProgramList) {
    switchToFmBand();

    auto halResult = mBroadcastRadioHal->seek(/* directionUp= */ false, /* skipSubChannel= */ true);

    ASSERT_TRUE(halResult.isOk());
    verifyUpdatedProgramInfo(kFmHdFreq2Sel1);
}

TEST_F(DefaultBroadcastRadioHalTest, SeekWithoutTunerCallback) {
    switchToFmBand();
    mBroadcastRadioHal->unsetTunerCallback();

    auto halResult = mBroadcastRadioHal->seek(/* directionUp= */ false, /* skipSubChannel= */ true);

    ASSERT_EQ(halResult.getServiceSpecificError(), utils::resultToInt(Result::INVALID_STATE));
}

TEST_F(DefaultBroadcastRadioHalTest, Cancel) {
    ASSERT_TRUE(mBroadcastRadioHal->setTunerCallback(mTunerCallback).isOk());
    mTunerCallback->reset();
    ASSERT_TRUE(mBroadcastRadioHal->tune(kFmSel1).isOk());

    auto halResult = mBroadcastRadioHal->cancel();

    ASSERT_TRUE(halResult.isOk());
    mTunerCallback->reset();
}

TEST_F(DefaultBroadcastRadioHalTest, SetConfigFlag) {
    ConfigFlag flag = ConfigFlag::FORCE_MONO;

    auto setResult = mBroadcastRadioHal->setConfigFlag(flag, /* value= */ true);

    ASSERT_TRUE(setResult.isOk());
}

TEST_F(DefaultBroadcastRadioHalTest, GetConfigFlag) {
    bool gotValue = false;
    ConfigFlag flag = ConfigFlag::FORCE_MONO;
    mBroadcastRadioHal->setConfigFlag(flag, /* value= */ true);

    auto getResult = mBroadcastRadioHal->isConfigFlagSet(flag, &gotValue);

    ASSERT_TRUE(getResult.isOk());
    ASSERT_TRUE(gotValue);
}

TEST_F(DefaultBroadcastRadioHalTest, StartProgramListUpdatesWithEmptyFilter) {
    switchToFmBand();

    auto programList = getProgramList();

    ASSERT_TRUE(programList.has_value());
    for (auto it = programList->begin(); it != programList->end(); it++) {
        EXPECT_EQ(utils::getBand(utils::getAmFmFrequency(it->selector)), utils::FrequencyBand::FM);
    }
}

TEST_F(DefaultBroadcastRadioHalTest, StartProgramListUpdatesWithAmFmFilter) {
    ProgramFilter amFmFilter = {.identifierTypes = {IdentifierType::AMFM_FREQUENCY_KHZ},
                                .identifiers = {},
                                .includeCategories = false,
                                .excludeModifications = false};
    switchToFmBand();

    auto programList = getProgramList(amFmFilter);

    ASSERT_TRUE(programList.has_value());
    for (auto it = programList->begin(); it != programList->end(); it++) {
        EXPECT_TRUE(utils::hasId(it->selector, IdentifierType::AMFM_FREQUENCY_KHZ));
        EXPECT_EQ(utils::getBand(utils::getAmFmFrequency(it->selector)), utils::FrequencyBand::FM);
    }
}

TEST_F(DefaultBroadcastRadioHalTest, StartProgramListUpdatesWhenHdIsDisabled) {
    switchToFmBand();
    mBroadcastRadioHal->setConfigFlag(ConfigFlag::FORCE_ANALOG_FM, /* value= */ true);

    auto programList = getProgramList();

    ASSERT_TRUE(programList.has_value());
    for (auto it = programList->begin(); it != programList->end(); it++) {
        EXPECT_FALSE(utils::hasId(it->selector, IdentifierType::HD_STATION_ID_EXT));
        EXPECT_EQ(utils::getBand(utils::getAmFmFrequency(it->selector)), utils::FrequencyBand::FM);
    }
}

}  // namespace aidl::android::hardware::broadcastradio
