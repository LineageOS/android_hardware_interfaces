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
#include "TunerTestingConfigReaderV1_0.h"

using android::hardware::tv::tuner::V1_1::AudioStreamType;
using android::hardware::tv::tuner::V1_1::AvStreamType;
using android::hardware::tv::tuner::V1_1::FrontendDvbsScanType;
using android::hardware::tv::tuner::V1_1::FrontendDvbsSettingsExt1_1;
using android::hardware::tv::tuner::V1_1::FrontendDvbtSettingsExt1_1;
using android::hardware::tv::tuner::V1_1::FrontendSettingsExt1_1;
using android::hardware::tv::tuner::V1_1::FrontendStatusExt1_1;
using android::hardware::tv::tuner::V1_1::FrontendStatusTypeExt1_1;
using android::hardware::tv::tuner::V1_1::VideoStreamType;

struct FrontendConfig1_1 {
    FrontendConfig config1_0;
    bool canConnectToCiCam;
    uint32_t ciCamId;
    FrontendSettingsExt1_1 settingsExt1_1;
    vector<FrontendStatusTypeExt1_1> tuneStatusTypes;
    vector<FrontendStatusExt1_1> expectTuneStatuses;
};

struct FilterConfig1_1 {
    FilterConfig config1_0;
    AvStreamType streamType;
    uint32_t ipCid;
    uint32_t monitorEventTypes;

    bool operator<(const FilterConfig& /*c*/) const { return false; }
};

struct TunerTestingConfigReader1_1 {
  public:
    static void readFrontendConfig1_1(map<string, FrontendConfig1_1>& frontendMap) {
        map<string, FrontendConfig> frontendMap1_0;
        TunerTestingConfigReader1_0::readFrontendConfig1_0(frontendMap1_0);
        for (auto it = frontendMap1_0.begin(); it != frontendMap1_0.end(); it++) {
            frontendMap[it->first].config1_0 = it->second;
        }

        auto hardwareConfig = TunerTestingConfigReader1_0::getHardwareConfig();
        if (hardwareConfig.hasFrontends()) {
            // TODO: b/182519645 complete the tune status config
            vector<FrontendStatusTypeExt1_1> types;
            types.push_back(FrontendStatusTypeExt1_1::UEC);
            types.push_back(FrontendStatusTypeExt1_1::IS_MISO);
            vector<FrontendStatusExt1_1> statuses;
            FrontendStatusExt1_1 status;
            status.uec(4);
            statuses.push_back(status);
            status.isMiso(true);
            statuses.push_back(status);

            auto frontends = *hardwareConfig.getFirstFrontends();

            for (auto feConfig : frontends.getFrontend()) {
                string id = feConfig.getId();
                switch (feConfig.getType()) {
                    case FrontendTypeEnum::DVBS:
                        frontendMap[id].settingsExt1_1.settingExt.dvbs(
                                readDvbsFrontendSettings1_1(feConfig));
                        break;
                    case FrontendTypeEnum::DVBT: {
                        frontendMap[id].settingsExt1_1.settingExt.dvbt(
                                readDvbtFrontendSettings1_1(feConfig));
                        break;
                    }
                    case FrontendTypeEnum::DTMB:
                        frontendMap[id].config1_0.type = static_cast<FrontendType>(
                                android::hardware::tv::tuner::V1_1::FrontendType::DTMB);
                        break;
                    case FrontendTypeEnum::UNKNOWN:
                        ALOGW("[ConfigReader] invalid frontend type");
                        return;
                    default:
                        ALOGW("[ConfigReader] fe already handled in 1_0 reader.");
                        break;
                }
                if (feConfig.hasEndFrequency()) {
                    frontendMap[id].settingsExt1_1.endFrequency =
                            (uint32_t)feConfig.getEndFrequency();
                }
                // TODO: b/182519645 complete the tune status config
                frontendMap[id].tuneStatusTypes = types;
                frontendMap[id].expectTuneStatuses = statuses;
                getCiCamInfo(feConfig, frontendMap[id].canConnectToCiCam, frontendMap[id].ciCamId);
            }
        }
    }

    static void readFilterConfig1_1(map<string, FilterConfig1_1>& filterMap) {
        map<string, FilterConfig> filterMap1_0;
        TunerTestingConfigReader1_0::readFilterConfig1_0(filterMap1_0);
        for (auto it = filterMap1_0.begin(); it != filterMap1_0.end(); it++) {
            filterMap[it->first].config1_0 = it->second;
        }
        auto hardwareConfig = TunerTestingConfigReader1_0::getHardwareConfig();
        if (hardwareConfig.hasFilters()) {
            auto filters = *hardwareConfig.getFirstFilters();
            for (auto filterConfig : filters.getFilter()) {
                string id = filterConfig.getId();
                if (filterConfig.hasMonitorEventTypes()) {
                    filterMap[id].monitorEventTypes = (uint32_t)filterConfig.getMonitorEventTypes();
                }
                if (filterConfig.hasAvFilterSettings_optional()) {
                    AvStreamType type;
                    auto av = filterConfig.getFirstAvFilterSettings_optional();
                    if (av->hasAudioStreamType_optional()) {
                        type.audio(static_cast<AudioStreamType>(av->getAudioStreamType_optional()));
                        filterMap[id].streamType = type;
                    }
                    if (av->hasVideoStreamType_optional()) {
                        type.video(static_cast<VideoStreamType>(av->getVideoStreamType_optional()));
                        filterMap[id].streamType = type;
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

  private:
    static void getCiCamInfo(Frontend feConfig, bool& canConnectToCiCam, uint32_t& ciCamId) {
        if (!feConfig.hasConnectToCicamId()) {
            canConnectToCiCam = false;
            ciCamId = -1;
        }
        canConnectToCiCam = true;
        ciCamId = static_cast<uint32_t>(feConfig.getConnectToCicamId());
    }

    static FrontendDvbsSettingsExt1_1 readDvbsFrontendSettings1_1(Frontend feConfig) {
        FrontendDvbsSettingsExt1_1 dvbsSettings;
        if (!feConfig.hasDvbsFrontendSettings_optional()) {
            return dvbsSettings;
        }
        auto dvbs = feConfig.getFirstDvbsFrontendSettings_optional();
        if (dvbs->hasScanType()) {
            dvbsSettings.scanType = static_cast<FrontendDvbsScanType>(dvbs->getScanType());
        }
        if (dvbs->hasIsDiseqcRxMessage()) {
            dvbsSettings.isDiseqcRxMessage = dvbs->getIsDiseqcRxMessage();
        }
        return dvbsSettings;
    }

    static FrontendDvbtSettingsExt1_1 readDvbtFrontendSettings1_1(Frontend feConfig) {
        FrontendDvbtSettingsExt1_1 dvbtSettings;
        if (!feConfig.hasDvbtFrontendSettings_optional()) {
            return dvbtSettings;
        }
        auto dvbt = feConfig.getFirstDvbtFrontendSettings_optional();
        auto trans = dvbt->getTransmissionMode();
        dvbtSettings.transmissionMode =
                static_cast<android::hardware::tv::tuner::V1_1::FrontendDvbtTransmissionMode>(
                        trans);
        if (dvbt->hasConstellation()) {
            dvbtSettings.constellation =
                    static_cast<android::hardware::tv::tuner::V1_1::FrontendDvbtConstellation>(
                            dvbt->getConstellation());
        }
        return dvbtSettings;
    }
};