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

#include "../../../config/TunerTestingConfigReaderV1_1.h"

using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DemuxAlpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxIpAddress;
using android::hardware::tv::tuner::V1_0::DemuxIpFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxIpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxMmtpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxRecordScIndexType;
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
using android::hardware::tv::tuner::V1_0::FrontendType;
using android::hardware::tv::tuner::V1_0::PlaybackSettings;
using android::hardware::tv::tuner::V1_0::RecordSettings;

using namespace std;
using namespace android::media::tuner::testing::configuration::V1_0;

const uint32_t FMQ_SIZE_4M = 0x400000;
const uint32_t FMQ_SIZE_16M = 0x1000000;

const string configFilePath = "/vendor/etc/tuner_vts_config_1_1.xml";

// Hardware configs
static map<string, FrontendConfig1_1> frontendMap;
static map<string, FilterConfig1_1> filterMap;
static map<string, DvrConfig> dvrMap;

// Hardware and test cases connections
static LiveBroadcastHardwareConnections live;
static ScanHardwareConnections scan;
static DvrRecordHardwareConnections record;

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
    frontendMap[defaultFeId].config1_0.type = FrontendType::DVBT;
    frontendMap[defaultFeId].config1_0.settings.dvbt(dvbtSettings);

    vector<FrontendStatusTypeExt1_1> types;
    types.push_back(FrontendStatusTypeExt1_1::UEC);
    types.push_back(FrontendStatusTypeExt1_1::IS_MISO);
    vector<FrontendStatusExt1_1> statuses;
    FrontendStatusExt1_1 status;
    status.uec(4);
    statuses.push_back(status);
    status.isMiso(true);
    statuses.push_back(status);
    frontendMap[defaultFeId].tuneStatusTypes = types;
    frontendMap[defaultFeId].expectTuneStatuses = statuses;
    frontendMap[defaultFeId].config1_0.isSoftwareFe = true;
    frontendMap[defaultFeId].canConnectToCiCam = true;
    frontendMap[defaultFeId].ciCamId = 0;
    frontendMap[defaultFeId].settingsExt1_1.settingExt.dvbt({
            .transmissionMode =
                    android::hardware::tv::tuner::V1_1::FrontendDvbtTransmissionMode::MODE_8K_E,
    });
    // Read customized config
    TunerTestingConfigReader1_1::readFrontendConfig1_1(frontendMap);
};

inline void initFilterConfig() {
    // The test will use the internal default filter when default filter is connected to any
    // data flow without overriding in the xml config.
    string defaultAudioFilterId = "FILTER_AUDIO_DEFAULT";
    string defaultVideoFilterId = "FILTER_VIDEO_DEFAULT";

    filterMap[defaultVideoFilterId].config1_0.type.mainType = DemuxFilterMainType::TS;
    filterMap[defaultVideoFilterId].config1_0.type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterMap[defaultVideoFilterId].config1_0.bufferSize = FMQ_SIZE_16M;
    filterMap[defaultVideoFilterId].config1_0.settings.ts().tpid = 256;
    filterMap[defaultVideoFilterId].config1_0.settings.ts().filterSettings.av(
            {.isPassthrough = false});
    filterMap[defaultVideoFilterId].monitorEventTypes =
            android::hardware::tv::tuner::V1_1::DemuxFilterMonitorEventType::SCRAMBLING_STATUS |
            android::hardware::tv::tuner::V1_1::DemuxFilterMonitorEventType::IP_CID_CHANGE;
    filterMap[defaultVideoFilterId].streamType.video(VideoStreamType::MPEG1);

    filterMap[defaultAudioFilterId].config1_0.type.mainType = DemuxFilterMainType::TS;
    filterMap[defaultAudioFilterId].config1_0.type.subType.tsFilterType(DemuxTsFilterType::AUDIO);
    filterMap[defaultAudioFilterId].config1_0.bufferSize = FMQ_SIZE_16M;
    filterMap[defaultAudioFilterId].config1_0.settings.ts().tpid = 256;
    filterMap[defaultAudioFilterId].config1_0.settings.ts().filterSettings.av(
            {.isPassthrough = false});
    filterMap[defaultAudioFilterId].monitorEventTypes =
            android::hardware::tv::tuner::V1_1::DemuxFilterMonitorEventType::SCRAMBLING_STATUS |
            android::hardware::tv::tuner::V1_1::DemuxFilterMonitorEventType::IP_CID_CHANGE;
    filterMap[defaultAudioFilterId].streamType.audio(AudioStreamType::MP3);
    // Read customized config
    TunerTestingConfigReader1_1::readFilterConfig1_1(filterMap);
};

/** Config all the dvrs that would be used in the tests */
inline void initDvrConfig() {
    // Read customized config
    TunerTestingConfigReader1_0::readDvrConfig1_0(dvrMap);
};

/** Read the vendor configurations of which hardware to use for each test cases/data flows */
inline void connectHardwaresToTestCases() {
    TunerTestingConfigReader1_0::connectLiveBroadcast(live);
    TunerTestingConfigReader1_0::connectScan(scan);
    TunerTestingConfigReader1_0::connectDvrRecord(record);
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

    bool dvrIsValid = frontendMap[live.frontendId].config1_0.isSoftwareFe
                              ? dvrMap.find(live.dvrSoftwareFeId) != dvrMap.end()
                              : true;

    if (record.support) {
        if (record.hasFrontendConnection) {
            if (frontendMap[record.frontendId].config1_0.isSoftwareFe) {
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
