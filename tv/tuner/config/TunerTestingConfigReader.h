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

struct FrontendConfig {
    bool isSoftwareFe;
    FrontendType type;
    FrontendSettings settings;
    vector<FrontendStatusType> tuneStatusTypes;
    vector<FrontendStatus> expectTuneStatuses;
};

struct LiveBroadcastHardwareConnections {
    string frontendId;
    /* string audioFilterId;
    string videoFilterId;
    list string of extra filters; */
};

struct ScanHardwareConnections {
    string frontendId;
};

struct DvrRecordHardwareConnections {
    bool support;
    string frontendId;
    /* string recordFilterId;
    string dvrId; */
};

struct DescramblingHardwareConnections {
    bool support;
    string frontendId;
    /* string descramblerId;
    string audioFilterId;
    string videoFilterId;
    list string of extra filters; */
};

struct LnbLiveHardwareConnections {
    bool support;
    string frontendId;
    /* string audioFilterId;
    string videoFilterId;
    list string of extra filters;
    string lnbId; */
};

struct LnbRecordHardwareConnections {
    bool support;
    string frontendId;
    /* string recordFilterId;
    list string of extra filters;
    string lnbId; */
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
            // TODO: complete the tune status config
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
                    // TODO: finish all other frontend settings
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
                // TODO: complete the tune status config
                frontendMap[id].tuneStatusTypes = types;
                frontendMap[id].expectTuneStatuses = statuses;
            }
        }
    }

    static void connectLiveBroadcast(LiveBroadcastHardwareConnections& live) {
        auto liveConfig = getDataFlowConfiguration().getFirstClearLiveBroadcast();
        live.frontendId = liveConfig->getFrontendConnection();
    }

    static void connectScan(ScanHardwareConnections& scan) {
        auto scanConfig = getDataFlowConfiguration().getFirstScan();
        scan.frontendId = scanConfig->getFrontendConnection();
    }

    static void connectDvrRecord(DvrRecordHardwareConnections& record) {
        auto dataFlow = getDataFlowConfiguration();
        if (!dataFlow.hasDvrRecord()) {
            record.support = false;
            return;
        }
        auto recordConfig = dataFlow.getFirstDvrRecord();
        record.frontendId = recordConfig->getFrontendConnection();
    }

    static void connectDescrambling(DescramblingHardwareConnections& descrambling) {
        auto dataFlow = getDataFlowConfiguration();
        if (!dataFlow.hasDescrambling()) {
            descrambling.support = false;
            return;
        }
        auto descConfig = dataFlow.getFirstDescrambling();
        descrambling.frontendId = descConfig->getFrontendConnection();
    }

    static void connectLnbLive(LnbLiveHardwareConnections& lnbLive) {
        auto dataFlow = getDataFlowConfiguration();
        if (!dataFlow.hasLnbLive()) {
            lnbLive.support = false;
            return;
        }
        auto lnbLiveConfig = dataFlow.getFirstLnbLive();
        lnbLive.frontendId = lnbLiveConfig->getFrontendConnection();
    }

    static void connectLnbRecord(LnbRecordHardwareConnections& lnbRecord) {
        auto dataFlow = getDataFlowConfiguration();
        if (!dataFlow.hasLnbRecord()) {
            lnbRecord.support = false;
            return;
        }
        auto lnbRecordConfig = dataFlow.getFirstLnbRecord();
        lnbRecord.frontendId = lnbRecordConfig->getFrontendConnection();
    }

  private:
    static FrontendDvbtSettings readDvbtFrontendSettings(Frontend feConfig) {
        ALOGW("[ConfigReader] type is dvbt");
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
        ALOGW("[ConfigReader] type is dvbs");
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

    static TunerConfiguration getTunerConfig() { return *read(configFilePath.c_str()); }

    static HardwareConfiguration getHardwareConfig() {
        return *getTunerConfig().getFirstHardwareConfiguration();
    }

    static DataFlowConfiguration getDataFlowConfiguration() {
        return *getTunerConfig().getFirstDataFlowConfiguration();
    }
};
