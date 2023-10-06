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

#pragma once

#include <android-base/logging.h>
#include <android_media_tuner_testing_configuration_V1_0.h>
#include <android_media_tuner_testing_configuration_V1_0_enums.h>

#include <aidl/android/hardware/tv/tuner/DataFormat.h>
#include <aidl/android/hardware/tv/tuner/DemuxAlpFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterAvSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterEvent.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterMainType.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterRecordSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterSectionSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxIpAddress.h>
#include <aidl/android/hardware/tv/tuner/DemuxIpFilterSettings.h>
#include <aidl/android/hardware/tv/tuner/DemuxIpFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxMmtpFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxRecordScIndexType.h>
#include <aidl/android/hardware/tv/tuner/DemuxTlvFilterType.h>
#include <aidl/android/hardware/tv/tuner/DemuxTsFilterType.h>
#include <aidl/android/hardware/tv/tuner/DvrSettings.h>
#include <aidl/android/hardware/tv/tuner/DvrType.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbsSettings.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtBandwidth.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtCoderate.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtConstellation.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtGuardInterval.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtHierarchy.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtPlpMode.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtSettings.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtStandard.h>
#include <aidl/android/hardware/tv/tuner/FrontendDvbtTransmissionMode.h>
#include <aidl/android/hardware/tv/tuner/FrontendSettings.h>
#include <aidl/android/hardware/tv/tuner/FrontendStatus.h>
#include <aidl/android/hardware/tv/tuner/FrontendStatusType.h>
#include <aidl/android/hardware/tv/tuner/FrontendType.h>
#include <aidl/android/hardware/tv/tuner/LnbPosition.h>
#include <aidl/android/hardware/tv/tuner/LnbTone.h>
#include <aidl/android/hardware/tv/tuner/LnbVoltage.h>
#include <aidl/android/hardware/tv/tuner/PlaybackSettings.h>
#include <aidl/android/hardware/tv/tuner/RecordSettings.h>

using namespace std;
using namespace aidl::android::hardware::tv::tuner;
using namespace android::media::tuner::testing::configuration::V1_0;

static bool hasHwFe = false;
static bool hasSwFe = false;
static bool configFileRead = false;
static bool configuredLive = false;
static bool configuredScan = false;
static bool configuredRecord = false;
static bool configuredLnbLive = false;
static bool configuredPlayback = false;
static bool configuredLnbRecord = false;
static bool configuredTimeFilter = false;
static bool configuredDescrambling = false;
static bool configuredLnbDescrambling = false;

const string emptyHardwareId = "";

static string mConfigFilePath;

static vector<string> playbackDvrIds;
static vector<string> ipFilterIds;
static vector<string> recordDvrIds;
static vector<string> pcrFilterIds;
static vector<string> timeFilterIds;
static vector<string> audioFilterIds;
static vector<string> videoFilterIds;
static vector<string> recordFilterIds;
static vector<string> sectionFilterIds;
static vector<string> frontendIds;
static vector<string> lnbIds;
static vector<string> diseqcMsgs;
static vector<string> descramblerIds;

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

struct FrontendConfig {
    bool isSoftwareFe;
    FrontendType type;
    bool canConnectToCiCam;
    int32_t ciCamId;
    int32_t removePid;
    FrontendSettings settings;
    vector<FrontendStatusType> tuneStatusTypes;
    vector<FrontendStatus> expectTuneStatuses;
};

struct FilterConfig {
    int32_t bufferSize;
    DemuxFilterType type;
    DemuxFilterSettings settings;
    bool getMqDesc;
    AvStreamType streamType;
    int32_t ipCid;
    int32_t monitorEventTypes;
    int timeDelayInMs = 0;
    int dataDelayInBytes = 0;

    bool operator<(const FilterConfig& /*c*/) const { return false; }
};

struct DvrConfig {
    DvrType type;
    int32_t bufferSize;
    DvrSettings settings;
    string playbackInputFile;
};

struct LnbConfig {
    string name;
    LnbVoltage voltage;
    LnbTone tone;
    LnbPosition position;
};

struct TimeFilterConfig {
    int64_t timeStamp;
};

struct DescramblerConfig {
    int32_t casSystemId;
    string provisionStr;
    vector<uint8_t> hidlPvtData;
};

struct LiveBroadcastHardwareConnections {
    bool hasFrontendConnection;
    string frontendId;
    string dvrSoftwareFeId;
    string audioFilterId;
    string videoFilterId;
    string sectionFilterId;
    string ipFilterId;
    string pcrFilterId;
    /* list string of extra filters; */
    vector<string> extraFilters;
};

struct ScanHardwareConnections {
    bool hasFrontendConnection;
    string frontendId;
};

struct DvrPlaybackHardwareConnections {
    bool support;
    bool hasExtraFilters = false;
    string frontendId;
    string dvrId;
    string audioFilterId;
    string videoFilterId;
    string sectionFilterId;
    vector<string> extraFilters;
};

struct DvrRecordHardwareConnections {
    bool support;
    bool hasFrontendConnection;
    string frontendId;
    string dvrRecordId;
    string dvrSoftwareFeId;
    string recordFilterId;
    string dvrSourceId;
};

struct DescramblingHardwareConnections {
    bool support;
    bool hasFrontendConnection;
    string frontendId;
    string dvrSoftwareFeId;
    string audioFilterId;
    string videoFilterId;
    string descramblerId;
    string dvrSourceId;
    vector<string> extraFilters;
};

struct LnbLiveHardwareConnections {
    bool support;
    string frontendId;
    string audioFilterId;
    string videoFilterId;
    string lnbId;
    vector<string> diseqcMsgs;
    vector<string> extraFilters;
};

struct LnbRecordHardwareConnections {
    bool support;
    string frontendId;
    string dvrRecordId;
    string recordFilterId;
    string lnbId;
    vector<string> diseqcMsgs;
    vector<string> extraFilters;
};

struct TimeFilterHardwareConnections {
    bool support;
    string timeFilterId;
};

struct LnbDescramblingHardwareConnections {
    bool support;
    string frontendId;
    string audioFilterId;
    string videoFilterId;
    string lnbId;
    string descramblerId;
    vector<string> diseqcMsgs;
};

struct TunerTestingConfigAidlReader1_0 {
  public:
    static void setConfigFilePath(string path) { mConfigFilePath = path; }

    static bool checkConfigFileExists() {
        auto res = read(mConfigFilePath.c_str());
        if (res == nullopt) {
            ALOGW("[ConfigReader] Couldn't read %s."
                  "Please check tuner_testing_dynamic_configuration.xsd"
                  "and sample_tuner_vts_config.xml for more details on how to config Tune VTS.",
                  mConfigFilePath.c_str());
        }
        return (res != nullopt);
    }

    static TunerConfiguration getTunerConfig() { return *read(mConfigFilePath.c_str()); }

    static DataFlowConfiguration getDataFlowConfiguration() {
        return *getTunerConfig().getFirstDataFlowConfiguration();
    }

    static HardwareConfiguration getHardwareConfig() {
        return *getTunerConfig().getFirstHardwareConfiguration();
    }

    static void readFrontendConfig1_0(map<string, FrontendConfig>& frontendMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasFrontends()) {
            // TODO: b/182519645 complete the tune status config
            vector<FrontendStatusType> types;
            vector<FrontendStatus> statuses;

            types.push_back(FrontendStatusType::DEMOD_LOCK);
            types.push_back(FrontendStatusType::UEC);
            types.push_back(FrontendStatusType::IS_MISO);

            FrontendStatus status;
            status.set<FrontendStatus::Tag::isDemodLocked>(true);
            statuses.push_back(status);
            status.set<FrontendStatus::Tag::uec>(4);
            statuses.push_back(status);
            status.set<FrontendStatus::Tag::isMiso>(true);
            statuses.push_back(status);

            auto frontends = *hardwareConfig.getFirstFrontends();
            for (auto feConfig : frontends.getFrontend()) {
                string id = feConfig.getId();
                frontendIds.push_back(id);
                if (id.compare(string("FE_DEFAULT")) == 0) {
                    // overrid default
                    frontendMap.erase(string("FE_DEFAULT"));
                }
                FrontendType type;
                switch (feConfig.getType()) {
                    case FrontendTypeEnum::UNDEFINED:
                        type = FrontendType::UNDEFINED;
                        break;
                    // TODO: b/182519645 finish all other frontend settings
                    case FrontendTypeEnum::ANALOG:
                        type = FrontendType::ANALOG;
                        break;
                    case FrontendTypeEnum::ATSC:
                        type = FrontendType::ATSC;
                        frontendMap[id].settings.set<
                            FrontendSettings::Tag::atsc>(
                                readAtscFrontendSettings(feConfig));
                        break;
                    case FrontendTypeEnum::ATSC3:
                        type = FrontendType::ATSC3;
                        break;
                    case FrontendTypeEnum::DVBC:
                        type = FrontendType::DVBC;
                        break;
                    case FrontendTypeEnum::DVBS:
                        type = FrontendType::DVBS;
                        frontendMap[id].settings.set<FrontendSettings::Tag::dvbs>(
                                readDvbsFrontendSettings(feConfig));
                        break;
                    case FrontendTypeEnum::DVBT: {
                        type = FrontendType::DVBT;
                        frontendMap[id].settings.set<FrontendSettings::Tag::dvbt>(
                                readDvbtFrontendSettings(feConfig));
                        break;
                    }
                    case FrontendTypeEnum::ISDBS:
                        type = FrontendType::ISDBS;
                        frontendMap[id].settings.set<FrontendSettings::Tag::isdbs>(
                                readIsdbsFrontendSettings(feConfig));
                        break;
                    case FrontendTypeEnum::ISDBS3:
                        type = FrontendType::ISDBS3;
                        break;
                    case FrontendTypeEnum::ISDBT:
                        type = FrontendType::ISDBT;
                        frontendMap[id].settings.set<FrontendSettings::Tag::isdbt>(
                                readIsdbtFrontendSettings(feConfig));
                        break;
                    case FrontendTypeEnum::DTMB:
                        type = FrontendType::DTMB;
                        break;
                    case FrontendTypeEnum::UNKNOWN:
                        ALOGW("[ConfigReader] invalid frontend type");
                        return;
                    default:
                        ALOGW("[ConfigReader] fe already handled in 1_0 reader.");
                        break;
                }
                frontendMap[id].type = type;
                frontendMap[id].isSoftwareFe = feConfig.getIsSoftwareFrontend();
                if (frontendMap[id].isSoftwareFe) {
                    hasSwFe = true;
                } else {
                    hasHwFe = true;
                }
                // TODO: b/182519645 complete the tune status config
                frontendMap[id].tuneStatusTypes = types;
                frontendMap[id].expectTuneStatuses = statuses;
                getCiCamInfo(feConfig, frontendMap[id].canConnectToCiCam, frontendMap[id].ciCamId,
                             frontendMap[id].removePid);
            }
        }
    }

    static void readFilterConfig1_0(map<string, FilterConfig>& filterMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasFilters()) {
            auto filters = *hardwareConfig.getFirstFilters();
            for (auto filterConfig : filters.getFilter()) {
                string id = filterConfig.getId();
                if (id.compare(string("FILTER_AUDIO_DEFAULT")) == 0) {
                    // overrid default
                    filterMap.erase(string("FILTER_AUDIO_DEFAULT"));
                }
                if (id.compare(string("FILTER_VIDEO_DEFAULT")) == 0) {
                    // overrid default
                    filterMap.erase(string("FILTER_VIDEO_DEFAULT"));
                }

                DemuxFilterType type;
                DemuxFilterSettings settings;
                if (!readFilterTypeAndSettings(filterConfig, type, settings)) {
                    ALOGW("[ConfigReader] invalid filter type");
                    return;
                }
                filterMap[id].type = type;
                filterMap[id].bufferSize = filterConfig.getBufferSize();
                filterMap[id].getMqDesc = filterConfig.getUseFMQ();
                filterMap[id].settings = settings;

                if (filterConfig.hasMonitorEventTypes()) {
                    filterMap[id].monitorEventTypes = (int32_t)filterConfig.getMonitorEventTypes();
                }
                if (filterConfig.hasTimeDelayInMs()) {
                    filterMap[id].timeDelayInMs = filterConfig.getTimeDelayInMs();
                }
                if (filterConfig.hasDataDelayInBytes()) {
                    filterMap[id].dataDelayInBytes = filterConfig.getDataDelayInBytes();
                }
                if (filterConfig.hasAvFilterSettings_optional()) {
                    auto av = filterConfig.getFirstAvFilterSettings_optional();
                    if (av->hasAudioStreamType_optional()) {
                        filterMap[id].streamType.set<AvStreamType::Tag::audio>(
                                static_cast<AudioStreamType>(av->getAudioStreamType_optional()));
                    }
                    if (av->hasVideoStreamType_optional()) {
                        filterMap[id].streamType.set<AvStreamType::Tag::video>(
                                static_cast<VideoStreamType>(av->getVideoStreamType_optional()));
                    }
                }
                if (filterConfig.hasIpFilterConfig_optional()) {
                    auto ip = filterConfig.getFirstIpFilterConfig_optional();
                    if (ip->hasIpCid()) {
                        filterMap[id].ipCid = ip->getIpCid();
                    }
                }
            }
        }
    }

    static void readDvrConfig1_0(map<string, DvrConfig>& dvrMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasDvrs()) {
            auto dvrs = *hardwareConfig.getFirstDvrs();
            for (auto dvrConfig : dvrs.getDvr()) {
                string id = dvrConfig.getId();
                DvrType type;
                switch (dvrConfig.getType()) {
                    case DvrTypeEnum::PLAYBACK:
                        playbackDvrIds.push_back(id);
                        type = DvrType::PLAYBACK;
                        dvrMap[id].settings.set<DvrSettings::Tag::playback>(
                                readPlaybackSettings(dvrConfig));
                        break;
                    case DvrTypeEnum::RECORD:
                        recordDvrIds.push_back(id);
                        type = DvrType::RECORD;
                        dvrMap[id].settings.set<DvrSettings::Tag::record>(
                                readRecordSettings(dvrConfig));
                        break;
                    case DvrTypeEnum::UNKNOWN:
                        ALOGW("[ConfigReader] invalid DVR type");
                        return;
                }
                dvrMap[id].type = type;
                dvrMap[id].bufferSize = static_cast<int32_t>(dvrConfig.getBufferSize());
                if (dvrConfig.hasInputFilePath()) {
                    dvrMap[id].playbackInputFile = dvrConfig.getInputFilePath();
                }
            }
        }
    }

    static void readLnbConfig1_0(map<string, LnbConfig>& lnbMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasLnbs()) {
            auto lnbs = *hardwareConfig.getFirstLnbs();
            for (auto lnbConfig : lnbs.getLnb()) {
                string id = lnbConfig.getId();
                lnbIds.push_back(id);
                if (lnbConfig.hasName()) {
                    lnbMap[id].name = lnbConfig.getName();
                } else {
                    lnbMap[id].name = emptyHardwareId;
                }
                lnbMap[id].voltage = static_cast<LnbVoltage>(lnbConfig.getVoltage());
                lnbMap[id].tone = static_cast<LnbTone>(lnbConfig.getTone());
                lnbMap[id].position = static_cast<LnbPosition>(lnbConfig.getPosition());
            }
        }
    }

    static void readDescramblerConfig1_0(map<string, DescramblerConfig>& descramblerMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasDescramblers()) {
            auto descramblers = *hardwareConfig.getFirstDescramblers();
            for (auto descramblerConfig : descramblers.getDescrambler()) {
                string id = descramblerConfig.getId();
                descramblerIds.push_back(id);
                descramblerMap[id].casSystemId =
                        static_cast<int32_t>(descramblerConfig.getCasSystemId());
                if (descramblerConfig.hasProvisionStr()) {
                    descramblerMap[id].provisionStr = descramblerConfig.getProvisionStr();
                } else {
                    descramblerMap[id].provisionStr = PROVISION_STR;
                }
                if (descramblerConfig.hasSesstionPrivatData()) {
                    auto privateData = descramblerConfig.getSesstionPrivatData();
                    int size = privateData.size();
                    descramblerMap[id].hidlPvtData.resize(size);
                    memcpy(descramblerMap[id].hidlPvtData.data(), privateData.data(), size);
                } else {
                    descramblerMap[id].hidlPvtData.resize(256);
                }
            }
        }
    }

    static void readDiseqcMessages(map<string, vector<uint8_t>>& diseqcMsgMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasDiseqcMessages()) {
            auto msgs = *hardwareConfig.getFirstDiseqcMessages();
            for (auto msgConfig : msgs.getDiseqcMessage()) {
                string name = msgConfig.getMsgName();
                diseqcMsgs.push_back(name);
                for (uint8_t atom : msgConfig.getMsgBody()) {
                    diseqcMsgMap[name].push_back(atom);
                }
            }
        }
    }

    static void readTimeFilterConfig1_0(map<string, TimeFilterConfig>& timeFilterMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasTimeFilters()) {
            auto timeFilters = *hardwareConfig.getFirstTimeFilters();
            for (auto timeFilterConfig : timeFilters.getTimeFilter()) {
                string id = timeFilterConfig.getId();
                timeFilterIds.push_back(id);
                timeFilterMap[id].timeStamp = static_cast<int64_t>(timeFilterConfig.getTimeStamp());
            }
        }
    }

    static void connectLiveBroadcast(LiveBroadcastHardwareConnections& live) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasClearLiveBroadcast()) {
            live.hasFrontendConnection = true;
            configuredLive = true;
        } else {
            live.hasFrontendConnection = false;
            return;
        }
        auto liveConfig = *dataFlow.getFirstClearLiveBroadcast();
        live.frontendId = liveConfig.getFrontendConnection();

        live.audioFilterId = liveConfig.getAudioFilterConnection();
        live.videoFilterId = liveConfig.getVideoFilterConnection();
        if (liveConfig.hasPcrFilterConnection()) {
            live.pcrFilterId = liveConfig.getPcrFilterConnection();
        } else {
            live.pcrFilterId = emptyHardwareId;
        }
        if (liveConfig.hasSectionFilterConnection()) {
            live.sectionFilterId = liveConfig.getSectionFilterConnection();
        } else {
            live.sectionFilterId = emptyHardwareId;
        }
        if (liveConfig.hasDvrSoftwareFeConnection()) {
            live.dvrSoftwareFeId = liveConfig.getDvrSoftwareFeConnection();
        }
        if (liveConfig.hasIpFilterConnection()) {
            live.ipFilterId = liveConfig.getIpFilterConnection();
        } else {
            live.ipFilterId = emptyHardwareId;
        }
        if (liveConfig.hasOptionalFilters()) {
            auto optionalFilters = liveConfig.getOptionalFilters();
            live.extraFilters = optionalFilters;
        }
    }

    static void connectScan(ScanHardwareConnections& scan) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasScan()) {
            scan.hasFrontendConnection = true;
            configuredScan = true;
        } else {
            scan.hasFrontendConnection = false;
            return;
        }
        auto scanConfig = *dataFlow.getFirstScan();
        scan.frontendId = scanConfig.getFrontendConnection();
    }

    static void connectDvrPlayback(DvrPlaybackHardwareConnections& playback) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasDvrPlayback()) {
            playback.support = true;
            configuredPlayback = true;
        } else {
            playback.support = false;
            return;
        }
        auto playbackConfig = *dataFlow.getFirstDvrPlayback();
        playback.dvrId = playbackConfig.getDvrConnection();
        playback.audioFilterId = playbackConfig.getAudioFilterConnection();
        playback.videoFilterId = playbackConfig.getVideoFilterConnection();
        if (playbackConfig.hasSectionFilterConnection()) {
            playback.sectionFilterId = playbackConfig.getSectionFilterConnection();
        } else {
            playback.sectionFilterId = emptyHardwareId;
        }
        if (playbackConfig.hasOptionalFilters()) {
            auto optionalFilters = playbackConfig.getOptionalFilters();
            playback.extraFilters = optionalFilters;
        }
    }

    static void connectDvrRecord(DvrRecordHardwareConnections& record) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasDvrRecord()) {
            record.support = true;
            configuredRecord = true;
        } else {
            record.support = false;
            return;
        }
        auto recordConfig = *dataFlow.getFirstDvrRecord();
        record.recordFilterId = recordConfig.getRecordFilterConnection();
        record.dvrRecordId = recordConfig.getDvrRecordConnection();
        if (recordConfig.hasDvrSoftwareFeConnection()) {
            record.dvrSoftwareFeId = recordConfig.getDvrSoftwareFeConnection();
        }
        if (recordConfig.getHasFrontendConnection()) {
            record.hasFrontendConnection = true;
            record.dvrSourceId = emptyHardwareId;
            record.frontendId = recordConfig.getFrontendConnection();
        } else {
            record.hasFrontendConnection = false;
            record.dvrSourceId = recordConfig.getDvrSourceConnection();
        }
    }

    static void connectDescrambling(DescramblingHardwareConnections& descrambling) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasDescrambling()) {
            descrambling.support = true;
            configuredDescrambling = true;
        } else {
            descrambling.support = false;
            return;
        }
        auto descConfig = *dataFlow.getFirstDescrambling();
        descrambling.descramblerId = descConfig.getDescramblerConnection();
        descrambling.audioFilterId = descConfig.getAudioFilterConnection();
        descrambling.videoFilterId = descConfig.getVideoFilterConnection();
        if (descConfig.hasDvrSoftwareFeConnection()) {
            descrambling.dvrSoftwareFeId = descConfig.getDvrSoftwareFeConnection();
        }
        if (descConfig.getHasFrontendConnection()) {
            descrambling.hasFrontendConnection = true;
            descrambling.dvrSourceId = emptyHardwareId;
            descrambling.frontendId = descConfig.getFrontendConnection();
        } else {
            descrambling.hasFrontendConnection = false;
            descrambling.dvrSourceId = descConfig.getDvrSourceConnection();
        }
        if (descConfig.hasOptionalFilters()) {
            auto optionalFilters = descConfig.getOptionalFilters();
            descrambling.extraFilters = optionalFilters;
        }
    }

    static void connectLnbLive(LnbLiveHardwareConnections& lnbLive) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasLnbLive()) {
            lnbLive.support = true;
            configuredLnbLive = true;
        } else {
            lnbLive.support = false;
            return;
        }
        auto lnbLiveConfig = *dataFlow.getFirstLnbLive();
        lnbLive.frontendId = lnbLiveConfig.getFrontendConnection();
        lnbLive.audioFilterId = lnbLiveConfig.getAudioFilterConnection();
        lnbLive.videoFilterId = lnbLiveConfig.getVideoFilterConnection();
        lnbLive.lnbId = lnbLiveConfig.getLnbConnection();
        if (lnbLiveConfig.hasDiseqcMsgSender()) {
            for (auto msgName : lnbLiveConfig.getDiseqcMsgSender()) {
                lnbLive.diseqcMsgs.push_back(msgName);
            }
        }
        if (lnbLiveConfig.hasOptionalFilters()) {
            auto optionalFilters = lnbLiveConfig.getOptionalFilters();
            lnbLive.extraFilters = optionalFilters;
        }
    }

    static void connectLnbRecord(LnbRecordHardwareConnections& lnbRecord) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasLnbRecord()) {
            lnbRecord.support = true;
            configuredLnbRecord = true;
        } else {
            lnbRecord.support = false;
            return;
        }
        auto lnbRecordConfig = *dataFlow.getFirstLnbRecord();
        lnbRecord.frontendId = lnbRecordConfig.getFrontendConnection();
        lnbRecord.recordFilterId = lnbRecordConfig.getRecordFilterConnection();
        lnbRecord.dvrRecordId = lnbRecordConfig.getDvrRecordConnection();
        lnbRecord.lnbId = lnbRecordConfig.getLnbConnection();
        if (lnbRecordConfig.hasDiseqcMsgSender()) {
            for (auto msgName : lnbRecordConfig.getDiseqcMsgSender()) {
                lnbRecord.diseqcMsgs.push_back(msgName);
            }
        }
        if (lnbRecordConfig.hasOptionalFilters()) {
            auto optionalFilters = lnbRecordConfig.getOptionalFilters();
            lnbRecord.extraFilters = optionalFilters;
        }
    }

    static void connectTimeFilter(TimeFilterHardwareConnections& timeFilter) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasTimeFilter()) {
            timeFilter.support = true;
            configuredTimeFilter = true;
        } else {
            timeFilter.support = false;
            return;
        }
        auto timeFilterConfig = *dataFlow.getFirstTimeFilter();
        timeFilter.timeFilterId = timeFilterConfig.getTimeFilterConnection();
    }

    static void connectLnbDescrambling(LnbDescramblingHardwareConnections& lnbDescrambling) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasLnbDescrambling()) {
            lnbDescrambling.support = true;
            configuredLnbDescrambling = true;
        } else {
            lnbDescrambling.support = false;
            return;
        }
        auto lnbDescramblingConfig = *dataFlow.getFirstLnbDescrambling();
        lnbDescrambling.frontendId = lnbDescramblingConfig.getFrontendConnection();
        lnbDescrambling.audioFilterId = lnbDescramblingConfig.getAudioFilterConnection();
        lnbDescrambling.videoFilterId = lnbDescramblingConfig.getVideoFilterConnection();
        lnbDescrambling.lnbId = lnbDescramblingConfig.getLnbConnection();
        lnbDescrambling.descramblerId = lnbDescramblingConfig.getDescramblerConnection();
        if (lnbDescramblingConfig.hasDiseqcMsgSender()) {
            for (auto& msgName : lnbDescramblingConfig.getDiseqcMsgSender()) {
                lnbDescrambling.diseqcMsgs.push_back(msgName);
            }
        }
    }

  private:
    static FrontendDvbtSettings readDvbtFrontendSettings(Frontend feConfig) {
        ALOGW("[ConfigReader] fe type is dvbt");
        FrontendDvbtSettings dvbtSettings{
                .frequency = (int64_t)feConfig.getFrequency(),
        };
        if (feConfig.hasEndFrequency()) {
            dvbtSettings.endFrequency = (int64_t)feConfig.getEndFrequency();
        }
        if (!feConfig.hasDvbtFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more dvbt settings");
            return dvbtSettings;
        }
        auto dvbt = feConfig.getFirstDvbtFrontendSettings_optional();
        int32_t trans = static_cast<int32_t>(dvbt->getTransmissionMode());
        dvbtSettings.transmissionMode = static_cast<FrontendDvbtTransmissionMode>(trans);
        dvbtSettings.bandwidth = static_cast<FrontendDvbtBandwidth>(dvbt->getBandwidth());
        dvbtSettings.isHighPriority = dvbt->getIsHighPriority();
        dvbtSettings.hierarchy = static_cast<FrontendDvbtHierarchy>(dvbt->getHierarchy());
        dvbtSettings.hpCoderate = static_cast<FrontendDvbtCoderate>(dvbt->getHpCoderate());
        dvbtSettings.lpCoderate = static_cast<FrontendDvbtCoderate>(dvbt->getLpCoderate());
        dvbtSettings.guardInterval =
                static_cast<FrontendDvbtGuardInterval>(dvbt->getGuardInterval());
        dvbtSettings.standard = static_cast<FrontendDvbtStandard>(dvbt->getStandard());
        dvbtSettings.isMiso = dvbt->getIsMiso();
        dvbtSettings.plpMode = static_cast<FrontendDvbtPlpMode>(dvbt->getPlpMode());
        dvbtSettings.plpId = dvbt->getPlpId();
        dvbtSettings.plpGroupId = dvbt->getPlpGroupId();
        if (dvbt->hasConstellation()) {
            dvbtSettings.constellation =
                    static_cast<FrontendDvbtConstellation>(dvbt->getConstellation());
        }
        return dvbtSettings;
    }

    static FrontendDvbsSettings readDvbsFrontendSettings(Frontend feConfig) {
        ALOGW("[ConfigReader] fe type is dvbs");
        FrontendDvbsSettings dvbsSettings{
                .frequency = (int64_t)feConfig.getFrequency(),
        };
        if (feConfig.hasEndFrequency()) {
            dvbsSettings.endFrequency = (int64_t)feConfig.getEndFrequency();
        }
        if (!feConfig.hasDvbsFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more dvbs settings");
            return dvbsSettings;
        }
        auto dvbs = feConfig.getFirstDvbsFrontendSettings_optional();
        dvbsSettings.symbolRate = static_cast<int32_t>(dvbs->getSymbolRate());
        dvbsSettings.inputStreamId = static_cast<int32_t>(dvbs->getInputStreamId());
        dvbsSettings.scanType = static_cast<FrontendDvbsScanType>(dvbs->getScanType());
        dvbsSettings.isDiseqcRxMessage = dvbs->getIsDiseqcRxMessage();
        dvbsSettings.inversion = static_cast<FrontendSpectralInversion>(dvbs->getInversion());
        dvbsSettings.modulation = static_cast<FrontendDvbsModulation>(dvbs->getModulation());
        dvbsSettings.rolloff = static_cast<FrontendDvbsRolloff>(dvbs->getRolloff());
        dvbsSettings.pilot = static_cast<FrontendDvbsPilot>(dvbs->getPilot());
        dvbsSettings.standard = static_cast<FrontendDvbsStandard>(dvbs->getStandard());
        dvbsSettings.vcmMode = static_cast<FrontendDvbsVcmMode>(dvbs->getVcmMode());
        return dvbsSettings;
    }

    static FrontendAtscSettings readAtscFrontendSettings(Frontend& feConfig) {
        ALOGW("[ConfigReader] fe type is atsc");
        FrontendAtscSettings atscSettings{
                .frequency = (int64_t)feConfig.getFrequency(),
        };
        if (feConfig.hasEndFrequency()) {
            atscSettings.endFrequency = (int64_t)feConfig.getEndFrequency();
        }
        if (!feConfig.hasAtscFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more atsc settings");
            return atscSettings;
        }
        auto atsc = feConfig.getFirstAtscFrontendSettings_optional();
        atscSettings.inversion = static_cast<FrontendSpectralInversion>(atsc->getInversion());
        atscSettings.modulation = static_cast<FrontendAtscModulation>(atsc->getModulation());
        return atscSettings;
    }

    static FrontendIsdbsSettings readIsdbsFrontendSettings(Frontend& feConfig) {
        ALOGW("[ConfigReader] fe type is isdbs");
        FrontendIsdbsSettings isdbsSettings{.frequency = (int64_t)feConfig.getFrequency()};
        if (feConfig.hasEndFrequency()) {
            isdbsSettings.endFrequency = (int64_t)feConfig.getEndFrequency();
        }
        if (!feConfig.hasIsdbsFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more isdbs settings");
            return isdbsSettings;
        }
        auto isdbs = feConfig.getFirstIsdbsFrontendSettings_optional();
        isdbsSettings.streamId = (int32_t)isdbs->getStreamId();
        isdbsSettings.symbolRate = (int32_t)isdbs->getSymbolRate();
        isdbsSettings.modulation = static_cast<FrontendIsdbsModulation>(isdbs->getModulation());
        isdbsSettings.coderate = static_cast<FrontendIsdbsCoderate>(isdbs->getCoderate());
        isdbsSettings.rolloff = static_cast<FrontendIsdbsRolloff>(isdbs->getRolloff());
        isdbsSettings.streamIdType =
                static_cast<FrontendIsdbsStreamIdType>(isdbs->getStreamIdType());
        return isdbsSettings;
    }

    static FrontendIsdbtSettings readIsdbtFrontendSettings(Frontend& feConfig) {
        ALOGW("[ConfigReader] fe type is isdbt");
        FrontendIsdbtSettings isdbtSettings{
                .frequency = (int64_t)feConfig.getFrequency(),
        };
        if (feConfig.hasEndFrequency()) {
            isdbtSettings.endFrequency = (int64_t)feConfig.getEndFrequency();
        }
        if (!feConfig.hasIsdbtFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more isdbt settings");
            return isdbtSettings;
        }
        auto isdbt = feConfig.getFirstIsdbtFrontendSettings_optional();
        isdbtSettings.inversion = static_cast<FrontendSpectralInversion>(isdbt->getInversion());
        isdbtSettings.bandwidth = static_cast<FrontendIsdbtBandwidth>(isdbt->getBandwidth());
        isdbtSettings.mode = static_cast<FrontendIsdbtMode>(isdbt->getMode());
        isdbtSettings.guardInterval =
                static_cast<FrontendIsdbtGuardInterval>(isdbt->getGuardInterval());
        isdbtSettings.serviceAreaId = (int32_t)isdbt->getServiceAreaId();
        isdbtSettings.partialReceptionFlag =
                static_cast<FrontendIsdbtPartialReceptionFlag>(isdbt->getPartialReceptionFlag());
        if (!isdbt->hasFrontendIsdbtLayerSettings()) {
            ALOGW("[ConfigReader] no isdbt layer settings");
            return isdbtSettings;
        }
        auto layerSettings = isdbt->getFirstFrontendIsdbtLayerSettings();
        ::aidl::android::hardware::tv::tuner::FrontendIsdbtLayerSettings mLayerSettings;
        mLayerSettings.modulation =
                static_cast<FrontendIsdbtModulation>(layerSettings->getModulation());
        mLayerSettings.coderate = static_cast<FrontendIsdbtCoderate>(layerSettings->getCoderate());
        mLayerSettings.timeInterleave =
                static_cast<FrontendIsdbtTimeInterleaveMode>(layerSettings->getTimeInterleave());
        mLayerSettings.numOfSegment = (int32_t)layerSettings->getNumOfSegment();
        isdbtSettings.layerSettings.push_back(mLayerSettings);
        return isdbtSettings;
    }

    static bool readFilterTypeAndSettings(Filter filterConfig, DemuxFilterType& type,
                                          DemuxFilterSettings& settings) {
        auto mainType = filterConfig.getMainType();
        auto subType = filterConfig.getSubType();

        if (subType == FilterSubTypeEnum::AUDIO) {
            audioFilterIds.push_back(filterConfig.getId());
        } else if (subType == FilterSubTypeEnum::VIDEO) {
            videoFilterIds.push_back(filterConfig.getId());
        } else if (subType == FilterSubTypeEnum::RECORD) {
            recordFilterIds.push_back(filterConfig.getId());
        } else if (subType == FilterSubTypeEnum::SECTION) {
            sectionFilterIds.push_back(filterConfig.getId());
        } else if (subType == FilterSubTypeEnum::PCR) {
            pcrFilterIds.push_back(filterConfig.getId());
        } else if (subType == FilterSubTypeEnum::IP) {
            ipFilterIds.push_back(filterConfig.getId());
        }

        switch (mainType) {
            case FilterMainTypeEnum::TS: {
                ALOGW("[ConfigReader] filter main type is ts");
                type.mainType = DemuxFilterMainType::TS;
                DemuxTsFilterSettings ts;
                bool isTsSet = false;
                switch (subType) {
                    case FilterSubTypeEnum::UNDEFINED:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::UNDEFINED);
                        break;
                    case FilterSubTypeEnum::SECTION:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::SECTION);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::section>(
                                readSectionFilterSettings(filterConfig));
                        isTsSet = true;
                        break;
                    case FilterSubTypeEnum::PES:
                        // TODO: b/182519645 support all the filter settings
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::PES);
                        break;
                    case FilterSubTypeEnum::TS:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::TS);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        isTsSet = true;
                        break;
                    case FilterSubTypeEnum::PCR:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::PCR);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        isTsSet = true;
                        break;
                    case FilterSubTypeEnum::TEMI:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::TEMI);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        isTsSet = true;
                        break;
                    case FilterSubTypeEnum::AUDIO:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::AUDIO);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::av>(
                                readAvFilterSettings(filterConfig));
                        isTsSet = true;
                        break;
                    case FilterSubTypeEnum::VIDEO:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::VIDEO);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::av>(
                                readAvFilterSettings(filterConfig));
                        isTsSet = true;
                        break;
                    case FilterSubTypeEnum::RECORD:
                        type.subType.set<DemuxFilterSubType::Tag::tsFilterType>(
                                DemuxTsFilterType::RECORD);
                        ts.filterSettings.set<DemuxTsFilterSettingsFilterSettings::Tag::record>(
                                readRecordFilterSettings(filterConfig));
                        isTsSet = true;
                        break;
                    default:
                        ALOGW("[ConfigReader] ts subtype is not supported");
                        return false;
                }
                if (filterConfig.hasPid()) {
                    ts.tpid = static_cast<int32_t>(filterConfig.getPid());
                    isTsSet = true;
                }
                if (isTsSet) {
                    settings.set<DemuxFilterSettings::Tag::ts>(ts);
                }
                break;
            }
            case FilterMainTypeEnum::MMTP: {
                ALOGW("[ConfigReader] filter main type is mmtp");
                type.mainType = DemuxFilterMainType::MMTP;
                DemuxMmtpFilterSettings mmtp;
                bool isMmtpSet = false;
                switch (subType) {
                    case FilterSubTypeEnum::UNDEFINED:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::UNDEFINED);
                        break;
                    case FilterSubTypeEnum::SECTION:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::SECTION);
                        mmtp.filterSettings
                                .set<DemuxMmtpFilterSettingsFilterSettings::Tag::section>(
                                        readSectionFilterSettings(filterConfig));
                        isMmtpSet = true;
                        break;
                    case FilterSubTypeEnum::PES:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::PES);
                        // TODO: b/182519645 support all the filter settings
                        break;
                    case FilterSubTypeEnum::MMTP:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::MMTP);
                        mmtp.filterSettings.set<DemuxMmtpFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        isMmtpSet = true;
                        break;
                    case FilterSubTypeEnum::AUDIO:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::AUDIO);
                        mmtp.filterSettings.set<DemuxMmtpFilterSettingsFilterSettings::Tag::av>(
                                readAvFilterSettings(filterConfig));
                        isMmtpSet = true;
                        break;
                    case FilterSubTypeEnum::VIDEO:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::VIDEO);
                        mmtp.filterSettings.set<DemuxMmtpFilterSettingsFilterSettings::Tag::av>(
                                readAvFilterSettings(filterConfig));
                        isMmtpSet = true;
                        break;
                    case FilterSubTypeEnum::RECORD:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::RECORD);
                        mmtp.filterSettings.set<DemuxMmtpFilterSettingsFilterSettings::Tag::record>(
                                readRecordFilterSettings(filterConfig));
                        isMmtpSet = true;
                        break;
                    case FilterSubTypeEnum::DOWNLOAD:
                        type.subType.set<DemuxFilterSubType::Tag::mmtpFilterType>(
                                DemuxMmtpFilterType::DOWNLOAD);
                        // TODO: b/182519645 support all the filter settings
                        break;
                    default:
                        ALOGW("[ConfigReader] mmtp subtype is not supported");
                        return false;
                }
                if (filterConfig.hasPid()) {
                    mmtp.mmtpPid = static_cast<int32_t>(filterConfig.getPid());
                    isMmtpSet = true;
                }
                if (isMmtpSet) {
                    settings.set<DemuxFilterSettings::Tag::mmtp>(mmtp);
                }
                break;
            }
            case FilterMainTypeEnum::IP: {
                ALOGW("[ConfigReader] filter main type is ip");
                type.mainType = DemuxFilterMainType::IP;
                DemuxIpFilterSettings ip;
                switch (subType) {
                    case FilterSubTypeEnum::UNDEFINED:
                        type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                                DemuxIpFilterType::UNDEFINED);
                        break;
                    case FilterSubTypeEnum::SECTION:
                        type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                                DemuxIpFilterType::SECTION);
                        ip.filterSettings.set<DemuxIpFilterSettingsFilterSettings::Tag::section>(
                                readSectionFilterSettings(filterConfig));
                        settings.set<DemuxFilterSettings::Tag::ip>(ip);
                        break;
                    case FilterSubTypeEnum::NTP:
                        type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                                DemuxIpFilterType::NTP);
                        ip.filterSettings.set<DemuxIpFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        settings.set<DemuxFilterSettings::Tag::ip>(ip);
                        break;
                    case FilterSubTypeEnum::IP: {
                        type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                                DemuxIpFilterType::IP);
                        ip.ipAddr = readIpAddress(filterConfig),
                        ip.filterSettings
                                .set<DemuxIpFilterSettingsFilterSettings::Tag::bPassthrough>(
                                        readPassthroughSettings(filterConfig));
                        settings.set<DemuxFilterSettings::Tag::ip>(ip);
                        break;
                    }
                    case FilterSubTypeEnum::IP_PAYLOAD:
                        type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                                DemuxIpFilterType::IP_PAYLOAD);
                        ip.filterSettings.set<DemuxIpFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        settings.set<DemuxFilterSettings::Tag::ip>(ip);
                        break;
                    case FilterSubTypeEnum::PAYLOAD_THROUGH:
                        type.subType.set<DemuxFilterSubType::Tag::ipFilterType>(
                                DemuxIpFilterType::PAYLOAD_THROUGH);
                        ip.filterSettings.set<DemuxIpFilterSettingsFilterSettings::Tag::noinit>(
                                true);
                        settings.set<DemuxFilterSettings::Tag::ip>(ip);
                        break;
                    default:
                        ALOGW("[ConfigReader] mmtp subtype is not supported");
                        return false;
                }
                break;
            }
            default:
                // TODO: b/182519645 support all the filter configs
                ALOGW("[ConfigReader] filter main type is not supported in dynamic config");
                return false;
        }
        return true;
    }

    static DemuxIpAddress readIpAddress(Filter filterConfig) {
        DemuxIpAddress ipAddress;
        vector<uint8_t> data;
        if (!filterConfig.hasIpFilterConfig_optional()) {
            return ipAddress;
        }
        auto ipFilterConfig = filterConfig.getFirstIpFilterConfig_optional();
        if (ipFilterConfig->hasSrcPort()) {
            ipAddress.srcPort = ipFilterConfig->getSrcPort();
        }
        if (ipFilterConfig->hasDestPort()) {
            ipAddress.dstPort = ipFilterConfig->getDestPort();
        }
        if (ipFilterConfig->getFirstSrcIpAddress()->getIsIpV4()) {
            data.resize(4);
            memcpy(data.data(), ipFilterConfig->getFirstSrcIpAddress()->getIp().data(), 4);
            ipAddress.srcIpAddress.set<DemuxIpAddressIpAddress::Tag::v4>(data);
        } else {
            data.resize(6);
            memcpy(data.data(), ipFilterConfig->getFirstSrcIpAddress()->getIp().data(), 6);
            ipAddress.srcIpAddress.set<DemuxIpAddressIpAddress::Tag::v6>(data);
        }
        if (ipFilterConfig->getFirstDestIpAddress()->getIsIpV4()) {
            data.resize(4);
            memcpy(data.data(), ipFilterConfig->getFirstDestIpAddress()->getIp().data(), 4);
            ipAddress.dstIpAddress.set<DemuxIpAddressIpAddress::Tag::v4>(data);
        } else {
            data.resize(6);
            memcpy(data.data(), ipFilterConfig->getFirstDestIpAddress()->getIp().data(), 6);
            ipAddress.dstIpAddress.set<DemuxIpAddressIpAddress::Tag::v6>(data);
        }
        return ipAddress;
    }

    static bool readPassthroughSettings(Filter filterConfig) {
        if (!filterConfig.hasIpFilterConfig_optional()) {
            return false;
        }
        auto ipFilterConfig = filterConfig.getFirstIpFilterConfig_optional();
        if (ipFilterConfig->hasDataPassthrough()) {
            return ipFilterConfig->getDataPassthrough();
        }
        return false;
    }

    static DemuxFilterSectionSettings readSectionFilterSettings(Filter filterConfig) {
        DemuxFilterSectionSettings settings;
        if (!filterConfig.hasSectionFilterSettings_optional()) {
            return settings;
        }
        auto section = filterConfig.getFirstSectionFilterSettings_optional();
        settings.isCheckCrc = section->getIsCheckCrc();
        settings.isRepeat = section->getIsRepeat();
        settings.isRaw = section->getIsRaw();
        settings.bitWidthOfLengthField = section->getBitWidthOfLengthField();
        return settings;
    }

    static DemuxFilterAvSettings readAvFilterSettings(Filter filterConfig) {
        DemuxFilterAvSettings settings;
        if (!filterConfig.hasAvFilterSettings_optional()) {
            return settings;
        }
        auto av = filterConfig.getFirstAvFilterSettings_optional();
        settings.isPassthrough = av->getIsPassthrough();
        settings.isSecureMemory = av->getIsSecureMemory();
        return settings;
    }

    static DemuxFilterRecordSettings readRecordFilterSettings(Filter filterConfig) {
        DemuxFilterRecordSettings settings;
        if (!filterConfig.hasRecordFilterSettings_optional()) {
            return settings;
        }
        auto record = filterConfig.getFirstRecordFilterSettings_optional();
        settings.tsIndexMask = record->getTsIndexMask();
        settings.scIndexType = static_cast<DemuxRecordScIndexType>(record->getScIndexType());
        return settings;
    }

    static PlaybackSettings readPlaybackSettings(Dvr dvrConfig) {
        ALOGW("[ConfigReader] dvr type is playback");
        PlaybackSettings playbackSettings{
                .statusMask = static_cast<int8_t>(dvrConfig.getStatusMask()),
                .lowThreshold = static_cast<int64_t>(dvrConfig.getLowThreshold()),
                .highThreshold = static_cast<int64_t>(dvrConfig.getHighThreshold()),
                .dataFormat = static_cast<DataFormat>(dvrConfig.getDataFormat()),
                .packetSize = static_cast<int64_t>(dvrConfig.getPacketSize()),
        };
        return playbackSettings;
    }

    static RecordSettings readRecordSettings(Dvr dvrConfig) {
        ALOGW("[ConfigReader] dvr type is record");
        RecordSettings recordSettings{
                .statusMask = static_cast<int8_t>(dvrConfig.getStatusMask()),
                .lowThreshold = static_cast<int64_t>(dvrConfig.getLowThreshold()),
                .highThreshold = static_cast<int64_t>(dvrConfig.getHighThreshold()),
                .dataFormat = static_cast<DataFormat>(dvrConfig.getDataFormat()),
                .packetSize = static_cast<int64_t>(dvrConfig.getPacketSize()),
        };
        return recordSettings;
    }

    static void getCiCamInfo(Frontend feConfig, bool& canConnectToCiCam, int32_t& ciCamId,
                             int32_t& removePid) {
        if (!feConfig.hasConnectToCicamId()) {
            canConnectToCiCam = false;
            ciCamId = -1;
            removePid = -1;
            return;
        }
        canConnectToCiCam = true;
        ciCamId = static_cast<int32_t>(feConfig.getConnectToCicamId());
        removePid = static_cast<int32_t>(feConfig.getRemoveOutputPid());
    }
};
