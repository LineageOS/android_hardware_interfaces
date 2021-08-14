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

#include "../../../config/TunerTestingConfigReaderV1_0.h"

using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
using android::hardware::tv::tuner::V1_0::FrontendDvbtBandwidth;
using android::hardware::tv::tuner::V1_0::FrontendDvbtSettings;
using android::hardware::tv::tuner::V1_0::FrontendDvbtTransmissionMode;
using android::hardware::tv::tuner::V1_0::FrontendSettings;
using android::hardware::tv::tuner::V1_0::FrontendStatus;
using android::hardware::tv::tuner::V1_0::FrontendStatusType;
using android::hardware::tv::tuner::V1_0::FrontendType;

using namespace std;
using namespace android::media::tuner::testing::configuration::V1_0;

const uint32_t FMQ_SIZE_4M = 0x400000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

const string configFilePath = "/vendor/etc/tuner_vts_config_1_0.xml";

#define FILTER_MAIN_TYPE_BIT_COUNT 5

// Hardware configs
static map<string, FrontendConfig> frontendMap;
static map<string, FilterConfig> filterMap;
static map<string, DvrConfig> dvrMap;
static map<string, LnbConfig> lnbMap;
static map<string, TimeFilterConfig> timeFilterMap;
static map<string, vector<uint8_t>> diseqcMsgMap;
static map<string, DescramblerConfig> descramblerMap;

// Hardware and test cases connections
static LiveBroadcastHardwareConnections live;
static ScanHardwareConnections scan;
static DvrPlaybackHardwareConnections playback;
static DvrRecordHardwareConnections record;
static DescramblingHardwareConnections descrambling;
static LnbLiveHardwareConnections lnbLive;
static LnbRecordHardwareConnections lnbRecord;
static TimeFilterHardwareConnections timeFilter;

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
    TunerTestingConfigReader1_0::readFrontendConfig1_0(frontendMap);
};

inline void initFilterConfig() {
    // The test will use the internal default filter when default filter is connected to any
    // data flow without overriding in the xml config.
    string defaultAudioFilterId = "FILTER_AUDIO_DEFAULT";
    string defaultVideoFilterId = "FILTER_VIDEO_DEFAULT";

    filterMap[defaultVideoFilterId].type.mainType = DemuxFilterMainType::TS;
    filterMap[defaultVideoFilterId].type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterMap[defaultVideoFilterId].bufferSize = FMQ_SIZE_16M;
    filterMap[defaultVideoFilterId].settings.ts().tpid = 256;
    filterMap[defaultVideoFilterId].settings.ts().filterSettings.av({.isPassthrough = false});

    filterMap[defaultAudioFilterId].type.mainType = DemuxFilterMainType::TS;
    filterMap[defaultAudioFilterId].type.subType.tsFilterType(DemuxTsFilterType::AUDIO);
    filterMap[defaultAudioFilterId].bufferSize = FMQ_SIZE_16M;
    filterMap[defaultAudioFilterId].settings.ts().tpid = 256;
    filterMap[defaultAudioFilterId].settings.ts().filterSettings.av({.isPassthrough = false});

    // Read customized config
    TunerTestingConfigReader1_0::readFilterConfig1_0(filterMap);
};

/** Config all the dvrs that would be used in the tests */
inline void initDvrConfig() {
    // Read customized config
    TunerTestingConfigReader1_0::readDvrConfig1_0(dvrMap);
};

/** Config all the lnbs that would be used in the tests */
inline void initLnbConfig() {
    // Read customized config
    TunerTestingConfigReader1_0::readLnbConfig1_0(lnbMap);
    TunerTestingConfigReader1_0::readDiseqcMessages(diseqcMsgMap);
};

/** Config all the time filters that would be used in the tests */
inline void initTimeFilterConfig() {
    // Read customized config
    TunerTestingConfigReader1_0::readTimeFilterConfig1_0(timeFilterMap);
};

/** Config all the descramblers that would be used in the tests */
inline void initDescramblerConfig() {
    // Read customized config
    TunerTestingConfigReader1_0::readDescramblerConfig1_0(descramblerMap);
};

/** Read the vendor configurations of which hardware to use for each test cases/data flows */
inline void connectHardwaresToTestCases() {
    TunerTestingConfigReader1_0::connectLiveBroadcast(live);
    TunerTestingConfigReader1_0::connectScan(scan);
    TunerTestingConfigReader1_0::connectDvrPlayback(playback);
    TunerTestingConfigReader1_0::connectDvrRecord(record);
    TunerTestingConfigReader1_0::connectDescrambling(descrambling);
    TunerTestingConfigReader1_0::connectLnbLive(lnbLive);
    TunerTestingConfigReader1_0::connectLnbRecord(lnbRecord);
    TunerTestingConfigReader1_0::connectTimeFilter(timeFilter);
};

inline bool validateConnections() {
    if ((!live.hasFrontendConnection || !scan.hasFrontendConnection) && !playback.support) {
        ALOGW("[vts config] VTS must support either a DVR source or a Frontend source.");
        return false;
    }

    if (record.support && !record.hasFrontendConnection &&
        record.dvrSourceId.compare(emptyHardwareId) == 0) {
        ALOGW("[vts config] Record must support either a DVR source or a Frontend source.");
        return false;
    }

    if (descrambling.support && !descrambling.hasFrontendConnection &&
        descrambling.dvrSourceId.compare(emptyHardwareId) == 0) {
        ALOGW("[vts config] Descrambling must support either a DVR source or a Frontend source.");
        return false;
    }

    bool feIsValid = live.hasFrontendConnection
                             ? frontendMap.find(live.frontendId) != frontendMap.end()
                             : true;
    feIsValid &= scan.hasFrontendConnection ? frontendMap.find(scan.frontendId) != frontendMap.end()
                                            : true;
    feIsValid &= record.support && record.hasFrontendConnection
                         ? frontendMap.find(record.frontendId) != frontendMap.end()
                         : true;
    feIsValid &= (descrambling.support && descrambling.hasFrontendConnection)
                         ? frontendMap.find(descrambling.frontendId) != frontendMap.end()
                         : true;
    feIsValid &= lnbLive.support ? frontendMap.find(lnbLive.frontendId) != frontendMap.end() : true;
    feIsValid &=
            lnbRecord.support ? frontendMap.find(lnbRecord.frontendId) != frontendMap.end() : true;

    if (!feIsValid) {
        ALOGW("[vts config] dynamic config fe connection is invalid.");
        return false;
    }

    bool dvrIsValid = (live.hasFrontendConnection && frontendMap[live.frontendId].isSoftwareFe)
                              ? dvrMap.find(live.dvrSoftwareFeId) != dvrMap.end()
                              : true;
    dvrIsValid &= playback.support ? dvrMap.find(playback.dvrId) != dvrMap.end() : true;
    if (record.support) {
        if (record.hasFrontendConnection) {
            if (frontendMap[record.frontendId].isSoftwareFe) {
                dvrIsValid &= dvrMap.find(record.dvrSoftwareFeId) != dvrMap.end();
            }
        } else {
            dvrIsValid &= dvrMap.find(record.dvrSourceId) != dvrMap.end();
        }
        dvrIsValid &= dvrMap.find(record.dvrRecordId) != dvrMap.end();
    }
    if (descrambling.support) {
        if (descrambling.hasFrontendConnection) {
            if (frontendMap[descrambling.frontendId].isSoftwareFe) {
                dvrIsValid &= dvrMap.find(descrambling.dvrSoftwareFeId) != dvrMap.end();
            }
        } else {
            dvrIsValid &= dvrMap.find(descrambling.dvrSourceId) != dvrMap.end();
        }
    }

    if (!dvrIsValid) {
        ALOGW("[vts config] dynamic config dvr connection is invalid.");
        return false;
    }

    bool filterIsValid = (live.hasFrontendConnection)
                             ? filterMap.find(live.audioFilterId) != filterMap.end() &&
                               filterMap.find(live.videoFilterId) != filterMap.end()
                             : true;
    filterIsValid &= playback.support
                             ? (filterMap.find(playback.audioFilterId) != filterMap.end() &&
                                filterMap.find(playback.videoFilterId) != filterMap.end())
                             : true;
    filterIsValid &=
            record.support ? filterMap.find(record.recordFilterId) != filterMap.end() : true;
    filterIsValid &= descrambling.support
                             ? (filterMap.find(descrambling.audioFilterId) != filterMap.end() &&
                                filterMap.find(descrambling.videoFilterId) != filterMap.end())
                             : true;
    filterIsValid &= lnbLive.support ? (filterMap.find(lnbLive.audioFilterId) != filterMap.end() &&
                                        filterMap.find(lnbLive.videoFilterId) != filterMap.end())
                                     : true;
    filterIsValid &=
            lnbRecord.support ? filterMap.find(lnbRecord.recordFilterId) != filterMap.end() : true;

    if (!filterIsValid) {
        ALOGW("[vts config] dynamic config filter connection is invalid.");
        return false;
    }

    bool lnbIsValid = lnbLive.support ? lnbMap.find(lnbLive.lnbId) != lnbMap.end() : true;
    lnbIsValid &= lnbRecord.support ? lnbMap.find(lnbRecord.lnbId) != lnbMap.end() : true;

    if (!lnbIsValid) {
        ALOGW("[vts config] dynamic config lnb connection is invalid.");
        return false;
    }

    bool descramblerIsValid =
            descrambling.support
                    ? descramblerMap.find(descrambling.descramblerId) != descramblerMap.end()
                    : true;

    if (!descramblerIsValid) {
        ALOGW("[vts config] dynamic config descrambler connection is invalid.");
        return false;
    }

    bool diseqcMsgIsValid = true;
    if (lnbLive.support) {
        for (auto msgName : lnbLive.diseqcMsgs) {
            diseqcMsgIsValid &= diseqcMsgMap.find(msgName) != diseqcMsgMap.end();
        }
    }
    if (lnbRecord.support) {
        for (auto msgName : lnbRecord.diseqcMsgs) {
            diseqcMsgIsValid &= diseqcMsgMap.find(msgName) != diseqcMsgMap.end();
        }
    }

    if (!diseqcMsgIsValid) {
        ALOGW("[vts config] dynamic config diseqcMsg sender is invalid.");
        return false;
    }

    return true;
}
