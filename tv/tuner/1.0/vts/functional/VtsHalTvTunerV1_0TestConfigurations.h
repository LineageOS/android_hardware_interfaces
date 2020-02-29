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

#include <android-base/logging.h>
#include <android/hardware/tv/tuner/1.0/IDemux.h>
#include <android/hardware/tv/tuner/1.0/IDescrambler.h>
#include <android/hardware/tv/tuner/1.0/IDvr.h>
#include <android/hardware/tv/tuner/1.0/IDvrCallback.h>
#include <android/hardware/tv/tuner/1.0/IFilter.h>
#include <android/hardware/tv/tuner/1.0/IFilterCallback.h>
#include <android/hardware/tv/tuner/1.0/IFrontend.h>
#include <android/hardware/tv/tuner/1.0/IFrontendCallback.h>
#include <android/hardware/tv/tuner/1.0/ITuner.h>
#include <android/hardware/tv/tuner/1.0/types.h>
#include <binder/MemoryDealer.h>
#include <fmq/MessageQueue.h>
#include <hidl/HidlSupport.h>
#include <hidl/HidlTransportSupport.h>
#include <hidl/Status.h>
#include <hidlmemory/FrameworkUtils.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <fstream>
#include <iostream>
#include <map>

using android::hardware::tv::tuner::V1_0::DemuxFilterEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterMainType;
using android::hardware::tv::tuner::V1_0::DemuxFilterPesDataSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterPesEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterRecordSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterSectionEvent;
using android::hardware::tv::tuner::V1_0::DemuxFilterSectionSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxFilterStatus;
using android::hardware::tv::tuner::V1_0::DemuxFilterType;
using android::hardware::tv::tuner::V1_0::DemuxQueueNotifyBits;
using android::hardware::tv::tuner::V1_0::DemuxTpid;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterSettings;
using android::hardware::tv::tuner::V1_0::DemuxTsFilterType;
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

namespace {

#define frontend_transponders_count 1
#define channels_count 1
#define frontend_scan_count 1
#define filter_count 2

struct FilterConfig {
    DemuxFilterType type;
    DemuxFilterSettings setting;
};

struct FrontendConfig {
    FrontendType type;
    FrontendSettings settings;
};

struct ChannelConfig {
    int32_t frontendId;
    int32_t channelId;
    std::string channelName;
    DemuxTpid videoPid;
    DemuxTpid audioPid;
};

static FrontendConfig frontendArray[frontend_transponders_count];
static FrontendConfig frontendScanArray[channels_count];
static ChannelConfig channelArray[frontend_scan_count];
static FilterConfig filterArray[filter_count];
static vector<string> goldenOutputFiles;

/** Configuration array for the frontend tune test */
inline void initFrontendConfig() {
    FrontendDvbtSettings dvbtSettings{
            .frequency = 578000,
            .transmissionMode = FrontendDvbtTransmissionMode::AUTO,
            .bandwidth = FrontendDvbtBandwidth::BANDWIDTH_8MHZ,
            .constellation = FrontendDvbtConstellation::AUTO,
            .hierarchy = FrontendDvbtHierarchy::AUTO,
            .hpCoderate = FrontendDvbtCoderate::AUTO,
            .lpCoderate = FrontendDvbtCoderate::AUTO,
            .guardInterval = FrontendDvbtGuardInterval::AUTO,
            .isHighPriority = true,
            .standard = FrontendDvbtStandard::T,
    };
    frontendArray[0].type = FrontendType::DVBT, frontendArray[0].settings.dvbt(dvbtSettings);
};

/** Configuration array for the frontend scan test */
inline void initFrontendScanConfig() {
    frontendScanArray[0].type = FrontendType::DVBT, frontendScanArray[0].settings.dvbt({
                                                            .frequency = 577000,
                                                    });
};

/** Configuration array for the filter test */
inline void initFilterConfig() {
    // TS Video filter setting
    filterArray[0].type.mainType = DemuxFilterMainType::TS;
    filterArray[0].type.subType.tsFilterType(DemuxTsFilterType::VIDEO);
    filterArray[0].setting.ts().tpid = 49;
    filterArray[0].setting.ts().filterSettings.av({.isPassthrough = false});
    // TS PES filter setting
    filterArray[1].type.mainType = DemuxFilterMainType::TS;
    filterArray[1].type.subType.tsFilterType(DemuxTsFilterType::PES);
    filterArray[1].setting.ts().tpid = 256;
    filterArray[1].setting.ts().filterSettings.pesData({
            .isRaw = true,
            .streamId = 0xbd,
    });
};

}  // namespace