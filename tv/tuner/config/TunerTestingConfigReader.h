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

#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <android_media_tuner_testing_configuration_V1_0.h>
#include <android_media_tuner_testing_configuration_V1_0_enums.h>
#include <binder/MemoryDealer.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>

using namespace std;
using namespace android::media::tuner::testing::configuration::V1_0;

using android::hardware::tv::tuner::V1_0::DataFormat;
using android::hardware::tv::tuner::V1_0::DemuxAlpFilterType;
using android::hardware::tv::tuner::V1_0::DemuxFilterAvSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterRecordSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterSectionSettings;
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
using android::hardware::tv::tuner::V1_0::FrontendDvbsSettings;
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

const string configFilePath = "/vendor/etc/tuner_vts_config.xml";
const string emptyHardwareId = "";

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
    FrontendSettings settings;
    vector<FrontendStatusType> tuneStatusTypes;
    vector<FrontendStatus> expectTuneStatuses;
};

struct FilterConfig {
    uint32_t bufferSize;
    DemuxFilterType type;
    DemuxFilterSettings settings;
    bool getMqDesc;

    bool operator<(const FilterConfig& /*c*/) const { return false; }
};

struct DvrConfig {
    DvrType type;
    uint32_t bufferSize;
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
    uint64_t timeStamp;
};

struct DescramblerConfig {
    uint32_t casSystemId;
    string provisionStr;
    vector<uint8_t> hidlPvtData;
};

struct LiveBroadcastHardwareConnections {
    string frontendId;
    string dvrSoftwareFeId;
    string audioFilterId;
    string videoFilterId;
    string sectionFilterId;
    string pcrFilterId;
    /* list string of extra filters; */
};

struct ScanHardwareConnections {
    string frontendId;
};

struct DvrPlaybackHardwareConnections {
    bool support;
    string frontendId;
    string dvrId;
    string audioFilterId;
    string videoFilterId;
    string sectionFilterId;
    /* list string of extra filters; */
};

struct DvrRecordHardwareConnections {
    bool support;
    string frontendId;
    string dvrRecordId;
    string dvrSoftwareFeId;
    string recordFilterId;
};

struct DescramblingHardwareConnections {
    bool support;
    string frontendId;
    string dvrSoftwareFeId;
    string audioFilterId;
    string videoFilterId;
    string descramblerId;
    /* list string of extra filters; */
};

struct LnbLiveHardwareConnections {
    bool support;
    string frontendId;
    string audioFilterId;
    string videoFilterId;
    string lnbId;
    vector<string> diseqcMsgs;
    /* list string of extra filters; */
};

struct LnbRecordHardwareConnections {
    bool support;
    string frontendId;
    string dvrRecordId;
    string recordFilterId;
    string lnbId;
    vector<string> diseqcMsgs;
    /* list string of extra filters; */
};

struct TimeFilterHardwareConnections {
    bool support;
    string timeFilterId;
};

struct TunerTestingConfigReader {
  public:
    static bool checkConfigFileExists() {
        auto res = read(configFilePath.c_str());
        if (res == nullopt) {
            ALOGW("[ConfigReader] Couldn't read /vendor/etc/tuner_vts_config.xml."
                  "Please check tuner_testing_dynamic_configuration.xsd"
                  "and sample_tuner_vts_config.xml for more details on how to config Tune VTS.");
        }
        return (res != nullopt);
    }

    static void readFrontendConfig1_0(map<string, FrontendConfig>& frontendMap) {
        auto hardwareConfig = getHardwareConfig();
        if (hardwareConfig.hasFrontends()) {
            // TODO: b/182519645 complete the tune status config
            vector<FrontendStatusType> types;
            types.push_back(FrontendStatusType::DEMOD_LOCK);
            FrontendStatus status;
            status.isDemodLocked(true);
            vector<FrontendStatus> statuses;
            statuses.push_back(status);

            auto frontends = *hardwareConfig.getFirstFrontends();
            for (auto feConfig : frontends.getFrontend()) {
                string id = feConfig.getId();
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
                        break;
                    case FrontendTypeEnum::ATSC3:
                        type = FrontendType::ATSC3;
                        break;
                    case FrontendTypeEnum::DVBC:
                        type = FrontendType::DVBC;
                        break;
                    case FrontendTypeEnum::DVBS:
                        type = FrontendType::DVBS;
                        frontendMap[id].settings.dvbs(readDvbsFrontendSettings(feConfig));
                        break;
                    case FrontendTypeEnum::DVBT: {
                        type = FrontendType::DVBT;
                        frontendMap[id].settings.dvbt(readDvbtFrontendSettings(feConfig));
                        break;
                    }
                    case FrontendTypeEnum::ISDBS:
                        type = FrontendType::ISDBS;
                        break;
                    case FrontendTypeEnum::ISDBS3:
                        type = FrontendType::ISDBS3;
                        break;
                    case FrontendTypeEnum::ISDBT:
                        type = FrontendType::ISDBT;
                        break;
                    case FrontendTypeEnum::DTMB:
                        // dtmb will be handled in readFrontendConfig1_1;
                        continue;
                    case FrontendTypeEnum::UNKNOWN:
                        ALOGW("[ConfigReader] invalid frontend type");
                        return;
                }
                frontendMap[id].type = type;
                frontendMap[id].isSoftwareFe = feConfig.getIsSoftwareFrontend();
                // TODO: b/182519645 complete the tune status config
                frontendMap[id].tuneStatusTypes = types;
                frontendMap[id].expectTuneStatuses = statuses;
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
                        type = DvrType::PLAYBACK;
                        dvrMap[id].settings.playback(readPlaybackSettings(dvrConfig));
                        break;
                    case DvrTypeEnum::RECORD:
                        type = DvrType::RECORD;
                        dvrMap[id].settings.record(readRecordSettings(dvrConfig));
                        break;
                    case DvrTypeEnum::UNKNOWN:
                        ALOGW("[ConfigReader] invalid DVR type");
                        return;
                }
                dvrMap[id].type = type;
                dvrMap[id].bufferSize = static_cast<uint32_t>(dvrConfig.getBufferSize());
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
                descramblerMap[id].casSystemId =
                        static_cast<uint32_t>(descramblerConfig.getCasSystemId());
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
                timeFilterMap[id].timeStamp =
                        static_cast<uint64_t>(timeFilterConfig.getTimeStamp());
            }
        }
    }

    static void connectLiveBroadcast(LiveBroadcastHardwareConnections& live) {
        auto liveConfig = *getDataFlowConfiguration().getFirstClearLiveBroadcast();
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
    }

    static void connectScan(ScanHardwareConnections& scan) {
        auto scanConfig = getDataFlowConfiguration().getFirstScan();
        scan.frontendId = scanConfig->getFrontendConnection();
    }

    static void connectDvrPlayback(DvrPlaybackHardwareConnections& playback) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasDvrPlayback()) {
            playback.support = true;
        } else {
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
    }

    static void connectDvrRecord(DvrRecordHardwareConnections& record) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasDvrRecord()) {
            record.support = true;
        } else {
            return;
        }
        auto recordConfig = *dataFlow.getFirstDvrRecord();
        record.frontendId = recordConfig.getFrontendConnection();
        record.recordFilterId = recordConfig.getRecordFilterConnection();
        record.dvrRecordId = recordConfig.getDvrRecordConnection();
        if (recordConfig.hasDvrSoftwareFeConnection()) {
            record.dvrSoftwareFeId = recordConfig.getDvrSoftwareFeConnection();
        }
    }

    static void connectDescrambling(DescramblingHardwareConnections& descrambling) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasDescrambling()) {
            descrambling.support = true;
        } else {
            return;
        }
        auto descConfig = *dataFlow.getFirstDescrambling();
        descrambling.frontendId = descConfig.getFrontendConnection();
        descrambling.descramblerId = descConfig.getDescramblerConnection();
        descrambling.audioFilterId = descConfig.getAudioFilterConnection();
        descrambling.videoFilterId = descConfig.getVideoFilterConnection();
        if (descConfig.hasDvrSoftwareFeConnection()) {
            descrambling.dvrSoftwareFeId = descConfig.getDvrSoftwareFeConnection();
        }
    }

    static void connectLnbLive(LnbLiveHardwareConnections& lnbLive) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasLnbLive()) {
            lnbLive.support = true;
        } else {
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
    }

    static void connectLnbRecord(LnbRecordHardwareConnections& lnbRecord) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasLnbRecord()) {
            lnbRecord.support = true;
        } else {
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
    }

    static void connectTimeFilter(TimeFilterHardwareConnections& timeFilter) {
        auto dataFlow = getDataFlowConfiguration();
        if (dataFlow.hasTimeFilter()) {
            timeFilter.support = true;
        } else {
            return;
        }
        auto timeFilterConfig = *dataFlow.getFirstTimeFilter();
        timeFilter.timeFilterId = timeFilterConfig.getTimeFilterConnection();
    }

  private:
    static FrontendDvbtSettings readDvbtFrontendSettings(Frontend feConfig) {
        ALOGW("[ConfigReader] fe type is dvbt");
        FrontendDvbtSettings dvbtSettings{
                .frequency = (uint32_t)feConfig.getFrequency(),
        };
        if (!feConfig.hasDvbtFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more dvbt settings");
            return dvbtSettings;
        }
        dvbtSettings.transmissionMode = static_cast<FrontendDvbtTransmissionMode>(
                feConfig.getFirstDvbtFrontendSettings_optional()->getTransmissionMode());
        dvbtSettings.bandwidth = static_cast<FrontendDvbtBandwidth>(
                feConfig.getFirstDvbtFrontendSettings_optional()->getBandwidth());
        dvbtSettings.isHighPriority =
                feConfig.getFirstDvbtFrontendSettings_optional()->getIsHighPriority();
        return dvbtSettings;
    }

    static FrontendDvbsSettings readDvbsFrontendSettings(Frontend feConfig) {
        ALOGW("[ConfigReader] fe type is dvbs");
        FrontendDvbsSettings dvbsSettings{
                .frequency = (uint32_t)feConfig.getFrequency(),
        };
        if (!feConfig.hasDvbsFrontendSettings_optional()) {
            ALOGW("[ConfigReader] no more dvbs settings");
            return dvbsSettings;
        }
        dvbsSettings.symbolRate = static_cast<uint32_t>(
                feConfig.getFirstDvbsFrontendSettings_optional()->getSymbolRate());
        dvbsSettings.inputStreamId = static_cast<uint32_t>(
                feConfig.getFirstDvbsFrontendSettings_optional()->getInputStreamId());
        return dvbsSettings;
    }

    static bool readFilterTypeAndSettings(Filter filterConfig, DemuxFilterType& type,
                                          DemuxFilterSettings& settings) {
        auto mainType = filterConfig.getMainType();
        auto subType = filterConfig.getSubType();
        uint32_t pid = static_cast<uint32_t>(filterConfig.getPid());
        switch (mainType) {
            case FilterMainTypeEnum::TS: {
                ALOGW("[ConfigReader] filter main type is ts");
                type.mainType = DemuxFilterMainType::TS;
                switch (subType) {
                    case FilterSubTypeEnum::UNDEFINED:
                        break;
                    case FilterSubTypeEnum::SECTION:
                        type.subType.tsFilterType(DemuxTsFilterType::SECTION);
                        settings.ts().filterSettings.section(
                                readSectionFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::PES:
                        // TODO: b/182519645 support all the filter settings
                        /*settings.ts().filterSettings.pesData(
                                getPesFilterSettings(filterConfig));*/
                        type.subType.tsFilterType(DemuxTsFilterType::PES);
                        break;
                    case FilterSubTypeEnum::TS:
                        type.subType.tsFilterType(DemuxTsFilterType::TS);
                        settings.ts().filterSettings.noinit();
                        break;
                    case FilterSubTypeEnum::PCR:
                        type.subType.tsFilterType(DemuxTsFilterType::PCR);
                        settings.ts().filterSettings.noinit();
                        break;
                    case FilterSubTypeEnum::TEMI:
                        type.subType.tsFilterType(DemuxTsFilterType::TEMI);
                        settings.ts().filterSettings.noinit();
                        break;
                    case FilterSubTypeEnum::AUDIO:
                        type.subType.tsFilterType(DemuxTsFilterType::AUDIO);
                        settings.ts().filterSettings.av(readAvFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::VIDEO:
                        type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
                        settings.ts().filterSettings.av(readAvFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::RECORD:
                        type.subType.tsFilterType(DemuxTsFilterType::RECORD);
                        settings.ts().filterSettings.record(readRecordFilterSettings(filterConfig));
                        break;
                    default:
                        ALOGW("[ConfigReader] ts subtype is not supported");
                        return false;
                }
                settings.ts().tpid = pid;
                break;
            }
            case FilterMainTypeEnum::MMTP: {
                ALOGW("[ConfigReader] filter main type is mmtp");
                type.mainType = DemuxFilterMainType::MMTP;
                switch (subType) {
                    case FilterSubTypeEnum::UNDEFINED:
                        break;
                    case FilterSubTypeEnum::SECTION:
                        type.subType.mmtpFilterType(DemuxMmtpFilterType::SECTION);
                        settings.mmtp().filterSettings.section(
                                readSectionFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::PES:
                        type.subType.mmtpFilterType(DemuxMmtpFilterType::PES);
                        // TODO: b/182519645 support all the filter settings
                        /*settings.mmtp().filterSettings.pesData(
                                getPesFilterSettings(filterConfig));*/
                        break;
                    case FilterSubTypeEnum::MMTP:
                        type.subType.mmtpFilterType(DemuxMmtpFilterType::MMTP);
                        settings.mmtp().filterSettings.noinit();
                        break;
                    case FilterSubTypeEnum::AUDIO:
                        type.subType.mmtpFilterType(DemuxMmtpFilterType::AUDIO);
                        settings.mmtp().filterSettings.av(readAvFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::VIDEO:
                        settings.mmtp().filterSettings.av(readAvFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::RECORD:
                        type.subType.mmtpFilterType(DemuxMmtpFilterType::RECORD);
                        settings.mmtp().filterSettings.record(
                                readRecordFilterSettings(filterConfig));
                        break;
                    case FilterSubTypeEnum::DOWNLOAD:
                        type.subType.mmtpFilterType(DemuxMmtpFilterType::DOWNLOAD);
                        // TODO: b/182519645 support all the filter settings
                        /*settings.mmtp().filterSettings.download(
                                getDownloadFilterSettings(filterConfig));*/
                        break;
                    default:
                        ALOGW("[ConfigReader] mmtp subtype is not supported");
                        return false;
                }
                settings.mmtp().mmtpPid = pid;
                break;
            }
            default:
                // TODO: b/182519645 support all the filter configs
                ALOGW("[ConfigReader] filter main type is not supported in dynamic config");
                return false;
        }
        return true;
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
        return settings;
    }

    static DemuxFilterAvSettings readAvFilterSettings(Filter filterConfig) {
        DemuxFilterAvSettings settings;
        if (!filterConfig.hasAvFilterSettings_optional()) {
            return settings;
        }
        auto av = filterConfig.getFirstAvFilterSettings_optional();
        settings.isPassthrough = av->getIsPassthrough();
        return settings;
    }

    static DemuxFilterRecordSettings readRecordFilterSettings(Filter filterConfig) {
        DemuxFilterRecordSettings settings;
        if (!filterConfig.hasRecordFilterSettings_optional()) {
            return settings;
        }
        auto record = filterConfig.getFirstRecordFilterSettings_optional();
        settings.tsIndexMask = static_cast<uint32_t>(record->getTsIndexMask());
        settings.scIndexType = static_cast<DemuxRecordScIndexType>(record->getScIndexType());
        return settings;
    }

    static PlaybackSettings readPlaybackSettings(Dvr dvrConfig) {
        ALOGW("[ConfigReader] dvr type is playback");
        PlaybackSettings playbackSettings{
                .statusMask = static_cast<uint8_t>(dvrConfig.getStatusMask()),
                .lowThreshold = static_cast<uint32_t>(dvrConfig.getLowThreshold()),
                .highThreshold = static_cast<uint32_t>(dvrConfig.getHighThreshold()),
                .dataFormat = static_cast<DataFormat>(dvrConfig.getDataFormat()),
                .packetSize = static_cast<uint8_t>(dvrConfig.getPacketSize()),
        };
        return playbackSettings;
    }

    static RecordSettings readRecordSettings(Dvr dvrConfig) {
        ALOGW("[ConfigReader] dvr type is record");
        RecordSettings recordSettings{
                .statusMask = static_cast<uint8_t>(dvrConfig.getStatusMask()),
                .lowThreshold = static_cast<uint32_t>(dvrConfig.getLowThreshold()),
                .highThreshold = static_cast<uint32_t>(dvrConfig.getHighThreshold()),
                .dataFormat = static_cast<DataFormat>(dvrConfig.getDataFormat()),
                .packetSize = static_cast<uint8_t>(dvrConfig.getPacketSize()),
        };
        return recordSettings;
    }

    static TunerConfiguration getTunerConfig() { return *read(configFilePath.c_str()); }

    static HardwareConfiguration getHardwareConfig() {
        return *getTunerConfig().getFirstHardwareConfiguration();
    }

    static DataFlowConfiguration getDataFlowConfiguration() {
        return *getTunerConfig().getFirstDataFlowConfiguration();
    }
};
