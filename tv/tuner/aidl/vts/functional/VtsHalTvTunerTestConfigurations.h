/*
 * Copyright 2021 The Android Open Source Project
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

#include <binder/MemoryDealer.h>

#include "../../../config/TunerTestingConfigAidlReaderV1_0.h"

#include <aidl/android/hardware/tv/tuner/DataFormat.h>
#include <aidl/android/hardware/tv/tuner/DemuxAlpFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterMainType.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterMonitorEventType.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxIpAddress.h>
#include <aidl/android/hardware/tv/tuner/DemuxIpFilterSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxIpFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxMmtpFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxRecordScIndexType.h>
#include <aidl/android/hardware/tv/tuner/DemuxTsFilterType.h>
#include <aidl/android/hardware/tv/tuner/DvrSettings.h>
#include <aidl/android/hardware/tv/tuner/DvrType.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtBandwidth.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtCoderate.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtConstellation.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtGuardInterval.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtHierarchy.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtSettings.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtStandard.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtTransmissionMode.h>
#include <aidl/android/hardware/tv/tuner/FrontendSettings.h>
#include <aidl/android/hardware/tv/tuner/FrontendType.h>
#include <aidl/android/hardware/tv/tuner/PlaybackSettings.h>
#include <aidl/android/hardware/tv/tuner/RecordSettings.h>

using namespace std;
using namespace aidl::android::hardware::tv::tuner;
using namespace android::media::tuner::testing::configuration::V1_0;

const int32_t FMQ_SIZE_4M = 0x400000;
const int32_t FMQ_SIZE_16M = 0x1000000;

const string configFilePath = "/vendor/etc/tuner_vts_config_1_1.xml";

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
    frontendMap[defaultFeId].settings.set<FrontendSettings::Tag::dvbt>(dvbtSettings);

    vector<FrontendStatusType> types;
    types.push_back(FrontendStatusType::UEC);
    types.push_back(FrontendStatusType::IS_MISO);

    vector<FrontendStatus> statuses;
    FrontendStatus status;
    status.set<FrontendStatus::Tag::uec>(4);
    statuses.push_back(status);
    status.set<FrontendStatus::Tag::isMiso>(true);
    statuses.push_back(status);

    frontendMap[defaultFeId].tuneStatusTypes = types;
    frontendMap[defaultFeId].expectTuneStatuses = statuses;
    frontendMap[defaultFeId].isSoftwareFe = true;
    frontendMap[defaultFeId].canConnectToCiCam = true;
    frontendMap[defaultFeId].ciCamId = 0;
    FrontendDvbtSettings dvbt;
    dvbt.transmissionMode = FrontendDvbtTransmissionMode::MODE_8K_E;
    frontendMap[defaultFeId].settings.set<FrontendSettings::Tag::dvbt>(dvbt);
    // Read customized config
    TunerTestingConfigAidlReader1_0::readFrontendConfig1_0(frontendMap);
};

inline void initFilterConfig() {
    // The test will use the internal default filter when default filter is connected to any
    // data flow without overriding in the xml config.
    string defaultAudioFilterId = "FILTER_AUDIO_DEFAULT";
    string defaultVideoFilterId = "FILTER_VIDEO_DEFAULT";

    filterMap[defaultVideoFilterId].type.mainType = DemuxFilterMainType::TS;
    filterMap[defaultVideoFilterId].type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
            DemuxTsFilterType::VIDEO);
    filterMap[defaultVideoFilterId].bufferSize = FMQ_SIZE_16M;
    filterMap[defaultVideoFilterId].settings =
            DemuxFilterSettings::make<DemuxFilterSettings::Tag::ts>();
    filterMap[defaultVideoFilterId].settings.get<DemuxFilterSettings::Tag::ts>().tpid = 256;
    DemuxFilterAvSettings video;
    video.isPassthrough = false;
    filterMap[defaultVideoFilterId]
            .settings.get<DemuxFilterSettings::Tag::ts>()
            .filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::av>(video);
    filterMap[defaultVideoFilterId].monitorEventTypes =
            static_cast<int32_t>(DemuxFilterMonitorEventType::SCRAMBLING_STATUS) |
            static_cast<int32_t>(DemuxFilterMonitorEventType::IP_CID_CHANGE);
    filterMap[defaultVideoFilterId].streamType.set<AvStreamType::Tag::video>(
            VideoStreamType::MPEG1);

    filterMap[defaultAudioFilterId].type.mainType = DemuxFilterMainType::TS;
    filterMap[defaultAudioFilterId].type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
            DemuxTsFilterType::AUDIO);
    filterMap[defaultAudioFilterId].bufferSize = FMQ_SIZE_16M;
    filterMap[defaultAudioFilterId].settings =
            DemuxFilterSettings::make<DemuxFilterSettings::Tag::ts>();
    filterMap[defaultAudioFilterId].settings.get<DemuxFilterSettings::Tag::ts>().tpid = 256;
    DemuxFilterAvSettings audio;
    audio.isPassthrough = false;
    filterMap[defaultAudioFilterId]
            .settings.get<DemuxFilterSettings::Tag::ts>()
            .filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::av>(audio);
    filterMap[defaultAudioFilterId].monitorEventTypes =
            static_cast<int32_t>(DemuxFilterMonitorEventType::SCRAMBLING_STATUS) |
            static_cast<int32_t>(DemuxFilterMonitorEventType::IP_CID_CHANGE);
    filterMap[defaultAudioFilterId].streamType.set<AvStreamType::Tag::audio>(AudioStreamType::MP3);
    // Read customized config
    TunerTestingConfigAidlReader1_0::readFilterConfig1_0(filterMap);
};

/** Config all the dvrs that would be used in the tests */
inline void initDvrConfig() {
    // Read customized config
    TunerTestingConfigAidlReader1_0::readDvrConfig1_0(dvrMap);
};

/** Read the vendor configurations of which hardware to use for each test cases/data flows */
inline void connectHardwaresToTestCases() {
    TunerTestingConfigAidlReader1_0::connectLiveBroadcast(live);
    TunerTestingConfigAidlReader1_0::connectScan(scan);
    TunerTestingConfigAidlReader1_0::connectDvrRecord(record);
};

inline bool validateConnections() {
    if (record.support && !record.hasFrontendConnection &&
        record.dvrSourceId.compare(emptyHardwareId) == 0) {
        ALOGW("[vts config] Record must support either a DVR source or a Frontend source.");
        return false;
    }
    bool feIsValid = frontendMap.find(live.frontendId) != frontendMap.end() &&
                     frontendMap.find(scan.frontendId) != frontendMap.end();
    feIsValid &= record.support ? frontendMap.find(record.frontendId) != frontendMap.end() : true;

    if (!feIsValid) {
        ALOGW("[vts config] dynamic config fe connection is invalid.");
        return false;
    }

    bool dvrIsValid = frontendMap[live.frontendId].isSoftwareFe
                              ? dvrMap.find(live.dvrSoftwareFeId) != dvrMap.end()
                              : true;

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

    if (!dvrIsValid) {
        ALOGW("[vts config] dynamic config dvr connection is invalid.");
        return false;
    }

    bool filterIsValid = filterMap.find(live.audioFilterId) != filterMap.end() &&
                         filterMap.find(live.videoFilterId) != filterMap.end();
    filterIsValid &=
            record.support ? filterMap.find(record.recordFilterId) != filterMap.end() : true;

    if (!filterIsValid) {
        ALOGW("[vts config] dynamic config filter connection is invalid.");
        return false;
    }

    return true;
}
