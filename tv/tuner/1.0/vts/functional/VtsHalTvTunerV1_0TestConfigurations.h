/*
 * Copyright 2020 The Android Open Source Project
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

#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>

#include "../../../config/TunerTestingConfigReader.h"

// TODO: remove unnecessary imports after config reader refactoring is done.
using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DemuxAlpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxIpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxMmtpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxRecordScIndexType;
using android::hardware::tv::tuner::V1_0::DemuxTlvFilterType;
using android::hardware::tv::tuner::V1_0::DemuxTpid;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
using android::hardware::tv::tuner::V1_0::DvrSettings;
using android::hardware::tv::tuner::V1_0::DvrType;
using android::hardware::tv::tuner::V1_0::FrontendDvbtBandwidth;
using android::hardware::tv::tuner::V1_0::FrontendDvbtCoderate;
using android::hardware::tv::tuner::V1_0::FrontendDvbtConstellation;
using android::hardware::tv::tuner::V1_0::FrontendDvbtGuardInterval;
using android::hardware::tv::tuner::V1_0::FrontendDvbtHierarchy;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtStandard;
using android::hardware::tv::tuner::V1_0::FrontendDvbtTransmissionMode;
using android::hardware::tv::tuner::V1_0::FrontendSettings;
using android::hardware::tv::tuner::V1_0::FrontendStatus;
using android::hardware::tv::tuner::V1_0::FrontendStatusType;
using android::hardware::tv::tuner::V1_0::FrontendType;
using android::hardware::tv::tuner::V1_0::LnbPosition;
using android::hardware::tv::tuner::V1_0::LnbTone;
using android::hardware::tv::tuner::V1_0::LnbVoltage;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::RecordSettings;

using namespace std;
using namespace android::media::tuner::testing::configuration::V1_0;

// TODO: remove all the constants and structs after config reader refactoring is done.
const uint32_t FMQ_SIZE_512K = 0x80000;
const uint32_t FMQ_SIZE_1M = 0x100000;
const uint32_t FMQ_SIZE_4M = 0x400000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

#define CLEAR_KEY_SYSTEM_ID 0xF6D8
#define FILTER_MAIN_TYPE_BIT_COUNT 32
#define PROVISION_STR                                      \
    "{                                                   " \
    "  \"id\": 21140844,                                 " \
    "  \"name\": \"Test Title\",                         " \
    "  \"lowercase_organization_name\": \"Android\",     " \
    "  \"asset_key\": {                                  " \
    "  \"encryption_key\": \"nezAr3CHFrmBR9R8Tedotw==\"  " \
    "  },                                                " \
    "  \"cas_type\": 1,                                  " \
    "  \"track_types\": [ ]                              " \
    "}                                                   "

typedef enum {
    TS_VIDEO0,
    TS_VIDEO1,
    TS_AUDIO0,
    TS_AUDIO1,
    TS_PES0,
    TS_PCR0,
    TS_SECTION0,
    TS_TS0,
    TS_RECORD0,
    FILTER_MAX,
} Filter;

typedef enum {
    TIMER0,
    TIMER_MAX,
} TimeFilter;

typedef enum {
    SOURCE,
    SINK,
    LINKAGE_DIR,
} Linkage;

typedef enum {
    LNB0,
    LNB_EXTERNAL,
    LNB_MAX,
} Lnb;

typedef enum {
    DISEQC_POWER_ON,
    DISEQC_MAX,
} Diseqc;

typedef enum {
    DESC_0,
    DESC_MAX,
} Descrambler;

struct FilterConfig {
    uint32_t bufferSize;
    DemuxFilterType type;
    DemuxFilterSettings settings;
    bool getMqDesc;

    bool operator<(const FilterConfig& /*c*/) const { return false; }
};

struct TimeFilterConfig {
    bool supportTimeFilter;
    uint64_t timeStamp;
};

struct LnbConfig {
    bool usingLnb;
    string name;
    LnbVoltage voltage;
    LnbTone tone;
    LnbPosition position;
};

struct DescramblerConfig {
    uint32_t casSystemId;
    string provisionStr;
    vector<uint8_t> hidlPvtData;
};

// TODO: remove all the manual config array after the dynamic config refactoring is done.
static LnbConfig lnbArray[LNB_MAX];
static vector<uint8_t> diseqcMsgArray[DISEQC_MAX];
static FilterConfig filterArray[FILTER_MAX];
static TimeFilterConfig timeFilterArray[TIMER_MAX];
static DemuxFilterType filterLinkageTypes[LINKAGE_DIR][FILTER_MAIN_TYPE_BIT_COUNT];
static DescramblerConfig descramblerArray[DESC_MAX];

// Hardware configs
static map<string, FrontendConfig> frontendMap;
static map<string, DvrConfig> dvrMap;

// Hardware and test cases connections
static LiveBroadcastHardwareConnections live;
static ScanHardwareConnections scan;
static DvrPlaybackHardwareConnections playback;
static DvrRecordHardwareConnections record;
static DescramblingHardwareConnections descrambling;
static LnbLiveHardwareConnections lnbLive;
static LnbRecordHardwareConnections lnbRecord;

/** Config all the frontends that would be used in the tests */
inline void initFrontendConfig() {
    // The test will use the internal default fe when default fe is connected to any data flow
    // without overriding in the xml config.
    string defaultFeId = "FE_DEFAULT";
    FrontendDvbtSettings dvbtSettings{
            .frequency = 578000,
            .transmissionMode = FrontendDvbtTransmissionMode::AUTO,
            .bandwidth = FrontendDvbtBandwidth::BANDWIDTH_8MHZ,
            .isHighPriority = true,
    };
    frontendMap[defaultFeId].type = FrontendType::DVBT;
    frontendMap[defaultFeId].settings.dvbt(dvbtSettings);

    vector<FrontendStatusType> types;
    types.push_back(FrontendStatusType::DEMOD_LOCK);
    FrontendStatus status;
    status.isDemodLocked(true);
    vector<FrontendStatus> statuses;
    statuses.push_back(status);
    frontendMap[defaultFeId].tuneStatusTypes = types;
    frontendMap[defaultFeId].expectTuneStatuses = statuses;
    frontendMap[defaultFeId].isSoftwareFe = true;

    // Read customized config
    TunerTestingConfigReader::readFrontendConfig1_0(frontendMap);
};

/** Config all the dvrs that would be used in the tests */
inline void initDvrConfig() {
    // Read customized config
    TunerTestingConfigReader::readDvrConfig1_0(dvrMap);
};

/** Read the vendor configurations of which hardware to use for each test cases/data flows */
inline void connectHardwaresToTestCases() {
    TunerTestingConfigReader::connectLiveBroadcast(live);
    TunerTestingConfigReader::connectScan(scan);
    TunerTestingConfigReader::connectDvrPlayback(playback);
    TunerTestingConfigReader::connectDvrRecord(record);
    TunerTestingConfigReader::connectDescrambling(descrambling);
    TunerTestingConfigReader::connectLnbLive(lnbLive);
    TunerTestingConfigReader::connectLnbRecord(lnbRecord);
};

inline bool validateConnections() {
    bool feIsValid = frontendMap.find(live.frontendId) != frontendMap.end() &&
                     frontendMap.find(scan.frontendId) != frontendMap.end();
    feIsValid &= record.support ? frontendMap.find(record.frontendId) != frontendMap.end() : true;
    feIsValid &= descrambling.support
                         ? frontendMap.find(descrambling.frontendId) != frontendMap.end()
                         : true;
    feIsValid &= lnbLive.support ? frontendMap.find(lnbLive.frontendId) != frontendMap.end() : true;
    feIsValid &=
            lnbRecord.support ? frontendMap.find(lnbRecord.frontendId) != frontendMap.end() : true;

    if (!feIsValid) {
        ALOGW("[vts config] dynamic config fe connection is invalid.");
        return false;
    }

    bool dvrIsValid = frontendMap[live.frontendId].isSoftwareFe
                              ? dvrMap.find(live.dvrSoftwareFeId) != dvrMap.end()
                              : true;
    dvrIsValid &= playback.support ? dvrMap.find(playback.dvrId) != dvrMap.end() : true;

    if (record.support) {
        if (frontendMap[record.frontendId].isSoftwareFe) {
            dvrIsValid &= dvrMap.find(record.dvrSoftwareFeId) != dvrMap.end();
        }
        dvrIsValid &= dvrMap.find(record.dvrRecordId) != dvrMap.end();
    }

    if (descrambling.support && frontendMap[descrambling.frontendId].isSoftwareFe) {
        dvrIsValid &= dvrMap.find(descrambling.dvrSoftwareFeId) != dvrMap.end();
    }

    if (!dvrIsValid) {
        ALOGW("[vts config] dynamic config dvr connection is invalid.");
        return false;
    }

    return true;
}

// TODO: remove all the manual configs after the dynamic config refactoring is done.
/** Configuration array for the Lnb test */
inline void initLnbConfig() {
    lnbArray[LNB0].usingLnb = true;
    lnbArray[LNB0].voltage = LnbVoltage::VOLTAGE_12V;
    lnbArray[LNB0].tone = LnbTone::NONE;
    lnbArray[LNB0].position = LnbPosition::UNDEFINED;
    lnbArray[LNB_EXTERNAL].usingLnb = true;
    lnbArray[LNB_EXTERNAL].name = "default_lnb_external";
    lnbArray[LNB_EXTERNAL].voltage = LnbVoltage::VOLTAGE_5V;
    lnbArray[LNB_EXTERNAL].tone = LnbTone::NONE;
    lnbArray[LNB_EXTERNAL].position = LnbPosition::UNDEFINED;
};

/** Diseqc messages array for the Lnb test */
inline void initDiseqcMsg() {
    diseqcMsgArray[DISEQC_POWER_ON] = {0xE, 0x0, 0x0, 0x0, 0x0, 0x3};
};

/** Configuration array for the filter test */
inline void initFilterConfig() {
    // TS VIDEO filter setting for default implementation testing
    filterArray[TS_VIDEO0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_VIDEO0].type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterArray[TS_VIDEO0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_VIDEO0].settings.ts().tpid = 256;
    filterArray[TS_VIDEO0].settings.ts().filterSettings.av({.isPassthrough = false});
    filterArray[TS_VIDEO1].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_VIDEO1].type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterArray[TS_VIDEO1].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_VIDEO1].settings.ts().tpid = 256;
    filterArray[TS_VIDEO1].settings.ts().filterSettings.av({.isPassthrough = false});
    // TS AUDIO filter setting
    filterArray[TS_AUDIO0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_AUDIO0].type.subType.tsFilterType(DemuxTsFilterType::AUDIO);
    filterArray[TS_AUDIO0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_AUDIO0].settings.ts().tpid = 256;
    filterArray[TS_AUDIO0].settings.ts().filterSettings.av({.isPassthrough = false});
    filterArray[TS_AUDIO1].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_AUDIO1].type.subType.tsFilterType(DemuxTsFilterType::AUDIO);
    filterArray[TS_AUDIO1].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_AUDIO1].settings.ts().tpid = 257;
    filterArray[TS_AUDIO1].settings.ts().filterSettings.av({.isPassthrough = false});
    // TS PES filter setting
    filterArray[TS_PES0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_PES0].type.subType.tsFilterType(DemuxTsFilterType::PES);
    filterArray[TS_PES0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_PES0].settings.ts().tpid = 256;
    filterArray[TS_PES0].settings.ts().filterSettings.pesData({
            .isRaw = false,
            .streamId = 0xbd,
    });
    filterArray[TS_PES0].getMqDesc = true;
    // TS PCR filter setting
    filterArray[TS_PCR0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_PCR0].type.subType.tsFilterType(DemuxTsFilterType::PCR);
    filterArray[TS_PCR0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_PCR0].settings.ts().tpid = 256;
    filterArray[TS_PCR0].settings.ts().filterSettings.noinit();
    // TS filter setting
    filterArray[TS_TS0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_TS0].type.subType.tsFilterType(DemuxTsFilterType::TS);
    filterArray[TS_TS0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_TS0].settings.ts().tpid = 256;
    filterArray[TS_TS0].settings.ts().filterSettings.noinit();
    // TS SECTION filter setting
    filterArray[TS_SECTION0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_SECTION0].type.subType.tsFilterType(DemuxTsFilterType::SECTION);
    filterArray[TS_SECTION0].bufferSize = FMQ_SIZE_16M;
    filterArray[TS_SECTION0].settings.ts().tpid = 256;
    filterArray[TS_SECTION0].settings.ts().filterSettings.section({
            .isRaw = false,
    });
    filterArray[TS_SECTION0].getMqDesc = true;
    // TS RECORD filter setting
    filterArray[TS_RECORD0].type.mainType = DemuxFilterMainType::TS;
    filterArray[TS_RECORD0].type.subType.tsFilterType(DemuxTsFilterType::RECORD);
    filterArray[TS_RECORD0].settings.ts().tpid = 81;
    filterArray[TS_RECORD0].settings.ts().filterSettings.record({
            .scIndexType = DemuxRecordScIndexType::NONE,
    });

    // TS Linkage filter setting
    filterLinkageTypes[SOURCE][0].mainType = DemuxFilterMainType::TS;
    filterLinkageTypes[SOURCE][0].subType.tsFilterType(DemuxTsFilterType::TS);
    filterLinkageTypes[SINK][0] = filterLinkageTypes[SOURCE][0];
    // MMTP Linkage filter setting
    filterLinkageTypes[SOURCE][1].mainType = DemuxFilterMainType::MMTP;
    filterLinkageTypes[SOURCE][1].subType.mmtpFilterType(DemuxMmtpFilterType::AUDIO);
    filterLinkageTypes[SINK][1] = filterLinkageTypes[SOURCE][1];
    // IP Linkage filter setting
    filterLinkageTypes[SOURCE][2].mainType = DemuxFilterMainType::IP;
    filterLinkageTypes[SOURCE][2].subType.ipFilterType(DemuxIpFilterType::IP);
    filterLinkageTypes[SINK][2] = filterLinkageTypes[SOURCE][2];
    // TLV Linkage filter setting
    filterLinkageTypes[SOURCE][3].mainType = DemuxFilterMainType::TLV;
    filterLinkageTypes[SOURCE][3].subType.tlvFilterType(DemuxTlvFilterType::TLV);
    filterLinkageTypes[SINK][3] = filterLinkageTypes[SOURCE][3];
    // ALP Linkage PTP filter setting
    filterLinkageTypes[SOURCE][4].mainType = DemuxFilterMainType::ALP;
    filterLinkageTypes[SOURCE][4].subType.alpFilterType(DemuxAlpFilterType::PTP);
    filterLinkageTypes[SINK][4] = filterLinkageTypes[SOURCE][4];
};

/** Configuration array for the timer filter test */
inline void initTimeFilterConfig() {
    timeFilterArray[TIMER0].supportTimeFilter = true;
    timeFilterArray[TIMER0].timeStamp = 1;
}

/** Configuration array for the descrambler test */
inline void initDescramblerConfig() {
    descramblerArray[DESC_0].casSystemId = CLEAR_KEY_SYSTEM_ID;
    descramblerArray[DESC_0].provisionStr = PROVISION_STR;
    descramblerArray[DESC_0].hidlPvtData.resize(256);
};
