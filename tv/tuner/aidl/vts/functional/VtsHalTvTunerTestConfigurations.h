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

const string configFilePath = "/vendor/etc/tuner_vts_config_aidl_V1.xml";

#define FILTER_MAIN_TYPE_BIT_COUNT 5
#define STATUS_CHECK_INTERVAL_MS 100L

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
static LnbDescramblingHardwareConnections lnbDescrambling;

/*
 * This function takes in a 2d vector of device Id's
 * The n vectors correlate to the ids for n different devices (eg frontends, filters)
 * The resultant 2d vector is every combination of id's with 1 id from each vector
 */
inline vector<vector<string>> generateIdCombinations(vector<vector<string>>& ids) {
    vector<vector<string>> combinations;

    // The index of each vector in ids that will be used in the next combination
    // EG {0, 2} means combo {ids[0][0] ids[1][2]} will be next
    const int size = static_cast<int>(ids.size());
    vector<int> indexes_used_in_combination(size, 0);

    // The vector number from ids whose elements we will cycle through to make combinations.
    // First, start at the right most vector
    int cycled_vector = size - 1;

    while (cycled_vector >= 0) {
        // Make a combination (one at a time)
        vector<string> combo;
        for (size_t i = 0; i < indexes_used_in_combination.size(); ++i) {
            const int combo_index = indexes_used_in_combination[i];
            combo.push_back(ids[i][combo_index]);
        }
        combinations.push_back(combo);

        // Find the right most vector that still has space [elements left] to cycle through and
        // create a combination
        while (cycled_vector >= 0 &&
               indexes_used_in_combination[cycled_vector] == ids[cycled_vector].size() - 1) {
            cycled_vector--;
        }

        // Use this check to avoid segmentation faults
        if (cycled_vector >= 0) {
            // Once found, we have a vector we can cycle through, so increase to its next element
            indexes_used_in_combination[cycled_vector]++;

            // Reset the other vectors to the right to their first element so we can cycle through
            // them again with the new element from cycled vector
            for (size_t i = cycled_vector + 1; i < indexes_used_in_combination.size(); ++i) {
                indexes_used_in_combination[i] = 0;
            }

            // all the vectors to the right were reset, so we can cycle through them again
            // Start at the furthest right vector
            cycled_vector = size - 1;
        }
    }

    return combinations;
}

/*
 * index 0 - playback dvr
 * index 1 - audio filters
 * index 2 - optional section filters
 */
static inline vector<DvrPlaybackHardwareConnections> generatePlaybackCombinations() {
    vector<DvrPlaybackHardwareConnections> combinations;
    vector<string> sectionFilterIds_optional = sectionFilterIds;
    sectionFilterIds_optional.push_back(emptyHardwareId);
    vector<vector<string>> deviceIds{playbackDvrIds, audioFilterIds, sectionFilterIds_optional};

    const int dvrIndex = 0;
    const int audioFilterIndex = 1;
    const int sectionFilterIndex = 2;

    auto idCombinations = generateIdCombinations(deviceIds);
    for (auto& combo : idCombinations) {
        DvrPlaybackHardwareConnections mPlayback;
        mPlayback.dvrId = combo[dvrIndex];
        mPlayback.audioFilterId = combo[audioFilterIndex];
        mPlayback.sectionFilterId = combo[sectionFilterIndex];
        const int videoFilterIndex =
                find(audioFilterIds.begin(), audioFilterIds.end(), mPlayback.audioFilterId) -
                audioFilterIds.begin();
        mPlayback.videoFilterId = videoFilterIds[videoFilterIndex];
        combinations.push_back(mPlayback);
    }

    return combinations;
}

static inline vector<DvrPlaybackHardwareConnections> generatePlaybackConfigs() {
    vector<DvrPlaybackHardwareConnections> playback_configs;
    if (configuredPlayback) {
        ALOGD("Using DVR playback configuration provided.");
        playback_configs = {playback};
    } else {
        ALOGD("Dvr playback not provided. Generating possible combinations. Consider adding it to "
              "the configuration file.");
        playback_configs = generatePlaybackCombinations();
    }

    return playback_configs;
}

/*
 * index 0 - frontends
 * index 1 - audio filters
 * index 2 - lnbs
 */
static inline vector<LnbLiveHardwareConnections> generateLnbLiveCombinations() {
    vector<LnbLiveHardwareConnections> combinations;
    vector<vector<string>> deviceIds{frontendIds, audioFilterIds, lnbIds};

    const int frontendIndex = 0;
    const int audioFilterIndex = 1;
    const int lnbIndex = 2;

    // TODO: Find a better way to vary diseqcMsgs, if at all
    auto idCombinations = generateIdCombinations(deviceIds);
    for (auto& combo : idCombinations) {
        const string feId = combo[frontendIndex];
        auto type = frontendMap[feId].type;
        if (type == FrontendType::DVBS || type == FrontendType::ISDBS ||
            type == FrontendType::ISDBS3) {
            LnbLiveHardwareConnections mLnbLive;
            mLnbLive.frontendId = feId;
            mLnbLive.audioFilterId = combo[audioFilterIndex];
            const int videoFilterIndex =
                    find(audioFilterIds.begin(), audioFilterIds.end(), mLnbLive.audioFilterId) -
                    audioFilterIds.begin();
            mLnbLive.videoFilterId = videoFilterIds[videoFilterIndex];
            mLnbLive.lnbId = combo[lnbIndex];
            mLnbLive.diseqcMsgs = diseqcMsgs;
            combinations.push_back(mLnbLive);
        }
    }

    return combinations;
}

static inline vector<LnbLiveHardwareConnections> generateLnbLiveConfigurations() {
    vector<LnbLiveHardwareConnections> lnbLive_configs;
    if (configuredLnbLive) {
        ALOGD("Using LnbLive configuration provided.");
        lnbLive_configs = {lnbLive};
    } else {
        ALOGD("LnbLive not provided. Generating possible combinations. Consider adding it to the "
              "configuration file.");
        lnbLive_configs = generateLnbLiveCombinations();
    }

    return lnbLive_configs;
}

static inline vector<ScanHardwareConnections> generateScanCombinations() {
    vector<ScanHardwareConnections> combinations;

    for (auto& id : frontendIds) {
        ScanHardwareConnections mScan;
        mScan.frontendId = id;
        combinations.push_back(mScan);
    }

    return combinations;
}

static inline vector<ScanHardwareConnections> generateScanConfigurations() {
    vector<ScanHardwareConnections> scan_configs;
    if (configuredScan) {
        ALOGD("Using scan configuration provided.");
        scan_configs = {scan};
    } else {
        ALOGD("Scan not provided. Generating possible combinations. Consider adding it to "
              "the configuration file.");
        scan_configs = generateScanCombinations();
    }

    return scan_configs;
}

/*
 * index 0 - frontends
 * index 1 - record filter
 * index 2 - Record Dvr
 * index 3 - Lnb
 */
static inline vector<LnbRecordHardwareConnections> generateLnbRecordCombinations() {
    vector<LnbRecordHardwareConnections> combinations;
    vector<vector<string>> deviceIds{frontendIds, recordFilterIds, recordDvrIds, lnbIds};

    const int frontendIndex = 0;
    const int recordFilterIndex = 1;
    const int dvrIndex = 2;
    const int lnbIndex = 3;

    auto idCombinations = generateIdCombinations(deviceIds);
    // TODO : Find a better way to vary diseqcMsgs, if at all
    for (auto& combo : idCombinations) {
        const string feId = combo[frontendIndex];
        auto type = frontendMap[feId].type;
        if (type == FrontendType::DVBS || type == FrontendType::ISDBS ||
            type == FrontendType::ISDBS3) {
            LnbRecordHardwareConnections mLnbRecord;
            mLnbRecord.frontendId = feId;
            mLnbRecord.recordFilterId = combo[recordFilterIndex];
            mLnbRecord.dvrRecordId = combo[dvrIndex];
            mLnbRecord.lnbId = combo[lnbIndex];
            mLnbRecord.diseqcMsgs = diseqcMsgs;
            combinations.push_back(mLnbRecord);
        }
    }

    return combinations;
}

static inline vector<LnbRecordHardwareConnections> generateLnbRecordConfigurations() {
    vector<LnbRecordHardwareConnections> lnbRecord_configs;
    if (configuredLnbRecord) {
        ALOGD("Using LnbRecord configuration provided.");
        lnbRecord_configs = {lnbRecord};
    } else {
        ALOGD("LnbRecord not provided. Generating possible combinations. Consider adding it to "
              "the configuration file.");
        lnbRecord_configs = generateLnbRecordCombinations();
    }

    return lnbRecord_configs;
}

/*
 * index 0 - decramblers
 * index 1 - frontends
 * index 2 - audio filters
 * index 3 - Dvr SW Fe Connections
 * index 4 - DVR Source Connections
 */
static inline vector<DescramblingHardwareConnections> generateDescramblingCombinations() {
    vector<DescramblingHardwareConnections> combinations;
    vector<string> mfrontendIds = frontendIds;
    vector<string> mDvrFeConnectionIds = playbackDvrIds;
    vector<string> mDvrSourceConnectionIds = playbackDvrIds;

    // Add the empty hardware id to each vector to include combinations where these 3 fields might
    // be optional
    mfrontendIds.push_back(emptyHardwareId);
    mDvrFeConnectionIds.push_back(emptyHardwareId);
    mDvrSourceConnectionIds.push_back(emptyHardwareId);

    const int descramblerIndex = 0;
    const int frontendIndex = 1;
    const int audioFilterIndex = 2;
    const int dvrFeIdIndex = 3;
    const int dvrSourceIdIndex = 4;

    vector<vector<string>> deviceIds{descramblerIds, mfrontendIds, audioFilterIds,
                                     mDvrFeConnectionIds, mDvrSourceConnectionIds};
    auto idCombinations = generateIdCombinations(deviceIds);
    for (auto& combo : idCombinations) {
        DescramblingHardwareConnections mDescrambling;
        const string feId = combo[frontendIndex];
        const string dvrSwFeId = combo[dvrFeIdIndex];
        const string dvrSourceId = combo[dvrSourceIdIndex];
        mDescrambling.hasFrontendConnection = feId.compare(emptyHardwareId) == 0 ? false : true;
        if (!mDescrambling.hasFrontendConnection) {
            if (dvrSourceId.compare(emptyHardwareId) == 0) {
                // If combination does not have a frontend or dvr source connection, do not include
                // it
                continue;
            }
        } else {
            if (frontendMap[feId].isSoftwareFe && dvrSwFeId.compare(emptyHardwareId) == 0) {
                // If combination has a software frontend and no dvr->software frontend connection,
                // do not include it
                continue;
            }
        }
        if (dvrSwFeId.compare(dvrSourceId) == 0) {
            // If dvr->software frontend connection is the same as dvr source input to tuner, do not
            // include it.
            continue;
        }
        mDescrambling.frontendId = feId;
        mDescrambling.audioFilterId = combo[audioFilterIndex];
        const int videoFilterIndex =
                find(audioFilterIds.begin(), audioFilterIds.end(), mDescrambling.audioFilterId) -
                audioFilterIds.begin();
        mDescrambling.videoFilterId = videoFilterIds[videoFilterIndex];
        mDescrambling.dvrSoftwareFeId = dvrSwFeId;
        mDescrambling.dvrSourceId = dvrSourceId;
        mDescrambling.descramblerId = combo[descramblerIndex];
        combinations.push_back(mDescrambling);
    }

    return combinations;
}

static inline vector<DescramblingHardwareConnections> generateDescramblingConfigurations() {
    vector<DescramblingHardwareConnections> descrambling_configs;
    if (configuredDescrambling) {
        ALOGD("Using Descrambling configuration provided.");
        descrambling_configs = {descrambling};
    } else {
        ALOGD("Descrambling not provided. Generating possible combinations. Consider adding it to "
              "the "
              "configuration file.");
        descrambling_configs = generateDescramblingCombinations();
    }

    return descrambling_configs;
}

static inline vector<TimeFilterHardwareConnections> generateTimeFilterCombinations() {
    vector<TimeFilterHardwareConnections> combinations;

    for (auto& id : timeFilterIds) {
        TimeFilterHardwareConnections mTimeFilter;
        mTimeFilter.timeFilterId = id;
        combinations.push_back(mTimeFilter);
    }

    return combinations;
}

static inline vector<TimeFilterHardwareConnections> generateTimeFilterConfigurations() {
    vector<TimeFilterHardwareConnections> timeFilter_configs;
    if (configuredTimeFilter) {
        ALOGD("Using TimeFilter configuration provided.");
        timeFilter_configs = {timeFilter};
    } else {
        ALOGD("TimeFilter not provided. Generating possible combinations. Consider adding it to "
              "the "
              "configuration file.");
        timeFilter_configs = generateTimeFilterCombinations();
    }

    return timeFilter_configs;
}

/*
 * index 0 - frontends
 * index 1 - record dvrs
 * index 2 - record filters
 */
static inline vector<DvrRecordHardwareConnections> generateRecordCombinations() {
    vector<DvrRecordHardwareConnections> combinations;

    const int frontendIdIndex = 0;
    const int recordDvrIndex = 1;
    const int recordFilterIndex = 2;

    vector<vector<string>> deviceIds{frontendIds, recordDvrIds, recordFilterIds};

    auto idCombinations = generateIdCombinations(deviceIds);
    for (auto& combo : idCombinations) {
        DvrRecordHardwareConnections mRecord;
        const string feId = combo[frontendIdIndex];
        mRecord.hasFrontendConnection = true;
        if (frontendMap[feId].isSoftwareFe) {
            // If we have a software frontend, do not include configuration for testing.
            continue;
        }
        mRecord.frontendId = feId;
        mRecord.support = true;
        mRecord.dvrSourceId = emptyHardwareId;
        mRecord.dvrSoftwareFeId = emptyHardwareId;
        mRecord.recordFilterId = combo[recordFilterIndex];
        mRecord.dvrRecordId = combo[recordDvrIndex];
        combinations.push_back(mRecord);
    }

    return combinations;
}

static inline vector<DvrRecordHardwareConnections> generateRecordConfigurations() {
    vector<DvrRecordHardwareConnections> record_configs;
    if (configuredRecord) {
        ALOGD("Using Record configuration provided.");
        record_configs = {record};
    } else {
        ALOGD("Record not provided. Generating possible combinations. Consider adding it to "
              "the "
              "configuration file.");
        record_configs = generateRecordCombinations();
    }

    return record_configs;
}

/*
 * index 0 - frontends
 * index 1 - audio filters
 * index 2 - playback dvrs
 * index 3 - section Filters
 */
static inline vector<LiveBroadcastHardwareConnections> generateLiveCombinations() {
    vector<LiveBroadcastHardwareConnections> combinations;
    vector<string> mSectionFilterIds = sectionFilterIds;
    vector<string> mDvrSwConnectionIds = playbackDvrIds;

    // Adding the empty hardware id to cover cases where fields are optional
    mSectionFilterIds.push_back(emptyHardwareId);
    mDvrSwConnectionIds.push_back(emptyHardwareId);

    const int frontendIdIndex = 0;
    const int audioFilterIdIndex = 1;
    const int dvrSwConnectionIdIndex = 2;
    const int sectionFilterIdIndex = 3;

    vector<vector<string>> deviceIds{frontendIds, audioFilterIds, mDvrSwConnectionIds,
                                     mSectionFilterIds};

    auto idCombinations = generateIdCombinations(deviceIds);
    for (auto& combo : idCombinations) {
        LiveBroadcastHardwareConnections mLive;
        const string feId = combo[frontendIdIndex];
        const string dvrSwConnectionId = combo[dvrSwConnectionIdIndex];
        mLive.hasFrontendConnection = true;

        if (frontendMap[feId].isSoftwareFe && dvrSwConnectionId.compare(emptyHardwareId) == 0) {
            // If the frontend is a software frontend and there is no dvr playback connected, do not
            // include configuration
            continue;
        }
        mLive.frontendId = feId;
        mLive.dvrSoftwareFeId = dvrSwConnectionId;
        mLive.audioFilterId = combo[audioFilterIdIndex];
        const int videoFilterIdIndex =
                find(audioFilterIds.begin(), audioFilterIds.end(), mLive.audioFilterId) -
                audioFilterIds.begin();
        mLive.videoFilterId = videoFilterIds[videoFilterIdIndex];
        mLive.sectionFilterId = combo[sectionFilterIdIndex];

        if (pcrFilterIds.empty()) {
            // If pcr Filters have not been provided, set it to empty hardware id
            mLive.pcrFilterId = emptyHardwareId;
        } else {
            // If pcr Filters have been provided, use the first index if there is only 1, or choose
            // the filter that corresponds to the correct audio and video filter pair
            const int pcrFilterIdIndex = pcrFilterIds.size() == 1 ? 0 : videoFilterIdIndex;
            mLive.pcrFilterId = pcrFilterIds[pcrFilterIdIndex];
        }

        combinations.push_back(mLive);
    }

    return combinations;
}

static inline vector<LiveBroadcastHardwareConnections> generateLiveConfigurations() {
    vector<LiveBroadcastHardwareConnections> live_configs;
    if (configuredLive) {
        ALOGD("Using Live configuration provided.");
        live_configs = {live};
    } else {
        ALOGD("Live not provided. Generating possible combinations. Consider adding it to "
              "the "
              "configuration file.");
        live_configs = generateLiveCombinations();
    }

    return live_configs;
}

static inline vector<LnbDescramblingHardwareConnections> generateLnbDescramblingCombinations() {
    vector<LnbDescramblingHardwareConnections> combinations;
    vector<vector<string>> deviceIds{frontendIds, audioFilterIds, lnbIds, descramblerIds};

    const int frontendIdIndex = 0;
    const int audioFilterIdIndex = 1;
    const int lnbIdIndex = 2;
    const int descramblerIdIndex = 3;

    auto idCombinations = generateIdCombinations(deviceIds);
    // TODO : Find a better way to vary diseqcMsgs, if at all
    for (auto& combo : idCombinations) {
        const string feId = combo[frontendIdIndex];
        auto type = frontendMap[feId].type;
        if (type == FrontendType::DVBS || type == FrontendType::ISDBS ||
            type == FrontendType::ISDBS3) {
            LnbDescramblingHardwareConnections mLnbDescrambling;
            mLnbDescrambling.support = true;
            mLnbDescrambling.frontendId = feId;
            mLnbDescrambling.audioFilterId = combo[audioFilterIdIndex];
            const int videoFilterIdIndex = find(audioFilterIds.begin(), audioFilterIds.end(),
                                                mLnbDescrambling.audioFilterId) -
                                           audioFilterIds.begin();
            mLnbDescrambling.videoFilterId = videoFilterIds[videoFilterIdIndex];
            mLnbDescrambling.lnbId = combo[lnbIdIndex];
            mLnbDescrambling.descramblerId = combo[descramblerIdIndex];
            mLnbDescrambling.diseqcMsgs = diseqcMsgs;
            combinations.push_back(mLnbDescrambling);
        }
    }

    return combinations;
}

static inline vector<LnbDescramblingHardwareConnections> generateLnbDescramblingConfigurations() {
    vector<LnbDescramblingHardwareConnections> lnbDescrambling_configs;
    if (configuredLnbDescrambling) {
        ALOGD("Using LnbDescrambling configuration provided");
        lnbDescrambling_configs = {lnbDescrambling};
    } else {
        ALOGD("LnbDescrambling not provided. Generating possible combinations. Consider adding it "
              "to the configuration file.");
        lnbDescrambling_configs = generateLnbDescramblingCombinations();
    }

    return lnbDescrambling_configs;
}

/** Config all the frontends that would be used in the tests */
inline void initFrontendConfig() {
    // The test will use the internal default fe when default fe is connected to any data flow
    // without overriding in the xml config.
    string defaultFeId = "FE_DEFAULT";
    FrontendDvbtSettings dvbtSettings{
            .frequency = 578000000,
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

inline void initTimeFilterConfig() {
    // Read customized config
    TunerTestingConfigAidlReader1_0::readTimeFilterConfig1_0(timeFilterMap);
};

inline void initDescramblerConfig() {
    // Read customized config
    TunerTestingConfigAidlReader1_0::readDescramblerConfig1_0(descramblerMap);
}

inline void initLnbConfig() {
    // Read customized config
    TunerTestingConfigAidlReader1_0::readLnbConfig1_0(lnbMap);
};

inline void initDiseqcMsgsConfig() {
    // Read customized config
    TunerTestingConfigAidlReader1_0::readDiseqcMessages(diseqcMsgMap);
};

inline void determineScan() {
    if (!frontendMap.empty()) {
        scan.hasFrontendConnection = true;
        ALOGD("Can support scan");
    }
}

inline void determineTimeFilter() {
    if (!timeFilterMap.empty()) {
        timeFilter.support = true;
        ALOGD("Can support time filter");
    }
}

inline void determineDvrPlayback() {
    if (!playbackDvrIds.empty() && !audioFilterIds.empty() && !videoFilterIds.empty()) {
        playback.support = true;
        ALOGD("Can support dvr playback");
    }
}

inline void determineLnbLive() {
    if (!audioFilterIds.empty() && !videoFilterIds.empty() && !frontendMap.empty() &&
        !lnbMap.empty()) {
        lnbLive.support = true;
        ALOGD("Can support lnb live");
    }
}

inline void determineLnbRecord() {
    if (!frontendMap.empty() && !recordFilterIds.empty() && !recordDvrIds.empty() &&
        !lnbMap.empty()) {
        lnbRecord.support = true;
        ALOGD("Can support lnb record");
    }
}

inline void determineLive() {
    if (videoFilterIds.empty() || audioFilterIds.empty() || frontendMap.empty()) {
        return;
    }
    if (hasSwFe && !hasHwFe && dvrMap.empty()) {
        ALOGD("Cannot configure Live. Only software frontends and no dvr connections");
        return;
    }
    ALOGD("Can support live");
    live.hasFrontendConnection = true;
}

inline void determineDescrambling() {
    if (descramblerMap.empty() || audioFilterIds.empty() || videoFilterIds.empty()) {
        return;
    }
    if (frontendMap.empty() && playbackDvrIds.empty()) {
        ALOGD("Cannot configure descrambling. No frontends or playback dvr's");
        return;
    }
    if (hasSwFe && !hasHwFe && playbackDvrIds.empty()) {
        ALOGD("cannot configure descrambling. Only SW frontends and no playback dvr's");
        return;
    }
    ALOGD("Can support descrambling");
    descrambling.support = true;
}

inline void determineDvrRecord() {
    if (recordDvrIds.empty() || recordFilterIds.empty()) {
        return;
    }
    if (frontendMap.empty() && playbackDvrIds.empty()) {
        ALOGD("Cannot support dvr record. No frontends and no playback dvr's");
        return;
    }
    if (hasSwFe && !hasHwFe && playbackDvrIds.empty()) {
        ALOGD("Cannot support dvr record. Only SW frontends and no playback dvr's");
        return;
    }
    ALOGD("Can support dvr record.");
    record.support = true;
}

inline void determineLnbDescrambling() {
    if (frontendIds.empty() || audioFilterIds.empty() || videoFilterIds.empty() || lnbIds.empty() ||
        descramblerIds.empty()) {
        return;
    }
    ALOGD("Can support LnbDescrambling.");
    lnbDescrambling.support = true;
}

/** Read the vendor configurations of which hardware to use for each test cases/data flows */
inline void connectHardwaresToTestCases() {
    TunerTestingConfigAidlReader1_0::connectLiveBroadcast(live);
    TunerTestingConfigAidlReader1_0::connectScan(scan);
    TunerTestingConfigAidlReader1_0::connectDvrRecord(record);
    TunerTestingConfigAidlReader1_0::connectTimeFilter(timeFilter);
    TunerTestingConfigAidlReader1_0::connectDescrambling(descrambling);
    TunerTestingConfigAidlReader1_0::connectLnbLive(lnbLive);
    TunerTestingConfigAidlReader1_0::connectLnbRecord(lnbRecord);
    TunerTestingConfigAidlReader1_0::connectDvrPlayback(playback);
    TunerTestingConfigAidlReader1_0::connectLnbDescrambling(lnbDescrambling);
};

inline void determineDataFlows() {
    determineScan();
    determineTimeFilter();
    determineDvrPlayback();
    determineLnbLive();
    determineLnbRecord();
    determineLive();
    determineDescrambling();
    determineDvrRecord();
    determineLnbDescrambling();
}

inline bool validateConnections() {
    if (record.support && !record.hasFrontendConnection &&
        record.dvrSourceId.compare(emptyHardwareId) == 0) {
        ALOGW("[vts config] Record must support either a DVR source or a Frontend source.");
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
    feIsValid &= descrambling.support && descrambling.hasFrontendConnection
                         ? frontendMap.find(descrambling.frontendId) != frontendMap.end()
                         : true;

    feIsValid &= lnbLive.support ? frontendMap.find(lnbLive.frontendId) != frontendMap.end() : true;

    feIsValid &=
            lnbRecord.support ? frontendMap.find(lnbRecord.frontendId) != frontendMap.end() : true;

    feIsValid &= lnbDescrambling.support
                         ? frontendMap.find(lnbDescrambling.frontendId) != frontendMap.end()
                         : true;

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

    if (descrambling.support) {
        if (descrambling.hasFrontendConnection) {
            if (frontendMap[descrambling.frontendId].isSoftwareFe) {
                dvrIsValid &= dvrMap.find(descrambling.dvrSoftwareFeId) != dvrMap.end();
            }
        } else {
            dvrIsValid &= dvrMap.find(descrambling.dvrSourceId) != dvrMap.end();
        }
    }

    dvrIsValid &= lnbRecord.support ? dvrMap.find(lnbRecord.dvrRecordId) != dvrMap.end() : true;

    dvrIsValid &= playback.support ? dvrMap.find(playback.dvrId) != dvrMap.end() : true;

    if (!dvrIsValid) {
        ALOGW("[vts config] dynamic config dvr connection is invalid.");
        return false;
    }

    bool filterIsValid = (live.hasFrontendConnection)
                             ? filterMap.find(live.audioFilterId) != filterMap.end() &&
                               filterMap.find(live.videoFilterId) != filterMap.end()
                             : true;
    filterIsValid &=
            record.support ? filterMap.find(record.recordFilterId) != filterMap.end() : true;

    filterIsValid &= descrambling.support
                             ? filterMap.find(descrambling.videoFilterId) != filterMap.end() &&
                                       filterMap.find(descrambling.audioFilterId) != filterMap.end()
                             : true;

    for (auto& filterId : descrambling.extraFilters) {
        filterIsValid &= filterMap.find(filterId) != filterMap.end();
    }

    filterIsValid &= lnbLive.support
                             ? filterMap.find(lnbLive.audioFilterId) != filterMap.end() &&
                                       filterMap.find(lnbLive.videoFilterId) != filterMap.end()
                             : true;

    filterIsValid &=
            lnbRecord.support ? filterMap.find(lnbRecord.recordFilterId) != filterMap.end() : true;

    for (auto& filterId : lnbRecord.extraFilters) {
        filterIsValid &= filterMap.find(filterId) != filterMap.end();
    }

    for (auto& filterId : lnbLive.extraFilters) {
        filterIsValid &= filterMap.find(filterId) != filterMap.end();
    }

    filterIsValid &= playback.support
                             ? filterMap.find(playback.audioFilterId) != filterMap.end() &&
                                       filterMap.find(playback.videoFilterId) != filterMap.end()
                             : true;
    filterIsValid &= playback.sectionFilterId.compare(emptyHardwareId) == 0
                             ? true
                             : filterMap.find(playback.sectionFilterId) != filterMap.end();

    for (auto& filterId : playback.extraFilters) {
        filterIsValid &=
                playback.hasExtraFilters ? filterMap.find(filterId) != filterMap.end() : true;
    }

    filterIsValid &=
            lnbDescrambling.support
                    ? filterMap.find(lnbDescrambling.audioFilterId) != filterMap.end() &&
                              filterMap.find(lnbDescrambling.videoFilterId) != filterMap.end()
                    : true;

    if (!filterIsValid) {
        ALOGW("[vts config] dynamic config filter connection is invalid.");
        return false;
    }

    if (audioFilterIds.size() != videoFilterIds.size()) {
        ALOGW("[vts config] the number of audio and video filters should be equal");
        return false;
    }

    if (!pcrFilterIds.empty() && pcrFilterIds.size() != 1 &&
        pcrFilterIds.size() != audioFilterIds.size()) {
        ALOGW("[vts config] When more than 1 pcr filter is configured, the number of pcr filters "
              "must equal the number of audio and video filters.");
        return false;
    }

    bool timeFilterIsValid =
            timeFilter.support ? timeFilterMap.find(timeFilter.timeFilterId) != timeFilterMap.end()
                               : true;

    if (!timeFilterIsValid) {
        ALOGW("[vts config] dynamic config time filter connection is invalid.");
    }

    bool descramblerIsValid =
            descrambling.support
                    ? descramblerMap.find(descrambling.descramblerId) != descramblerMap.end()
                    : true;

    descramblerIsValid &=
            lnbDescrambling.support
                    ? descramblerMap.find(lnbDescrambling.descramblerId) != descramblerMap.end()
                    : true;

    if (!descramblerIsValid) {
        ALOGW("[vts config] dynamic config descrambler connection is invalid.");
        return false;
    }

    bool lnbIsValid = lnbLive.support ? lnbMap.find(lnbLive.lnbId) != lnbMap.end() : true;

    lnbIsValid &= lnbRecord.support ? lnbMap.find(lnbRecord.lnbId) != lnbMap.end() : true;

    lnbIsValid &=
            lnbDescrambling.support ? lnbMap.find(lnbDescrambling.lnbId) != lnbMap.end() : true;

    if (!lnbIsValid) {
        ALOGW("[vts config] dynamic config lnb connection is invalid.");
        return false;
    }

    bool diseqcMsgsIsValid = true;

    for (auto& msg : lnbRecord.diseqcMsgs) {
        diseqcMsgsIsValid &= diseqcMsgMap.find(msg) != diseqcMsgMap.end();
    }

    for (auto& msg : lnbLive.diseqcMsgs) {
        diseqcMsgsIsValid &= diseqcMsgMap.find(msg) != diseqcMsgMap.end();
    }

    for (auto& msg : lnbDescrambling.diseqcMsgs) {
        diseqcMsgsIsValid &= diseqcMsgMap.find(msg) != diseqcMsgMap.end();
    }

    if (!diseqcMsgsIsValid) {
        ALOGW("[vts config] dynamic config diseqcMsg is invalid.");
        return false;
    }

    return true;
}
